#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>

#define MAX_GLYPHS 10000
#define MAX_LINE 1024
#define MAX_MAPPINGS 16

typedef struct {
    int src_start;
    int src_end;
    int dest_start;
} Mapping;

typedef struct {
    int encoding;
    char *raw_data; // STARTCHAR行以降の全データ
} Glyph;

typedef struct {
    char header[MAX_GLYPHS][128];
    int header_lines;
    Glyph *glyphs;
    int count;
    int capacity;
} Font;

Font* create_font(int capacity);
void free_font(Font *font);
void load_bdf(const char *path, Font *font);
void write_remapped_bdf(const char *path, Font *font, Mapping *maps, int map_count);
long parse_hex(const char *str);
int parse_mapping(const char *arg, Mapping *map);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input.bdf> <output.bdf> <map1> [map2 ...]\n", argv[0]);
        fprintf(stderr, "  Map format: SRC_START-SRC_END:DEST_START (Hex)\n");
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];
    
    Mapping maps[MAX_MAPPINGS];
    int map_count = 0;

    for (int i = 3; i < argc; i++) {
        if (map_count >= MAX_MAPPINGS) break;
        if (parse_mapping(argv[i], &maps[map_count])) map_count++;
    }

    Font *font = create_font(MAX_GLYPHS);
    if (!font) return 1;
    
    load_bdf(input_path, font);
    write_remapped_bdf(output_path, font, maps, map_count);

    free_font(font);
    return 0;
}

long parse_hex(const char *str) {
    char *endptr;
    long val = strtol(str, &endptr, 0);
    return val;
}

int parse_mapping(const char *arg, Mapping *map) {
    char buf1[32], buf2[32], buf3[32];
    const char *p = arg;
    const char *hyphen = strchr(p, '-');
    const char *colon = strchr(p, ':');

    if (!hyphen || !colon || colon < hyphen) return 0;

    int len1 = hyphen - p;
    int len2 = colon - (hyphen + 1);
    
    strncpy(buf1, p, len1); buf1[len1] = '\0';
    strncpy(buf2, hyphen + 1, len2); buf2[len2] = '\0';
    strcpy(buf3, colon + 1);

    map->src_start = parse_hex(buf1);
    map->src_end = parse_hex(buf2);
    map->dest_start = parse_hex(buf3);

    return 1;
}

Font* create_font(int capacity) {
    Font *f = (Font *)calloc(1, sizeof(Font));
    if (!f) return NULL;
    f->glyphs = (Glyph *)calloc(capacity, sizeof(Glyph));
    f->capacity = capacity;
    return f;
}

void free_font(Font *font) {
    if (!font) return;
    for (int i = 0; i < font->count; i++) {
        if (font->glyphs[i].raw_data) free(font->glyphs[i].raw_data);
    }
    free(font->glyphs);
    free(font);
}

void load_bdf(const char *path, Font *font) {
    FILE *fp = fopen(path, "r");
    if (!fp) exit(1);

    char line[MAX_LINE];
    int in_header = 1;
    Glyph *current = NULL;
    size_t raw_buf_size = 16384; 

    while (fgets(line, sizeof(line), fp)) {
        char trim_line[MAX_LINE];
        strcpy(trim_line, line);
        trim_line[strcspn(trim_line, "\r\n")] = 0;

        if (strncmp(trim_line, "STARTCHAR", 9) == 0) {
            in_header = 0;
            current = &font->glyphs[font->count];
            current->raw_data = (char *)calloc(1, raw_buf_size);
            strcpy(current->raw_data, line);
        } else if (in_header) {
            if (font->header_lines < MAX_GLYPHS) {
                strcpy(font->header[font->header_lines++], trim_line);
            }
        } else if (current) {
            strcat(current->raw_data, line);
            if (strncmp(trim_line, "ENCODING", 8) == 0) {
                current->encoding = atoi(trim_line + 9);
            } else if (strncmp(trim_line, "ENDCHAR", 7) == 0) {
                font->count++;
                current = NULL;
            }
        }
    }
    fclose(fp);
}

void write_remapped_bdf(const char *path, Font *font, Mapping *maps, int map_count) {
    FILE *fp = fopen(path, "w");
    if (!fp) exit(1);

    // 1. 出力対象のグリフを特定し、新しいエンコーディングを計算
    int *new_encodings = (int *)malloc(sizeof(int) * font->count);
    int total_to_output = 0;

    for (int i = 0; i < font->count; i++) {
        new_encodings[i] = -1;
        int enc = font->glyphs[i].encoding;
        for (int m = 0; m < map_count; m++) {
            if (enc >= maps[m].src_start && enc <= maps[m].src_end) {
                new_encodings[i] = maps[m].dest_start + (enc - maps[m].src_start);
                total_to_output++;
                break;
            }
        }
    }

    // 2. ヘッダの書き出し (CHARS の値を total_to_output で上書き)
    for (int i = 0; i < font->header_lines; i++) {
        char *line = font->header[i];
        if (strncmp(line, "CHARS", 5) == 0) {
            fprintf(fp, "CHARS %d\n", total_to_output); // 整合性を確保
        } else if (strncmp(line, "CHARSET_REGISTRY", 16) == 0) {
            fprintf(fp, "CHARSET_REGISTRY \"ISO10646\"\n");
        } else if (strncmp(line, "CHARSET_ENCODING", 16) == 0) {
            fprintf(fp, "CHARSET_ENCODING \"1\"\n");
        } else {
            fprintf(fp, "%s\n", line);
        }
    }

    // 3. グリフの書き出し
    for (int i = 0; i < font->count; i++) {
        if (new_encodings[i] == -1) continue;

        char *line_ptr = font->glyphs[i].raw_data;
        char line_buf[MAX_LINE];
        int pos = 0;
        int new_enc = new_encodings[i];
        
        while (line_ptr[pos] != '\0') {
            int line_len = 0;
            while (line_ptr[pos + line_len] != '\0' && line_ptr[pos + line_len] != '\n') {
                line_len++;
            }
            if (line_ptr[pos + line_len] == '\n') line_len++;
            
            strncpy(line_buf, line_ptr + pos, line_len);
            line_buf[line_len] = '\0';
            
            // STARTCHAR と ENCODING を新コードで出力
            if (strncmp(line_buf, "STARTCHAR", 9) == 0) {
                fprintf(fp, "STARTCHAR %04X\n", new_enc);
            } else if (strncmp(line_buf, "ENCODING", 8) == 0) {
                fprintf(fp, "ENCODING %d\n", new_enc);
            } else {
                fprintf(fp, "%s", line_buf);
            }
            pos += line_len;
        }
    }
    
    fprintf(fp, "ENDFONT\n");
    free(new_encodings);
    fclose(fp);
}

