# eink_render — žmonos ekrano peržiūra iš TIKRŲ įrenginio duomenų

Renderina `DisplayWifeMode()` ekraną (960×540) **be įrenginio**, vykdant tą pačią piešimo
logiką kaip firmware: skaito TIKRUS šriftų glifus iš `include/opensans*.h` (zlib 4-bit,
`get_char_bounds`/`draw_char` logika kaip `lib/.../font.c`) ir TIKRUS drabužių bitmapus iš
`include/clothing_icons.h` (`DrawIcon`). Skirta layout'o patikrai pagal „iš matavimų, ne iš
akies" discipliną (žr. `CLAUDE.md` „Ekrano piešimo taisyklės").

## Naudojimas

```
%USERPROFILE%\.platformio\penv\Scripts\python.exe scripts\eink_render\render_wife.py
```

Reikia `Pillow` (penv jau turi). Sukuria `wife_variants.png` (variantų palyginimas) ir
`wife_final_v20.png` (vienas ekranas) šalia skripto (PNG — gitignore).

- `epd_render.py` — šriftų (`Font`) ir ikonų parseris + glifų/ikonų piešimas į PIL paveikslą.
  `Font.draw_top()` = `drawStringTop` (viršus = `yTop + y1 + h`); `draw_str()` = senasis `drawString`.
- `render_wife.py` — atkartoja `DisplayWifeMode()` koordinačių seką; scenarijaus reikšmės `S` dict'e.

Keičiant firmware layout'ą — pirma čia perrenderinti ir patikrinti kolizijas, tik tada flash'inti
(e-ink refresh lėtas). Orų ikona ir maži dienos-eigos ratukai — placeholder'iai (tikras
`Cloudy()/Rain()` iš dalies portuotas `wreal()`).
