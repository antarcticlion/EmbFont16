#ifndef BDF_PARSER_H
#define BDF_PARSER_H

#include "embfont16_common.h"

// RawGlyph is now defined in common.h

extern RawGlyph* g_glyphs; // Changed to pointer
extern uint32_t g_max_code;
extern uint32_t g_total_chars;
extern char g_font_names[FONT_NAME_MAX_ENTRIES][FONT_NAME_ENTRY_SIZE];
extern uint32_t g_font_count;

void parse_bdf(const char* filename);
void free_bdf_glyphs();

#endif
