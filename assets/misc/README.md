# assets/misc

Supplementary font sources used by the build.

| File | License / source |
|---|---|
| `jiskan16-2000-2.bdf` | Public Domain (JIS X 0213:2000 plane 2) |
| `jiskan16-2004-1.bdf` | Public Domain (JIS X 0213:2004 plane 1) |

The JIS X 0213 BDF files were designed by Toshiyuki Imamura and
HANATAKA Shinya. Their BDF headers declare them to be in the public
domain.

Other fonts used by the build come from the submodules:

- `assets/yaft/fonts/milkjf/` — milkjf (Public Domain)
- `assets/xorg-font-daewoo/hanglm16.bdf` — Daewoo Electronics (X11-style license)
- `assets/xorg-font-isas/gb16st.bdf` — Institute of Software, Academia Sinica
  (X11-style license)

GNU Unifont is stored separately under `assets/unifont/` and is licensed
under the SIL Open Font License 1.1. See `assets/licenses/LICENSE_UNIFONT`.

## PC-98 ROM data

The following files are **not** part of this repository because they contain
copyrighted PC-98 font ROM data:

- `FONT.ROM`
- `pc98_font_ank8x8.BDF`
- `pc98_font_ank8x16.BDF`
- `pc98_font_kanji.BDF`

They are listed in `.gitignore` to prevent accidental commits. The tools in
`experimental/` can work with a locally supplied ROM, but no ROM data is
distributed here.
