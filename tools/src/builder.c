#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "embfont16.h"        // ライブラリの公開ヘッダ
#include "embfont16_common.h" // 共通構造体とマクロ
#include "bdf_parser.h"
#include "builder.h"

#define WORK_BUFFER_SIZE 0x400000 
static uint8_t* rom_buffer = NULL;

void load_license_file(uint8_t* buffer, size_t size, const char* path) {
    memset(buffer, 0, size);
    if (!path) return;
    FILE* fp = fopen(path, "rb");
    if (fp) {
        size_t read = fread(buffer, 1, size - 1, fp);
        buffer[read] = '\0';
        fclose(fp);
    }
}

void build_rom(const char* output_filename, const char* license_path, size_t target_size) {
    rom_buffer = (uint8_t*)calloc(1, WORK_BUFFER_SIZE);
    if (!rom_buffer) exit(1);

    uint8_t* table_area = &rom_buffer[TABLE_OFFSET];
    uint8_t* glyph_area = &rom_buffer[GLYPH_AREA_OFFSET];
    uint32_t current_relative_offset = 0;

    for (uint32_t block_id = 0; block_id <= (MAX_CODEPOINT >> 8); block_id++) {
        uint32_t start_code = block_id * 256;
        int min_line = 16, max_line = -1;
        bool has_any_glyph = false;

        for (int i = 0; i < 256; i++) {
            RawGlyph* g = &g_glyphs[start_code + i];
            if (!g->exists) continue;
            has_any_glyph = true;

            for (int l = 0; l < 16; l++) {
                bool row_has_data = false;
                if (g->half) {
                    if (g->data[l] != 0) row_has_data = true;
                } else {
                    if (g->data[l*2] != 0 || g->data[l*2+1] != 0) row_has_data = true;
                }
                if (row_has_data) {
                    if (l < min_line) min_line = l;
                    if (l > max_line) max_line = l;
                }
            }
        }

        BlockTableEntry entry = {0};
        entry.block_id = (uint16_t)block_id;

        if (!has_any_glyph) {
            entry.glyph_offset = INVALID_OFFSET;
            entry.line_value = 0;
        } else {
            if (max_line == -1) { min_line = 0; max_line = 0; }
            uint8_t line_count = (uint8_t)(max_line - min_line + 1);
            entry.line_value = (min_line & START_LINE_OFFSET) | (((line_count - 1) & 0x0F) << 4);
            
            uint32_t block_size = 256 * RECORD_SIZE; 
            entry.glyph_offset = current_relative_offset;

            for (int i = 0; i < 256; i++) {
                RawGlyph* src = &g_glyphs[start_code + i];
                uint8_t* dest_record = &glyph_area[current_relative_offset + (i * RECORD_SIZE)];

                if (src->exists) {
                    dest_record[0] = 1;         // exists
                    dest_record[1] = src->half; // half flag
                    dest_record[2] = src->font_id & FONT_ID_MASK;
                    
                    for (int l = 0; l < line_count; l++) {
                        int src_row = min_line + l;
                        uint8_t* dest_row = &dest_record[4 + (l * 2)];
                        if (src->half) {
                            dest_row[0] = src->data[src_row];
                            dest_row[1] = 0;
                        } else {
                            dest_row[0] = src->data[src_row * 2];
                            dest_row[1] = src->data[src_row * 2 + 1];
                        }
                    }
                }
            }
            current_relative_offset += block_size;
        }
        memcpy(table_area + (block_id * sizeof(BlockTableEntry)), &entry, sizeof(BlockTableEntry));
    }

    for (uint32_t i = 0; i < g_font_count && i < FONT_NAME_MAX_ENTRIES; i++) {
        size_t name_len = strlen(g_font_names[i]);
        if (name_len >= FONT_NAME_ENTRY_SIZE) name_len = FONT_NAME_ENTRY_SIZE - 1;
        memcpy(&rom_buffer[FONT_NAME_AREA_OFFSET + (i * FONT_NAME_ENTRY_SIZE)],
               g_font_names[i], name_len);
    }

    FontHeader header = {0};
    header.magic = 0x544E4F46; 
    header.version = FORMAT_VERSION;
    header.char_count = g_total_chars;
    header.credits_offset = CREDITS_OFFSET;
    header.credits_size = CREDITS_SIZE;
    header.table_offset = TABLE_OFFSET;
    header.table_size = TABLE_SIZE;
    header.reserved_area_offset = RESERVED_OFFSET;
    header.reserved_area_size = RESERVED_SIZE;
    header.glyph_area_offset = GLYPH_AREA_OFFSET;
    header.glyph_area_size = current_relative_offset;
    memcpy(&rom_buffer[HEADER_OFFSET], &header, sizeof(header));
    load_license_file(&rom_buffer[CREDITS_OFFSET], CREDITS_SIZE, license_path);

    FILE* fp = fopen(output_filename, "wb");
    if (fp) {
        size_t actual_size = GLYPH_AREA_OFFSET + current_relative_offset;
        fwrite(rom_buffer, 1, actual_size, fp);

        if (target_size > actual_size) {
            size_t pad_len = target_size - actual_size;
            uint8_t* pad_buf = (uint8_t*)calloc(1, 1024);
            if (pad_buf) {
                while (pad_len > 0) {
                    size_t chunk = (pad_len > 1024) ? 1024 : pad_len;
                    fwrite(pad_buf, 1, chunk, fp);
                    pad_len -= chunk;
                }
                free(pad_buf);
            }
        }
        fclose(fp);

		// --- 使用効率の統計表示 ---
        double glyph_usage = ((double)current_relative_offset / GLYPH_AREA_SIZE) * 100.0;
        // 追加：引数(target_size)に対する全体の使用率
        double total_usage = ((double)actual_size / target_size) * 100.0;

        printf("\n--- Build Statistics ---\n");
        printf("Output File:      %s\n", output_filename); 
        printf("Target ROM Size:  %zu bytes\n", target_size); 
        printf("Header & Table:   %d bytes\n", GLYPH_AREA_OFFSET);
        printf("Glyph Data Area:  %u / %d bytes (%.2f%% used)\n", 
                current_relative_offset, GLYPH_AREA_SIZE, glyph_usage); 
        printf("Total Actual:     %zu / %zu bytes (%.2f%% used)\n", 
                actual_size, target_size, total_usage); 
        printf("Free Space:       %zd bytes\n", (size_t)target_size - (size_t)actual_size); 
        
        if (actual_size > target_size) {
            printf("WARNING: Actual size exceeds target size by %zu bytes!\n", actual_size - target_size);
        }
        printf("------------------------\n");
    }
    free(rom_buffer);
}

