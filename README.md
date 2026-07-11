# eInkWeather — orų stotelė su e-ink ekranu

Baterija maitinama namų orų stotelė: **ESP32-S3 + LilyGo EPD47 4,7" e-ink ekranas** (960×540).
Orus gauna iš OpenWeatherMap, atsinaujina kas 30 min., naktį miega — su viena baterija veikia
mėnesiais. Sąsaja lietuvių kalba, su tikromis lietuviškomis raidėmis (ą, č, ę, ė, į, š, ų, ū, ž).

## Du režimai

Perjungiami plokštės mygtuku (GPIO21) — įrenginys pabunda, parsisiunčia šviežius orus ir
perpiešia ekraną. Pasirinkimas išlieka per miego ciklus.

### Pilnas režimas

Visa informacija: vėjo rožė lietuviškomis kryptimis, astronomija, slėgis (mmHg) su tendencija,
3 parų prognozė kas 3 val. ir grafikai (sniego grafikas rodomas tik žiemą — kitu metu jo vietoje drėgnumas).

![Pilnas režimas](docs/mockup_pilnas.svg)

### Paprastasis („žmonos") režimas

Be grafikų: didelė temperatūra, **jutiminė** temperatūra ir šmaikštus patarimas, kaip rengtis,
su drabužių piktogramomis (maikutė, megztinis, striukė, kepurė, skėtis) bei dienos eiga
rytas / diena / vakaras.

![Paprastasis režimas](docs/mockup_zmonos.svg)

## Savaime besimokantys patarimai (Telegram)

- Vakare botas paklausia: *„Kaip šiandien tiko apranga?"* — 🥶 Buvo šalta / 👍 Kaip tik / 🥵 Buvo karšta.
- Atsakymas koreguoja „šilumos koeficientą" (±0,5 °C, ribos −5…+1 °C, saugomas NVS) — stotelė
  per kelias savaites prisitaiko prie šeimininkės, kuri, tarkim, nemėgsta šalčio.
- Baterijai nusekus iki 10 % botas atsiunčia perspėjimą 🪫.

## Savybės

| | |
|---|---|
| WiFi nustatymas | WiFiManager AP portalas (`OruStotele-Setup` → 192.168.4.1), instrukcijos rodomos pačiame e-ink ekrane |
| Baterijos taupymas | Gilus miegas tarp atnaujinimų; naktį (23–5 val.) miega vienu ypu; pabudus iš miego AP portalas neatidaromas |
| Lietuviški šriftai | Generuojami `scripts/fontconvert_lt.py` (Latin Extended-A glifai + 48 pt skaitmenų šriftas temperatūrai) |
| Telegram | Grįžtamasis ryšys, koeficiento mokymasis, baterijos perspėjimai — be jokio išorinio serverio |
| Konfigūracija | `include/owm_credentials.h` (gitignore); šablonas `owm_credentials_template.h` |

## Paleidimas

1. Nukopijuokite `include/owm_credentials_template.h` → `include/owm_credentials.h`, įrašykite
   [OpenWeatherMap API raktą](https://openweathermap.org/) ir (nebūtina) Telegram boto token'ą.
2. `pio run -t upload` (VS Code + PlatformIO, plokštė `esp32-s3-devkitc-1`).
3. WiFi nustatykite telefonu per AP portalą — instrukcijos ekrane.

Išsamiau — [NAUDOTOJO_VADOVAS.md](NAUDOTOJO_VADOVAS.md).

## Autorystė ir licencijos

Šis projektas yra asmeninė, nekomercinė modifikacija atvirojo kodo pagrindu:

- **Bazinis kodas**: [LilyGo-EPD-4-7-OWM-Weather-Display](https://github.com/G6EJD/LilyGo-EPD-4-7-OWM-Weather-Display)
  v2.5 — **Copyright (c) David Bird (G6EJD) 2021**. Visos teisės saugomos; naudojama asmeniniais,
  nekomerciniais tikslais pagal autoriaus licencijos sąlygas, autorystės pranešimas išsaugotas
  `src/main.cpp` antraštėje. Autoriaus svetainė: http://g6ejd.dynu.com/
- **Ekrano tvarkyklė**: [LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) (Xinyuan-LilyGO) — GPL-3.0,
  vendorinta `lib/LilyGo-EPD47-1.0.1/` (licencijos tekstas ten pat).
- **WiFiManager**: [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager) — MIT.
- **ArduinoJson**: [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) — MIT.
- **Šriftai**: glifai sugeneruoti iš Windows sisteminio Segoe UI Bold (naudojama asmeniniame
  įrenginyje; viešam platinimui rekomenduojama pergeneruoti iš [Open Sans](https://fonts.google.com/specimen/Open+Sans),
  SIL OFL licencija — žr. `scripts/fontconvert_lt.py`).

Modifikacijos (WiFiManager integracija, lietuvinimas, paprastasis režimas, Telegram grįžtamasis
ryšys, klaidų taisymai) — © 2026 Viktoras Sidlauskas, tomis pačiomis nekomercinėmis sąlygomis
kaip bazinis D. Bird kodas.
