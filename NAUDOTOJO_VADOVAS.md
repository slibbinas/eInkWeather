# eInk orų stotelė — naudotojo vadovas

Baterija maitinama orų stotelė su 4,7" e-ink ekranu (LilyGo EPD47 + ESP32-S3).
Orus gauna iš OpenWeatherMap, atsinaujina kas 30 min., naktį miega ir taip taupo bateriją.

---

## 1. Pirmas paleidimas (WiFi nustatymas)

Įjungus įrenginį, jei jis neranda išsaugoto WiFi tinklo, ekrane pasirodys
**„Nepavyko prisijungti prie WiFi"** su užuomina. Portalas automatiškai **neatsidaro** —
jį įjungiate patys:

1. **Palaikykite mygtuką 3–8 sekundes ir atleiskite** — ekrane pasirodys „Nustatymų režimas".
2. Telefonu ar kompiuteriu prisijunkite prie WiFi tinklo **`OruStotele-Setup`** (be slaptažodžio).
3. Naršyklėje atidarykite **`192.168.4.1`** (dažnai atsidaro automatiškai).
4. Spauskite *Configure WiFi*, pasirinkite savo namų tinklą, įveskite slaptažodį, spauskite *Save*.
5. Įrenginys pasileis iš naujo, prisijungs ir toliau dirbs savarankiškai.

**Jei nieko nedarysite** — įrenginys tyliai bandys jungtis kas 30 min. (pvz., router'is
laikinai dingo — viskas atsistatys savaime, baterija nesėdinama portalo laukimu).

## 2. Kasdienis veikimas

- Atsinaujina **kas 30 min.** (suderinta su valandos ribomis: :00 ir :30).
- Veikia nuo **5:00 iki 23:00**. Naktį miega vienu ypu iki 5 ryto — ekrane lieka paskutinė
  vakaro prognozė (e-ink vaizdą rodo be jokios energijos).
- Viršuje rodoma: miestas, data, paskutinio atnaujinimo laikas, baterijos % ir įtampa,
  WiFi signalo stiprumas.

## 3. Du režimai: pilnas ir paprastasis

Įrenginys turi du ekranus, perjungiamus **trumpu plokštės mygtuko paspaudimu** (šoninis
mygtukas, ne RESET):

- **Pilnas** — visa informacija su grafikais (aprašyta žemiau).
- **Paprastasis** — **jutiminė** temperatūra dideliu skaičiumi („jaučiasi kaip"), termometro
  rodmuo mažesniu, šmaikštus patarimas, kaip šiandien rengtis (su drabužių piktogramomis:
  maikutė, megztinis, striukė, kepurė, skėtis), ir dienos eiga: rytas / diena / vakaras.

> Didelis skaičius rodo, kaip oras **jaučiasi** (su vėju ir drėgme), o ne teorinį termometro
> rodmenį — būtent tai svarbu renkantis, kaip rengtis.

Paspaudus mygtuką įrenginys pabunda, parsisiunčia šviežius orus ir perpiešia ekraną kitu
režimu (užtrunka ~20–30 s — e-ink lėtas, bet taupus). Pasirinktas režimas **įsimenamas
pastovioje atmintyje** ir išlieka ne tik per miego ciklus, bet ir po perkrovimo ar maitinimo
atjungimo.

Abiejuose ekranuose viršuje rodomas **trikampis ▲** su užuomina, kurį fizinį mygtuką spausti
(mygtukas yra viršuje, ties laiko rodmeniu), o **status baras** (miestas, data, WiFi, baterija) —
ekrano **apačioje**.

Patarimai „kaip rengtis" specialiai sukalibruoti šalčio nemėgstantiems: rekomendacijos
visada puse laiptelio šiltesnės, frazės keičiasi kasdien, o pats „šilumos koeficientas"
mokosi iš jūsų atsakymų Telegram'e (žr. 6 skyrių).

## 4. Ekrano elementai (pilnas režimas)

| Zona | Kas rodoma |
|---|---|
| Viršus | Miestas, data, atnaujinimo laikas, baterija, WiFi |
| Kairė | Vėjo rožė: kryptis (lietuviškai — Š, PV, RŠR...) ir greitis m/s |
| Kairė žemiau | Saulėtekis, saulėlydis, mėnulio fazė |
| Centras | Temperatūra, drėgnumas, aukšt./žem. temperatūra, aprašymas, slėgis (mmHg) su tendencijos rodykle, matomumas, debesuotumas |
| Dešinė | Dabartinių orų piktograma |
| Vidurys | 3 parų prognozė kas 3 val. (laikas, piktograma, temperatūra) |
| Apačia | 4 grafikai: slėgis, temperatūra, lietus ir **sniegas arba drėgnumas** (sniego grafikas rodomas tik kai prognozuojamas sniegas; kitu atveju — drėgnumas) |

## 5. Nustatymai per naršyklę (API raktas, lokacija, naktinis režimas)

Visus nustatymus galima pakeisti telefonu, nieko neredaguojant kode:

1. **Palaikykite plokštės mygtuką nuspaustą ~3 sekundes ir atleiskite** (ne RESET; laikant
   ilgiau nei 8 s įsijungs OTA įkėlimo režimas — žr. 9 skyrių). Ekrane pasirodys
   „Nustatymų režimas" su instrukcijomis.
2. Telefonu prisijunkite prie WiFi tinklo **`OruStotele-Setup`** ir naršyklėje atidarykite
   **`192.168.4.1`**.
3. Meniu pasirinkite:
   - **„Setup"** — įveskite OWM API raktą, miestą, šalies kodą, ryto ir nakties pradžios
     valandas (naktį įrenginys nesirefreshina — miega iki ryto), Telegram token'ą ir klausimo
     valandą. Spauskite *Save*.
   - **„Configure WiFi"** — jei norite pakeisti WiFi tinklą.
4. Išsaugojus įrenginys pats pasileidžia iš naujo su naujais nustatymais.

Portalas veikia 5 minutes; jei nieko neišsaugosite, įrenginys grįžta į įprastą darbą.
Nustatymai saugomi įrenginio atmintyje ir išlieka atjungus maitinimą.

**WiFi tinklo keitimas / pamiršimas** (pvz., persikrausčius):
- Portale pasirinkite **„Configure WiFi"** ir įveskite naują tinklą, arba
- **„Info"** puslapyje spauskite **„Erase"** — visiškai pamiršti WiFi (po to atsidarys naujo tinklo langas), arba
- per Telegram parašykite **`/wifireset`**.

## 6. Telegram: „ar tiko apranga?" ir baterijos perspėjimai

Stotelė gali bendrauti per Telegram botą (įjungimas — šio skyriaus pabaigoje). Botas aptarnauja
**du žmones**: *adminą* (jus) ir *žmoną*.

- **Vakare** (numatyta 20:00) botas atsiunčia **žmonai** klausimą *„Kaip šiandien tiko apranga
  pagal mano patarimą?"* su keturiais mygtukais:
  🥶 Buvo šalta / 👍 Kaip tik / 🥵 Buvo karšta / 🤷 Nesilaikiau patarimo.
- Mygtukai yra **atsakymo (reply) klaviatūra** po žinutės lauku. Paspaudus, telefone iškart
  **matoma išsiųsta žinutė** (pvz. „🥶 Buvo šalta"), tad aišku, kad nuomonė nukeliavo. Stotelė
  ją perskaito pabudusi (per ~30 min.), pakoreguoja koeficientą ir atsako patvirtinimu; žmonos
  atsakymo kopiją gauna ir adminas.
  > Techninė pastaba: naudojami reply, o ne inline mygtukai sąmoningai — įrenginys miega iki
  > 30 min., o inline mygtukui Telegram reikalauja atsakyti per kelias sekundes, todėl telefone
  > nieko nebūtų matoma. Reply žinutė lieka pokalbyje ir nedingsta.
- **Adminas** gauna įrenginio būseną (`/status`, `/log`) ir **baterijos perspėjimą** 🪫, kai
  baterija nusenka iki 10 % (kartą per dieną).

Atsakyti galima bet kada — stotelė atsakymą pasiims kito pabudimo metu (per ~30 min.).

### Kaip vertinami atsiliepimai ir kur saugomi

- Patarimas parenkamas pagal **jutiminę** temperatūrą + korekciją `ChillBias`.
- „Buvo šalta" → `ChillBias` mažėja 0,5 °C (kitąkart ta pati temperatūra gaus **šiltesnę**
  rekomendaciją); „Buvo karšta" → didėja 0,5 °C (**lengvesnę**). Ribos: nuo −5 iki +1 °C.
- „Nesilaikiau patarimo" koeficiento **nekeičia** — tik įskaitomas į statistiką (nes patarimas
  nebuvo išbandytas).
- Kaupiami skaitikliai (šalta / gerai / karšta / nesilaikyta) ir paskutinio atsiliepimo data.
  Pagal juos daroma **išvada** (pvz. „dažniau jaučiate šaltį — renku šilčiau" arba „dažnai
  nesilaikote patarimų"), kuri rodoma **žmonos ekrane** kartu su korekcijos reikšme ir paskutinio
  atsiliepimo data — kad matytųsi, jog atsiliepimai veikia.
- Viskas saugoma įrenginio **pastovioje atmintyje (NVS)** ir išlieka net atjungus maitinimą.
  (Detalesnės istorijos SD kortelėje kol kas nėra — tai planuojama funkcija.)

### Komandos botui

Parašykite botui komandą — atsakymą gausite per artimiausią pabudimą (iki ~30 min.):

| Komanda | Ką daro |
|---|---|
| `/status` | Būsena: laikas, temperatūra, jutiminė, drėgnumas, slėgis, vėjas, baterija, WiFi, korekcija + išvada, atsiliepimų suvestinė, režimas, atmintis |
| `/log` | Veikimo žurnalas (kaip „serial" nuotoliniu būdu — WiFi, laikas, orų parsisiuntimas) |
| `/wifireset` | Pamiršta WiFi tinklą ir pasileidžia iš naujo su konfigūravimo portalu |
| `/adminas` | Užregistruoja siuntėją kaip **adminą** (būsena, baterija) |
| `/zmona` | Užregistruoja siuntėją kaip **žmoną** (tik klausimas apie aprangą) |
| `/help` | Komandų sąrašas |

`/status`, `/log`, `/wifireset` priimami tik iš admino telefono.

### Įjungimas

1. Telefone Telegram'e susiraskite **@BotFather** → komanda `/newbot` → sugalvokite vardą.
   Gausite **token'ą** (ilga eilutė su dvitaškiu).
2. Įrašykite token'ą per naršyklės portalą (žr. 5 sk., laukas „Telegram bot token") arba į
   `include/owm_credentials.h` (`telegramBotToken`) ir įkelkite programą.
3. **Jūs** parašykite botui bet ką — tapsite adminu.

### Žmonos prijungimas (jai nieko nereikia nustatinėti)

1. Parašykite botui **`/kvietimas`** — botas atsiųs paruoštą žinutę su nuoroda.
2. **Persiųskite** tą žinutę žmonai (ilgai palaikykite ant žinutės → *Forward*).
3. Žmona bakstelėja nuorodą ir paspaudžia **PRADĖTI** — viskas. Ji užregistruota,
   vakarais gaus klausimą apie aprangą ir atsakinės vienu mygtuko paspaudimu.

(Atsarginis būdas: žmona pati susiranda botą ir parašo `/zmona`.)

## 7. Baterija

- Li-ion / LiPo baterija kraunama per USB-C jungtį.
- 100 % ≈ 4,2 V, 0 % ≈ 3,2 V. Rodoma apatiniame status bare (dešinėje).
- Su 30 min. intervalu ir nakties miegu baterijos paprastai užtenka keliems mėnesiams
  (priklauso nuo talpos ir WiFi signalo stiprumo).

## 8. Trikčių šalinimas

| Problema | Sprendimas |
|---|---|
| Ekranas neatsinaujina | Patikrinkite bateriją; spauskite RESET |
| „WiFi nesukonfigūruotas" nors tinklas veikia | Patikrinkite, ar router'is transliuoja 2,4 GHz (ESP32 nemato 5 GHz) |
| Rodomas klaidingas laikas | Palaukite kito atsinaujinimo — laikas imamas iš NTP kas pabudimą |
| Nerodo orų (tuščias ekranas po pabudimo) | OWM API raktas negalioja arba nėra interneto; patikrinkite raktą `owm_credentials.h` |
| Klaustukas vietoj orų piktogramos | OWM grąžino nežinomą piktogramos kodą — praeis su kitu atsinaujinimu |
| Botas neatsako / klausimų nesiunčia | Patikrinkite `telegramBotToken`; parašykite botui žinutę, kad įsimintų chat ID |

## 9. Programuotojui

### Aplinka
- VS Code + PlatformIO, plokštė `esp32-s3-devkitc-1` (16 MB flash, PSRAM).
- Įkėlimas: `pio run -t upload` (portas `COM4`, žr. `platformio.ini`).

### Konfigūracija
- Nukopijuokite `include/owm_credentials_template.h` → `include/owm_credentials.h`
  ir įrašykite savo OWM API raktą (nemokamas: https://openweathermap.org/) bei
  Telegram boto token'ą (žr. 6 skyrių). Šis failas yra `.gitignore` sąraše —
  slaptažodžiai į git nekeliauja.
- Miestas, laiko juosta, kalba, klausimo valanda (`FeedbackHour`) — tame pačiame faile.
- Miego intervalas ir veikimo langas — `SleepDuration`, `WakeupHour`, `SleepHour`
  failo `src/main.cpp` viršuje.

### Programos įkėlimas per WiFi (OTA, be laido)

1. Įrenginyje **palaikykite mygtuką ~8 sekundes** (ilgiau nei nustatymų portalui).
   Ekrane pasirodys **„OTA įkėlimo režimas"** su įrenginio IP adresu ir tuščia progreso juosta.
2. Kompiuteryje (tame pačiame WiFi tinkle): `pio run -e ota -t upload`
   (jei `orustotele.local` vardas nesurandamas — `pio run -e ota -t upload --upload-port <IP iš ekrano>`).
3. Įkėlimo eiga rodoma **progreso juosta ekrane**; pabaigoje — „Įkelta! Perkraunama...".
   Jei per 5 min. niekas neįkeliama, įrenginys pats grįžta į įprastą darbą.

Mygtuko laikymo pakopos: **trumpai** — perjungti režimą · **~3 s** — nustatymų portalas ·
**~8 s** — OTA įkėlimas.

### Derinimas
- Atkomentuokite `#define SERIAL_DEBUG` `src/main.cpp` viršuje → pranešimai per USB
  (115200 baud): `pio device monitor`.

### Lietuviški šriftai
Šriftai (`include/opensans*.h`) sugeneruoti su lietuviškų raidžių glifais iš Segoe UI Bold.
Pergeneravimas (pvz., pakeitus šriftą ar dydį):

```
%USERPROFILE%\.platformio\penv\Scripts\python.exe scripts\fontconvert_lt.py OpenSans12B 12 C:\Windows\Fonts\segoeuib.ttf --compress
```

Išvestį išsaugoti UTF-8 formatu į `include/opensans12b.h`. Norint autentiško Open Sans —
atsisiųskite `OpenSans-Bold.ttf` ir nurodykite jį vietoj `segoeuib.ttf`.

## 10. Versijų istorija

- **2026-07 (redizainas)** — status baras perkeltas į apačią, viršuje mygtuko indikatorius ▲;
  žmonos režime jutiminė didele, reali mažu; maks/min/vėjas/lietus dideli su rodyklėmis;
  aprangos blokas be rėmelio su fiksuota teksto vieta; apvalesnės drabužių ikonos; adaptacijos
  info ekrane (korekcija, paskutinis atsiliepimas, išvada); didesni Rytas/Diena/Vakaras; Telegram
  4-tas mygtukas „Nesilaikiau", patvirtinimas „Užskaityta ✅", du gavėjai (adminas + žmona,
  `/adminas` `/zmona`); režimas įsimenamas po restarto (NVS).
- **2026-07** — nustatymai per naršyklę (ilgas mygtuko paspaudimas: API raktas, lokacija,
  naktinis režimas, Telegram); žmonos režime dideliu fontu rodoma jutiminė temperatūra;
  Telegram grįžtamasis ryšys („ar tiko apranga?") su savaime besimokančiu
  šilumos koeficientu ir baterijos perspėjimu; paprastasis („žmonos") režimas su aprangos
  patarimais, perjungiamas mygtuku; WiFiManager konfigūravimo portalas; lietuviškos raidės
  ekrane (ą, č, ę, ė, į, š, ų, ū, ž); lietuviškos vėjo kryptys; pataisyta klaida, dėl kurios
  vasarą būdavo rodomas sniego grafikas vietoj drėgnumo; naktį miegama iki ryto; baterijos %
  apsauga nuo klaidingų reikšmių.
- **Bazinė versija** — D. Bird LilyGo-EPD-4-7-OWM-Weather-Display v2.5 su mmHg ir
  baterijos kalibracijos modifikacijomis.

## 11. Planuojamos funkcijos

- Atsakymų istorijos žurnalas SD kortelėje (su užpildymo priežiūra) — detalesnei
  koeficiento analizei pagal sezonus ir temperatūrų juostas.
- Pranešimų dubliavimas į WhatsApp per CallMeBot (tik viena kryptimi).
