#include <stdio.h>
#include <stdlib.h>
#include "embfont16.h"
#include "embfont16_common.h"

// posix用の初期化関数のプロトタイプ宣言
bool hal_embfont16_posix_init(const char* path);
void hal_embfont16_posix_close(void);

void render_glyph(uint32_t code) {
    embfont16_glyph_t glyph;
    
    if (!embfont16_get_glyph(code, &glyph)) {
        printf("U+%05X: Not found\n", code);
        return;
    }

    char font_name[FONT_NAME_ENTRY_SIZE] = "";
    embfont16_get_font_name(glyph.font_id, font_name, sizeof(font_name));
    printf("--- U+%05X (%s, Rows: %d-%d, Font: %u %s) ---\n",
           code, glyph.is_half ? "Half" : "Full",
           glyph.start_line, glyph.start_line + glyph.line_count - 1,
           glyph.font_id, font_name);

    for (int y = 0; y < 16; y++) {
        printf("%02X: ", y);
        if (y < glyph.start_line || y >= (glyph.start_line + glyph.line_count)) {
            printf(glyph.is_half ? "........\n" : "................\n");
            continue;
        }

        const uint8_t* row_data = &glyph.bitmap[(y - glyph.start_line) * 2];
        for (int i = 7; i >= 0; i--) printf((row_data[0] >> i) & 1 ? "##" : "..");
        if (!glyph.is_half) {
            for (int i = 7; i >= 0; i--) printf((row_data[1] >> i) & 1 ? "##" : "..");
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    if (!hal_embfont16_posix_init(argv[1])) {
        fprintf(stderr, "Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        render_glyph((uint32_t)strtoul(argv[i], NULL, 16));
    }

    hal_embfont16_posix_close();
    return 0;
}