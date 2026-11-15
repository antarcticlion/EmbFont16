#ifndef BUILDER_H
#define BUILDER_H

#include <stddef.h> // size_t用

/**
 * ROM生成のメイン関数
 * @param output_filename 出力ファイルパス
 * @param license_path    ライセンスファイルパス (NULL可)
 * @param target_size     出力固定サイズ (バイト)。0の場合はデータ実サイズで出力。
 */
void build_rom(const char* output_filename, const char* license_path, size_t target_size);

#endif // BUILDER_H