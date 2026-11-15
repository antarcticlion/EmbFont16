#ifndef EMBFONT16_COMMON_H
#define EMBFONT16_COMMON_H

#include <stdint.h>
#include <stdbool.h>

// --- Memory Map Configuration ---
#define TOTAL_SIZE          0x400000 //4MB
//#define TOTAL_SIZE          0x300000 //3MB

// ROM format version (bumped when the memory map changes)
#define FORMAT_VERSION      0x00020000

// 1. Header (256B)
#define HEADER_OFFSET       0x000000
#define HEADER_SIZE         0x000100

// 2. Credits (8KB-256B)
#define CREDITS_OFFSET      0x000100
#define CREDITS_SIZE        0x001F00  // End at 0x2000

// 3. Block Table
// Range: 0x000000 - 0x03FFFF
// Blocks: 262,144 chars / 256 chars-per-block = 1024 blocks
// Size: 1024 blocks * 8 bytes/entry = 8192 bytes (0x2000, 8KB)
#define TABLE_OFFSET        0x002000
#define TABLE_SIZE          0x002000

// 4. Reserved Area
// Start: 0x004000, End: 0x007FFF
// Size:  0x008000 - 0x004000 = 0x4000 (16KB)
#define RESERVED_OFFSET     (TABLE_OFFSET + TABLE_SIZE)
#define RESERVED_SIZE       (GLYPH_AREA_OFFSET - RESERVED_OFFSET)

#define FONT_NAME_AREA_OFFSET  RESERVED_OFFSET
#define FONT_NAME_ENTRY_SIZE   64
#define FONT_NAME_MAX_ENTRIES  16
#define FONT_NAME_AREA_SIZE    (FONT_NAME_ENTRY_SIZE * FONT_NAME_MAX_ENTRIES)
#define FONT_ID_MASK           0x0F

// 5. Glyph Data Area
// Starts at 0x008000; extends to the end of the 4MB image.
#define GLYPH_AREA_OFFSET   0x008000
#define GLYPH_AREA_SIZE     (TOTAL_SIZE - GLYPH_AREA_OFFSET)

// --- Constants ---
#define INVALID_OFFSET      0xFFFFFFFF
#define START_LINE_OFFSET   0x0F
#define LINES_AVAILABLE     0xF0 // High nibble: (line count - 1)

#define GET_START_LINE_OFFSET(value)  ((value) & START_LINE_OFFSET)
#define GET_LINES_AVAILABLE(value)    ((((value) & LINES_AVAILABLE) >> 4) + 1)

// Supported range: U+00000 - U+3FFFF
#define MAX_CODEPOINT       0x3FFFF
#define BLOCK_CHAR_COUNT    256


#define RECORD_SIZE         sizeof(RawGlyph) // 36 bytes
#define BLOCK_DATA_SIZE     (BLOCK_CHAR_COUNT * RECORD_SIZE) // 9216 bytes

// --- Shared Structures ---

// Glyph data structure (used by gen_map, bdf_parser, builder)
typedef struct {
    uint8_t exists;      // 0 = missing, 1 = present
    uint8_t half;        // 0 = 16 pixels wide, 1 = 8 pixels wide
    uint8_t font_id;
    uint8_t reserved_4;
    uint8_t data[32];    // Max 32 bytes (16x16 bitmap)
} RawGlyph; // 36 bytes
            // 1 block = 36 bytes x 256 = 9KB
            // max blocks = (4MB - 32KB) / 9KB = 451



// ROM Table Entry (8 bytes)
typedef struct {
    uint32_t glyph_offset;   		// Offset relative to GLYPH_AREA_OFFSET
    uint16_t block_id;       		// 0x0000 - 0xFFFF (Supports full 0xFFFFFF range)
    uint8_t  line_value;	        // 上位4ビットが(行数-1)、下位4ビットが開始行。1行は16ドット
    uint8_t  reserved;              // TBD 
} __attribute__((packed)) BlockTableEntry;



// ROM Header
typedef struct {
    uint32_t magic;           // "FONT"
    uint32_t version;         // 0x00020000
    uint32_t char_count;
    uint32_t reserved1;

    uint8_t  font_name[64];

    uint32_t credits_offset;
    uint32_t credits_size;
    
    uint32_t table_offset;
    uint32_t table_size;

    uint32_t reserved_area_offset;
    uint32_t reserved_area_size;

    uint32_t glyph_area_offset;
    uint32_t glyph_area_size;
} __attribute__((packed)) FontHeader;

// These sizes are part of the on-ROM format. Keep them in sync with
// docs/rom_format.md.
typedef char embfont16_rawglyph_size_check[(sizeof(RawGlyph) == 36) ? 1 : -1];
typedef char embfont16_blockentry_size_check[
    (sizeof(BlockTableEntry) == 8) ? 1 : -1];
typedef char embfont16_header_size_check[(sizeof(FontHeader) == 112) ? 1 : -1];

#endif // EMBFONT16_COMMON_H
