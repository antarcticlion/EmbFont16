#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "label_font.h"
#include "embfont16_common.h" // Includes RawGlyph definition and MAX_CODEPOINT

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define MAX_LINE_LEN 256

// --- Globals ---
static RawGlyph* input_glyphs = NULL; // Pointer for dynamic allocation
static uint32_t g_max_code = 0;
static uint32_t g_total_chars = 0;
static char g_font_names[FONT_NAME_MAX_ENTRIES][FONT_NAME_ENTRY_SIZE];
static uint32_t g_font_count = 0;

// --- Helpers ---
int hex2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

typedef struct {
    const char *stem;
    const char *display_name;
} FontDisplayName;

static const FontDisplayName FONT_DISPLAY_NAMES[] = {
    {"milkjf_8x16_cut", "milkjf 8x16"},
    {"milkjf_k16_uni", "milkjf k16"},
    {"milkjf_8x16r_uni", "milkjf 8x16r"},
    {"jiskan16-2004-1_uni", "JIS X 0213:2004 plane 1"},
    {"jiskan16-2000-2_uni", "JIS X 0213:2000 plane 2"},
    {"hanglm16_uni", "Daewoo hanglm16"},
    {"hanglm16_kana_uni", "Daewoo hanglm16 kana"},
    {"gb16st_uni", "Academia Sinica gb16st"},
    {"gb16st_kana_uni", "Academia Sinica gb16st kana"},
    {"unifont_jp", "GNU Unifont JP"},
};

static const char *display_name_for(const char *stem) {
    for (size_t i = 0; i < sizeof(FONT_DISPLAY_NAMES) / sizeof(FONT_DISPLAY_NAMES[0]);
         i++) {
        if (strcmp(FONT_DISPLAY_NAMES[i].stem, stem) == 0) {
            return FONT_DISPLAY_NAMES[i].display_name;
        }
    }
    return stem;
}

static void register_font_name(const char* filename, uint8_t font_id) {
    if (font_id >= FONT_NAME_MAX_ENTRIES) {
        fprintf(stderr, "Error: too many font files (max %d).\n", FONT_NAME_MAX_ENTRIES);
        exit(1);
    }

    const char* base = strrchr(filename, '/');
    base = base ? base + 1 : filename;
    size_t len = strlen(base);
    if (len > 4 && base[len - 4] == '.' &&
        tolower((unsigned char)base[len - 3]) == 'b' &&
        tolower((unsigned char)base[len - 2]) == 'd' &&
        tolower((unsigned char)base[len - 1]) == 'f') {
        len -= 4;
    }
    if (len >= FONT_NAME_ENTRY_SIZE) len = FONT_NAME_ENTRY_SIZE - 1;

    char stem[FONT_NAME_ENTRY_SIZE];
    memcpy(stem, base, len);
    stem[len] = '\0';

    const char* display = display_name_for(stem);
    size_t display_len = strlen(display);
    if (display_len >= FONT_NAME_ENTRY_SIZE) {
        display_len = FONT_NAME_ENTRY_SIZE - 1;
    }
    memcpy(g_font_names[font_id], display, display_len);
    g_font_names[font_id][display_len] = '\0';
    if (font_id + 1 > g_font_count) g_font_count = font_id + 1;
}

// BDF Loader (Modified for dynamic allocation & MAX_CODEPOINT)
void load_bdf_file(const char* filename, uint8_t font_id) {
    FILE* fp = fopen(filename, "r");
    if (!fp) { fprintf(stderr, "Error: %s\n", filename); exit(1); }
    printf("Reading BDF: %s...\n", filename);
    register_font_name(filename, font_id);

    char line[MAX_LINE_LEN];
    int current_code = -1;
    int current_width = 0;
    bool in_bitmap = false;
    int bitmap_row = 0;
    RawGlyph temp_glyph;
    bool skip_overwrite = false;
    bool has_pixels = false;
    memset(&temp_glyph, 0, sizeof(RawGlyph));

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && isspace(line[len-1])) line[--len] = '\0';
        if (len == 0) continue;

        if (strncmp(line, "ENCODING", 8) == 0) {
            current_code = atoi(line + 9);
            in_bitmap = false;
            has_pixels = false;
            memset(&temp_glyph, 0, sizeof(RawGlyph));
            
            if (current_code >= 0 && current_code <= MAX_CODEPOINT && input_glyphs[current_code].exists) {
                skip_overwrite = true;
            } else {
                skip_overwrite = false;
            }
        }
        else if (strncmp(line, "BBX", 3) == 0) {
            if (!skip_overwrite) {
                int w, h, x, y;
                if (sscanf(line + 4, "%d %d %d %d", &w, &h, &x, &y) >= 2) current_width = w;
            }
        }
        else if (strncmp(line, "BITMAP", 6) == 0) {
            in_bitmap = true;
            bitmap_row = 0;
        }
        else if (strncmp(line, "ENDCHAR", 7) == 0) {
            in_bitmap = false;
            if (!skip_overwrite && current_code >= 0 && current_code <= MAX_CODEPOINT) {
                bool is_allowed_space = (current_code == 0x20 || current_code == 0xA0 || current_code == 0x3000);
                if (has_pixels || is_allowed_space) {
					temp_glyph.half = (current_width > 8) ? 0 : 1;
                    temp_glyph.font_id = font_id & FONT_ID_MASK;
                    temp_glyph.exists = true;
                    memcpy(&input_glyphs[current_code], &temp_glyph, sizeof(RawGlyph));
                    g_total_chars++;
                    if ((uint32_t)current_code > g_max_code) g_max_code = current_code;
                }
            }
            current_code = -1;
            skip_overwrite = false;
        }
        else if (in_bitmap && !skip_overwrite) {
            if (current_code >= 0 && current_code <= MAX_CODEPOINT && bitmap_row < 16) {
                int bytes_to_read = (current_width > 8) ? 2 : 1;
                for (int i = 0; i < bytes_to_read; i++) {
                    char hi = line[i*2];
                    char lo = line[i*2+1];
                    if (hi == '\0' || lo == '\0') break;
                    uint8_t val = (hex2int(hi) << 4) | hex2int(lo);
                    if (val != 0) has_pixels = true;
                    if (current_width > 8) temp_glyph.data[bitmap_row*2+i] = val;
                    else temp_glyph.data[bitmap_row] = val;
                }
                bitmap_row++;
            }
        }
    }
    fclose(fp);
}

// --- Image Drawing ---
typedef struct { int w, h; uint8_t* pixels; } Image;

Image* create_image(int w, int h) {
    Image* img = (Image*)malloc(sizeof(Image));
    img->w = w; img->h = h;
    img->pixels = (uint8_t*)malloc(w * h);
    memset(img->pixels, 255, w * h);
    return img;
}
void free_image(Image* img) { free(img->pixels); free(img); }
void draw_pixel(Image* img, int x, int y, uint8_t color) {
    if (x >= 0 && x < img->w && y >= 0 && y < img->h) img->pixels[y * img->w + x] = color;
}
void draw_line(Image* img, int x0, int y0, int x1, int y1, uint8_t color) {
    if (y0 == y1) {
        int s = (x0 < x1) ? x0 : x1; int e = (x0 < x1) ? x1 : x0;
        for (int x = s; x <= e; x++) draw_pixel(img, x, y0, color);
    } else if (x0 == x1) {
        int s = (y0 < y1) ? y0 : y1; int e = (y0 < y1) ? y1 : y0;
        for (int y = s; y <= e; y++) draw_pixel(img, x0, y, color);
    }
}
void draw_rect(Image* img, int x, int y, int w, int h, uint8_t color) {
    draw_line(img, x, y, x+w-1, y, color); draw_line(img, x, y+h-1, x+w-1, y+h-1, color);
    draw_line(img, x, y, x, y+h-1, color); draw_line(img, x+w-1, y, x+w-1, y+h-1, color);
}
void draw_text(Image* img, int x, int y, const char* str, uint8_t color) {
    while (*str) {
        uint8_t c = (uint8_t)*str++;
        int idx = 0; 
        if (c >= 0x20 && c <= 0x7E) idx = c - 0x20;
        else { c = '?'; idx = '?' - 0x20; }
        for (int r = 0; r < 16; r++) {
            uint8_t val = label_font[idx][r];
            for (int bit = 7; bit >= 0; bit--) {
                if ((val >> bit) & 1) draw_pixel(img, x + (7 - bit), y + r, color);
            }
        }
        x += 8;
    }
}

// --- PNG Export ---
void export_png_charts(const char* output_dir) {
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
#ifdef _WIN32
        mkdir(output_dir);
#else
        mkdir(output_dir, 0755);
#endif
    }
    printf("Exporting PNG charts to %s/ ...\n", output_dir);

    const int CELL_W = 24, CELL_H = 24;
    const int MARGIN_L = 32, MARGIN_T = 64; 
    int img_w = MARGIN_L + 16 * CELL_W + 16;
    int img_h = MARGIN_T + 16 * CELL_H + 16;

    time_t t = time(NULL); struct tm tm = *localtime(&t);
    char date_str[64]; sprintf(date_str, "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    
    int generated = 0;
    uint32_t limit_block = (g_max_code >> 8);
    // Cap at MAX_CODEPOINT block
    if (limit_block > (MAX_CODEPOINT >> 8)) limit_block = (MAX_CODEPOINT >> 8);

    for (uint32_t b = 0; b <= limit_block; b++) {
        uint32_t start_code = b * 256;
        bool has = false;
        for (int i = 0; i < 256; i++) {
            if (input_glyphs[start_code + i].exists) { has = true; break; }
        }
        if (!has) continue;

        Image* img = create_image(img_w, img_h);
        char buf[128];
        
        sprintf(buf, "EmbFont16 %s", date_str);
        draw_text(img, MARGIN_L, 10, buf, 0);
        sprintf(buf, "Block: %02X (0x%06X-0x%06X)", b, start_code, start_code + 255);
        draw_text(img, MARGIN_L, 30, buf, 0);

        const char hex[] = "0123456789ABCDEF";
        for (int i = 0; i < 16; i++) {
            char lbl[2] = {hex[i], 0};
            draw_text(img, MARGIN_L + i * CELL_W + 8, MARGIN_T - 18, lbl, 0);
            draw_text(img, 8, MARGIN_T + i * CELL_H + 4, lbl, 0);
        }

        for (int r = 0; r < 16; r++) {
            for (int c = 0; c < 16; c++) {
                int x = MARGIN_L + c * CELL_W;
                int y = MARGIN_T + r * CELL_H;
                draw_rect(img, x, y, CELL_W, CELL_H, 200);
                uint32_t code = start_code + r * 16 + c;
                if (input_glyphs[code].exists) {
                    RawGlyph* g = &input_glyphs[code];
                    int gx = x + (CELL_W - (g->half?8:16)) / 2;
                    int gy = y + (CELL_H - 16) / 2;
                    for (int gr = 0; gr < 16; gr++) {
                        for (int gc = 0; gc < (g->half ? 1 : 2); gc++) {
                            uint8_t val = !g->half ? g->data[gr*2+gc] : g->data[gr];
                            for (int bit = 7; bit >= 0; bit--) {
                                if ((val >> bit) & 1) draw_pixel(img, gx + gc*8 + (7-bit), gy + gr, 0);
                            }
                        }
                    }
                }
            }
        }
        
        char fname[MAX_LINE_LEN];
        sprintf(fname, "%s/EmbFont16-%s-%06X-%06X.png", output_dir, date_str, start_code, start_code + 255);
        stbi_write_png(fname, img->w, img->h, 1, img->pixels, img->w);
        free_image(img);
        generated++;
    }
    printf("PNG Export done. Generated %d files.\n", generated);
}

void export_glyph_map(void) {
    const char* f = "mapping/glyph_map.txt";
    struct stat st = {0}; if (stat("mapping", &st) == -1) mkdir("mapping", 0755);
    FILE* fp = fopen(f, "w"); if (!fp) return;
    fprintf(fp, "# Map\n");
    for(uint32_t i=0; i<=MAX_CODEPOINT; i++) {
        if(input_glyphs[i].exists) fprintf(fp, "0x%06X, %d\n", i, (input_glyphs[i].half?8:16));
    }
    fclose(fp);
}

void export_bdf(const char* filename) {
    FILE* out = fopen(filename, "w"); if(!out) return;
    printf("Exporting BDF: %s\n", filename);
    
    // STARTPROPERTIES を 2 から 4 に変更し、CHARSET を追加
    fprintf(out, "STARTFONT 2.1\n");
    fprintf(out, "FONT EmbFont16\n");
    fprintf(out, "SIZE 16 75 75\n");
    fprintf(out, "FONTBOUNDINGBOX 16 16 0 0\n");
    fprintf(out, "STARTPROPERTIES 4\n"); // プロパティ数 4 つ
    fprintf(out, "FONT_ASCENT 16\n");
    fprintf(out, "FONT_DESCENT 0\n");
    fprintf(out, "CHARSET_REGISTRY \"ISO10646\"\n"); // Unicode 規格の指定
    fprintf(out, "CHARSET_ENCODING \"1\"\n");        // UCS の指定
    fprintf(out, "ENDPROPERTIES\n");
    fprintf(out, "COMMENT EMBFONT16_FONT_COUNT %u\n", g_font_count);
    for (uint32_t f = 0; f < g_font_count; f++) {
        fprintf(out, "COMMENT EMBFONT16_FONT %u %s\n", f, g_font_names[f]);
    }
    fprintf(out, "CHARS %d\n", g_total_chars);

    for(uint32_t i=0; i<=MAX_CODEPOINT; i++) {
        if(!input_glyphs[i].exists) continue;
        RawGlyph* g = &input_glyphs[i];
        fprintf(out, "STARTCHAR U+%06X\nENCODING %d\nCOMMENT EMBFONT16_FONT_ID %u\nSWIDTH %d 0\nDWIDTH %d 0\nBBX %d 16 0 0\nBITMAP\n", i, i, (unsigned)(g->font_id & FONT_ID_MASK), (g->half?8:16)*60, (g->half?8:16), (g->half?8:16));
        for(int r=0; r<16; r++) {
            for(int b=0; b<(g->half?1:2); b++) fprintf(out, "%02X", !g->half?g->data[r*2+b]:g->data[r]);
            fprintf(out, "\n");
        }
        fprintf(out, "ENDCHAR\n");
    }
    fprintf(out, "ENDFONT\n"); fclose(out);
}

int main(int argc, char* argv[]) {
    // Allocate one RawGlyph per code point (about 9 MB)
    input_glyphs = (RawGlyph*)calloc(MAX_CODEPOINT + 1, sizeof(RawGlyph));
    if (!input_glyphs) {
        fprintf(stderr, "Error: Failed to allocate memory for %d glyphs.\n", MAX_CODEPOINT + 1);
        return 1;
    }

    if (argc < 2) { 
        fprintf(stderr, "Usage: %s <bdf>... [--bdf <out>] [--png <dir>]\n", argv[0]); 
        free(input_glyphs);
        return 1; 
    }
    
    const char *out_bdf = NULL, *out_png = NULL;
    uint8_t font_id = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--bdf")) {
            if (++i < argc) out_bdf = argv[i];
        } else if (!strcmp(argv[i], "--png")) {
            if (++i < argc) out_png = argv[i];
        } else {
            if (font_id >= FONT_NAME_MAX_ENTRIES) {
                fprintf(stderr, "Error: too many font files (max %d).\n", FONT_NAME_MAX_ENTRIES);
                free(input_glyphs);
                return 1;
            }
            load_bdf_file(argv[i], font_id++);
        }
    }
    export_glyph_map();
    if (out_bdf) export_bdf(out_bdf);
    if (out_png) export_png_charts(out_png);
    
    free(input_glyphs);
    return 0;
}

