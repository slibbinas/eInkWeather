# -*- coding: utf-8 -*-
# Sugeneruoja docs/mockup_zmonos.png - TIKSLU v20 zmonos ekrana (README iliustracijai)
# is TIKRU opensans*.h glifu + clothing_icons.h bitmapu. Paleisti: penv python make_mockup.py
import os
from PIL import Image, ImageDraw
import render_wife as R

R.S.update(dict(
    city="Vilnius", date="2026-04-14", time="20:00",
    feels="14", term="16", dmax="18", dmin="12", wind="5 m/s ŠV", pop="50",
    adv="Gaivu - lengva striukė",
    note="Vakare atvės iki 12° - pasiimk skėtį!",
    corr="-2.5", ans="04-13", nxt="rytoj 20:00",
    concl="Dažniau jaučiate šaltį - renku šilčiau",
    main="icon_striuke", acc=["icon_sketis"],
    parts=[("Rytas","13"),("Diena","16"),("Vakaras","14")]))

img = R.proposed(True)                       # v20 (B variantas - kaip idiegta)
SC = 2
big = img.resize((R.W*SC, R.H*SC), Image.NEAREST)
b = 10
canvas = Image.new("L", (R.W*SC+2*b, R.H*SC+2*b), 255)
ImageDraw.Draw(canvas).rectangle((b-1, b-1, b+R.W*SC, b+R.H*SC), outline=170, width=2)
canvas.paste(big, (b, b))
out = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "mockup_zmonos.png"))
canvas.save(out)
print("saved", out, canvas.size)
