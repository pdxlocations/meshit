# T-Deck compact fonts

These ASCII GFX bitmap fonts are generated from GNU FreeFont 20120503:

- `FreeSans8pt7b.h`: FreeSans regular, 8 points (message bodies).
- `FreeSansBold7pt7b.h`: FreeSans bold, 7 points (Plus labels).

Source archive: https://ftp.gnu.org/gnu/freefont/freefont-ttf-20120503.zip
Copyright holders and credits are provided in the archive's `AUTHORS` and
`CREDITS` files. GNU FreeFont is GPL-3.0-or-later with the font embedding
exception; see `COPYING` and `LICENSE-NOTICE` here.

Regenerate using Adafruit GFX Library 1.12.4's `fontconvert/fontconvert.c`,
compiled against FreeType, then run:

```sh
fontconvert FreeSans.ttf 8 > FreeSans8pt7b.h
fontconvert FreeSansBold.ttf 7 > FreeSansBold7pt7b.h
```

Small is the default. Large uses the library's existing 9-point fonts.
The Plus title remains bold 9-point in either mode. Glyphs are rasterized at
their native sizes rather than shrinking the larger bitmap on the device.
