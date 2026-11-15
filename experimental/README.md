# experimental

Standalone experiments, not used by the main build.

- `insp.c` — inspects a PC-98 font ROM and emits BDF files.
- `pc98rom2bdf.c` — converts a PC-98 font ROM to BDF.

These tools expect a user-supplied PC-98 ROM at `assets/misc/FONT.ROM`.
The ROM file and any BDFs generated from it are copyrighted and are **not**
included in this repository. They are ignored by `.gitignore`.
