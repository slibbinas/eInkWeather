# eInk orų stotelė — naudotojo vadovas

Baterija maitinama orų stotelė su 4,7" e-ink ekranu (LilyGo EPD47 + ESP32-S3).
Orus gauna iš OpenWeatherMap, atsinaujina kas 30 min., naktį miega ir taip taupo bateriją.

---

## 1. Pirmas paleidimas (WiFi nustatymas)

Įjungus įrenginį pirmą kartą (arba paspaudus RESET), jei jis neranda išsaugoto WiFi tinklo:

1. Ekrane pasirodys užrašas **„WiFi nesukonfigūruotas"** su instrukcijomis.
2. Telefonu ar kompiuteriu prisijunkite prie WiFi tinklo **`OruStotele-Setup`** (be slaptažodžio).
3. Naršyklėje atidarykite **`192.168.4.1`** (dažnai atsidaro automatiškai).
4. Spauskite *Configure WiFi*, pasirinkite savo namų tinklą, įveskite slaptažodį, spauskite *Save*.
5. Įrenginys prisijungs, parodys orus ir toliau dirbs savarankiškai.

Portalas veikia **3 minutes** — jei nespėjote, paspauskite RESET ir bandykite iš naujo.

**Svarbu:** pabudęs iš miego įrenginys portalo neatidaro (kad routerio gedimas nenusėstų
baterijos) — jis tiesiog bando jungtis ir, nepavykus, miega iki kito karto. Norėdami pakeisti
WiFi tinklą (pvz., persikrausčius), spauskite **RESET** — jei senas tinklas nepasiekiamas,
atsidarys konfigūravimo portalas.

## 2. Kasdienis veikimas

- Atsinaujina **kas 30 min.** (suderinta su valandos ribomis: :00 ir :30).
- Veikia nuo **5:00 iki 23:00**. Naktį miega vienu ypu iki 5 ryto — ekrane lieka paskutinė
  vakaro prognozė (e-ink vaizdą rodo be jokios energijos).
- Viršuje rodoma: miestas, data, paskutinio atnaujinimo laikas, baterijos % ir įtampa,
  WiFi signalo stiprumas.

## 3. Du režimai: pilnas ir paprastasis

Įrenginys turi du ekranus, perjungiamus **plokštės mygtuku** (šoninis mygtukas, ne RESET):

- **Pilnas** — visa informacija su grafikais (aprašyta žemiau).
- **Paprastasis** — didelė temperatūra, jutiminė temperatūra, šmaikštus patarimas,
  kaip šiandien rengtis (su drabužių piktogramomis: maikutė, megztinis, striukė, kepurė,
  skėtis), ir dienos eiga: rytas / diena / vakaras.

Paspaudus mygtuką įrenginys pabunda, parsisiunčia šviežius orus ir perpiešia ekraną kitu
režimu (užtrunka ~20–30 s — e-ink lėtas, bet taupus). Pasirinktas režimas įsimenamas ir
išlieka per visus miego ciklus; atjungus maitinimą grįžtama į pilną režimą.

Patarimai „kaip rengtis" specialiai sukalibruoti šalčio nemėgstantiems: rekomendacijos
visada puse laiptelio šiltesnės, frazės keičiasi kasdien, o pats „šilumos koeficientas"
mokosi iš jūsų atsakymų Telegram'e (žr. 5 skyrių).

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

## 5. Telegram: „ar tiko apranga?" ir baterijos perspėjimai

Stotelė gali bendrauti per Telegram botą (žr. įjungimą 8 skyriuje):

- **Vakare (20:00)** botas atsiunčia klausimą *„Kaip šiandien tiko apranga pagal mano
  patarimą?"* su mygtukais: 🥶 Buvo šalta / 👍 Kaip tik / 🥵 Buvo karšta.
- Atsakymas koreguoja patarimų „šilumos koeficientą" po 0,5 °C: jei buvo šalta — kitąkart
  ta pati temperatūra gaus šiltesnę rekomendaciją, jei karšta — lengvesnę. Taip stotelė
  per kelias savaites **prisitaiko prie žmogaus** (ribos: nuo −5 °C iki +1 °C korekcijos).
- Kai baterija nusenka iki **10 %**, botas atsiunčia perspėjimą 🪫 (kartą per dieną).

Atsakyti galima bet kada — stotelė atsakymą pasiims kito pabudimo metu (per ~30 min.)
ir patvirtins žinute. Koeficientas saugomas pastovioje atmintyje ir išlieka net atjungus
maitinimą.

### Įjungimas

1. Telefone Telegram'e susiraskite **@BotFather** → komanda `/newbot` → sugalvokite vardą.
   Gausite **token'ą** (ilga eilutė su dvitaškiu).
2. Įrašykite token'ą į `include/owm_credentials.h` (`telegramBotToken`) ir įkelkite programą.
3. Telegram'e susiraskite savo naują botą ir parašykite jam bet ką (pvz., „labas").
   Stotelė per artimiausią pabudimą įsimins jūsų chat ID ir prisistatys.

## 6. Baterija

- Li-ion / LiPo baterija kraunama per USB-C jungtį.
- 100 % ≈ 4,2 V, 0 % ≈ 3,2 V. Rodoma viršuje dešinėje.
- Su 30 min. intervalu ir nakties miegu baterijos paprastai užtenka keliems mėnesiams
  (priklauso nuo talpos ir WiFi signalo stiprumo).

## 7. Trikčių šalinimas

| Problema | Sprendimas |
|---|---|
| Ekranas neatsinaujina | Patikrinkite bateriją; spauskite RESET |
| „WiFi nesukonfigūruotas" nors tinklas veikia | Patikrinkite, ar router'is transliuoja 2,4 GHz (ESP32 nemato 5 GHz) |
| Rodomas klaidingas laikas | Palaukite kito atsinaujinimo — laikas imamas iš NTP kas pabudimą |
| Nerodo orų (tuščias ekranas po pabudimo) | OWM API raktas negalioja arba nėra interneto; patikrinkite raktą `owm_credentials.h` |
| Klaustukas vietoj orų piktogramos | OWM grąžino nežinomą piktogramos kodą — praeis su kitu atsinaujinimu |
| Botas neatsako / klausimų nesiunčia | Patikrinkite `telegramBotToken`; parašykite botui žinutę, kad įsimintų chat ID |

## 8. Programuotojui

### Aplinka
- VS Code + PlatformIO, plokštė `esp32-s3-devkitc-1` (16 MB flash, PSRAM).
- Įkėlimas: `pio run -t upload` (portas `COM4`, žr. `platformio.ini`).

### Konfigūracija
- Nukopijuokite `include/owm_credentials_template.h` → `include/owm_credentials.h`
  ir įrašykite savo OWM API raktą (nemokamas: https://openweathermap.org/) bei
  Telegram boto token'ą (žr. 5 skyrių). Šis failas yra `.gitignore` sąraše —
  slaptažodžiai į git nekeliauja.
- Miestas, laiko juosta, kalba, klausimo valanda (`FeedbackHour`) — tame pačiame faile.
- Miego intervalas ir veikimo langas — `SleepDuration`, `WakeupHour`, `SleepHour`
  failo `src/main.cpp` viršuje.

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

## 9. Versijų istorija

- **2026-07** — Telegram grįžtamasis ryšys („ar tiko apranga?") su savaime besimokančiu
  šilumos koeficientu ir baterijos perspėjimu; paprastasis („žmonos") režimas su aprangos
  patarimais, perjungiamas mygtuku; WiFiManager konfigūravimo portalas; lietuviškos raidės
  ekrane (ą, č, ę, ė, į, š, ų, ū, ž); lietuviškos vėjo kryptys; pataisyta klaida, dėl kurios
  vasarą būdavo rodomas sniego grafikas vietoj drėgnumo; naktį miegama iki ryto; baterijos %
  apsauga nuo klaidingų reikšmių.
- **Bazinė versija** — D. Bird LilyGo-EPD-4-7-OWM-Weather-Display v2.5 su mmHg ir
  baterijos kalibracijos modifikacijomis.

## 10. Planuojamos funkcijos

- Atsakymų istorijos žurnalas SD kortelėje (su užpildymo priežiūra) — detalesnei
  koeficiento analizei pagal sezonus ir temperatūrų juostas.
- Pranešimų dubliavimas į WhatsApp per CallMeBot (tik viena kryptimi).
