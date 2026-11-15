#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <iconv.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_LINE_LEN 256

// Unicode 変換用関数
uint32_t convert_to_unicode(iconv_t cd, uint32_t code) {
    unsigned char in_buf[4];
    size_t in_bytes = 0;

    // EUC_JISX0213 2面形式 (0x8FA1A1 - 0x8FFEFE) を iconv へ
    if (code >= 0x8FA1A1 && code <= 0x8FFEFE) {
        in_buf[0] = (code >> 16) & 0xFF;
        in_buf[1] = (code >> 8) & 0xFF;
        in_buf[2] = code & 0xFF;
		in_bytes = 3;
	} else
    // JIS形式 (0x2121 - 0x7E7E) を EUC 領域へシフトして iconv へ
    if (code >= 0x2121 && code <= 0x7E7E) {
        in_buf[0] = ((code >> 8) & 0xFF) | 0x80;
        in_buf[1] = (code & 0xFF) | 0x80;
        in_bytes = 2;
    } else {
        if (code > 0xFF) {
            in_buf[0] = (code >> 8) & 0xFF;
            in_buf[1] = code & 0xFF;
            in_bytes = 2;
        } else {
            in_buf[0] = code & 0xFF;
            in_bytes = 1;
        }
    }

    char out_buf[8] = {0};
    char* in_ptr = (char*)in_buf;
    char* out_ptr = out_buf;
    size_t out_len = sizeof(out_buf);

    if (iconv(cd, &in_ptr, &in_bytes, &out_ptr, &out_len) != (size_t)-1) {
        unsigned char* u = (unsigned char*)out_buf;
        size_t len = sizeof(out_buf) - out_len;
        if (len == 1) return u[0];
        if ((u[0] & 0xE0) == 0xC0) return ((u[0]&0x1F)<<6) | (u[1]&0x3F);
        if ((u[0] & 0xF0) == 0xE0) return ((u[0]&0x0F)<<12) | ((u[1]&0x3F)<<6) | (u[2]&0x3F);
        if ((u[0] & 0xF8) == 0xF0) return ((u[0]&0x07)<<18) | ((u[1]&0x3F)<<12)  | ((u[2]&0x3F)<<6)  | (u[3]&0x3F);
    }
    return 0;
}

void recode_bdf(const char* input_file, const char* from_enc, const char* output_file) {
    FILE* fin = fopen(input_file, "r");
    FILE* fout = fopen(output_file, "w");
    if (!fin || !fout) { perror("File open error"); exit(1); }

    const char* target_enc = from_enc;
    if (strcasecmp(from_enc, "ISO-2022-JP") == 0 || strcasecmp(from_enc, "JIS") == 0) {
        target_enc = "EUC-JISX0213";
    }

    iconv_t cd = iconv_open("UTF-8", target_enc);
    if (cd == (iconv_t)-1) {
        cd = iconv_open("UTF-8", "EUC-JP");
        if (cd == (iconv_t)-1) { perror("iconv_open"); exit(1); }
    }

    char line[MAX_LINE_LEN];
    char glyph_data[16384]; // 1グリフ分のバッファ
    size_t glyph_pos = 0;
    uint32_t current_code = 0xFFFFFFFF;
    int success_count = 0;
    bool is_jisx0213 = false;
    bool is_jisx0208 = false;

    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, "CHARSET_REGISTRY", 16) == 0) {
            if (strstr(line, "JISX0213")) is_jisx0213 = true;
            else if (strstr(line, "JISX0208")) is_jisx0208 = true;
            fprintf(fout, "CHARSET_REGISTRY \"ISO10646\"\n");
        } else if (strncmp(line, "CHARSET_ENCODING", 16) == 0) {
            fprintf(fout, "CHARSET_ENCODING \"1\"\n");
        } else if (strncmp(line, "CHARS ", 6) == 0) {
            fputs(line, fout); // 数値は後で gen_map 等が処理
        } else if (strncmp(line, "STARTCHAR", 9) == 0) {
            glyph_pos = 0;
            current_code = 0xFFFFFFFF;
            glyph_pos += snprintf(glyph_data + glyph_pos, sizeof(glyph_data) - glyph_pos, "%s", line);
        } else if (strncmp(line, "ENCODING", 8) == 0) {
            current_code = atoi(line + 9);
            // ここでは出力せず、ENDCHARでまとめて変換・出力する
        } else if (strncmp(line, "ENDCHAR", 7) == 0) {
            if (current_code != 0xFFFFFFFF) {
                uint32_t uni = convert_to_unicode(cd, current_code);
                if (uni > 0) {
                    // STARTCHAR 名の正規化
                    if (is_jisx0213) fprintf(fout, "STARTCHAR U+%04X\n", uni);
                    else if (is_jisx0208) fprintf(fout, "STARTCHAR %04X\n", uni);
                    else fprintf(fout, "STARTCHAR U+%04X\n", uni);

                    fprintf(fout, "ENCODING %u\n", uni);
                    // STARTCHAR行とENCODING行以外をバッファから出力
                    char *p = strchr(glyph_data, '\n'); // 最初の改行（STARTCHARの終わり）を探す
                    if (p) p++;
                    if (p) {
                        // 次の行から出力するが、もしENCODING行が含まれていれば飛ばす
                        char *enc_ptr = strstr(p, "ENCODING");
                        if (enc_ptr) {
                             // ENCODING行を飛ばしてそれ以降を出力
                             char *next_line = strchr(enc_ptr, '\n');
                             if (next_line) {
                                 fwrite(p, 1, enc_ptr - p, fout);
                                 fputs(next_line + 1, fout);
                             }
                        } else {
                             fputs(p, fout);
                        }
                    }
                    fputs(line, fout); // ENDCHAR
                    success_count++;
                }
            }
        } else if (glyph_pos > 0) {
            glyph_pos += snprintf(glyph_data + glyph_pos, sizeof(glyph_data) - glyph_pos, "%s", line);
        } else {
            fputs(line, fout); // ヘッダなど
        }
    }

    iconv_close(cd);
    fclose(fin); fclose(fout);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <input.bdf> <from_enc> <output.bdf>\n", argv[0]);
        return 1;
    }
    recode_bdf(argv[1], argv[2], argv[3]);
    return 0;
}