# -*- coding: utf-8 -*-
# TIKSLUS renderis: skaito TIKRUS irenginio sriftus (opensans*.h, zlib 4-bit glifai) ir
# TIKRUS drabuziu bitmapus (clothing_icons.h), vykdo ta pacia glifu/ikonu piesimo logika
# kaip LilyGo-EPD47 font.c (draw_char/get_char_bounds) ir main.cpp DrawIcon.
import re, zlib, os
from PIL import Image, ImageDraw, ImageFont
# px = PIL pikseliu prieigos objektas (img.load()); indeksavimas px[x,y].

INC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "include")  # <repo>/include
W, H = 960, 540

# ---------- sriftu parsinimas ----------
class Font:
    def __init__(self, name, path):
        t = open(path, "r", encoding="utf-8", errors="replace").read()
        bm = re.search(name+r"Bitmaps\[\d+\]\s*=\s*\{(.*?)\};", t, re.S).group(1)
        self.bitmap = bytes(int(x,16) for x in re.findall(r"0x[0-9A-Fa-f]+", bm))
        gl = re.search(name+r"Glyphs\[\]\s*=\s*\{(.*?)\};", t, re.S).group(1)
        self.glyphs = []
        for row in re.findall(r"\{([^}]*)\}", gl):
            nums = [int(v) for v in re.findall(r"-?\d+", row)]
            # width,height,advance_x,left,top,compressed_size,data_offset
            self.glyphs.append(nums[:7])
        iv = re.search(name+r"Intervals\[\]\s*=\s*\{(.*?)\};", t, re.S).group(1)
        self.intervals = []
        for row in re.findall(r"\{([^}]*)\}", iv):
            a,b,c = [int(v,16) if v.lower().startswith("0x") else int(v) for v in re.findall(r"0x[0-9A-Fa-f]+|\d+", row)]
            self.intervals.append((a,b,c))
        blk = re.search(r"const GFXfont "+name+r"\s*=\s*\{(.*?)\};", t, re.S).group(1)
        tail = [int(v) for v in re.findall(r"^\s*(-?\d+)\s*,", blk, re.M)]
        # interval_count, compressed, advance_y, ascender, descender
        self.compressed = tail[1]; self.advance_y = tail[2]; self.ascender = tail[3]; self.descender = tail[4]

    def glyph(self, cp):
        for first,last,off in self.intervals:
            if first <= cp <= last:
                return self.glyphs[off + (cp-first)]
            if cp < first:
                return None
        return None

    # get_text_bounds (be background) -> (x1,y1,w,h) su pen pradzia x=0,y=0
    def bounds(self, text):
        x=0; minx=100000; miny=100000; maxx=-1; maxy=-1
        for ch in text:
            g=self.glyph(ord(ch))
            if not g:
                # tarpas/nezinomas: tik advance jei tarpas
                sp=self.glyph(0x20)
                if ch==' ' and sp: x+=sp[2]
                continue
            wdt,hgt,adv,left,top,cs,do=g
            x1=x+left; y1=0+(top-hgt); x2=x1+wdt; y2=y1+hgt
            minx=min(minx,x1); miny=min(miny,y1); maxx=max(maxx,x2); maxy=max(maxy,y2)
            x+=adv
        if maxx<0: return (0,0,0,0)
        return (minx, miny, maxx-minx, maxy-miny)

    def _blit_glyph(self, px, g, penx, baseline):
        wdt,hgt,adv,left,top,cs,do=g
        if self.compressed:
            byte_width=(wdt//2 + wdt%2)
            raw=zlib.decompress(self.bitmap[do:do+cs])
        else:
            byte_width=(wdt//2 + wdt%2); raw=self.bitmap[do:do+byte_width*hgt]
        for gy in range(hgt):
            sy=baseline - top + gy
            if sy<0 or sy>=H: continue
            for gx in range(wdt):
                b=raw[gy*byte_width + gx//2]
                nib = (b & 0xF) if (gx & 1)==0 else (b>>4)
                if nib==0: continue
                sx=penx+left+gx
                if sx<0 or sx>=W: continue
                val = round((15-nib)/15*255)   # color_lut[bm]=15-bm ; 15=white
                if val < px[sx,sy]: px[sx,sy]=val   # tamsesnis rasalas nugali (perpiesimas ant baltos)

    def _render(self, px, x, baseline, text):
        pen=x
        for ch in text:
            g=self.glyph(ord(ch))
            if not g:
                sp=self.glyph(0x20)
                if ch==' ' and sp: pen+=sp[2]
                continue
            self._blit_glyph(px,g,pen,baseline)
            pen+=g[2]

    # drawStringTop(x,yTop,text,align)  align: 'L'/'C'/'R'  (baseline = yTop + y1 + h)
    def draw_top(self, px, x, yTop, text, align='L'):
        minx,y1,w,h=self.bounds(text)
        if align=='R': x=x-w
        if align=='C': x=x-w//2
        self._render(px, x, yTop + y1 + h, text)

    # senasis drawString(x,y,text,align)  (baseline = y + h)
    def draw_str(self, px, x, y, text, align='L'):
        minx,y1,w,h=self.bounds(text)
        if align=='R': x=x-w
        if align=='C': x=x-w//2
        self._render(px, x, y + h, text)

FONTS={}
def load_fonts():
    m={8:"OpenSans8B",10:"OpenSans10B",12:"OpenSans12B",18:"OpenSans18B",24:"OpenSans24B",48:"OpenSans48B"}
    for k,name in m.items():
        p=os.path.join(INC, f"opensans{k}b.h")
        FONTS[k]=Font(name,p)
        f=FONTS[k]
        print(f"  {name}: {len(f.glyphs)} glifu, advance_y={f.advance_y}, compressed={f.compressed}")

# ---------- clothing icons ----------
ICONS={}
def load_icons():
    t=open(os.path.join(INC,"clothing_icons.h"),encoding="utf-8",errors="replace").read()
    for name,body in re.findall(r"const uint8_t (icon_\w+)\[\d+\]\s*=\s*\{(.*?)\};", t, re.S):
        arr=[int(x) for x in re.findall(r"\d+", body)]
        ICONS[name]=bytes(arr)
    print("  ikonos:", ", ".join(ICONS.keys()))

def draw_icon(px, x, y, name, w, h):
    bmp=ICONS[name]; bpr=(w+7)//8
    for row in range(h):
        for col in range(w):
            if bmp[row*bpr + (col>>3)] & (0x80>>(col&7)):
                sx,sy=x+col,y+row
                if 0<=sx<W and 0<=sy<H: px[sx,sy]=0
