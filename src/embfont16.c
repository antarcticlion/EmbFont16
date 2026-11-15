#include "embfont16.h"
#include "embfont16_common.h"
#include <string.h>

bool embfont16_get_glyph(uint32_t cp, embfont16_glyph_t *glyph) {
    if (!glyph || cp > MAX_CODEPOINT) return false;

    // 1. ブロックテーブルからエントリを取得
    BlockTableEntry entry;
    uint32_t block_id = (cp >> 8) & 0x3FF;
    uint32_t entry_addr = TABLE_OFFSET + (block_id * sizeof(BlockTableEntry));

    if (!hal_embfont16_read(entry_addr, (uint8_t *)&entry, sizeof(BlockTableEntry))) {
        return false;
    }

    if (entry.glyph_offset == INVALID_OFFSET) return false;

    // 2. 文字レコードのヘッダを読み出し (exists, half フラグ)
    uint32_t char_idx = cp & 0xFF;
    uint32_t char_addr = GLYPH_AREA_OFFSET + entry.glyph_offset + (char_idx * RECORD_SIZE);

    uint8_t header[4];
    if (!hal_embfont16_read(char_addr, header, 4)) return false;
    if (header[0] == 0) return false; // exists == 0

    // 3. line_value から配置情報を展開
    glyph->cp = cp;
    glyph->is_half = (header[1] == 1);
    glyph->font_id = header[2] & FONT_ID_MASK;
    glyph->start_line = GET_START_LINE_OFFSET(entry.line_value);
    glyph->line_count = GET_LINES_AVAILABLE(entry.line_value);

    // 4. パッキングされたビットマップ実体を読み出し (常に2バイト幅)
    memset(glyph->bitmap, 0, sizeof(glyph->bitmap));
    if (!hal_embfont16_read(char_addr + 4, glyph->bitmap, glyph->line_count * 2)) {
        return false;
    }

    return true;
}

bool embfont16_get_font_name(uint8_t font_id, char *name, uint32_t name_size) {
    if (!name || name_size == 0) return false;
    name[0] = '\0';
    if (font_id >= FONT_NAME_MAX_ENTRIES) return false;

    uint8_t entry[FONT_NAME_ENTRY_SIZE];
    uint32_t addr = FONT_NAME_AREA_OFFSET + (font_id * FONT_NAME_ENTRY_SIZE);
    if (!hal_embfont16_read(addr, entry, sizeof(entry))) return false;

    uint32_t i = 0;
    while (i < sizeof(entry) && entry[i] != '\0' && (i + 1) < name_size) {
        name[i] = (char)entry[i];
        i++;
    }
    name[i] = '\0';
    return true;
}

const char *embfont16_get_font_name_ptr(uint8_t font_id) {
    static char name[FONT_NAME_ENTRY_SIZE];

    if (font_id >= FONT_NAME_MAX_ENTRIES) return NULL;

    uint8_t entry[FONT_NAME_ENTRY_SIZE];
    uint32_t addr = FONT_NAME_AREA_OFFSET + (font_id * FONT_NAME_ENTRY_SIZE);
    if (!hal_embfont16_read(addr, entry, sizeof(entry))) return NULL;

    uint32_t len = 0;
    while (len < sizeof(entry) && entry[len] != '\0') {
        len++;
    }
    if (len == 0) return NULL;

    if (len >= sizeof(name)) len = sizeof(name) - 1;
    memcpy(name, entry, len);
    name[len] = '\0';
    return name;
}


/**
 * @brief UTF-8 文字列から次の Unicode コードポイントを取得する
 * @param str 文字列ポインタのポインタ（処理後に次の文字へ進む）
 * @return Unicode コードポイント。不正なシーケンスの場合は U+FFFD
 */
static uint32_t utf8_invalid(const char **str) {
    (*str)++;
    return 0xFFFD; // U+FFFD REPLACEMENT CHARACTER
}

static uint32_t embfont16_decode_utf8(const char **str) {
    if (!str || !*str) return 0;

    const uint8_t *s = (const uint8_t *)*str;
    uint32_t cp;

    if (s[0] < 0x80) {
        *str = (const char *)(s + 1);
        return s[0];
    }

    if ((s[0] & 0xE0) == 0xC0) {
        if (s[1] == 0 || (s[1] & 0xC0) != 0x80) return utf8_invalid(str);
        cp = ((uint32_t)(s[0] & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
        if (cp < 0x80) return utf8_invalid(str);
        *str = (const char *)(s + 2);
        return cp;
    }

    if ((s[0] & 0xF0) == 0xE0) {
        if (s[1] == 0 || s[2] == 0 ||
            (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) {
            return utf8_invalid(str);
        }
        cp = ((uint32_t)(s[0] & 0x0F) << 12) |
             ((uint32_t)(s[1] & 0x3F) << 6) |
             (uint32_t)(s[2] & 0x3F);
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return utf8_invalid(str);
        }
        *str = (const char *)(s + 3);
        return cp;
    }

    if ((s[0] & 0xF8) == 0xF0) {
        if (s[1] == 0 || s[2] == 0 || s[3] == 0 ||
            (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 ||
            (s[3] & 0xC0) != 0x80) {
            return utf8_invalid(str);
        }
        cp = ((uint32_t)(s[0] & 0x07) << 18) |
             ((uint32_t)(s[1] & 0x3F) << 12) |
             ((uint32_t)(s[2] & 0x3F) << 6) |
             (uint32_t)(s[3] & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return utf8_invalid(str);
        *str = (const char *)(s + 4);
        return cp;
    }

    return utf8_invalid(str);
}


void embfont16_draw_string(int16_t x, int16_t y, const char *utf8_str, uint32_t color, 
                            embfont16_draw_pixel_t draw_fn, void *user_data) {
    if (!draw_fn) return;

    int16_t cur_x = x;
    uint32_t cp;

    while ((cp = embfont16_decode_utf8(&utf8_str)) != 0) {
        embfont16_glyph_t glyph;
        if (!embfont16_get_glyph(cp, &glyph)) {
            // グリフが見つからない場合は半角スペース分進める（または代替文字）
            cur_x += 8;
            continue;
        }

        // 16行の描画ループ
        for (int row = 0; row < 16; row++) {
            // ブロック共通の有効範囲外ならスキップ
            if (row < glyph.start_line || row >= (glyph.start_line + glyph.line_count)) {
                continue;
            }

            const uint8_t *row_ptr = &glyph.bitmap[(row - glyph.start_line) * 2];
            
            // 左半分 (8px)
            for (int b = 0; b < 8; b++) {
                if (row_ptr[0] & (0x80 >> b)) {
                    draw_fn(cur_x + b, y + row, color, user_data);
                }
            }
            // 右半分 (全角の場合のみ 8px)
            if (!glyph.is_half) {
                for (int b = 0; b < 8; b++) {
                    if (row_ptr[1] & (0x80 >> b)) {
                        draw_fn(cur_x + 8 + b, y + row, color, user_data);
                    }
                }
            }
        }
        // カーソルを進める（半角=8, 全角=16）
        cur_x += (glyph.is_half ? 8 : 16);
    }
}

