#include "glyph_manupirate.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int clamp_int(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

/* --- 16x16 glyph operations --- */

void glyph16_invert(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        glyph16_set_row(g, row, (uint16_t)~glyph16_get_row(g, row));
    }
}

void glyph16_flip_h(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        uint16_t value = glyph16_get_row(g, row);
        value = (uint16_t)(((value >> 1) & 0x5555) | ((value & 0x5555) << 1));
        value = (uint16_t)(((value >> 2) & 0x3333) | ((value & 0x3333) << 2));
        value = (uint16_t)(((value >> 4) & 0x0F0F) | ((value & 0x0F0F) << 4));
        value = (uint16_t)(((value >> 8) & 0x00FF) | ((value & 0x00FF) << 8));
        glyph16_set_row(g, row, value);
    }
}

void glyph16_flip_v(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < GLYPH16_HEIGHT / 2; row++) {
        uint16_t top = glyph16_get_row(g, row);
        uint16_t bottom = glyph16_get_row(g, GLYPH16_HEIGHT - 1 - row);
        glyph16_set_row(g, row, bottom);
        glyph16_set_row(g, GLYPH16_HEIGHT - 1 - row, top);
    }
}

void glyph16_shift(glyph16x16_std_t *g, int dx, int dy) {
    if (!g) return;
    dx = clamp_int(dx, -15, 15);
    dy = clamp_int(dy, -15, 15);

    glyph16x16_std_t result = {0};
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        int target_row = row + dy;
        if (target_row < 0 || target_row >= GLYPH16_HEIGHT) continue;

        uint16_t value = glyph16_get_row(g, row);
        if (dx > 0) {
            value >>= dx;
        } else if (dx < 0) {
            value <<= -dx;
        }
        glyph16_set_row(&result, target_row, value);
    }
    *g = result;
}

void glyph16_rotate_90_cw(glyph16x16_std_t *g) {
    if (!g) return;
    glyph16x16_std_t result = {0};
    for (int y = 0; y < GLYPH16_HEIGHT; y++) {
        uint16_t row_value = glyph16_get_row(g, y);
        for (int x = 0; x < GLYPH16_WIDTH; x++) {
            if (row_value & (0x8000u >> x)) {
                uint16_t target = glyph16_get_row(&result, x);
                target |= (uint16_t)(0x8000u >> (15 - y));
                glyph16_set_row(&result, x, target);
            }
        }
    }
    *g = result;
}

/* --- Half extraction and insertion --- */

void glyph16_get_left_half(const glyph16x16_std_t *src, glyph8x16_t *dst) {
    if (!src || !dst) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst->data[row] = src->line[row].part.hi;
    }
}

void glyph16_get_right_half(const glyph16x16_std_t *src, glyph8x16_t *dst) {
    if (!src || !dst) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst->data[row] = src->line[row].part.lo;
    }
}

void glyph16_set_left_half(glyph16x16_std_t *dst, const glyph8x16_t *src) {
    if (!dst || !src) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst->line[row].part.hi = src->data[row];
    }
}

void glyph16_set_right_half(glyph16x16_std_t *dst, const glyph8x16_t *src) {
    if (!dst || !src) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst->line[row].part.lo = src->data[row];
    }
}

void glyph16_swap_halves(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        uint8_t left = g->line[row].part.hi;
        g->line[row].part.hi = g->line[row].part.lo;
        g->line[row].part.lo = left;
    }
}

/* --- Quadrant access and rearrangement --- */

static int quadrant_row_offset(quadrant_t quad) {
    return (quad == GLYPH_QUAD_BL || quad == GLYPH_QUAD_BR) ? 8 : 0;
}

static int quadrant_is_right(quadrant_t quad) {
    return quad == GLYPH_QUAD_TR || quad == GLYPH_QUAD_BR;
}

void glyph16_get_quadrant(const glyph16x16_std_t *src, glyph8x8_t *dst,
                          quadrant_t quad) {
    if (!src || !dst || quad < GLYPH_QUAD_TL || quad > GLYPH_QUAD_BR) return;
    int row_offset = quadrant_row_offset(quad);
    int right = quadrant_is_right(quad);
    for (int row = 0; row < 8; row++) {
        dst->data[row] = right ? src->line[row_offset + row].part.lo
                               : src->line[row_offset + row].part.hi;
    }
}

void glyph16_set_quadrant(glyph16x16_std_t *dst, const glyph8x8_t *src,
                          quadrant_t quad) {
    if (!dst || !src || quad < GLYPH_QUAD_TL || quad > GLYPH_QUAD_BR) return;
    int row_offset = quadrant_row_offset(quad);
    int right = quadrant_is_right(quad);
    for (int row = 0; row < 8; row++) {
        if (right) {
            dst->line[row_offset + row].part.lo = src->data[row];
        } else {
            dst->line[row_offset + row].part.hi = src->data[row];
        }
    }
}

void glyph16_swap_quadrants(glyph16x16_std_t *g, quadrant_t q1, quadrant_t q2) {
    if (!g || q1 == q2) return;
    glyph8x8_t first;
    glyph8x8_t second;
    glyph16_get_quadrant(g, &first, q1);
    glyph16_get_quadrant(g, &second, q2);
    glyph16_set_quadrant(g, &first, q2);
    glyph16_set_quadrant(g, &second, q1);
}

void glyph16_mirror_quadrants_h(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        g->line[row].part.lo = g->line[row].part.hi;
    }
}

void glyph16_mirror_quadrants_v(glyph16x16_std_t *g) {
    if (!g) return;
    for (int row = 0; row < 8; row++) {
        glyph16_set_row(g, row + 8, glyph16_get_row(g, row));
    }
}

/* --- 8x16 glyph operations --- */

void glyph8x16_invert(glyph8x16_t *g) {
    if (!g) return;
    for (int row = 0; row < 16; row++) {
        g->data[row] = (uint8_t)~g->data[row];
    }
}

void glyph8x16_flip_v(glyph8x16_t *g) {
    if (!g) return;
    for (int row = 0; row < 8; row++) {
        uint8_t value = g->data[row];
        g->data[row] = g->data[15 - row];
        g->data[15 - row] = value;
    }
}

void glyph8x16_flip_h(glyph8x16_t *g) {
    if (!g) return;
    for (int row = 0; row < 16; row++) {
        uint8_t value = g->data[row];
        value = (uint8_t)(((value >> 1) & 0x55) | ((value & 0x55) << 1));
        value = (uint8_t)(((value >> 2) & 0x33) | ((value & 0x33) << 2));
        value = (uint8_t)(((value >> 4) & 0x0F) | ((value & 0x0F) << 4));
        g->data[row] = value;
    }
}

void glyph8x16_shift(glyph8x16_t *g, int dx, int dy) {
    if (!g) return;
    dx = clamp_int(dx, -7, 7);
    dy = clamp_int(dy, -15, 15);

    glyph8x16_t result = {0};
    for (int row = 0; row < 16; row++) {
        int target_row = row + dy;
        if (target_row < 0 || target_row >= 16) continue;

        uint8_t value = g->data[row];
        if (dx > 0) {
            value >>= dx;
        } else if (dx < 0) {
            value <<= -dx;
        }
        result.data[target_row] = value;
    }
    *g = result;
}

/* --- 8x8 glyph operations --- */

void glyph8x8_invert(glyph8x8_t *g) {
    if (!g) return;
    for (int row = 0; row < 8; row++) {
        g->data[row] = (uint8_t)~g->data[row];
    }
}

void glyph8x8_flip_v(glyph8x8_t *g) {
    if (!g) return;
    for (int row = 0; row < 4; row++) {
        uint8_t value = g->data[row];
        g->data[row] = g->data[7 - row];
        g->data[7 - row] = value;
    }
}

void glyph8x8_flip_h(glyph8x8_t *g) {
    if (!g) return;
    for (int row = 0; row < 8; row++) {
        uint8_t value = g->data[row];
        value = (uint8_t)(((value >> 1) & 0x55) | ((value & 0x55) << 1));
        value = (uint8_t)(((value >> 2) & 0x33) | ((value & 0x33) << 2));
        value = (uint8_t)(((value >> 4) & 0x0F) | ((value & 0x0F) << 4));
        g->data[row] = value;
    }
}

void glyph8x8_shift(glyph8x8_t *g, int dx, int dy) {
    if (!g) return;
    dx = clamp_int(dx, -7, 7);
    dy = clamp_int(dy, -7, 7);

    glyph8x8_t result = {0};
    for (int row = 0; row < 8; row++) {
        int target_row = row + dy;
        if (target_row < 0 || target_row >= 8) continue;

        uint8_t value = g->data[row];
        if (dx > 0) {
            value >>= dx;
        } else if (dx < 0) {
            value <<= -dx;
        }
        result.data[target_row] = value;
    }
    *g = result;
}

void glyph8x8_rotate_90_cw(glyph8x8_t *g) {
    if (!g) return;
    glyph8x8_t result = {0};
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (g->data[y] & (0x80u >> x)) {
                result.data[x] |= (uint8_t)(0x80u >> (7 - y));
            }
        }
    }
    *g = result;
}

/* --- BDF output --- */

static void sanitize_bdf_name(const char *src, char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    size_t out = 0;
    if (src) {
        for (const char *p = src; *p && out + 1 < dst_size; p++) {
            unsigned char c = (unsigned char)*p;
            dst[out++] = (isalnum(c) || c == '-' || c == '_' || c == '.')
                             ? (char)c
                             : '-';
        }
    }
    dst[out] = '\0';
    if (out == 0) {
        snprintf(dst, dst_size, "EmbFont16");
    }
}

void bdf_print_header(FILE *fp, const char *font_name, int glyph_count) {
    if (!fp) return;
    char name[64];
    sanitize_bdf_name(font_name, name, sizeof(name));
    fprintf(fp, "STARTFONT 2.1\n");
    fprintf(fp,
            "FONT -EmbFont16-%s-Medium-R-Normal--16-150-75-75-C-160-ISO10646-1\n",
            name);
    fprintf(fp, "SIZE 16 75 75\n");
    fprintf(fp, "FONTBOUNDINGBOX 16 16 0 0\n");
    fprintf(fp, "STARTPROPERTIES 4\n");
    fprintf(fp, "FONT_ASCENT 14\n");
    fprintf(fp, "FONT_DESCENT 2\n");
    fprintf(fp, "FAMILY_NAME \"%s\"\n", name);
    fprintf(fp, "COPYRIGHT \"Generated by EmbFont16\"\n");
    fprintf(fp, "ENDPROPERTIES\n");
    fprintf(fp, "CHARS %d\n", glyph_count);
}

void bdf_print_glyph16(FILE *fp, int encoding, const glyph16x16_std_t *g) {
    if (!fp || !g) return;
    fprintf(fp, "STARTCHAR U+%04X\n", encoding);
    fprintf(fp, "ENCODING %d\n", encoding);
    fprintf(fp, "SWIDTH 1000 0\n");
    fprintf(fp, "DWIDTH 16 0\n");
    fprintf(fp, "BBX 16 16 0 0\n");
    fprintf(fp, "BITMAP\n");
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        fprintf(fp, "%04X\n", glyph16_get_row(g, row));
    }
    fprintf(fp, "ENDCHAR\n");
}

void bdf_print_footer(FILE *fp) {
    if (!fp) return;
    fprintf(fp, "ENDFONT\n");
}
