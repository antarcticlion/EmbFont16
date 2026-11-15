#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builder.h"
#include "bdf_parser.h" // 【修正】これを追加

int main(int argc, char* argv[]) {
    char* input_bdf = NULL;
    char* output_bin = NULL;
    char* license_path = NULL;
    size_t target_size = 0; // 0なら可変長

    // 引数解析
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                license_path = argv[++i];
            } else {
                fprintf(stderr, "Error: -l option requires a filename.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-s") == 0) { // サイズ指定オプション
            if (i + 1 < argc) {
                // 10進数、16進数(0x)両対応
                target_size = strtoul(argv[++i], NULL, 0);
            }
        } else if (!input_bdf) {
            input_bdf = argv[i];
        } else if (!output_bin) {
            output_bin = argv[i];
        }
    }

    if (!input_bdf) {
        fprintf(stderr, "Usage: %s <input.bdf> [output.bin] [-l license.txt] [-s size_in_bytes]\n", argv[0]);
        return 1;
    }

    if (!output_bin) {
        output_bin = "font_rom.bin";
    }

    // 1. BDF読み込み
    parse_bdf(input_bdf);

    // 2. ROM生成 (ライセンスパスとサイズを渡す)
    build_rom(output_bin, license_path, target_size);

    return 0;
}

