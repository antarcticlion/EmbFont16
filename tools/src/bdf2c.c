#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 256

int hex2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.bdf>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error opening %s\n", argv[1]);
        return 1;
    }

    // ASCII 0x20(32) - 0x7E(126) -> 95 chars
    // Fixed size 8x16 assumed for labels
    unsigned char font_data[95][16];
    memset(font_data, 0, sizeof(font_data));

    char line[MAX_LINE];
    int current_code = -1;
    int bitmap_row = 0;
    int in_bitmap = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "ENCODING", 8) == 0) {
            current_code = atoi(line + 9);
            in_bitmap = 0;
        } else if (strncmp(line, "BITMAP", 6) == 0) {
            in_bitmap = 1;
            bitmap_row = 0;
        } else if (strncmp(line, "ENDCHAR", 7) == 0) {
            in_bitmap = 0;
            current_code = -1;
        } else if (in_bitmap) {
            if (current_code >= 32 && current_code <= 126 && bitmap_row < 16) {
                // Read 1 byte (2 hex chars) assuming 8px width
                // If the font is wider, we just take the first byte (left part)
                char h = line[0];
                char l = line[1];
                if (isxdigit(h) && isxdigit(l)) {
                    unsigned char val = (hex2int(h) << 4) | hex2int(l);
                    font_data[current_code - 32][bitmap_row] = val;
                }
                bitmap_row++;
            }
        }
    }
    fclose(fp);

    // Output Header Format
    printf("#ifndef LABEL_FONT_H\n");
    printf("#define LABEL_FONT_H\n\n");
	printf("#include <stdint.h>\n\n");
    printf("// Auto-generated from %s by bdf2c\n", argv[1]);
    printf("// ASCII 0x20(' ') to 0x7E('~')\n\n");
    printf("static const uint8_t label_font[95][16] = {\n");

    for (int i = 0; i < 95; i++) {
        printf("    {");
        for (int r = 0; r < 16; r++) {
            printf("0x%02X", font_data[i][r]);
            if (r < 15) printf(",");
        }
        printf("}, // '%c' (0x%02X)\n", (i + 32) == '\\' ? ' ' : (i + 32), i + 32);
    }

    printf("};\n\n");
    printf("#endif // LABEL_FONT_H\n");

    return 0;
}

