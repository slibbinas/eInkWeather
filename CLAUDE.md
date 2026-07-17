# eInkWeather — orų stotelė su LilyGo EPD 4.7" ekranu

## Kas tai

Baterija maitinama orų stotelė: ESP32-S3 + LilyGo EPD47 4.7" e-ink ekranas (960x540).
Pagrindas — David Bird LilyGo-EPD-4-7-OWM-Weather-Display v2.5 (open source), stipriai modifikuotas.
Ciklas: pabunda → WiFi → OpenWeatherMap (current + forecast) → atvaizduoja → gilus miegas.

## Kalba ir komunikacija

Su vartotoju bendrauti **lietuviškai**. Kodo komentarai — lietuviškai arba paliekami originalūs angliški.

## Struktūra

- `src/main.cpp` — visas kodas (~1200 eil.): WiFi, OWM JSON dekodavimas, piešimas, miegas.
  `src/main.00`, `src/main.01` — senos atsarginės kopijos, nekompiliuojamos.
- `include/owm_credentials.h` — OWM API raktas, miestas, laiko juosta. **Slaptažodžiai — nekelti į viešą repo.**
- `include/lang_lt.h` — lietuviški tekstai (UTF-8, su diakritikais). `lang.h` — originalus angliškas, nenaudojamas.
- `include/opensans{8,10,12,18,24}b.h` — šriftai, sugeneruoti su lietuviškais glifais iš Segoe UI Bold.
- `scripts/fontconvert_lt.py` — šriftų generatorius (modifikuotas LilyGo fontconvert.py su LT intervalais).
  Pergeneruoti: `%USERPROFILE%\.platformio\penv\Scripts\python.exe scripts\fontconvert_lt.py OpenSans12B 12 C:\Windows\Fonts\segoeuib.ttf --compress > include\opensans12b.h` (rašyti UTF-8!).
  Norint tikro Open Sans — įdėti OpenSans-Bold.ttf ir naudoti jį vietoj segoeuib.ttf.
- `lib/LilyGo-EPD47-1.0.1/` — ekrano tvarkyklė (vendorinta). Šriftų piešimas palaiko UTF-8 per UnicodeInterval lenteles.
- `Font Files/` — originalūs (be LT raidžių) šriftų headeriai, atsargai.

## Techniniai faktai

- Plokštė: `esp32-s3-devkitc-1`, 16MB flash, PSRAM (framebuffer į PSRAM per `ps_calloc`).
- Baterijos ADC: GPIO14, daliklio koef. 6.100, kalibruota per `esp_adc_cal` (ADC_ATTEN_DB_12).
- **Mygtukas** (GPIO21, EXT0, `ButtonHoldMs()` iki atleidimo): trumpai <3 s — žmonos⇄pilnas ekranas
  (NVS `wifeMode`, išlieka po restarto); 3–8 s — nustatymų portalas; ≥8 s — OTA režimas
  (OTA lange dar vienas paspaudimas — Telegram testas). RESET — priverstinis atnaujinimas.
- WiFi: **WiFiManager (tzapu)** — portalas `OruStotele-Setup` (192.168.4.1) atidaromas TIK
  sąmoningai (mygtukas 3–8 s), automatiškai NIEKADA. Nepavykus prisijungti (ne taimerio
  žadinimas) — info langas ekrane, retry kas 30 min.
- **OTA**: `pio run -e ota -t upload` į `orustotele.local` (progreso juosta ekrane per
  `epd_push_pixels` dalinį atnaujinimą). Laidinis COM4 — atsarginis (dingsta miegant).
- **Telegram**: reply klaviatūra (ne inline — callback pasensta miegant); du gavėjai
  (chatAdmin/chatWife), /kvietimas deep-link, /vardas, /statistika (fbHist), /vadovas,
  ChillBias ±0.5° mokymasis, ryto patarimo citata vakariniame klausime. getUpdates: limit=3,
  offset saugomas po kiekvieno update.
- Piešimo helper'iai: `drawStringTop` (tikslus viršus), `WrapMeasured` (px laužymas),
  `ClearScreen` (ekranas+buferis — vien epd_clear buferio nevalo!). Papildomas šriftas
  `opensans48b.h` — tik skaitmenys/°, didelei temperatūrai.
- Miegas: kas 30 min. (`SleepDuration`), tik tarp `WakeupHour`(5) ir `SleepHour`(23); naktį miega vienu ypu iki ryto.
- Derinimas: atkomentuoti `#define SERIAL_DEBUG` main.cpp viršuje; `DBG(x)`; nuotolinis — Telegram `/log` (`LOGT`).
- Slėgis rodomas mmHg (`hPa_to_mmHg`), nors Units="M".
- OWM API: 2.5 (weather + forecast, cnt=24 → 3 paros po 3 val.).

## Ekrano piešimo taisyklės — layout'as iš matavimų, ne „iš debesų"

GUI piešiamas TIK nuo realių skaičių. Prieš dedant bet kokį elementą:

1. **Faktai iš kodo, ne iš atminties.** Ekranas 960×540, landscape. Prieš
   piešiant NAUJĄ ekraną — perskaityti esamą piešimo kodą (`DisplayWeather()`
   ir helper'ius) ir perimti jo koordinačių konvencijas bei helper'ius
   (`drawString` su LEFT/CENTER/RIGHT lygiavimu), o ne išradinėti savas.

2. **Koordinačių biudžetas PIRMA, kodas PASKUI.** Naujam ekranui pirmiausia
   surašyti regionų lentelę (pavadinimas, x, y, w, h) taip, kad regionai
   nepersidengtų ir sumoje tilptų į 960×540 su paraštėmis. Kiekvienas
   elementas priklauso regionui. Tik tada rašyti piešimo kodą — kiekviena
   koordinatė kode turi atitikti lentelę.

3. **Teksto dydis MATUOJAMAS, ne spėjamas.** Bibliotekoje yra
   `get_text_bounds()` — naudoti ją (arba paskaičiuoti: glifų advance sumos)
   PRIEŠ parenkant vietą. Matuoti su BLOGIAUSIU realiu turiniu, ne su demo
   reikšme: „-25.5°" (ne „5°"), ilgiausias miesto vardas, ilgiausia LT eilutė
   iš lang_lt.h. Netelpa → mažinti šriftą arba trumpinti tekstą, NE grūsti.

4. **Baseline, ne viršutinis kraštas.** GFX stiliaus šriftų kursorius yra
   BAZINĖJE LINIJOJE — klasikinis užlipimų šaltinis. Eilutės žingsnis =
   šrifto advance_y (ne „apie tiek"). LT diakritikai (Ą, Č, Š, Ž) kyšo
   aukščiau — tarp eilučių palikti pilną šrifto aukštį, ne cap-height.

5. **Kolizijų patikra skaičiais prieš flash'ą.** Baigus kodą — mintyse (ar
   ant popieriaus) perskaičiuoti visų užimtų stačiakampių ribas ir patikrinti
   persidengimus. Tik tada flash'inti. „Sufleškinsim ir pažiūrėsim" ant e-ink
   ypač brangus — refresh'as lėtas.

6. **E-ink specifika:** spalvos nėra — hierarchija daroma DYDŽIU ir STORIU
   (16 pilkumo lygių kontrastui menki); smulkus šriftas ant pilko fono
   neįskaitomas. Turimi šriftai: opensans 8/10/12/18/24 Bold — rinktis iš jų,
   negeneruoti naujo dėl vieno užrašo.

7. **Kai vartotojas sako „užlipa / negražu"** — matuoti to elemento realias
   ribas ir taisyti lentelę, o ne aklai stumdyti ±2 px.

## Ateities planai

Atlikta (2026-07): baterijos perspėjimas į Telegram, „žmonos režimas" su aprangos patarimais
ir mygtuko perjungimu, Telegram grįžtamasis ryšys su besimokančiu ChillBias koeficientu
(istorija kol kas NVS `fbHist`). Liko:

1. **SD kortelė** — atsakymų istorijos žurnalas su užpildymo priežiūra; koeficientai pagal
   sezonus/temperatūrų juostas. SD pinai S3: MISO 16, MOSI 15, SCLK 11, CS 42.
2. **Drabužių ikonos** — dailinti iki docs/mockup_zmonos.svg lygio (žr. memory taisykles:
   drabužiai, ne pakaitalai; be 🧶; nespalvota).
3. Pasirinktinai: pranešimų dubliavimas į WhatsApp per CallMeBot (tik viena kryptim).
4. Repo viešinimas (daro vartotojas; prieš tai — šriftai iš Open Sans vietoj Segoe UI).

## Pastabos saugumui

`owm_credentials.h` turi realų WiFi slaptažodį, OWM API raktą ir Telegram token —
failas yra .gitignore, į repo keliauja tik `owm_credentials_template.h` šablonas.
