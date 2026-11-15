#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bdf_parser.h"

#define MAX_LINE_LEN 256

RawGlyph* g_glyphs = NULL;
uint32_t g_max_code = 0;
uint32_t g_total_chars = 0;
char g_font_names[FONT_NAME_MAX_ENTRIES][FONT_NAME_ENTRY_SIZE];
uint32_t g_font_count = 0;

int hex2int_p(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void parse_bdf(const char* filename) {
    // Allocate memory on first call
    if (!g_glyphs) {
        // Allocate one RawGlyph per code point (about 9 MB)
        g_glyphs = (RawGlyph*)calloc(MAX_CODEPOINT + 1, sizeof(RawGlyph));
        if (!g_glyphs) {
            fprintf(stderr, "Error: Failed to allocate memory for glyphs (Parser).\n");
            exit(1);
        }
    }

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open input file %s\n", filename);
        exit(1);
    }
    printf("Parsing BDF: %s...\n", filename);

    char line[MAX_LINE_LEN];
    int current_code = -1;
    int current_width = 0;
    bool in_bitmap = false;
    int bitmap_row = 0;
    RawGlyph temp_glyph;
    bool skip_overwrite = false;
    bool has_pixels = false;
    uint8_t pending_font_id = 0;

    memset(&temp_glyph, 0, sizeof(RawGlyph));

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && isspace(line[len-1])) line[--len] = '\0';
        if (len == 0) continue;

        if (strncmp(line, "COMMENT EMBFONT16_FONT_ID ", 26) == 0) {
            unsigned id;
            if (sscanf(line + 26, "%u", &id) == 1) {
                pending_font_id = (uint8_t)(id & FONT_ID_MASK);
            }
        }
        else if (strncmp(line, "COMMENT EMBFONT16_FONT ", 23) == 0) {
            unsigned id;
            char name[FONT_NAME_ENTRY_SIZE];
            if (sscanf(line + 23, "%u %63[^\n]", &id, name) == 2 && id < FONT_NAME_MAX_ENTRIES) {
                size_t name_len = strlen(name);
                while (name_len > 0 && isspace((unsigned char)name[name_len - 1])) {
                    name[--name_len] = '\0';
                }
                if (name_len >= FONT_NAME_ENTRY_SIZE) name_len = FONT_NAME_ENTRY_SIZE - 1;
                memcpy(g_font_names[id], name, name_len);
                g_font_names[id][name_len] = '\0';
                if (id + 1 > g_font_count) g_font_count = id + 1;
            }
        }
        else if (strncmp(line, "ENCODING", 8) == 0) {
            current_code = atoi(line + 9);
            in_bitmap = false;
            has_pixels = false;
            pending_font_id = 0;
            memset(&temp_glyph, 0, sizeof(RawGlyph));
            
            // Check MAX_CODEPOINT
            if (current_code >= 0 && current_code <= MAX_CODEPOINT && g_glyphs[current_code].exists) {
                skip_overwrite = true;
            } else {
                skip_overwrite = false;
            }
        }
        else if (strncmp(line, "BBX", 3) == 0) {
            if (!skip_overwrite) {
                int w, h, x, y;
                if (sscanf(line + 4, "%d %d %d %d", &w, &h, &x, &y) >= 2) {
                    current_width = w;
                }
            }
        }
        else if (strncmp(line, "BITMAP", 6) == 0) {
            in_bitmap = true;
            bitmap_row = 0;
        }
        else if (strncmp(line, "ENDCHAR", 7) == 0) {
            in_bitmap = false;
            if (!skip_overwrite && current_code >= 0 && current_code <= MAX_CODEPOINT) {
                // Allow spaces
                bool is_allowed_space = (current_code == 0x20 || current_code == 0xA0 || current_code == 0x3000);
                if (has_pixels || is_allowed_space) {
					temp_glyph.half = (current_width > 8) ? 0 : 1;
                    temp_glyph.font_id = pending_font_id;
                    temp_glyph.exists = true;
                    memcpy(&g_glyphs[current_code], &temp_glyph, sizeof(RawGlyph));
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
                    uint8_t val = (hex2int_p(hi) << 4) | hex2int_p(lo);
                    if (val != 0) has_pixels = true;
                    if (current_width > 8) {
                        temp_glyph.data[bitmap_row * 2 + i] = val;
                    } else {
                        temp_glyph.data[bitmap_row] = val;
                    }
                }
                bitmap_row++;
            }
        }
    }
    fclose(fp);
}

void free_bdf_glyphs() {
    if (g_glyphs) {
        free(g_glyphs);
        g_glyphs = NULL;
    }
}
