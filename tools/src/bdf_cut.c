#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 2048

typedef struct {
    char *data;
    int code;
} GlyphBlock;

int main(int argc, char *argv[]) {
    const char *input_path = NULL;
    int cut_code = -1;
    int backward = 0;
    int positional_arg = 0;

    // --- 真面目な引数解析 ---
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0) {
            backward = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 < argc) i++; // -d 自体は後方互換性のために読み飛ばす
        } else {
            // オプション以外の引数を順番に割り当て
            if (positional_arg == 0) {
                input_path = argv[i];
                positional_arg++;
            } else if (positional_arg == 1) {
                cut_code = (int)strtol(argv[i], NULL, 0);
                positional_arg++;
            }
        }
    }

    if (!input_path || positional_arg < 2) {
        fprintf(stderr, "Usage: %s <input.bdf> <cut_code> [-u (backward)]\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(input_path, "r");
    if (!fp) {
        perror("Input file open failed");
        fprintf(stderr, "Attempted to open: %s\n", input_path);
        return 1;
    }

    char line[MAX_LINE];
    // --- 1. ヘッダ出力 (CHARSはスキップ) ---
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "CHARS", 5) == 0) continue; 
        printf("%s", line);
        if (strncmp(line, "ENDPROPERTIES", 13) == 0) break;
    }

    // --- 2. グリフ抽出 ---
    GlyphBlock *glyphs = NULL;
    int count = 0;
    char glyph_buf[32768];
    int in_glyph = 0;
    size_t buf_pos = 0;
    int has_encoding = 0;
    char current_name[256] = "";

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "STARTCHAR", 9) == 0) {
            in_glyph = 1;
            buf_pos = 0;
            has_encoding = 0;
            sscanf(line + 10, "%s", current_name);
            buf_pos += snprintf(glyph_buf + buf_pos, sizeof(glyph_buf) - buf_pos, "%s", line);
        } else if (in_glyph) {
            if (strncmp(line, "ENCODING", 8) == 0) {
                has_encoding = 1;
                buf_pos += snprintf(glyph_buf + buf_pos, sizeof(glyph_buf) - buf_pos, "%s", line);
            } else if (strncmp(line, "ENDCHAR", 7) == 0) {
                buf_pos += snprintf(glyph_buf + buf_pos, sizeof(glyph_buf) - buf_pos, "%s", line);
                
                int current_code = (int)strtol(current_name, NULL, 16);
                int match = backward ? (current_code <= cut_code) : (current_code >= cut_code);
                
                if (match) {
                    GlyphBlock *tmp = realloc(glyphs, sizeof(GlyphBlock) * (count + 1));
                    if (!tmp) exit(1);
                    glyphs = tmp;
                    glyphs[count].code = current_code;
                    glyphs[count].data = strdup(glyph_buf);
                    count++;
                }
                in_glyph = 0;
            } else {
                if (strncmp(line, "BITMAP", 6) == 0 && !has_encoding) {
                    int enc = (int)strtol(current_name, NULL, 16);
                    buf_pos += snprintf(glyph_buf + buf_pos, sizeof(glyph_buf) - buf_pos, "ENCODING %d\n", enc);
                    has_encoding = 1;
                }
                buf_pos += snprintf(glyph_buf + buf_pos, sizeof(glyph_buf) - buf_pos, "%s", line);
            }
        }
    }

    // --- 3. 出力 ---
    printf("CHARS %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("%s", glyphs[i].data);
        free(glyphs[i].data);
    }
    printf("ENDFONT\n");

    free(glyphs);
    fclose(fp);
    return 0;
}