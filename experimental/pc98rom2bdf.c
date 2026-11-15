#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ANK8_GLYPH_SIZE  8   // 8x8 = 64bit = 8bytes
#define ANK16_GLYPH_SIZE 16  // 8x16 = 128bit = 16bytes
#define KANJI_GLYPH_SIZE 32  // 16x16 = 256bit = 32bytes
#define ROM_DATA_OFFSET  0

void write_bdf_header(FILE *fp, int glyph_count) {
    fprintf(fp, "STARTFONT 2.1\n");
    fprintf(fp, "FONT PC98_COMPOSITE_FONT\n");
    fprintf(fp, "SIZE 16 75 75\n");
    fprintf(fp, "FONTBOUNDINGBOX 16 16 0 0\n");
    fprintf(fp, "STARTPROPERTIES 4\n");
    fprintf(fp, "FONT_ASCENT 16\n");
    fprintf(fp, "FONT_DESCENT 0\n");
    fprintf(fp, "CHARSET_REGISTRY \"JISX0208.1983\"\n");
    fprintf(fp, "CHARSET_ENCODING \"0\"\n");
    fprintf(fp, "ENDPROPERTIES\n");
    fprintf(fp, "CHARS %d\n", glyph_count);
}

int main() {
    FILE *rom = fopen("assets/misc/FONT.ROM", "rb");
    if (!rom) { perror("FONT.ROM open error"); return 1; }

    int total_chars = 256 + 256 + (84 * 94);
    write_bdf_header(stdout, total_chars);

    uint8_t glyph[KANJI_GLYPH_SIZE];

    // --- 1. 8x8 ANK領域 (0x00 - 0xFF) ---
    for (int code = 0; code < 256; code++) {
        fseek(rom, ROM_DATA_OFFSET + (code * ANK8_GLYPH_SIZE), SEEK_SET);
        fread(glyph, 1, ANK8_GLYPH_SIZE, rom);

        fprintf(stdout, "STARTCHAR 8x8_%02X\n", code);
        fprintf(stdout, "ENCODING %d\n", code);
        fprintf(stdout, "SWIDTH 500 0\n");
        fprintf(stdout, "DWIDTH 8 0\n");
        fprintf(stdout, "BBX 8 8 4 4\n"); // 中央配置
        fprintf(stdout, "BITMAP\n");
        for (int i = 0; i < 8; i++) fprintf(stdout, "%02X\n", glyph[i]);
        fprintf(stdout, "ENDCHAR\n");
    }

    // --- 2. 8x16 ANK領域 (0x100 - 0x1FF) ---
    long offset_8x16 = ROM_DATA_OFFSET + (256 * ANK8_GLYPH_SIZE);
    for (int code = 0; code < 256; code++) {
        fseek(rom, offset_8x16 + (code * ANK16_GLYPH_SIZE), SEEK_SET);
        fread(glyph, 1, ANK16_GLYPH_SIZE, rom);

        fprintf(stdout, "STARTCHAR 8x16_%02X\n", code);
        fprintf(stdout, "ENCODING %d\n", 0x100 + code);
        fprintf(stdout, "SWIDTH 500 0\n");
        fprintf(stdout, "DWIDTH 8 0\n");
        fprintf(stdout, "BBX 8 16 0 0\n");
        fprintf(stdout, "BITMAP\n");
        for (int i = 0; i < 16; i++) fprintf(stdout, "%02X\n", glyph[i]);
        fprintf(stdout, "ENDCHAR\n");
    }

    // --- 3. 漢字領域 (01区 - 84区) ---
    // 8x8(2KB) + 8x16(4KB) = 6KB の直後からと想定
    long kanji_start_offset = offset_8x16 + (256 * ANK16_GLYPH_SIZE);

    for (int ku = 1; ku <= 84; ku++) {
        for (int ten = 1; ten <= 94; ten++) {
            long offset = kanji_start_offset + (long)(((ku - 1) * 94) + (ten - 1)) * KANJI_GLYPH_SIZE;
            fseek(rom, offset, SEEK_SET);
            if (fread(glyph, 1, KANJI_GLYPH_SIZE, rom) != KANJI_GLYPH_SIZE) break;

            uint16_t encoding = ((ku + 0x20) << 8) | (ten + 0x20);
            fprintf(stdout, "STARTCHAR %04X\n", encoding);
            fprintf(stdout, "ENCODING %d\n", encoding);
            fprintf(stdout, "SWIDTH 1000 0\n");
            fprintf(stdout, "DWIDTH 16 0\n");
            fprintf(stdout, "BBX 16 16 0 0\n");
            fprintf(stdout, "BITMAP\n");
            for (int i = 0; i < 16; i++) fprintf(stdout, "%02X%02X\n", glyph[i * 2 + 1], glyph[i * 2]);
            fprintf(stdout, "ENDCHAR\n");
        }
    }

    fprintf(stdout, "ENDFONT\n");
    fclose(rom);
    return 0;
}