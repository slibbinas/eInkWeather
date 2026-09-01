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
    s.tri(LX+9,52,True);   s.T(LX+26,44,S["dmax"]+"°",18)
    s.tri(LX+120,72,False);s.T(LX+136,44,S["dmin"]+"°",18)
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

def build2(r2fn,r4fn):  # ekranas su pasirenkamu R2 ir R4 piesiniu
    s=Screen(); _r1_left(s); _r1_today(s); r2fn(s); _r3_parts(s); r4fn(s); bottom(s); return s.img

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
 # NORMALI (#3) vs BLOGIAUSIA (#1 ilgiausia isvada + korekcija -5.0 -> saugiklis 10B)
 imgN=build2(_r2j,_r4_jusu)
 S.update(concl="Renkuosi patarimus, reikia daugiau atsiliepimų",corr="-5.0")
 imgW=build2(_r2j,_r4_jusu)
 two("safe","NORMALI isvada (#3)  -  isvada 12B eil.1, Korekcija 10B eil.2",imgN,
          "BLOGIAUSIA isvada (#1, 545px) + Korekcija -5.0  -  vis tiek telpa (isvada viena eil.1)",imgW,
          "wife_final.png")
