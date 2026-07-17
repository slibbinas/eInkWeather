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
- WiFi: **WiFiManager (tzapu)** — pirmas paleidimas/RESET atidaro AP `OruStotele-Setup` (192.168.4.1, 3 min. timeout), instrukcijos rodomos e-ink ekrane. Pabudus iš taimerio portalas NEatidaromas (baterijos taupymas) — tik bandoma jungtis prie išsaugoto tinklo.
- Miegas: kas 30 min. (`SleepDuration`), tik tarp `WakeupHour`(5) ir `SleepHour`(23); naktį miega vienu ypu iki ryto.
- Derinimas: atkomentuoti `#define SERIAL_DEBUG` main.cpp viršuje; `DBG(x)` makrokomanda.
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

## Ateities planai (eilės tvarka dar nespręsta)

1. **Pranešimai apie bateriją** — kai lieka ~10%, žinutė į Telegram arba WhatsApp (pasirinktinai).
   Telegram paprasčiau (Bot API, nemokamas); WhatsApp reikia Meta Cloud API arba Twilio.
2. **„Žmonos režimas"** — supaprastintas ekranas be grafikų: pagrindiniai parametrai + patarimas
   kaip rengtis (lengva striukė / megztinis / maikutė / lietpaltis...). Žmona — šalčmyrė (nemėgsta
   šalčio), tad patarimus krypti į šiltesnę pusę. Turi būti lengvas persijungimas tarp pilno ir
   paprastojo ekrano (pvz., LilyGo mygtukas ar lietimui jautrus ekranas).
3. **Grįžtamasis ryšys** — per WhatsApp/Telegram įrenginys vakare paklausia „ar prognozė buvo
   teisinga?" (taip/ne). Atsakymai kaupiami SD kortelėje ir naudojami prognozės korekcijai
   („mini AI" — pvz., paprastas bias koeficientas temperatūros pojūčiui). Sekti, kad SD neužsipildytų.
4. **SD kortelė** — duomenų žurnalas ir korekcijų saugykla.

## Pastabos saugumui

`owm_credentials.h` turi realų WiFi slaptažodį ir OWM API raktą — jei projektas keliamas į GitHub,
failą įtraukti į .gitignore ir palikti šabloną.
