#ifndef EMBFONT16_H
#define EMBFONT16_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t cp;
    bool     is_half;
    uint8_t  start_line;
    uint8_t  line_count;
    uint8_t  font_id;
    uint8_t  bitmap[32];
} embfont16_glyph_t;

/**
 * @brief 1ピクセル描画用のコールバック関数
 * @param x, y 描画座標
 * @param color 色指定
 * @param user_data ユーザー定義データ（ディスプレイオブジェクトなど）
 */
typedef void (*embfont16_draw_pixel_t)(int16_t x, int16_t y, uint32_t color, void *user_data);

/**
 * @brief 指定した座標に文字列を描画する
 */
void embfont16_draw_string(int16_t x, int16_t y, const char *utf8_str, uint32_t color, 
                            embfont16_draw_pixel_t draw_fn, void *user_data);

bool embfont16_get_glyph(uint32_t cp, embfont16_glyph_t *glyph);
bool embfont16_get_font_name(uint8_t font_id, char *name, uint32_t name_size);

/**
 * @brief フォントIDに対応する表示名へのポインタを返す
 *
 * ROM内のフォント名領域（64バイト×16エントリ）を読み、静的バッファに
 * NUL終端文字列として格納して返す。戻り値は次にこの関数を呼ぶまで有効。
 * スレッドセーフではない。コピーが必要な場合は
 * embfont16_get_font_name() を使う。
 *
 * @param font_id グリフレコードの font_id（0-15）
 * @return フォント名。未定義のID、空の名前、ROM読み出し失敗時は NULL
 */
const char *embfont16_get_font_name_ptr(uint8_t font_id);

extern bool hal_embfont16_read(uint32_t addr, uint8_t *buf, uint32_t size);



#endif