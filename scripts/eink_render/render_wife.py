# -*- coding: utf-8 -*-
# Renderina DABARTINI ir SIULOMA zmonos rezima su TIKRAIS sriftais (opensans*.h) ir
# TIKROMIS drabuziu ikonomis (clothing_icons.h). Formos - PIL ImageDraw; tekstas/ikonos - epd_render.
import os
from PIL import Image, ImageDraw, ImageFont
import epd_render as E

E.load_fonts(); E.load_icons()
W,H=E.W,E.H; F=E.FONTS

def wrap_measured(font, text, maxW, maxlines):   # atitinka main.cpp WrapMeasured
    out=[]; text=text.strip()
    while text and len(out)<maxlines:
        line=text
        while line and font.bounds(line)[2] > maxW:
            sp=line.rfind(' ')
            if sp<=0: break
            line=line[:sp]
        out.append(line)
        if len(line)>=len(text): break
        text=text[len(line):].strip()
    return out

class Screen:
    def __init__(s):
        s.img=Image.new("L",(W,H),255); s.d=ImageDraw.Draw(s.img); s.px=s.img.load()
    def T(s,x,y,txt,fn,al='L'): F[fn].draw_top(s.px,x,y,txt,al)
    def icon(s,x,y,name,w,h): E.draw_icon(s.px,x,y,name,w,h)
    def hline(s,x0,x1,y,val=0,wd=1): s.d.line((x0,y,x1,y),fill=val,width=wd)
    def vline(s,y0,y1,x,val=0,wd=1): s.d.line((x,y0,x,y1),fill=val,width=wd)
    def tri(s,x,y,up=True): s.d.polygon([(x,y),(x-10,y+20),(x+10,y+20)] if up else [(x,y),(x-10,y-20),(x+10,y-20)],fill=0)
    def rrect(s,x0,y0,x1,y1,val=128,r=12): s.d.rounded_rectangle((x0,y0,x1,y1),radius=r,outline=val,width=2)
    def wicon(s,cx,cy):  # oru ikona - PLACEHOLDER
        s.d.ellipse((cx-56,cy-18,cx-4,cy+34),outline=0,width=3)
        s.d.ellipse((cx-26,cy-46,cx+38,cy+18),outline=0,width=3)
        s.d.ellipse((cx+10,cy-18,cx+58,cy+30),outline=0,width=3)
        s.d.rectangle((cx-54,cy+20,cx+58,cy+36),fill=255)
        s.d.line((cx-56,cy+34,cx+58,cy+34),fill=0,width=3)
        for dx in (-40,-12,16,44): s.d.line((cx+dx,cy+40,cx+dx-8,cy+58),fill=0,width=3)
    def wreal(s,cx,cy):  # TIKRA Rain() LargeIcon: addcloud(20,5)+addrain (portuota is main.cpp)
        def fc(x,y,r,c): s.d.ellipse((x-int(r),y-int(r),x+int(r),y+int(r)),fill=c)
        def fr(x,y,w,h,c): s.d.rectangle((int(x),int(y),int(x)+int(w)-1,int(y)+int(h)-1),fill=c)
        sc,ls=20,5
        fc(cx-60,cy,sc,0); fc(cx+60,cy,sc,0); fc(cx-20,cy-20,sc*1.4,0); fc(cx+30,cy-26,sc*1.75,0)
        fr(cx-61,cy-20,sc*6,sc*2+1,0)
        fc(cx-60,cy,sc-ls,255); fc(cx+60,cy,sc-ls,255); fc(cx-20,cy-20,sc*1.4-ls,255); fc(cx+30,cy-26,sc*1.75-ls,255)
        fr(cx-58,cy-16,sc*5.9,sc*2-ls*2+2,255)
        F[18].draw_str(s.px, cx-60, cy+25, "///////")   # addrain LargeIcon (18B, baseline=y+h)
    def sicon(s,cx,cy):  # SmallIcon placeholder - mazas debesis (reprezentatyvu; tikras Cloudy/Sunny neatkartotas)
        s.d.ellipse((cx-22,cy-4,cx-2,cy+16),outline=0,width=2)
        s.d.ellipse((cx-10,cy-16,cx+14,cy+8),outline=0,width=2)
        s.d.ellipse((cx+2,cy-4,cx+22,cy+16),outline=0,width=2)
        s.d.rectangle((cx-20,cy+2,cx+20,cy+16),fill=255)
        s.d.line((cx-20,cy+14,cx+20,cy+14),fill=0,width=2)

# --- TIKROS SmallIcon ikonos (portuota 1:1 iš main.cpp addcloud/addsun; scale=Small=8) ---
def _fc(d,cx,cy,r,c):
    r=int(r)
    if r<=0: return
    d.ellipse((int(cx)-r,int(cy)-r,int(cx)+r,int(cy)+r),fill=c)
def _fr(d,x,y,w,h,c):
    w=int(w); h=int(h)
    if w<=0 or h<=0: return
    d.rectangle((int(x),int(y),int(x)+w-1,int(y)+h-1),fill=c)
def vaddcloud(d,x,y,s):
    ls=max(1,s//4)                                   # v32.1 pataisa: kontūras pagal dydį
    _fc(d,x-s*3,y,s,0); _fc(d,x+s*3,y,s,0)
    _fc(d,x-s,y-s,s*1.4,0); _fc(d,x+s*1.5,y-s*1.3,s*1.75,0)
    _fr(d,x-s*3-1,y-s,s*6,s*2+1,0)
    _fc(d,x-s*3,y,s-ls,255); _fc(d,x+s*3,y,s-ls,255)
    _fc(d,x-s,y-s,s*1.4-ls,255); _fc(d,x+s*1.5,y-s*1.3,s*1.75-ls,255)
    _fr(d,x-s*3+2,y-s+ls-1,s*5.9,s*2-ls*2+2,255)
def vaddsun(d,x,y,s,solid=False):
    ls=5
    _fr(d,x-s*2,y,s*4,ls,0); _fr(d,x,y-s*2,ls,s*4,0)
    d.line((x-s*1.3,y-s*1.3,x+s*1.3,y+s*1.3),fill=0)
    d.line((x-s*1.3,y+s*1.3,x+s*1.3,y-s*1.3),fill=0)
    _fc(d,x,y,s*1.3,255); _fc(d,x,y,s,0)
    if not solid: _fc(d,x,y,s-ls,255)               # žiedas (dabartinis) vs solid (siūlomas)
def vaddmoon(d,x,y,s):                                # SmallIcon pusmenulis (kaip main.cpp addmoon)
    _fc(d,x-28,y-37,s,0); _fc(d,x-20,y-37,s*1.6,255)
def vicon(d,x,y,code,sunS=8,solid=False,skipMoon=True):           # v36.1: menulio mazoms ikonoms NEPIESIAM
    S8=8
    if code.endswith("n") and not skipMoon: vaddmoon(d,x,y+10,S8)   # (naktinis menulis = "(" artefaktas)
    if code[:2] in ("02","04"):                      # MostlySunny/Cloudy su saule
        vaddsun(d,x-S8*1.8,y-S8*1.8,sunS,solid); vaddcloud(d,x,y,S8)
    elif code[:2]=="03":                             # Cloudy (tik debesys)
        vaddcloud(d,x+15,y-22,S8//2); vaddcloud(d,x-10,y-18,S8//2); vaddcloud(d,x,y,S8)
    elif code[:2]=="01":                             # Sunny
        vaddsun(d,x,y-3,int(S8*1.6),solid)
    else:
        vaddcloud(d,x,y,S8)

def sun_variant(d,cx,cy,sunS,solid,offx,offy):       # partly-cloudy: saule (parametrizuota) UZ debesies
    S8=8
    vaddsun(d,cx-S8*offx,cy-S8*offy,sunS,solid)
    vaddcloud(d,cx,cy,S8)

S=dict(city="Vilnius",date="Pirmadienis, 01-09-2026",time="13:55:07",feels="21",term="22",dmax="22",dmin="12",
       wind="9 m/s PPR",pop="98",adv="Vėsoka vasara - plonas švarkelis",
       note="Vakare atvės iki 9° - pasiimk šiltesnį.",
       corr="-3.5",ask="07-30",ans="07-30",nxt="rytoj 8:00",
       concl="Dažniau jaučiate šaltį - renku šilčiau",
       main="icon_svarkelis",acc=["icon_sketis"],
       parts=[("Rytas","12"),("Diena","22"),("Vakaras","15")])

def bottom(s):
    s.hline(5,955,498,128)
    s.T(15,505,S["city"],12)
    dt=S["date"]+"  @  "+S["time"][:5]                         # HH:MM be sekundžių
    s.T(150,505,dt,12)                                         # versija NEBE bare - ji R4 dešinėje
    s.d.rectangle((680,514,724,529),outline=0,width=2); s.d.rectangle((724,518,730,525),fill=0)  # baterija @655 (ikona@680)
    s.T(740,505,"80% 4.02v",12); s.T(940,505,"WiFi",12,'R')   # WiFi dešiniuoju kraštu ties 940 (= versija)

def current():
    s=Screen()
    s.wicon(120,85)
    s.T(410,6,"jaučiasi kaip",18,'C'); s.T(410,52,S["feels"]+"°",48,'C')
    s.T(270,128,"termometras rodo "+S["term"]+"°",12)
    s.tri(632,12,True);  s.T(654,6,S["dmax"]+"°",18)
    s.tri(632,88,False); s.T(654,52,S["dmin"]+"°",18)
    s.T(622,100,S["wind"],12); s.T(622,130,"lietus "+S["pop"]+"%",12)
    s.hline(20,940,164)
    s.icon(24,182,S["main"],124,124)
    ax=160
    for a in S["acc"]: s.icon(ax,208,a,72,72); ax+=80
    s.T(350,168,"KAIP RENGTIS",8)
    for i,ln in enumerate(wrap_measured(F[18],S["adv"],590,2)): s.T(350,192+i*48,ln,18)
    nl=wrap_measured(F[12],S["note"],590,1)
    if nl: s.T(350,288,nl[0],12)
    s.hline(20,940,324)
    s.T(30,330,"Korekcija "+S["corr"]+"°    klausta: "+S["ask"]+"    atsakyta: "+S["ans"]+"    kitas: "+S["nxt"],10)
    s.T(30,358,S["concl"],12)
    for (lbl,t),x in zip(S["parts"],(160,480,800)):
        s.T(x,394,lbl,12,'C'); s.sicon(x-46,452); s.T(x+44,430,t+"°",18,'C')
    s.vline(390,490,320,200); s.vline(390,490,640,200)
    bottom(s); return s.img

def proposed(big_concl=False):
    s=Screen()
    s.wreal(150,84)                                  # TIKRA oru ikona (Rain LargeIcon)
    s.T(470,10,"jaučiasi kaip",18,'C'); s.T(470,52,S["feels"]+"°",48,'C')
    s.T(470,128,"termometras rodo "+S["term"]+"°",12,'C')
    s.rrect(686,16,930,142,128,12)                            # skydelis centr. viršus..L1
    s.T(808,22,"ŠIANDIEN",10,'C')
    s.tri(736,54,True);  s.T(752,44,S["dmax"]+"°",18)
    s.tri(838,70,False); s.T(852,44,S["dmin"]+"°",18)
    s.T(808,84,"Vėjas "+S["wind"],12,'C'); s.T(808,112,"Lietus "+S["pop"]+"%",12,'C')
    s.hline(20,940,158)                                    # v23 layout
    s.icon(24,186,S["main"],124,124)
    ax=160
    for a in S["acc"]: s.icon(ax,212,a,72,72); ax+=80
    s.T(360,162,"ŠIANDIEN RENKIS",10)
    _lines=wrap_measured(F[24],S["adv"],580,2)              # patarimas 24B (žingsnis 56)
    for i,ln in enumerate(_lines): s.T(360,186+i*56,ln,24)
    nl=wrap_measured(F[12],S["note"],580,1)                 # pastaba VISADA po patarimo
    if nl: s.T(360,186+len(_lines)*56+6,nl[0],12)
    s.hline(20,940,340)
    if big_concl:
        for (lbl,t),x in zip(S["parts"],(160,480,800)):
            s.T(x,346,lbl,12,'C'); s.sicon(x-40,398); s.T(x+40,378,t+"°",24,'C')
        s.vline(342,430,320,200); s.vline(342,430,640,200)
        s.hline(20,940,434,200)
        s.T(30,438,S["concl"],12)                    # isvada RYSKI (12B)
        s.T(30,466,"Korekcija "+S["corr"]+"°   ·   atsakyta "+S["ans"]+"   ·   kitas "+S["nxt"],10)
        s.T(940,466,"v27",10,'R')                    # versija - R4 desineje
    else:
        for (lbl,t),x in zip(S["parts"],(160,480,800)):
            s.T(x,336,lbl,12,'C'); s.sicon(x-40,404); s.T(x+40,388,t+"°",24,'C')
        s.vline(332,462,320,200); s.vline(332,462,640,200)
        s.T(24,474,"Korekcija "+S["corr"]+"°  ·  atsakyta "+S["ans"]+"  ·  kitas "+S["nxt"]+"  ·  "+S["concl"],10)
    bottom(s); return s.img

# ---- R1..R4 bendra (vienoda abiem versijom): oru ikona, jutimine, R2 apranga, R4 grizt. rysys ----
def _r1_left(s):
    s.wreal(150,84)
    s.T(470,10,"jaučiasi kaip",18,'C'); s.T(470,52,S["feels"]+"°",48,'C')
    s.T(470,128,"termometras rodo "+S["term"]+"°",12,'C')
def _r2(s):
    s.icon(24,186,S["main"],124,124)
    ax=160
    for a in S["acc"]: s.icon(ax,212,a,72,72); ax+=80
    s.T(360,162,"ŠIANDIEN RENKIS",10)
    _lines=wrap_measured(F[24],S["adv"],580,2)
    for i,ln in enumerate(_lines): s.T(360,186+i*56,ln,24)
    nl=wrap_measured(F[12],S["note"],580,1)
    if nl: s.T(360,186+len(_lines)*56+6,nl[0],12)
    s.hline(20,940,340)
def _r4(s,ver):
    s.hline(20,940,434,200)
    s.T(30,438,S["concl"],12)
    s.T(30,466,"Korekcija "+S["corr"]+"°   ·   atsakyta "+S["ans"]+"   ·   kitas "+S["nxt"],10)
    s.T(940,466,ver,10,'R')

def v27f():  # TIKRAS dabartinis v27 (remelis; Vejas/Lietus centruoti; dienos dalys per vidury)
    s=Screen(); _r1_left(s)
    s.rrect(686,16,930,142,128,12)
    s.T(808,22,"ŠIANDIEN",10,'C')
    s.tri(736,54,True);  s.T(752,44,S["dmax"]+"°",18)
    s.tri(838,70,False); s.T(852,44,S["dmin"]+"°",18)
    s.T(808,84,"Vėjas "+S["wind"],12,'C'); s.T(808,112,"Lietus "+S["pop"]+"%",12,'C')
    s.hline(20,940,158)
    _r2(s)
    for (lbl,t),x in zip(S["parts"],(160,480,800)):     # dienos dalys: antraste+temp CENTER
        s.T(x,346,lbl,12,'C'); s.sicon(x-40,398); s.T(x+40,378,t+"°",24,'C')
    s.vline(342,430,320,200); s.vline(342,430,640,200)
    _r4(s,"v27"); bottom(s); return s.img

def _r2c(s):  # R2 su VERTIKALIAI CENTRUOTU tekstu (v28.1) - atitinka main.cpp
    s.icon(24,186,S["main"],124,124)
    ax=160
    for a in S["acc"]: s.icon(ax,212,a,72,72); ax+=80
    lines=wrap_measured(F[24],S["adv"],580,2); n=len(lines)
    nl=wrap_measured(F[12],S["note"],580,1); hasNote=len(nl)>0
    HEAD_INK,HEAD_GAP,ADV_STEP,ADV_INK,NOTE_GAP,NOTE_INK=20,10,56,50,12,24
    advBlock=(n-1)*ADV_STEP+ADV_INK
    stackH=HEAD_INK+HEAD_GAP+advBlock+(NOTE_GAP+NOTE_INK if hasNote else 0)
    top=158+(182-stackH)//2
    if top<164: top=164
    s.T(360,top,"ŠIANDIEN RENKIS",10)
    advTop=top+HEAD_INK+HEAD_GAP
    for i,ln in enumerate(lines): s.T(360,advTop+i*ADV_STEP,ln,24)
    if hasNote: s.T(360,advTop+(n-1)*ADV_STEP+ADV_INK+NOTE_GAP,nl[0],12)
    s.hline(20,940,340)

def _r4c(s,ver):  # R4 su CENTRUOTOMIS eilutemis (v28.1)
    s.hline(20,940,434,200)
    s.T(30,442,S["concl"],12)
    s.T(30,470,"Korekcija "+S["corr"]+"°   ·   atsakyta "+S["ans"]+"   ·   kitas "+S["nxt"],10)
    s.T(940,470,ver,10,'R')

def _r1_today(s):  # bendra R1 desine ("SIANDIEN" blokas, v28)
    LX=702
    s.vline(12,150,680,128)
    s.T(LX,14,"ŠIANDIEN",10)
    s.tri(LX+9,60,True);   s.T(LX+26,52,S["dmax"]+"°",18)   # v36.1: nuleista nuo antrastes, arciau Vejo
    s.tri(LX+120,80,False);s.T(LX+136,52,S["dmin"]+"°",18)
    s.T(LX,96,"Vėjas "+S["wind"],12); s.T(LX,124,"Lietus "+S["pop"]+"%",12)
    s.hline(20,940,158)

def _r3_parts(s):  # bendra R3 (dienos dalys, v29.1: be vert. skirtuku)
    for (lbl,t),x in zip(S["parts"],(160,480,800)):
        tx=x+8
        s.T(tx,346,lbl,12); s.sicon(x-44,398); s.T(tx,384,t+"°",24)   # v30.1: temp nuleista (ikona nekeista, 398)

def _r2j(s):  # MANO SIULYMAS: JUSTIFY - antraste prie virsaus, pastaba prie apacios, patarimas centre
    s.icon(24,186,S["main"],124,124)
    ax=160
    for a in S["acc"]: s.icon(ax,212,a,72,72); ax+=80
    lines=wrap_measured(F[24],S["adv"],580,2); n=len(lines)
    nl=wrap_measured(F[12],S["note"],580,1); hasNote=len(nl)>0
    HEAD_INK,ADV_STEP,ADV_INK,NOTE_INK=20,56,50,24
    L1,L2,PAD=158,340,8
    s.T(360,L1+PAD,"ŠIANDIEN RENKIS",10)                 # antraste prisegta prie virsaus
    headB=L1+PAD+HEAD_INK
    noteTop=(L2-PAD)-NOTE_INK if hasNote else None
    advBot=(noteTop-8) if hasNote else (L2-PAD)
    advH=(n-1)*ADV_STEP+ADV_INK
    advTop=headB+((advBot-headB)-advH)//2
    if advTop<headB+6: advTop=headB+6
    for i,ln in enumerate(lines): s.T(360,advTop+i*ADV_STEP,ln,24)
    if hasNote: s.T(360,noteTop,nl[0],12)
    s.hline(20,940,340)

def _r4_mano(s):  # MANO: 2 eil., korekcija PAKELTA i 1-a eil. greta isvados; datos+versija nublankintos apacioje
    s.hline(20,940,434,200)
    s.T(30,442,S["concl"]+"   ·   Korekcija "+S["corr"]+"°",12)         # svarbu: isvada + korekcija (12B)
    s.T(30,470,"atsakyta "+S["ans"]+"   ·   kitas "+S["nxt"],10)        # retai reikalinga (10B)
    s.T(940,470,"v28",10,'R')

def _r4_jusu(s):  # JUSU (patobulinta): skirtukas x680 (sulygiuotas su SIANDIEN); kaire isvada+korekcija (12B);
                  # desine atsakyta/kitas DVI mazos eil. (8B); versija - kampe apacioje (kaip anksciau, 10B)
    s.hline(20,940,434,200)
    s.T(30,441,S["concl"],12)                                           # kaire eil.1: isvada (12B) - max 545 telpa
    s.T(30,470,"Korekcija "+S["corr"]+"°",10)                           # kaire eil.2: korekcija (10B, mazesnis)
    s.vline(438,494,680,128)                                            # skirtukas x680 = SIANDIEN bloko linija
    s.T(690,444,"Atsakyta "+S["ans"],8)                                 # desine eil.1
    s.T(690,469,"Kitas "+S["nxt"],8)                                    # desine eil.2
    s.T(940,476,"v28",10,'R')                                           # versija - apatinis desinys kampas

def _r1_vallox(s):  # NAUJAS R1: didysis (jausmas is rekupo) pastumtas kairen + trio (Rekup/Jausm/Dabar)
    s.wreal(150,84)                                        # oru ikona (kaire, desinys krastas ~x235)
    s.T(385,16,"jaučiasi kaip",12,'C')                     # kaption - pastumta desiniau
    s.T(385,52,S["big"]+"°",48,'C')                        # DIDYSIS 48B, CENTRAS x385 (nuleista nuo 42->52)
    LBLX,VALX=518,610                                      # trio: etiketes x518, reiksmes x610 (worst -> ~656 < 680)
    for i,(lbl,v) in enumerate([("Rekup.",S["recup"]),("Jausm.",S["owmfeels"]),("Dabar",S["owmcur"])]):
        y=34+i*40                                          # 34/74/114 (12B, telpa iki 156)
        s.T(LBLX,y,lbl,12); s.T(VALX,y,v+"°",12)
    _r1_today(s)                                           # "SIANDIEN" blokas desineje + L1

def _r1_vallox_p(s):  # SIULOMA (task3): didysis desiniau (405) + trio REIKSMES 18B, praretintos i apacia
    s.wreal(150,84)
    s.T(405,16,"jaučiasi kaip",12,'C')
    s.T(405,52,S["big"]+"°",48,'C')
    LBLX,VALX=518,604
    rows=[("Rekup.",S["recup"]),("Jausm.",S["owmfeels"]),("Dabar",S["owmcur"])]
    for i,(lbl,v) in enumerate(rows):
        yv=20+i*46                       # value 18B tops 20/66/112 (praretinta, telpa iki 156)
        s.T(LBLX,yv+8,lbl,12)            # etikete 12B, sulygiuota su value centru
        s.T(VALX,yv,v+"°",18)            # REIKSME 18B (worst -25 -> x674 < 680)
    _r1_today(s)

def build2(r2fn,r4fn):  # ekranas su pasirenkamu R2 ir R4 piesiniu
    s=Screen(); _r1_left(s); _r1_today(s); r2fn(s); _r3_parts(s); r4fn(s); bottom(s); return s.img

def build_vallox():  # maketas su NAUJU R1 (Vallox), R2 justify, R3, R4 JUSU
    s=Screen(); _r1_vallox(s); _r2j(s); _r3_parts(s); _r4_jusu(s); bottom(s); return s.img
def build_v_cur():   # DABAR: R1 su 12B trio, saule B (tuscioaviduris s12) dienos eigoje
    s=Screen(); _r1_vallox(s);   _r2j(s); _r3_real(s,12,False); _r4_jusu(s); bottom(s); return s.img
def build_v_prop():  # SIULOMA: R1 didysis desiniau + trio 18B; saule B
    s=Screen(); _r1_vallox_p(s); _r2j(s); _r3_real(s,12,False); _r4_jusu(s); bottom(s); return s.img

def _r3_real(s,sunS,solid):  # dienos eiga su TIKROMIS ikonomis (Rytas 02d, Diena 03d, Vakaras 02d)
    codes=["02d","03d","02d"]
    for (lbl,t),x,code in zip(S["parts"],(160,480,800),codes):
        tx=x+8
        s.T(tx,346,lbl,12); vicon(s.d,x-44,398,code,sunS,solid); s.T(tx,384,t+"°",24)

def build_vallox_real(sunS,solid):
    s=Screen(); _r1_vallox(s); _r2j(s); _r3_real(s,sunS,solid); _r4_jusu(s); bottom(s); return s.img

def build(r2fn):  # bendras ekranas su pasirenkamu R2 varianto piesiniu
    return build2(r2fn,_r4c)

if __name__=="__main__":
 OUTDIR=os.path.dirname(os.path.abspath(__file__))
 SC=3
 def up(im): return im.resize((W*SC,H*SC),Image.NEAREST)
 big=ImageFont.truetype(r"C:\Windows\Fonts\segoeuib.ttf",42)
 def two(fnameA,labA,imA,labB,imB,out):
     a3,b3=up(imA),up(imB); pad,top,gap=30,74,92; cw=W*SC
     canvas=Image.new("L",(cw+2*pad, top+H*SC+gap+top+H*SC+pad),255)
     cd=ImageDraw.Draw(canvas)
     cd.text((pad,16),labA,font=big,fill=0)
     canvas.paste(a3,(pad,top)); cd.rectangle((pad-1,top-1,pad+cw,top+H*SC),outline=0,width=2)
     y2=top+H*SC+gap
     cd.text((pad,y2-52),labB,font=big,fill=0)
     canvas.paste(b3,(pad,y2)); cd.rectangle((pad-1,y2-1,pad+cw,y2+H*SC),outline=0,width=2)
     p=os.path.join(OUTDIR,out); canvas.save(p); print("saved",p,canvas.size)
 # MAKETAS: naujas R1 su Vallox rekuperatoriaus temp (didysis kaire + trio Rekup/Jausm/Dabar)
 S.update(big="4",recup="8",owmfeels="7",owmcur="9",
          dmax="11",dmin="3",wind="5 m/s V",pop="60",
          adv="Striukė ir šalikas",note="Vakare lietinga - pasiimk skėtį.",
          concl="Dažniau jaučiate šaltį - renku šilčiau",corr="-2.0",ans="09-09",nxt="rytoj 8:00",
          main="icon_striuke",acc=["icon_salikas","icon_sketis"])
 # TASK3: DABAR (trio 12B) vs SIULOMA (didysis @405, trio reiksmes 18B praretintos). Saule = B (abiejuose).
 two("t3","DABAR: didysis @385, trio 12B",build_v_cur(),
         "SIULOMA: didysis desiniau @405, trio reiksmes 18B (praretintos), saule B tuscioavidure",build_v_prop(),
         "wife_task3.png")
 # Priartinta R1 juosta (kad matytusi didysis + trio + saules ikona)
 ca=build_v_cur().crop((120,10,700,160)).resize((580*3,150*3),Image.NEAREST)
 cb=build_v_prop().crop((120,10,700,160)).resize((580*3,150*3),Image.NEAREST)
 zc=Image.new("L",(580*3+40,150*3*2+110),255); zd=ImageDraw.Draw(zc)
 zd.text((20,6),"DABAR (trio 12B, didysis @385)",font=big,fill=0); zc.paste(ca,(20,50))
 zd.text((20,150*3+80),"SIULOMA (trio 18B, didysis @405)",font=big,fill=0); zc.paste(cb,(20,150*3+124))
 zp=os.path.join(OUTDIR,"wife_task3_zoom.png"); zc.save(zp); print("saved",zp,zc.size)
 # INSPEKCIJA: ryto (Rytas) dienos-dalies ikona (02d) LABAI priartinta - ieskom "(" artefakto
 full=build_v_prop()
 iz=full.crop((70,344,300,432)).resize((230*8,88*8),Image.NEAREST)  # apie Rytas ikona (centras x116 y398)
 izc=Image.new("L",(iz.width,iz.height+50),255); ImageDraw.Draw(izc).text((10,10),"RYTAS ikona 02d (v36 hollow saule) - 8x",font=big,fill=0)
 izc.paste(iz,(0,50))
 ip=os.path.join(OUTDIR,"wife_rytas_zoom.png"); izc.save(ip); print("saved",ip,izc.size)
 # MOON artefaktas: 02n SU menuliu (dabar) vs BE menulio (siulomas fix) - 8x
 def moon_cell(skip):
     im=Image.new("L",(230,110),255); vicon(ImageDraw.Draw(im),116-70+30,55,"02n",12,False,skip); return im
 ma=moon_cell(False).resize((230*6,110*6),Image.NEAREST); mb=moon_cell(True).resize((230*6,110*6),Image.NEAREST)
 mc=Image.new("L",(230*6*2+60,110*6+70),255); md=ImageDraw.Draw(mc)
 md.text((10,10),"02n SU menuliu (DABAR - matosi '(' )",font=big,fill=0); mc.paste(ma,(10,60))
 md.text((230*6+40,10),"02n BE menulio (FIX)",font=big,fill=0); mc.paste(mb,(230*6+40,60))
 mp=os.path.join(OUTDIR,"wife_moon.png"); mc.save(mp); print("saved",mp,mc.size)
