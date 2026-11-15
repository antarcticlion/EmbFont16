#include "glyph_manupirate.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* --- Slice conversion --- */

void slice_to_glyph16(const slice_t *s, int index, glyph16x16_std_t *dst) {
    if (!s || !s->data || !dst || index < 0 || index >= GLYPHS_PER_SLICE) {
        return;
    }
    const uint8_t *src = &s->data[index * SLICE_GLYPH_BYTES];
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst->line[row].part.hi = src[row];
        dst->line[row].part.lo = src[row + GLYPH16_HEIGHT];
    }
}

void glyph16_to_slice(const glyph16x16_std_t *src, slice_t *s, int index) {
    if (!src || !s || !s->data || index < 0 || index >= GLYPHS_PER_SLICE) {
        return;
    }
    uint8_t *dst = &s->data[index * SLICE_GLYPH_BYTES];
    for (int row = 0; row < GLYPH16_HEIGHT; row++) {
        dst[row] = src->line[row].part.hi;
        dst[row + GLYPH16_HEIGHT] = src->line[row].part.lo;
    }
}

/* --- CRC32 (IEEE 802.3) --- */

static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

void init_crc32_table(void) {
    if (crc32_table_initialized) return;

    const uint32_t polynomial = 0xEDB88320u;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ polynomial) : (crc >> 1);
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    if (!data) return 0;
    init_crc32_table();

    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (uint8_t)((crc ^ data[i]) & 0xFFu);
        crc = (crc >> 8) ^ crc32_table[index];
    }
    return crc ^ 0xFFFFFFFFu;
}

void update_slice_crc(slice_t *slice) {
    if (slice && slice->data) {
        slice->crc32 = calculate_crc32(slice->data, SLICE_BYTES);
    }
}

/* --- Chunk management --- */

chunk_t *chunk_create(uint8_t slice_cnt) {
    chunk_t *chunk = (chunk_t *)calloc(1, sizeof(chunk_t));
    if (!chunk) return NULL;

    chunk->slice_cnt = slice_cnt;
    if (slice_cnt == 0) return chunk;

    chunk->slices = (slice_t *)calloc(slice_cnt, sizeof(slice_t));
    if (!chunk->slices) {
        free(chunk);
        return NULL;
    }

    for (uint8_t i = 0; i < slice_cnt; i++) {
        chunk->slices[i].data = (uint8_t *)calloc(1, SLICE_BYTES);
        if (!chunk->slices[i].data) {
            chunk_free(chunk);
            return NULL;
        }
    }
    return chunk;
}

void chunk_free(chunk_t *chunk) {
    if (!chunk) return;
    if (chunk->slices) {
        for (uint8_t i = 0; i < chunk->slice_cnt; i++) {
            free(chunk->slices[i].data);
        }
        free(chunk->slices);
    }
    free(chunk);
}

void chunk_foreach_glyph(chunk_t *chunk, int base_index, glyph_callback_t cb,
                         void *user_data) {
    if (!chunk || !chunk->slices || !cb) return;

    int glyph_index = 0;
    for (uint8_t slice_index = 0; slice_index < chunk->slice_cnt;
         slice_index++) {
        if (!chunk->slices[slice_index].data) {
            glyph_index += GLYPHS_PER_SLICE;
            continue;
        }
        for (int i = 0; i < GLYPHS_PER_SLICE; i++) {
            glyph16x16_std_t glyph;
            slice_to_glyph16(&chunk->slices[slice_index], i, &glyph);
            cb(&glyph, base_index + glyph_index, user_data);
            glyph_index++;
        }
    }
}

void slice_foreach_glyph_all(chunk_t *chunk, glyph_callback_t cb,
                             void *user_data) {
    chunk_foreach_glyph(chunk, 0, cb, user_data);
}
