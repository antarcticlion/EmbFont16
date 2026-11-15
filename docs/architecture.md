# Architecture

EmbFont16 builds fixed-size bitmap-font ROM images from BDF sources and
provides a small C runtime for drawing UTF-8 text from those images.

## Components

| Path | Role |
|---|---|
| `tools/` | Host-side pipeline: BDF preprocessing, merging, ROM generation, preview |
| `src/` | Embedded runtime library, glyph helpers, and POSIX HAL |
| `assets/` | Font sources, licenses, and submodules |
| `experimental/` | Standalone PC-98 ROM analysis tools (not built by default) |
| `sgnano_viewer/` | Optional mruby viewer for sgnano devices |
| `docs/` | Format and architecture documentation |

## Build pipeline

```text
BDF sources
  │
  ├─ bdf_cut          extract a code-point range
  ├─ bdf_remap        move code points (kana → Unicode/PUA)
  ├─ bdf_jis2eucjp    shift JIS/EUC code points
  └─ bdf_recode       convert to Unicode with iconv
  │
  ▼
output/work/*.bdf
  │
  ├─ gen_map          merge fonts (first source wins)
  │                   → output/EmbFont16*.bdf
  │                   → mapping/glyph_map.txt
  │                   → output/charts_*/*.png
  │
  ▼
output/EmbFont16*.bdf
  │
  ├─ font_builder     pack into a 4 MiB ROM
  │                   → output/EmbFont16*.bin
  │
  ├─ picotool         optional UF2 packaging
  └─ fontforge        optional TTF conversion
```

`gen_map` merges in argument order. A code point already present in an earlier
font is not overwritten by a later one, so the order of `SET_*_LIST` in the
Makefile is significant.

`font_builder` (`main.c` + `builder.c` + `bdf_parser.c`) reads the merged BDF
and writes the binary layout described in
[`rom_format.md`](rom_format.md).

## Runtime

The public API is declared in `src/include/embfont16.h`:

```c
bool embfont16_get_glyph(uint32_t cp, embfont16_glyph_t *glyph);
bool embfont16_get_font_name(uint8_t font_id, char *name, uint32_t name_size);
const char *embfont16_get_font_name_ptr(uint8_t font_id);
void embfont16_draw_string(int16_t x, int16_t y, const char *utf8_str,
                           uint32_t color,
                           embfont16_draw_pixel_t draw_fn,
                           void *user_data);
```

Access to the ROM is isolated behind one HAL function:

```c
bool hal_embfont16_read(uint32_t addr, uint8_t *buf, uint32_t size);
```

The only bundled implementation is POSIX file I/O
(`src/hal/posix/hal_posix.c`). A target port only needs to implement this
function; the rest of the runtime is platform-independent.

`embfont16_draw_string()` decodes UTF-8 one code point at a time, fetches the
glyph, and calls the user-supplied pixel callback for every foreground pixel.
The cursor advances by 8 pixels for half-width glyphs and 16 pixels for
full-width glyphs.

Each glyph record carries the ID of the source font. Use
`embfont16_get_font_name_ptr(font_id)` to obtain a display name for that source
(for example, `milkjf 8x16` or `GNU Unifont JP`). The returned pointer refers to
a static buffer and remains valid until the next call.
`embfont16_get_font_name()` is the reentrant, caller-buffer variant.

## Glyph helpers

`src/glyph_manupirate.c` and `src/slice_manupirate.c` provide optional
host-side helpers for manipulating 16x16, 8x16, and 8x8 glyphs:

- invert, horizontal/vertical flip, shift, and 90-degree rotation
- half and quadrant extraction/insertion
- slice conversion (`slice_to_glyph16`, `glyph16_to_slice`) and CRC32
- chunk allocation and iteration
- BDF output (`bdf_print_header`, `bdf_print_glyph16`, `bdf_print_footer`)

These helpers are compiled into `build/libembfont16.a` by `make lib`. They are
not required by `embfont16_get_glyph()` or `embfont16_draw_string()`.

## Runtime library

`make lib` builds `build/libembfont16.a` from:

- `src/embfont16.c`
- `src/glyph_manupirate.c`
- `src/slice_manupirate.c`

The archive does not include a HAL implementation. Link it with
`src/hal/posix/hal_posix.c` for host testing, or provide a target-specific
implementation of `hal_embfont16_read()`. The POSIX initialization and cleanup
functions (`hal_embfont16_posix_init()` and `hal_embfont16_posix_close()`) are
only needed when using the POSIX HAL.

## Output sets

| Target | Sources |
|---|---|
| `EmbFont16` (Full) | milkjf 8x16/k16/8x16r, JIS X 0213 plane 1/2, Korean, Simplified Chinese, plus PUA kana fallbacks |
| `EmbFont16JP` | Japanese sources only |
| `EmbFont16Uni` | milkjf, JIS X 0213 plane 2, GNU Unifont JP |
| `EmbFont16Ank` | milkjf 8x16 only (ANK characters) |

Full and Uni embed their respective license texts inside the ROM. JP and Ank
contain only public-domain glyphs and leave the license area empty.

## Host tools

| Tool | Purpose |
|---|---|
| `font_builder` | BDF → fixed-size ROM |
| `gen_map` | Merge BDFs, write merged BDF, glyph map, and PNG charts |
| `bdf_cut` | Extract a code-point range |
| `bdf_remap` | Remap a code-point range |
| `bdf_jis2eucjp` | Shift JIS/EUC code points |
| `bdf_recode` | Convert between encodings with iconv |
| `bdf2c` | Generate `tools/src/label_font.h` for chart labels |
| `viewer` | Print a glyph from a ROM as text art |
| `bdf_diff` | Compare two BDF files (manual diagnostic tool) |
| `hex2bdf` | Convert Unifont-style HEX data to BDF (standalone utility) |

## Design constraints

- ROM images are fixed at 4 MiB and cover `U+00000`–`U+3FFFF`.
- Code points are indexed in 256-character blocks; present blocks occupy
  9,216 bytes each.
- Cell height is fixed at 16 pixels. Width is 8 or 16 pixels per glyph.
- The runtime assumes the ROM is available at a byte-addressable location via
  the HAL. It does not parse the header; it uses the constants in
  `src/include/embfont16_common.h`.
- TTF output is a development convenience and is not part of the embedded
  runtime.
