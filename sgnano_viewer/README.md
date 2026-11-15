# sgnano viewer

`fontview.rb` is an mruby 4.0 viewer for EmbFont16 ROM images on sgnano
devices. It displays one 128-code-point page (16 × 8 cells) at a time.

## Usage

1. Flash one of the ROM images (`output/EmbFont16*.bin`) to the device.
2. Set `FONT_SET` at the top of `fontview.rb` to match the flashed image:
   `:full`, `:jp`, or `:uni`.
3. Run `fontview.rb` on the device.

## Controls

| Key | Action |
|---|---|
| Left / Right | Previous / next page (128 code points) |
| Up / Down | Move by `0x80` code points |
| Page Up / Page Down | Move by `0x800` code points |
| Home / End | First / last page |
| `q` | Quit |

## Generated tables

The `FONT_SETS` tables map each code-point page to the font ID that supplies
it. They were generated from the `output/EmbFont16*.bin` images and therefore
reflect the current `Makefile` source order and merge rules. If you change the
font sets, regenerate or update these tables before flashing a new ROM.

The generated tables contain font-derived data (bitmap exceptions and source
mappings). The MIT license covers the viewer code, but the embedded font data
remains under the font licenses listed in `../assets/licenses/`.

The viewer is optional and is not part of the host build.
