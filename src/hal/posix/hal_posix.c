#include "embfont16.h"
#include <stdio.h>

static FILE* f_rom = NULL;

/**
 * @brief POSIX環境用：ROMファイルの初期化
 */
bool hal_embfont16_posix_init(const char* path) {
    f_rom = fopen(path, "rb");
    return (f_rom != NULL);
}

/**
 * @brief POSIX環境用：ROMファイルのクローズ
 */
void hal_embfont16_posix_close(void) {
    if (f_rom) {
        fclose(f_rom);
        f_rom = NULL;
    }
}

/**
 * @brief 共通HALインターフェースの実装
 */
bool hal_embfont16_read(uint32_t addr, uint8_t *buf, uint32_t size) {
    if (!f_rom) return false;

    if (fseek(f_rom, addr, SEEK_SET) != 0) {
        return false;
    }

    return (fread(buf, 1, size, f_rom) == size);
}
