# claude.ai „Project" paruošimas — eInkWeather

Šis failas — instrukcija, kaip susikurti claude.ai „Projects" darbo erdvę eInkWeather'iui.
Claude Code CLI to sukurti automatiškai negali, todėl darai rankomis claude.ai sąsajoje.
Viskas paruošta kopijuoti.

---

## C. Žingsniai (~5 min)

1. Eik į **claude.ai → Projects → Create project**.
2. Pavadinimas: **eInkWeather** · aprašymas: *ESP32-S3 + e-ink orų stotelė su Telegram grįžtamuoju ryšiu*.
3. Atsidaryk projekto **„Set custom instructions"** ir įklijuok visą **A dalį** (žemiau).
4. **„Add content" / „Add files"** — įkelk **B dalyje** išvardytus failus iš repo.
5. Gatava — kiekvienas naujas pokalbis tame projekte jau žinos visą eInkWeather kontekstą.

> Pastaba: claude.ai Project žinios yra atskira nuo Claude Code CLI skilzo/atminties.
> Abu naudingi: CLI dirba lokaliai su kodu, claude.ai Project — pokalbiams naršyklėje
> ar telefone. Šaltinis abiem lieka repo — atnaujinus failus, įkelk į Project iš naujo.

---

## A. Custom instructions (įklijuok į Project nustatymus)

```
Tu padedi su projektu „eInkWeather" — baterine orų stotele: ESP32-S3-DevKitC-1 (16MB flash,
PSRAM) + LilyGo EPD47 4.7" e-ink ekranas (960×540). Įrenginys pabunda, paima orus iš
OpenWeatherMap, nupiešia ekraną ir grįžta į gilų miegą 30 min. Pagrindas — David Bird (G6EJD)
LilyGo-EPD-4-7-OWM-Weather-Display v2.5, stipriai modifikuotas. main.cpp antraštėje autorystę
IŠSAUGOTI (jo licencija reikalauja).

BENDRAVIMAS: atsakinėk lietuviškai. Vartotojas — įrenginio kūrėjas (elektronika + firmware).

APARATŪRA:
- Plokštė: ESP32-S3-DevKitC-1 (16MB flash, PSRAM). Ekranas: LilyGo EPD47 4.7" (960×540).
- Mygtukas: GPIO21 (BUTTON_1, EXT0 wake). Baterijos ADC: GPIO14, koef. 6.100, esp_adc_cal
  (ADC_ATTEN_DB_12). SD kortelė (planuojama): MISO 16, MOSI 15, SCLK 11, CS 42.

VEIKIMO CIKLAS: pabunda → WiFi → OWM (current + forecast cnt=24) → piešia → deep sleep 30 min.
Naktį miega vienu ypu iki ryto. Būsena tarp miegų — RTC_DATA_ATTR; nustatymai — NVS (Preferences).

DU REŽIMAI (perjungiami mygtuku, būsena WifeMode RTC atmintyje):
- Pilnas — DisplayWeather(): vėjo rožė, astronomija, slėgis mmHg, grafikai.
- Paprastas/„žmonos" — DisplayWifeMode(): didelis JUTIMINĖS temperatūros skaičius (48pt)
  „jaučiasi kaip", aprangos patarimas su piktogramomis, rytas/diena/vakaras.

FUNKCIJOS:
- Aprangos patarimai GetClothingAdvice(): pagal jutiminę temp + ChillBias (mokosi iš Telegram);
  frazės keičiasi kasdien; lietaus/sniego/vėjo modifikatoriai; piktogramos piešiamos linijomis.
- Telegram TelegramSync() (WiFiClientSecure + Bot API, be serverio): vakare klausia „ar tiko
  apranga?" su mygtukais; atsakymas keičia ChillBias ±0.5°C (ribos −5..+1, NVS); baterijos ≤10%
  perspėjimas; chat ID auto-įsimenamas iš pirmos žinutės.
- WiFiManager (tzapu): AP „OruStotele-Setup" (192.168.4.1) atidaromas tik šaltam paleidime/RESET
  arba ilgu (>3s) mygtuko paspaudimu → StartConfigPortal() (API raktas, miestas, šalis, valandos,
  Telegram token). Pabudus iš miego portalas NEatidaromas (baterijos taupymas).

LIETUVIŠKI ŠRIFTAI: opensans{8,10,12,18,24,48}b.h — su LT glifais (Latin Extended-A). drawString()
piešia UTF-8 TIESIOGIAI, transliteracijos NĖRA. Lietuviškos vėjo kryptys (Š/PV/RŠR) — lang_lt.h.
Šriftų generatorius: scripts/fontconvert_lt.py (modif. LilyGo su --intervals). Viešinimui geriau
Open Sans (SIL OFL) nei Segoe UI.

KRITINĖS PASTABOS:
- Slaptos reikšmės TIK include/owm_credentials.h (gitignore!) — OWM raktas, WiFi, Telegram token,
  City, laiko juosta. Šablonas: owm_credentials_template.h. Į git NIEKADA nekelti slaptų reikšmių.
- LilyGo-EPD47 tvarkyklė vendorinta (lib/, GPL-3.0). UTF-8 per UnicodeInterval lenteles.
- src/main.00, main.01 — senos kopijos, NEKOMPILIUOJAMOS.
- ReadBattery() atskirtas nuo piešimo; baterijos % constrain.
- Ištaisyta: SumOfPrecip off-by-one (rodydavo sniegą vietoj drėgnumo), framebuffer NULL apsauga.

Projekto katalogas: C:\Users\SViktoras\Documents\PlatformIO\Projects\eInkWeather
Repo privatus: github.com/slibbinas/eInkWeather (push per Windows Credential Manager; gh CLI nėra).
```

---

## B. Įkeliami failai (Project knowledge)

Iš repo `github.com/slibbinas/eInkWeather` (arba lokaliai) įkelk į Project:

| Failas | Kam Project'e |
|---|---|
| `src/main.cpp` | visas firmware (~1500 eil.) |
| `platformio.ini` | build konfigūracija, plokštė, bibliotekos |
| `include/lang_lt.h` | lietuviški tekstai ir vėjo kryptys |
| `README.md` | dvikalbė (LT/EN) dokumentacija |
| `NAUDOTOJO_VADOVAS.md` | naudotojo vadovas |
| `CLAUDE.md` | projekto pastabos / konvencijos |

NEĮKELK:
- `include/owm_credentials.h` — slapta (OWM raktas, WiFi, Telegram token).
- `src/main.00`, `src/main.01` — senos nekompiliuojamos kopijos.
- `include/opensans*.h` — dideli šriftų masyvai, Project'e nenaudingi (jei reiks — įkelsi ad hoc).

Nuoroda aprašymui:
- Repo: https://github.com/slibbinas/eInkWeather
```
