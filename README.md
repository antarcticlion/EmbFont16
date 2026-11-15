# EmbFont16

16x16 dot Unicode bitmap font builder and embedded runtime for embedded
devices. This project was previously named **EmbUniFont**.

## Features

- Merges multiple BDF fonts into a single fixed-size 4 MiB ROM image.
- Supports Japanese (JIS X 0208/0213), Korean, Simplified Chinese, and
  GNU Unifont as fallback.
- Provides a small C runtime (`src/`) that reads glyphs from the ROM image and
  draws UTF-8 strings through a user-supplied pixel callback.
- Exposes the source font name for each glyph ID via
  `embfont16_get_font_name_ptr()`.
- Generates optional UF2 images for RP2350 boards and TTF files via FontForge.
- Documents the binary layout in [`docs/rom_format.md`](docs/rom_format.md).

## ROM images

| Target | Description |
|---|---|
| `output/EmbFont16.bin` | Full set (Japanese + Korean + Simplified Chinese) |
| `output/EmbFont16JP.bin` | Japanese set |
| `output/EmbFont16Uni.bin` | Japanese set with GNU Unifont fallback |
| `output/EmbFont16Ank.bin` | ANK-only set |

All images are 4 MiB and use ROM format version 2.0. See
[`docs/rom_format.md`](docs/rom_format.md) for the memory map and record
layout.

## Build

Requirements:

- GNU Make and a C99 compiler
- `iconv`
- `wget` only when the tracked Unifont archive is missing
- `fontforge` (optional, for `make ttf`)
- `picotool` (optional, for `make uf2`)

Initialize the submodules first:

```sh
git submodule update --init --recursive
```

Then build:

```sh
make          # tools + runtime library + Full/JP/Uni ROM images
make lib      # build/libembfont16.a (runtime + glyph helpers)
make ank      # ANK-only ROM image
make uf2      # UF2 images for Full/JP/Uni (requires picotool)
make ttf      # TTF conversion (requires fontforge)
make check    # build all ROM images and inspect representative glyphs
make clean    # remove output/, mapping/, build/, tools/bin/, generated headers,
              # and the extracted Unifont BDF
```

`make` does not build UF2 images. `make check` builds the Full, JP, Uni, and
Ank images before running `viewer`.

## Repository layout

```text
src/                 Embedded runtime library and POSIX HAL
tools/               Host-side BDF conversion and ROM build tools
assets/              Font sources, licenses, and submodules
docs/                ROM format and architecture documentation
experimental/        Standalone PC-98 analysis tools (not part of the build)
sgnano_viewer/       Viewer for sgnano (mruby)
Makefile             Build pipeline
```

## Documents

- [`docs/rom_format.md`](docs/rom_format.md) — ROM binary format (version 2.0)
- [`docs/architecture.md`](docs/architecture.md) — build pipeline and runtime
  architecture
- [`assets/misc/README.md`](assets/misc/README.md) — supplementary font sources
- [`assets/licenses/README.md`](assets/licenses/README.md) — font license texts
- [`sgnano_viewer/README.md`](sgnano_viewer/README.md) — optional device viewer

## Font assets and licensing

- The project source code is licensed under the MIT License (see `LICENSE`).
- Bundled fonts and submodules keep their own licenses:
  - milkjf: Public Domain (`assets/yaft/fonts/milkjf/`)
  - JIS X 0213 BDFs: Public Domain (`assets/misc/`)
  - Daewoo and Academia Sinica fonts: X11-style licenses
    (`assets/licenses/CREDITS`)
  - GNU Unifont: SIL Open Font License 1.1
    (`assets/licenses/LICENSE_UNIFONT`)
- The Full and Uni ROM images embed the corresponding license text in their
  license area. See [`docs/rom_format.md`](docs/rom_format.md).
- **PC-98 ROM images and ROM-derived font files are not included in this
  repository.** They are copyrighted and cannot be redistributed. The tools in
  `experimental/` are provided for users who supply their own ROM locally.

## Submodules

| Path | Upstream |
|---|---|
| `assets/yaft` | <https://github.com/uobikiemukot/yaft> |
| `assets/xorg-font-daewoo` | <https://gitlab.freedesktop.org/xorg/font/daewoo-misc> |
| `assets/xorg-font-isas` | <https://gitlab.freedesktop.org/xorg/font/isas-misc> |
| `tools/stb` | <https://github.com/nothings/stb> |
