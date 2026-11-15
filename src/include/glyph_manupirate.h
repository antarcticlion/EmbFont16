#ifndef GLYPH_MANUPIRATE_H
#define GLYPH_MANUPIRATE_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "font_types.h"

/* --- 16x16 glyph operations --- */
void glyph16_invert(glyph16x16_std_t *g);
void glyph16_flip_h(glyph16x16_std_t *g);
void glyph16_flip_v(glyph16x16_std_t *g);
void glyph16_shift(glyph16x16_std_t *g, int dx, int dy);
void glyph16_rotate_90_cw(glyph16x16_std_t *g);

/* Half extraction and insertion */
void glyph16_get_left_half(const glyph16x16_std_t *src, glyph8x16_t *dst);
void glyph16_get_right_half(const glyph16x16_std_t *src, glyph8x16_t *dst);
void glyph16_set_left_half(glyph16x16_std_t *dst, const glyph8x16_t *src);
void glyph16_set_right_half(glyph16x16_std_t *dst, const glyph8x16_t *src);
void glyph16_swap_halves(glyph16x16_std_t *g);

/* Quadrant access and rearrangement */
void glyph16_get_quadrant(const glyph16x16_std_t *src, glyph8x8_t *dst,
                          quadrant_t quad);
void glyph16_set_quadrant(glyph16x16_std_t *dst, const glyph8x8_t *src,
                          quadrant_t quad);
void glyph16_swap_quadrants(glyph16x16_std_t *g, quadrant_t q1, quadrant_t q2);
void glyph16_mirror_quadrants_h(glyph16x16_std_t *g);
void glyph16_mirror_quadrants_v(glyph16x16_std_t *g);

/* --- 8x16 glyph operations --- */
void glyph8x16_invert(glyph8x16_t *g);
void glyph8x16_flip_v(glyph8x16_t *g);
void glyph8x16_flip_h(glyph8x16_t *g);
void glyph8x16_shift(glyph8x16_t *g, int dx, int dy);

/* --- 8x8 glyph operations --- */
void glyph8x8_invert(glyph8x8_t *g);
void glyph8x8_flip_v(glyph8x8_t *g);
void glyph8x8_flip_h(glyph8x8_t *g);
void glyph8x8_shift(glyph8x8_t *g, int dx, int dy);
void glyph8x8_rotate_90_cw(glyph8x8_t *g);

/* --- BDF output (host-side convenience) --- */
void bdf_print_header(FILE *fp, const char *font_name, int glyph_count);
void bdf_print_glyph16(FILE *fp, int encoding, const glyph16x16_std_t *g);
void bdf_print_footer(FILE *fp);

/* --- Slice conversion and CRC32 --- */
void slice_to_glyph16(const slice_t *s, int index, glyph16x16_std_t *dst);
void glyph16_to_slice(const glyph16x16_std_t *src, slice_t *s, int index);
void init_crc32_table(void);
uint32_t calculate_crc32(const uint8_t *data, size_t length);
void update_slice_crc(slice_t *slice);

static inline glyph16x16_std_t *get_glyph16_from_slice(slice_t *s, int index) {
    if (!s || !s->data || index < 0 || index >= GLYPHS_PER_SLICE) {
        return NULL;
    }
    return &((glyph16x16_std_t *)s->data)[index];
}

/* --- Chunk management --- */
chunk_t *chunk_create(uint8_t slice_cnt);
void chunk_free(chunk_t *c);

/* Callback index is the glyph index within the chunk, plus the caller's
 * base_index. A chunk with 6 slices visits 96 glyphs. */
typedef void (*glyph_callback_t)(glyph16x16_std_t *g, int index,
                                 void *user_data);

void chunk_foreach_glyph(chunk_t *c, int base_index, glyph_callback_t cb,
                         void *user_data);
void slice_foreach_glyph_all(chunk_t *c, glyph_callback_t cb, void *user_data);

#endif /* GLYPH_MANUPIRATE_H */
