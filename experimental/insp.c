#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ANK8_SIZE    8   // 8x8
#define ANK16_SIZE   16  // 8x16
#define KANJI_HALF   16  // 8x16 (漢字の半分)

void write_bdf_header(FILE *fp, int glyph_count) {
    fprintf(fp, "STARTFONT 2.1\n");
    fprintf(fp, "FONT PC98_FONT_DECODE\n");
    fprintf(fp, "SIZE 16 75 75\n");
    fprintf(fp, "FONT_ASCENT 16\n");
    fprintf(fp, "FONT_DESCENT 0\n");
    fprintf(fp, "FONTBOUNDINGBOX 16 16 0 -2\n");
    fprintf(fp, "CHARSET_REGISTRY \"JISX0208.1983\"\n");
    fprintf(fp, "CHARSET_ENCODING \"0\"\n");
    fprintf(fp, "CHARS %d\n", glyph_count);
//    fprintf(fp, "CHARS 9999\n");
}


static inline bool is_blank_8(uint8_t *value){
	uint8_t count = 0;
	for(int i=0; i<16; i++){
		count += (value[i]?1:0);
	}
	return(count?0:1);
}

static inline bool is_blank_16(uint8_t *upper, uint8_t *lower){
	uint8_t count = 0;
	for(int i=0; i<16; i++){
		count += (upper[i]?1:0)+(lower[i]?1:0);
	}
	return(count?0:1);
}




int main() {
    FILE *rom = fopen("assets/misc/FONT.ROM", "rb");
    if (!rom) { perror("FONT.ROM open error"); return 1; }

    // ANK8(256) + ANK16(256) + 非漢字(16文字*49行=784文字)
    int non_kanji_count = 16 * 48;
    int total_chars = 256 + 256 + non_kanji_count;

    write_bdf_header(stdout, total_chars);

    uint8_t glyph_l[16], glyph_r[16];
    
    uint32_t code_point = 0;
    uint32_t code_addr = 0;
    uint32_t glyph_total = 0;

	fprintf(stderr, "[DEBUG] FONT START\n");

    // 1. 8x8 ANK (0x0000 - 0x00FF)
    FILE *ank8 = fopen("output/work/pc98_font_ank8x8.BDF", "wb");
    if (!ank8) { perror("ANK8 open error"); return 1; }
	write_bdf_header(ank8, 256);
    for (int i = 0; i < 256; i++) {
        fread(glyph_l, 1, 8, rom);
        fprintf(ank8, "STARTCHAR ANK8_%02X\nENCODING %d\n", code_point, code_point);
        fprintf(ank8, "DWIDTH 8 0\nBBX 8 8 0 8\nBITMAP\n");
        for (int j = 0; j < 8; j++) fprintf(ank8, "%02X\n", glyph_l[j]);
        fprintf(ank8, "ENDCHAR\n");
        code_point++;
        code_addr += 8;
    }
	fclose(ank8);
	fprintf(stderr, "[DEBUG] ANK8x8 ～0x&%08X \n", code_addr);

    code_point = 0;

    // 2. 8x16 ANK (0x0100 - 0x01FF)
    FILE *ank16 = fopen("output/work/pc98_font_ank8x16.BDF", "wb");
    if (!ank16) { perror("ANK16 open error"); return 1; }
	write_bdf_header(ank8, 256);
    for (int i = 0; i < 256; i++) {
        fread(glyph_l, 1, 16, rom);
        fprintf(ank16, "STARTCHAR ANK16_%02X\nENCODING %d\n", code_point, code_point);
        fprintf(ank16, "DWIDTH 8 0\nBBX 8 16 0 0\nBITMAP\n");
        for (int j = 0; j < 16; j++) fprintf(ank16, "%02X\n", glyph_l[j]);
        fprintf(ank16, "ENDCHAR\n");
        code_point++;
        code_addr += 16;
    }
	fclose(ank16);
	fprintf(stderr, "[DEBUG] ANK8x16 ～0x&%08X \n", code_addr);









    FILE *kanji16 = fopen("output/work/pc98_font_kanji.BDF", "wb");
    if (!kanji16) { perror("KANAJ open error"); return 1; }
    write_bdf_header(kanji16, 9999);
    // 3. 非漢字 16x16 エリア １区～８区
    // 「左半分(16byte)・右半分(16byte)」が交互に並んでいる想定で合成

    for (int ku_loop = 1; ku_loop < 9; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
			if(is_blank_16(glyph_l, glyph_r)?(code_point==0x2121?1:0):1){
		        fprintf(kanji16, "STARTCHAR ANK_16x16_%04X\n", code_point);
		        fprintf(kanji16, "ENCODING %d\n", code_point);
		        fprintf(kanji16, "SWIDTH 1000 0\nDWIDTH 16 0\nBBX 16 16 0 0\nBITMAP\n");
		        for (int j = 0; j < 16; j++) {
		            // 左8bitと右8bitを結合して16進数4桁で出力
		            fprintf(kanji16, "%02X%02X\n", glyph_l[j], glyph_r[j]);
		        }
		        fprintf(kanji16, "ENDCHAR\n");
		        glyph_total ++;
		    }
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 非漢字 %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

    // 2. 9区～11区 左半分だけつかうANK 右半分は読み捨て
    for (int ku_loop = 9; ku_loop < 12; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
//			if(is_blank_8(glyph_l)?0:1){
			if(is_blank_8(glyph_l)?0:1){
		        fprintf(kanji16, "STARTCHAR ANK_8x16_%04X\n", code_point);
		        fprintf(kanji16, "ENCODING %d\n", code_point);
		        fprintf(kanji16, "SWIDTH 1000 0\nDWIDTH 16 0\nBBX 16 16 0 0\nBITMAP\n");
		        for (int j = 0; j < 16; j++) {
		            // 左8bitと右8bitを結合して16進数4桁で出力
		            fprintf(kanji16, "%02X%02X\n", glyph_l[j], glyph_r[j]);
		        }
		        fprintf(kanji16, "ENDCHAR\n");
		        glyph_total ++;
		    }
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 左半分 ANK8x16 %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

	// 12区 罫線 13区 NEC特殊文字
    for (int ku_loop = 12; ku_loop < 14; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
			if(is_blank_16(glyph_l, glyph_r)?(code_point==0x2121?1:0):1){
		        fprintf(kanji16, "STARTCHAR ANK_16x16_%04X\n", code_point);
		        fprintf(kanji16, "ENCODING %d\n", code_point);
		        fprintf(kanji16, "SWIDTH 1000 0\nDWIDTH 16 0\nBBX 16 16 0 0\nBITMAP\n");
		        for (int j = 0; j < 16; j++) {
		            // 左8bitと右8bitを結合して16進数4桁で出力
		            fprintf(kanji16, "%02X%02X\n", glyph_l[j], glyph_r[j]);
		        }
		        fprintf(kanji16, "ENDCHAR\n");
		        glyph_total ++;
		    }
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 非漢字 %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

	// 14-15区 読み捨て
    for (int ku_loop = 14; ku_loop < 16; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
			if(is_blank_16(glyph_l, glyph_r)?(code_point==0x2121?1:0):1){
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 読み捨て %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

	//code_point = 0x3020;
	//第一水準漢字 16-47区 第二水準 48-84区 (84区は豆腐)
    for (int ku_loop = 16; ku_loop < 85; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
			if(is_blank_16(glyph_l, glyph_r)?(code_point==0x2121?1:0):1){
		        fprintf(kanji16, "STARTCHAR KANJI_%04X\n", code_point);
		        fprintf(kanji16, "ENCODING %d\n", code_point);
		        fprintf(kanji16, "SWIDTH 1000 0\nDWIDTH 16 0\nBBX 16 16 0 0\nBITMAP\n");
		        for (int j = 0; j < 16; j++) {
		            // 左8bitと右8bitを結合して16進数4桁で出力
		            fprintf(kanji16, "%02X%02X\n", glyph_l[j], glyph_r[j]);
		        }
		        fprintf(kanji16, "ENDCHAR\n");
		        glyph_total ++;
		    }
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 漢字 %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

	// 4区分未収録　読み捨て (85-88)
    for (int ku_loop = 85; ku_loop < 89; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] 読み捨て %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

	// NEC選定IBM拡張文字 (89-92)
    for (int ku_loop = 89; ku_loop < 93; ku_loop++) {
		code_point = 0x2020 + ku_loop * 0x0100;
	    for (int i = 0; i < 96; i++) {
	        if (fread(glyph_l, 1, 16, rom) != 16) break;
	        if (fread(glyph_r, 1, 16, rom) != 16) break;
			if(is_blank_16(glyph_l, glyph_r)?(code_point==0x2121?1:0):1){
		        fprintf(kanji16, "STARTCHAR KANJI_%04X\n", code_point);
		        fprintf(kanji16, "ENCODING %d\n", code_point);
		        fprintf(kanji16, "SWIDTH 1000 0\nDWIDTH 16 0\nBBX 16 16 0 0\nBITMAP\n");
		        for (int j = 0; j < 16; j++) {
		            // 左8bitと右8bitを結合して16進数4桁で出力
		            fprintf(kanji16, "%02X%02X\n", glyph_l[j], glyph_r[j]);
		        }
		        fprintf(kanji16, "ENDCHAR\n");
		        glyph_total ++;
		    }
	        code_point++;
	        code_addr += 16;
	    }
		fprintf(stderr, "[DEBUG] NEC選定IBM拡張文字 %d区 / Code point 0x%04X / Offset ～0x&%08X / total glyphs %d \n", ku_loop, code_point, code_addr, glyph_total);
    }

    fprintf(kanji16, "ENDFONT\n");
    fclose(kanji16);
    fclose(rom);
    return 0;
}