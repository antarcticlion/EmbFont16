#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * Enhanced HEX to BDF Converter
 * Based on logic from Unscii's hex2bdf.pl
 */

#define MAX_LINE_LEN 1024
#define MAX_COMMENTS 100

// デフォルト設定
#define DEF_FOUNDRY "Viznut"
#define DEF_FAMILY  "Unscii"
#define DEF_WEIGHT  "Medium"
#define DEF_SLANT   "R"
#define DEF_WIDTH   "Normal"
#define PIXEL_SIZE  16
#define RESOLUTION  75

// グローバル設定変数
char g_foundry[64] = DEF_FOUNDRY;
char g_family[64]  = DEF_FAMILY;
char g_weight[64]  = DEF_WEIGHT;

// コメント保持用
char g_comments[MAX_COMMENTS][MAX_LINE_LEN];
int g_comment_count = 0;
char g_copyright[MAX_LINE_LEN] = "See comments";

// 16進変換
int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// BDFヘッダ出力
void write_header(FILE* out, int char_count, int max_width) {
    // XLFD (X Logical Font Description) の構築
    // フォーマット: -Foundry-Family-Weight-Slant-SetWidth-Style-PixelSize-PointSize-ResX-ResY-Spacing-AvgWidth-Registry-Encoding
    // Unsciiは固定幅(C: Cell)フォント
    int avg_width_tenths = (max_width == 16) ? 120 : 80; // 暫定値 (8px -> 80, 16px -> 160 だが平均をとる)

    fprintf(out, "STARTFONT 2.1\n");
    
    // コメントの出力
    for(int i=0; i<g_comment_count; i++) {
        fprintf(out, "COMMENT %s\n", g_comments[i]);
    }
    
    fprintf(out, "FONT -%s-%s-%s-%s-%s--%d-%d-%d-%d-C-%d-iso10646-1\n",
            g_foundry, g_family, g_weight, DEF_SLANT, DEF_WIDTH,
            PIXEL_SIZE, PIXEL_SIZE * 10, RESOLUTION, RESOLUTION, avg_width_tenths);

    fprintf(out, "SIZE %d %d %d\n", PIXEL_SIZE, RESOLUTION, RESOLUTION);
    // 全体のバウンディングボックス: 幅は最大幅、高さ16、オフセット0
    fprintf(out, "FONTBOUNDINGBOX %d %d 0 0\n", max_width, PIXEL_SIZE);

    // プロパティブロック
    fprintf(out, "STARTPROPERTIES 6\n");
    fprintf(out, "FONT_ASCENT %d\n", PIXEL_SIZE);
    fprintf(out, "FONT_DESCENT 0\n");
    fprintf(out, "DEFAULT_CHAR 32\n"); // Space
    fprintf(out, "COPYRIGHT \"%s\"\n", g_copyright);
    fprintf(out, "PIXEL_SIZE %d\n", PIXEL_SIZE);
    fprintf(out, "RESOLUTION_X %d\n", RESOLUTION);
    fprintf(out, "ENDPROPERTIES\n");

    fprintf(out, "CHARS %d\n", char_count);
}

// 1文字処理
void process_char(FILE* out, const char* line) {
    const char* sep = strchr(line, ':');
    if (!sep) return;

    // コードポイント
    char code_buf[16] = {0};
    strncpy(code_buf, line, sep - line);
    unsigned int code = (unsigned int)strtoul(code_buf, NULL, 16);

    const char* bitmap_hex = sep + 1;
    size_t len = strlen(bitmap_hex);
    while(len > 0 && isspace(bitmap_hex[len-1])) len--;

    int width = 0;
    if (len == 32) width = 8;
    else if (len == 64) width = 16;
    else return;

    // SWIDTH計算: (DWIDTH * 72000) / (Size * Res)
    // Size=16, Res=75 => 16*75 = 1200
    // SWIDTH = DWIDTH * 72000 / 1200 = DWIDTH * 60
    int swidth = width * 60;

    fprintf(out, "STARTCHAR U+%04X\n", code);
    fprintf(out, "ENCODING %u\n", code);
    fprintf(out, "SWIDTH %d 0\n", swidth);
    fprintf(out, "DWIDTH %d 0\n", width);
    fprintf(out, "BBX %d %d 0 0\n", width, PIXEL_SIZE);
    fprintf(out, "BITMAP\n");

    for (size_t i = 0; i < len; i += 2) {
        fprintf(out, "%c%c\n", bitmap_hex[i], bitmap_hex[i+1]);
    }

    fprintf(out, "ENDCHAR\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.hex> [output.bdf] [FamilyName]\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = (argc >= 3) ? argv[2] : "out.bdf";
    if (argc >= 4) strncpy(g_family, argv[3], 63);

    FILE* in = fopen(input_file, "r");
    if (!in) { perror("Input open failed"); return 1; }

    // Pass 1: 解析 (文字数カウント、最大幅検出、メタデータ収集)
    int char_count = 0;
    int max_width = 8;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), in)) {
        // 改行削除
        size_t l = strlen(line);
        while(l > 0 && isspace(line[l-1])) line[--l] = '\0';
        if(l == 0) continue;

        // コメント解析 (#)
        if (line[0] == '#') {
            if (g_comment_count < MAX_COMMENTS) {
                // 先頭の # とスペースをスキップ
                char* p = line + 1;
                while(*p == ' ') p++;
                snprintf(g_comments[g_comment_count++], sizeof(g_comments[0]), "%s", p);

                if (strstr(line, "Copyright") || strstr(line, "License")) {
                    snprintf(g_copyright, sizeof(g_copyright), "%s", p);
                }
            }
            continue;
        }

        // データ行解析
        char* sep = strchr(line, ':');
        if (sep) {
            size_t hex_len = strlen(sep + 1);
            if (hex_len >= 64) max_width = 16;
            char_count++;
        }
    }

    // Pass 2: 出力
    FILE* out = fopen(output_file, "w");
    if (!out) { perror("Output open failed"); fclose(in); return 1; }

    write_header(out, char_count, max_width);

    rewind(in);
    while (fgets(line, sizeof(line), in)) {
        size_t l = strlen(line);
        while(l > 0 && isspace(line[l-1])) line[--l] = '\0';
        if (l == 0 || line[0] == '#') continue;
        
        process_char(out, line);
    }

    fprintf(out, "ENDFONT\n");

    fclose(in);
    fclose(out);

    printf("Converted: %s -> %s\n", input_file, output_file);
    printf("  Chars: %d\n  Family: %s\n  Max Width: %d\n", char_count, g_family, max_width);
    
    return 0;
}
