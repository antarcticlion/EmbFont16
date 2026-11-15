#ifndef FONT_TYPES_H
#define FONT_TYPES_H

#include <stdint.h>
#include <stddef.h>

#define GLYPH16_WIDTH        16
#define GLYPH16_HEIGHT       16
#define GLYPH8_WIDTH         8
#define GLYPH8_HEIGHT        8

#define SLICE_BYTES          512
#define GLYPHS_PER_SLICE     (SLICE_BYTES / 32)
#define SLICE_GLYPH_BYTES    32
#define CHUNK_SLICES         6
#define CHUNK_GLYPHS         (CHUNK_SLICES * GLYPHS_PER_SLICE)

/* --- Basic glyph structures --- */

/* 8x8 character (8 bytes, one byte per row) */
typedef struct {
    uint8_t data[8];
} glyph8x8_t;

/* 8x16 character (16 bytes, one byte per row) */
typedef struct {
    uint8_t data[16];
} glyph8x16_t;

/* 16x16 character, PC-98 layout: left 16 bytes followed by right 16 bytes */
typedef struct {
    uint8_t left[16];
    uint8_t right[16];
} glyph16x16_pc98_t;

/* 16x16 character, project layout: one 16-bit word per row.
 * The high byte (part.hi) is the left 8 pixels; the low byte (part.lo)
 * is the right 8 pixels. */
typedef struct {
    union {
        uint16_t full;
        struct {
            uint8_t hi;
            uint8_t lo;
        } part;
    } line[16];
} glyph16x16_std_t;

/* Quadrant order used by the 16x16 helpers. */
typedef enum {
    GLYPH_QUAD_TL = 0,
    GLYPH_QUAD_TR = 1,
    GLYPH_QUAD_BL = 2,
    GLYPH_QUAD_BR = 3
} quadrant_t;

/* --- Row accessors --- */

static inline uint16_t glyph16_get_row(const glyph16x16_std_t *g, int row) {
    return (uint16_t)(((uint16_t)g->line[row].part.hi << 8) |
                      (uint16_t)g->line[row].part.lo);
}

static inline void glyph16_set_row(glyph16x16_std_t *g, int row, uint16_t value) {
    g->line[row].part.hi = (uint8_t)(value >> 8);
    g->line[row].part.lo = (uint8_t)(value & 0xFF);
}

/* --- Slice / chunk storage units --- */

/* A slice is a fixed 512-byte buffer holding 16 PC-98 style glyphs. */
typedef struct {
    uint8_t *data;
    uint32_t crc32;
} slice_t;

/* A chunk groups slices (conventionally 6 slices = 3 KB). */
typedef struct {
    slice_t *slices;
    uint8_t slice_cnt;
    uint8_t slice_type;
    uint8_t slice_subtype;
    uint8_t slice_param;
} chunk_t;

#endif /* FONT_TYPES_H */
