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

S=dict(city="Vilnius",date="Penktadienis, 24-07-2026",time="13:55:07",feels="6",term="8",dmax="11",dmin="3",
       wind="9 m/s PV",pop="40",adv="Paltas ir šiltas šalikas nepakenks",
       note="Vakare atvės iki 3° - pasiimk šiltesnį.  Pasiimk skėtį!",
       corr="-2.0",ask="04-14",ans="04-13",nxt="rytoj 20:00",
       concl="Dažniau jaučiate šaltį - renku šilčiau",
       main="icon_paltas",acc=["icon_salikas","icon_sketis"],
       parts=[("Rytas","5"),("Diena","11"),("Vakaras","4")])

def bottom(s):
    s.hline(5,955,498,128)
    s.T(15,505,S["city"],12)
    dt=S["date"]+"  @  "+S["time"][:5]                         # HH:MM be sekundžių
    s.T(150,505,dt,12)
    vx=150+F[12].bounds(dt)[2]+22                              # versija po IŠMATUOTOS datos
    s.T(vx,505,"v25",12)
    s.d.rectangle((645,514,689,529),outline=0,width=2); s.d.rectangle((689,518,695,525),fill=0)  # baterijos ikona @645
    s.T(705,505,"80% 4.02v",12); s.T(878,505,"WiFi",12)

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
    else:
        for (lbl,t),x in zip(S["parts"],(160,480,800)):
            s.T(x,336,lbl,12,'C'); s.sicon(x-40,404); s.T(x+40,388,t+"°",24,'C')
        s.vline(332,462,320,200); s.vline(332,462,640,200)
        s.T(24,474,"Korekcija "+S["corr"]+"°  ·  atsakyta "+S["ans"]+"  ·  kitas "+S["nxt"]+"  ·  "+S["concl"],10)
    bottom(s); return s.img

if __name__=="__main__":
 panA=proposed(False); panB=proposed(True)
 SC=3
 def up(im): return im.resize((W*SC,H*SC),Image.NEAREST)
 a3,b3=up(panA),up(panB)
 pad,top,gap=30,72,92; cw=W*SC
 canvas=Image.new("L",(cw+2*pad, top+H*SC+gap+top+H*SC+pad),255)
 cd=ImageDraw.Draw(canvas); big=ImageFont.truetype(r"C:\Windows\Fonts\segoeuib.ttf",42)
 cd.text((pad,16),"A) v2b + TIKRA oru ikona (Rain LargeIcon portuota)  -  meta viena rami 10B eilute",font=big,fill=0)
 canvas.paste(a3,(pad,top)); cd.rectangle((pad-1,top-1,pad+cw,top+H*SC),outline=0,width=2)
 y2=top+H*SC+gap
 cd.text((pad,y2-52),"B) v2c + TIKRA ikona  -  ISVADA sava 12B eilute (ryskesne), korekcija/data maza 10B",font=big,fill=0)
 canvas.paste(b3,(pad,y2)); cd.rectangle((pad-1,y2-1,pad+cw,y2+H*SC),outline=0,width=2)
 OUTDIR=os.path.dirname(os.path.abspath(__file__))
 out=os.path.join(OUTDIR,"wife_variants.png")
 canvas.save(out); print("saved",out,canvas.size)
 # GALUTINIS vienas ekranas (kaip idiegta v20), su remeliu
 fin=panB.resize((W*4,H*4),Image.NEAREST)
 fc=Image.new("L",(W*4+2*20, H*4+2*20),255); ImageDraw.Draw(fc).rectangle((19,19,20+W*4,20+H*4),outline=0,width=2)
 fc.paste(fin,(20,20))
 fout=os.path.join(OUTDIR,"wife_final_v20.png")
 fc.save(fout); print("saved",fout,fc.size)
