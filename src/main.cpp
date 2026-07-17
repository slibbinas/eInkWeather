// ESP32 Weather Display and a LilyGo EPD 4.7" Display, obtains Open Weather Map data, decodes and then displays it.
// This software, the ideas and concepts is Copyright (c) David Bird 2021. All rights to this software are reserved.
// #################################################################################################################

 //#define SERIAL_DEBUG
 
#include <Arduino.h>            // In-built
#include <esp_task_wdt.h>       // In-built
#include "freertos/FreeRTOS.h"  // In-built
#include "freertos/task.h"      // In-built
#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include "utilities.h"          // Plokštės pinai (BUTTON_1 = GPIO21 S3 versijoje)
#include "driver/rtc_io.h"      // RTC GPIO pull-up gilaus miego metu
#include "esp_adc_cal.h"        // In-built

#include "ArduinoJson.h"        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built

#include <WiFi.h>               // In-built
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager - AP konfigūravimo portalas
#include <WiFiClientSecure.h>   // In-built - HTTPS ryšiui su Telegram API
#include <ArduinoOTA.h>         // In-built - programos įkėlimas per WiFi (be laido)
#include <Preferences.h>        // In-built - NVS: šalčmyrės koeficientas, Telegram chat ID ir offset
#include <SPI.h>                // In-built
#include <time.h>               // In-built

#ifdef SERIAL_DEBUG
  #define DBG(x) Serial.println(x)
#else
  #define DBG(x)
#endif

#include "owm_credentials.h"
#include "forecast_record.h"
#include "lang_lt.h"            //Using Lithuanian translation
#include "functions.h"          // All the functions used in this program are in this file, to keep the main code tidy and easy to read 

#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT

//################  VERSION  ##################################################
String version = "2.5 / 4.7in";  // Programme version, see change log at end
//################ VARIABLES ##################################################

// enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

boolean LargeIcon   = true;
boolean SmallIcon   = false;
#define Large  20           // For icon drawing
#define Small  8            // For icon drawing
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, EventCnt = 0, vref = 1100;
//################ PROGRAM VARIABLES and OBJECTS ##########################################
#define max_readings 24 // Limited to 3-days here, but could go to 5-days = 40  

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};

long SleepDuration   = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour      = 5;  //5 7 Don't wakeup until after 06:00 to save battery power
int  SleepHour       = 23; // 23 Sleep after 23:00 to save battery power
long StartTime       = 0;
long SleepTimer      = 0;
long Delta           = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power

//fonts
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"
#include "opensans48b.h"        // Tik skaitmenys, '-', '.', '°' - didelei temperatūrai paprastame režime

GFXfont  currentFont;
uint8_t *framebuffer;

// Paprastasis ("žmonos") režimas - saugomas NVS, kad išliktų ir po ESP.restart()/maitinimo atjungimo
bool WifeMode = false;

// Telegram grįžtamasis ryšys (datos saugomos NVS, kad klausimas/perspėjimas nepasikartotų po restarto)
int LastAskDay       = -1; // kada paskutinį kartą klausta apie aprangą
int LastBattAlertDay = -1; // kada paskutinį kartą perspėta apie bateriją
float ChillBias = -2.0;                  // šalčmyrės korekcija °C; koreguojama pagal atsakymus, saugoma NVS
int   BatteryPct = -1;                   // -1 = nenuskaityta
float BatteryVoltage = 0;
Preferences prefs;

// Atsiliepimų statistika (NVS) - kaupiama, kad matytųsi įtaka ir būtų daroma išvada
int FbCold = 0, FbHot = 0, FbOk = 0, FbSkip = 0; // atsakymų kiekiai
int FbLastDay = -1;                              // paskutinio atsiliepimo diena (time/86400)

// Konfigūracija, keičiama per web portalą (ilgas mygtuko paspaudimas). Numatytos reikšmės
// iš owm_credentials.h, tikrosios užkraunamos iš NVS per LoadConfig().
String TgToken;      // Telegram bot token (perrašo const telegramBotToken)
int    FeedbackHr;   // vakarinio klausimo valanda (perrašo const FeedbackHour)

// Trumpas veikimo žurnalas - grąžinamas per Telegram komandą /log (kaip serial nuotoliniu būdu)
String RunLog;
void LOGT(const String& s) {
  DBG(s);
  RunLog += s + "\n";
  if (RunLog.length() > 1500) RunLog = RunLog.substring(RunLog.length() - 1500); // Telegram žinutės riba
}


void BeginSleep() {
  epd_poweroff_all();
  bool timeValid = UpdateLocalTime();
  bool insideWindow;
  if (WakeupHour > SleepHour) insideWindow = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
  else                        insideWindow = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
  if (!timeValid) {
    SleepTimer = SleepDuration * 60; // laikas nežinomas - miegoti standartinį intervalą
  }
  else if (insideWindow) {
    SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta; //Some ESP32 have a RTC that is too fast to maintain accurate time, so add an offset
  }
  else { // Naktį miegoti iki pat WakeupHour, o ne budintis kas 30 min. - taupo bateriją
    long hoursToWake = (WakeupHour - CurrentHour + 24) % 24;
    SleepTimer = hoursToWake * 3600L - (CurrentMin * 60 + CurrentSec) + Delta;
    if (SleepTimer <= 0) SleepTimer = SleepDuration * 60;
  }
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL); // in Secs, 1000000LL converts to Secs as unit = 1uSec
  // Mygtukas (GPIO21, aktyvus žemas) žadina bet kada ir perjungia paprastą/pilną režimą
  rtc_gpio_pullup_en((gpio_num_t)BUTTON_1);
  rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_1);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_1, 0);
  #ifdef SERIAL_DEBUG 
   DBG("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
   DBG("Entering " + String(SleepTimer) + " (secs) of sleep time");
   DBG("Starting deep-sleep period...");
  #endif
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  return UpdateLocalTime();
}

const char* WIFI_AP_NAME = "OruStotele-Setup"; // Konfigūravimo AP pavadinimas

void ConfigModeCallback(WiFiManager *wm) {
  // Rodomos instrukcijos e-ink ekrane, kai atsidaro konfigūravimo portalas
  epd_poweron();
  epd_clear();
  setFont(&OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 120, "WiFi nesukonfigūruotas", CENTER);
  setFont(&OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 200, "1. Telefonu prisijunkite prie WiFi tinklo:", CENTER);
  setFont(&OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 240, String(WIFI_AP_NAME), CENTER);
  setFont(&OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 300, "2. Naršyklėje atidarykite: 192.168.4.1", CENTER);
  drawString(SCREEN_WIDTH / 2, 340, "3. Pasirinkite savo tinklą ir įveskite slaptažodį", CENTER);
  drawString(SCREEN_WIDTH / 2, 420, "Portalas veiks 3 minutes", CENTER);
  edp_update();
  epd_poweroff_all();
}

uint8_t StartWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  #ifndef SERIAL_DEBUG
    wm.setDebugOutput(false);
  #endif
  wm.setConnectTimeout(20); // sek. bandymui jungtis prie išsaugoto tinklo
  bool coldBoot = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED);
  if (!coldBoot) {
    // Pabudimas iš miego (taimeris ar mygtukas): portalo neatidaryti (taupom bateriją)
    wm.setEnableConfigPortal(false);
  }
  else {
    // Pirmas paleidimas / RESET mygtukas: nepavykus prisijungti atidaryti portalą
    if (!wm.getWiFiIsSaved() && strlen(ssid) > 0) wm.preloadWiFi(ssid, password); // pradinis užpildymas iš owm_credentials.h, jei yra
    wm.setConfigPortalTimeout(180);
    wm.setAPCallback(ConfigModeCallback);
  }
  if (wm.autoConnect(WIFI_AP_NAME)) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    LOGT("WiFi OK " + WiFi.localIP().toString() + " RSSI=" + String(wifi_signal));
  }
  else {
    LOGT("WiFi FAIL");
  }
  return WiFi.status();
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  #ifdef SERIAL_DEBUG
    DBG("WiFi switched Off");
  #endif
}

//################ KONFIGŪRACIJA (WEB PORTALAS) #########################################
// Nustatymai (API raktas, lokacija, naktinis režimas, Telegram) keičiami per naršyklę.
// Portalas atidaromas ilgu (>3 s) plokštės mygtuko paspaudimu. Reikšmės saugomos NVS.

void LoadConfig() { // NVS reikšmės perrašo owm_credentials.h numatytąsias
  apikey     = prefs.getString("apikey",   apikey);
  City       = prefs.getString("city",     City);
  Country    = prefs.getString("country",  Country);
  WakeupHour = prefs.getInt("wakeHour",    WakeupHour);
  SleepHour  = prefs.getInt("sleepHour",   SleepHour);
  TgToken    = prefs.getString("tgToken",  String(telegramBotToken));
  FeedbackHr = prefs.getInt("fbHour",      FeedbackHour);
}

// Kiek ms mygtukas laikomas nuspaustas (matuojama iki atleidimo, riba 9 s).
// <3 s = režimo perjungimas, 3-8 s = nustatymų portalas, >=8 s = OTA įkėlimo režimas.
unsigned long ButtonHoldMs() {
  pinMode(BUTTON_1, INPUT_PULLUP);
  unsigned long start = millis();
  while (digitalRead(BUTTON_1) == LOW) { // aktyvus žemas
    if (millis() - start >= 9000) break;
    delay(50);
  }
  return millis() - start;
}

static bool cfgSaved = false;
void OnSaveConfigParams() { cfgSaved = true; }

void ConfigPortalScreen() { // instrukcijos e-ink ekrane konfigūracijos metu
  epd_poweron();
  epd_clear();
  setFont(&OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 110, "Nustatymų režimas", CENTER);
  setFont(&OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 185, "1. Telefonu prisijunkite prie WiFi tinklo:", CENTER);
  setFont(&OpenSans18B);
  drawString(SCREEN_WIDTH / 2, 225, String(WIFI_AP_NAME), CENTER);
  setFont(&OpenSans12B);
  drawString(SCREEN_WIDTH / 2, 285, "2. Naršyklėje atidarykite: 192.168.4.1", CENTER);
  drawString(SCREEN_WIDTH / 2, 320, "3. \"Setup\" - API raktas, miestas, nakties laikas, Telegram", CENTER);
  drawString(SCREEN_WIDTH / 2, 355, "   \"Configure WiFi\" - pakeisti tinklą", CENTER);
  drawString(SCREEN_WIDTH / 2, 430, "Portalas veiks 5 minutes, po to įrenginys pasileis iš naujo", CENTER);
  edp_update();
  epd_poweroff_all();
}

void StartConfigPortal() { // blokuojanti; po išsaugojimo įrenginys pasileidžia iš naujo
  ConfigPortalScreen();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  #ifndef SERIAL_DEBUG
    wm.setDebugOutput(false);
  #endif
  WiFiManagerParameter p_api("apikey", "OWM API raktas", apikey.c_str(), 40);
  WiFiManagerParameter p_city("city", "Miestas", City.c_str(), 40);
  WiFiManagerParameter p_country("country", "Šalies kodas (pvz. LT)", Country.c_str(), 6);
  WiFiManagerParameter p_wake("wakeHour", "Ryto pradžia, val. (0-23)", String(WakeupHour).c_str(), 4);
  WiFiManagerParameter p_sleep("sleepHour", "Nakties pradžia, val. (0-23)", String(SleepHour).c_str(), 4);
  WiFiManagerParameter p_tg("tgToken", "Telegram bot token (nebūtina)", TgToken.c_str(), 64);
  WiFiManagerParameter p_fb("fbHour", "Klausimo apie aprangą valanda (0-23)", String(FeedbackHr).c_str(), 4);
  // Chat ID paprastai užsiregistruoja automatiškai (/adminas, /zmona), bet galima įvesti/išvalyti ir čia
  WiFiManagerParameter p_adm("chatAdmin", "Admin chat ID (nebūtina)", prefs.getString("chatAdmin", "").c_str(), 24);
  WiFiManagerParameter p_wife("chatWife", "Žmonos chat ID (nebūtina)", prefs.getString("chatWife", "").c_str(), 24);
  wm.addParameter(&p_api);
  wm.addParameter(&p_city);
  wm.addParameter(&p_country);
  wm.addParameter(&p_wake);
  wm.addParameter(&p_sleep);
  wm.addParameter(&p_tg);
  wm.addParameter(&p_fb);
  wm.addParameter(&p_adm);
  wm.addParameter(&p_wife);
  cfgSaved = false;
  wm.setSaveParamsCallback(OnSaveConfigParams);
  wm.setShowInfoErase(true); // „Info" puslapyje - mygtukas WiFi nustatymams išvalyti
  std::vector<const char*> menu = {"wifi", "param", "info", "sep", "restart", "exit"};
  wm.setMenu(menu);
  wm.setConfigPortalTimeout(300);
  wm.startConfigPortal(WIFI_AP_NAME); // blokuoja, kol išsaugoma arba baigiasi laikas
  if (cfgSaved) {
    prefs.putString("apikey",  p_api.getValue());
    prefs.putString("city",    p_city.getValue());
    prefs.putString("country", p_country.getValue());
    prefs.putInt("wakeHour",   constrain(atoi(p_wake.getValue()), 0, 23));
    prefs.putInt("sleepHour",  constrain(atoi(p_sleep.getValue()), 0, 23));
    prefs.putString("tgToken", p_tg.getValue());
    prefs.putInt("fbHour",     constrain(atoi(p_fb.getValue()), 0, 23));
    if (strlen(p_adm.getValue()))  prefs.putString("chatAdmin", p_adm.getValue());  // rankinis įvedimas nebūtinas
    if (strlen(p_wife.getValue())) prefs.putString("chatWife",  p_wife.getValue());
  }
  ESP.restart(); // nauji nustatymai įsigalioja po perkrovimo
}

//################ OTA ĮKĖLIMAS PER WIFI (be laido) #####################################
// Įjungiama palaikius mygtuką >=8 s. Ekrane rodomas IP ir progreso juosta; kompiuteryje
// įkeliama komanda: pio run -e ota -t upload (arba --upload-port <IP iš ekrano>).
// Progresas piešiamas epd_push_pixels - daliniu atnaujinimu, be lėto pilno refresh'o.

static int OtaBarPx = 0;                       // kiek progreso juostos jau užpildyta (px)
const int OTA_BAR_X = 183, OTA_BAR_Y = 334, OTA_BAR_W = 594, OTA_BAR_H = 32;

void StartOtaMode() {
  if (StartWiFi() != WL_CONNECTED) {
    epd_poweron(); epd_clear();
    setFont(&OpenSans18B);
    drawStringTop(SCREEN_WIDTH / 2, 240, "OTA: nepavyko prisijungti prie WiFi", CENTER);
    edp_update(); epd_poweroff_all();
    delay(3000);
    ESP.restart();
  }
  epd_poweron();
  epd_clear();
  setFont(&OpenSans18B);
  drawStringTop(SCREEN_WIDTH / 2, 80, "OTA įkėlimo režimas", CENTER);
  setFont(&OpenSans12B);
  drawStringTop(SCREEN_WIDTH / 2, 160, "Laukiu programos per WiFi (5 min.)", CENTER);
  drawStringTop(SCREEN_WIDTH / 2, 200, "Kompiuteryje: pio run -e ota -t upload", CENTER);
  drawStringTop(SCREEN_WIDTH / 2, 240, "Adresas: " + WiFi.localIP().toString() + "  (orustotele.local)", CENTER);
  drawStringTop(SCREEN_WIDTH / 2, 430, "Mygtukas dar kartą - Telegram testas", CENTER);
  drawRect(OTA_BAR_X - 3, OTA_BAR_Y - 3, OTA_BAR_W + 6, OTA_BAR_H + 6, Black); // juostos rėmelis
  drawRect(OTA_BAR_X - 2, OTA_BAR_Y - 2, OTA_BAR_W + 4, OTA_BAR_H + 4, Black);
  edp_update();                                // ekranas lieka įjungtas progreso piešimui
  OtaBarPx = 0;
  ArduinoOTA.setHostname("orustotele");
  ArduinoOTA.onProgress([](unsigned int cur, unsigned int total) {
    if (!total) return;
    int px = (int)((uint64_t)cur * OTA_BAR_W / total);
    if (px - OtaBarPx >= 12 || (px == OTA_BAR_W && px != OtaBarPx)) { // piešiam kas ~2%
      Rect_t r = { OTA_BAR_X + OtaBarPx, OTA_BAR_Y, px - OtaBarPx, OTA_BAR_H };
      for (int i = 0; i < 3; i++) epd_push_pixels(r, 50, 0); // 3 impulsai - sodrus juodumas
      OtaBarPx = px;
    }
  });
  ArduinoOTA.onEnd([]() {
    setFont(&OpenSans18B);
    drawStringTop(SCREEN_WIDTH / 2, 410, "Įkelta! Perkraunama...", CENTER);
    edp_update();
    epd_poweroff_all();                        // po šio callback'o ArduinoOTA pats perkrauna
  });
  ArduinoOTA.onError([](ota_error_t e) {
    setFont(&OpenSans18B);
    drawStringTop(SCREEN_WIDTH / 2, 410, "OTA klaida " + String((int)e) + " - perkraunama", CENTER);
    edp_update();
    epd_poweroff_all();
    delay(2000);
    ESP.restart();
  });
  ArduinoOTA.begin();
  pinMode(BUTTON_1, INPUT_PULLUP);
  while (digitalRead(BUTTON_1) == LOW) delay(50); // palaukti, kol atleis 8 s laikytą mygtuką
  unsigned long start = millis();
  while (millis() - start < 5UL * 60UL * 1000UL) { // 5 min langas
    ArduinoOTA.handle();
    if (digitalRead(BUTTON_1) == LOW) {            // dar vienas paspaudimas -> Telegram testas
      delay(50);
      if (digitalRead(BUTTON_1) == LOW) TelegramTestMode(); // baigiasi restart
    }
    delay(10);
  }
  epd_poweroff_all();                          // niekas neatsiuntė - grįžtam į įprastą darbą
  ESP.restart();
}

void InitialiseSystem() {
  StartTime = millis();
  #ifdef SERIAL_DEBUG 
    Serial.begin(115200);
   // while (!Serial);
    Serial.println(String(__FILE__) + "\nStarting...");
  #endif
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) { // Nepavyko išskirti PSRAM - miegoti ir bandyti iš naujo, kitaip memset nulūžtų
    DBG("Memory alloc failed!");
    esp_sleep_enable_timer_wakeup(SleepDuration * 60 * 1000000LL);
    esp_deep_sleep_start();
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void loop() {
  // Nothing to do here
}

void setup() {
  InitialiseSystem();
  prefs.begin("eink", false);
  LoadConfig();
  // Būsena iš NVS (išlieka po restarto/maitinimo)
  WifeMode         = prefs.getBool("wifeMode", false);
  LastAskDay       = prefs.getInt("lastAsk", -1);
  LastBattAlertDay = prefs.getInt("lastBatt", -1);
  ChillBias        = prefs.getFloat("chillBias", -2.0);
  FbCold = prefs.getInt("fbCold", 0);  FbHot  = prefs.getInt("fbHot", 0);
  FbOk   = prefs.getInt("fbOk", 0);    FbSkip = prefs.getInt("fbSkip", 0);
  FbLastDay = prefs.getInt("fbLast", -1);
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    unsigned long held = ButtonHoldMs();
    if      (held >= 8000) StartOtaMode();       // labai ilgas -> OTA įkėlimas per WiFi (baigiasi restart)
    else if (held >= 3000) StartConfigPortal();  // ilgas -> nustatymai (baigiasi restart)
    else { WifeMode = !WifeMode; prefs.putBool("wifeMode", WifeMode); } // trumpas -> perjungti ir įsiminti
  }
  // Mygtukas ar įjungimas -> atnaujinti bet kada; tik planinis (taimerio) žadinimas paiso nakties lango
  bool forceRefresh = (wakeCause != ESP_SLEEP_WAKEUP_TIMER);
  ReadBattery();
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    bool WakeUp = forceRefresh;
    if (!WakeUp) {
      if (WakeupHour > SleepHour)
        WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
      else
        WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
    }
    if (WakeUp) {
      byte Attempts = 1;
      bool RxWeather  = false;
      bool RxForecast = false;
      WiFiClient client;   // wifi client object
      while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
        if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "weather");
        if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
        Attempts++;
      }
      LOGT("Laikas " + Date_str + " " + Time_str);
      LOGT("Orai w=" + String(RxWeather) + " f=" + String(RxForecast) +
           (RxWeather ? (" T=" + String(WxConditions[0].Temperature, 1) + " jaučiasi=" + String(WxConditions[0].Feelslike, 1)) : ""));
      if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
        TelegramSync();     // Atsakymai, koeficiento korekcija, perspėjimai - kol WiFi dar veikia
        StopWiFi();         // Reduces power consumption
        epd_poweron();      // Switch on EPD display
        epd_clear();        // Clear the screen
        if (WifeMode) DisplayWifeMode(); // Paprastas ekranas: temperatūra + patarimas kaip rengtis
        else          DisplayWeather();  // Pilnas ekranas su grafikais
        edp_update();       // Update the display to show the information
        epd_poweroff_all(); // Switch off all power to EPD
      }
    }
  }
  BeginSleep();
}

void Convert_Readings_to_Imperial() { // Only the first 3-hours are used
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[0].Rainfall   = mm_to_inches(WxForecast[0].Rainfall);
  WxForecast[0].Snowfall   = mm_to_inches(WxForecast[0].Snowfall);
}

bool DecodeWeather(WiFiClient& json, String Type) {
  #ifdef SERIAL_DEBUG 
    Serial.print(F("\nCreating object...and "));
  #endif
  DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {      
    #ifdef SERIAL_DEBUG                                        // Test if parsing succeeds.
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    #endif
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  #ifdef SERIAL_DEBUG 
    DBG(" Decoding " + Type + " data");
  #endif
  if (Type == "weather") {
    // All Serial.println statements are for diagnostic purposes and some are not required, remove if not needed with //
    //WxConditions[0].lon         = root["coord"]["lon"].as<float>();              DBG(" Lon: " + String(WxConditions[0].lon));
    //WxConditions[0].lat         = root["coord"]["lat"].as<float>();              DBG(" Lat: " + String(WxConditions[0].lat));
    WxConditions[0].Main0       = root["weather"][0]["main"].as<char*>();        DBG("Main: " + String(WxConditions[0].Main0));
    WxConditions[0].Forecast0   = root["weather"][0]["description"].as<char*>(); DBG("For0: " + String(WxConditions[0].Forecast0));
    //WxConditions[0].Forecast1   = root["weather"][1]["description"].as<char*>(); DBG("For1: " + String(WxConditions[0].Forecast1));
    //WxConditions[0].Forecast2   = root["weather"][2]["description"].as<char*>(); DBG("For2: " + String(WxConditions[0].Forecast2));
    WxConditions[0].Icon        = root["weather"][0]["icon"].as<char*>();        DBG("Icon: " + String(WxConditions[0].Icon));
    WxConditions[0].Temperature = root["main"]["temp"].as<float>();              DBG("Temp: " + String(WxConditions[0].Temperature));
    WxConditions[0].Feelslike   = root["main"]["feels_like"].as<float>();        DBG("FLik: " + String(WxConditions[0].Feelslike));
    WxConditions[0].Pressure    = root["main"]["pressure"].as<float>();          DBG("Pres: " + String(WxConditions[0].Pressure));
    WxConditions[0].Humidity    = root["main"]["humidity"].as<float>();          DBG("Humi: " + String(WxConditions[0].Humidity));
    WxConditions[0].Low         = root["main"]["temp_min"].as<float>();          DBG("TLow: " + String(WxConditions[0].Low));
    WxConditions[0].High        = root["main"]["temp_max"].as<float>();          DBG("THig: " + String(WxConditions[0].High));
    WxConditions[0].Windspeed   = root["wind"]["speed"].as<float>();             DBG("WSpd: " + String(WxConditions[0].Windspeed));
    WxConditions[0].Winddir     = root["wind"]["deg"].as<float>();               DBG("WDir: " + String(WxConditions[0].Winddir));
    WxConditions[0].Cloudcover  = root["clouds"]["all"].as<int>();               DBG("CCov: " + String(WxConditions[0].Cloudcover)); // in % of cloud cover
    WxConditions[0].Visibility  = root["visibility"].as<int>();                  DBG("Visi: " + String(WxConditions[0].Visibility)); // in metres
    WxConditions[0].Rainfall    = root["rain"]["1h"].as<float>();                DBG("Rain: " + String(WxConditions[0].Rainfall));
    WxConditions[0].Snowfall    = root["snow"]["1h"].as<float>();                DBG("Snow: " + String(WxConditions[0].Snowfall));
    //WxConditions[0].Country     = root["sys"]["country"].as<char*>();            DBG("Ctry: " + String(WxConditions[0].Country));
    WxConditions[0].Sunrise     = root["sys"]["sunrise"].as<int>();              DBG("SRis: " + String(WxConditions[0].Sunrise));
    WxConditions[0].Sunset      = root["sys"]["sunset"].as<int>();               DBG("SSet: " + String(WxConditions[0].Sunset));
    WxConditions[0].Timezone    = root["timezone"].as<int>();                    DBG("TZon: " + String(WxConditions[0].Timezone));
  }
  if (Type == "forecast") {
    #ifdef SERIAL_DEBUG 
      Serial.println(json);
      Serial.print(F("\nReceiving Forecast period - "));
    #endif
    JsonArray list = root["list"];
    for (byte r = 0; r < max_readings; r++) {
      #ifdef SERIAL_DEBUG 
        DBG("\nPeriod-" + String(r) + "--------------");
      #endif
      WxForecast[r].Dt                = list[r]["dt"].as<int>();
      WxForecast[r].Temperature       = list[r]["main"]["temp"].as<float>();              DBG("Temp: " + String(WxForecast[r].Temperature));
      WxForecast[r].Low               = list[r]["main"]["temp_min"].as<float>();          DBG("TLow: " + String(WxForecast[r].Low));
      WxForecast[r].High              = list[r]["main"]["temp_max"].as<float>();          DBG("THig: " + String(WxForecast[r].High));
      WxForecast[r].Pressure          = list[r]["main"]["pressure"].as<float>();          DBG("Pres: " + String(WxForecast[r].Pressure));
      WxForecast[r].Humidity          = list[r]["main"]["humidity"].as<float>();          DBG("Humi: " + String(WxForecast[r].Humidity));
      //WxForecast[r].Forecast0         = list[r]["weather"][0]["main"].as<char*>();        DBG("For0: " + String(WxForecast[r].Forecast0));
      //WxForecast[r].Forecast1         = list[r]["weather"][1]["main"].as<char*>();        DBG("For1: " + String(WxForecast[r].Forecast1));
      //WxForecast[r].Forecast2         = list[r]["weather"][2]["main"].as<char*>();        DBG("For2: " + String(WxForecast[r].Forecast2));
      WxForecast[r].Icon              = list[r]["weather"][0]["icon"].as<char*>();        DBG("Icon: " + String(WxForecast[r].Icon));
      //WxForecast[r].Description       = list[r]["weather"][0]["description"].as<char*>(); DBG("Desc: " + String(WxForecast[r].Description));
      //WxForecast[r].Cloudcover        = list[r]["clouds"]["all"].as<int>();               DBG("CCov: " + String(WxForecast[r].Cloudcover)); // in % of cloud cover
      //WxForecast[r].Windspeed         = list[r]["wind"]["speed"].as<float>();             DBG("WSpd: " + String(WxForecast[r].Windspeed));
      //WxForecast[r].Winddir           = list[r]["wind"]["deg"].as<float>();               DBG("WDir: " + String(WxForecast[r].Winddir));
      WxForecast[r].Pop               = list[r]["pop"].as<float>();                       DBG("Pop:  " + String(WxForecast[r].Pop));
      WxForecast[r].Feelslike         = list[r]["main"]["feels_like"].as<float>();
      WxForecast[r].Rainfall          = list[r]["rain"]["3h"].as<float>();                DBG("Rain: " + String(WxForecast[r].Rainfall));
      WxForecast[r].Snowfall          = list[r]["snow"]["3h"].as<float>();                DBG("Snow: " + String(WxForecast[r].Snowfall));
      WxForecast[r].Period            = list[r]["dt_txt"].as<char*>();                    DBG("Peri: " + String(WxForecast[r].Period));
    }
    //------------------------------------------
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0; // Remove any small variations less than 0.1
    WxConditions[0].Trend = "=";
    if (pressure_trend > 0)  WxConditions[0].Trend = "+";
    if (pressure_trend < 0)  WxConditions[0].Trend = "-";
    if (pressure_trend == 0) WxConditions[0].Trend = "0";

    if (Units == "I") Convert_Readings_to_Imperial();
  }
  return true;
}
//#########################################################################################
String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (Units == "I") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    //VS MOD
    //strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  return output;
}
//#########################################################################################
bool obtainWeatherData(WiFiClient & client, const String & RequestType) {
  const String units = (Units == "M" ? "metric" : "imperial");
  client.stop(); // close connection before sending a new request
  HTTPClient http;
  String uri = "/data/2.5/" + RequestType + "?q=" + City + "," + Country + "&APPID=" + apikey + "&mode=json&units=" + units + "&lang=" + Language;
  if (RequestType != "weather")
  {
    uri += "&cnt=" + String(max_readings);
  }
  http.begin(client, server, 80, uri); //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
    http.end();
    return true;
  }
  else
  {
    #ifdef SERIAL_DEBUG 
      Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    #endif
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

float mm_to_inches(float value_mm) {
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa) {
  return 0.02953 * value_hPa;
}

//VSADD
float hPa_to_mmHg(float value_hPa) {
  return 0.750062 * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i < readings; i++) { // buvo <=, skaitydavo už masyvo ribų

    sum += DataArray[i];
  }
  return sum;
}

String TitleCase(String text) {
  if (text.length() > 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); // Title-case the string
  }
  else return text;
}

double NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  //Calculate approximate moon phase
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}

//################ TELEGRAM: GRĮŽTAMASIS RYŠYS IR PERSPĖJIMAI #############################
// Vakare botas paklausia "ar tiko apranga?" su mygtukais Šalta/Kaip tik/Karšta.
// Atsakymai nuskaitomi kito pabudimo metu ir koreguoja ChillBias koeficientą (saugomas NVS).
// Taip pat siunčiamas perspėjimas, kai baterija <= 10%.

String TgApiCall(const String& method, const String& jsonBody) {
  WiFiClientSecure client;
  client.setInsecure(); // sertifikato netikrinam - pakanka slapto boto tokeno URL
  HTTPClient https;
  https.setTimeout(10000);
  if (!https.begin(client, "https://api.telegram.org/bot" + TgToken + "/" + method)) return "";
  int code;
  if (jsonBody.length() > 0) {
    https.addHeader("Content-Type", "application/json");
    code = https.POST(jsonBody);
  }
  else code = https.GET();
  String resp = (code > 0) ? https.getString() : "";
  int q = method.indexOf('?');
  String name = method.substring(0, q > 0 ? q : method.length());
  // Klaidas rašom į RunLog (matoma per /log) - anksčiau jos buvo visiškai nematomos
  if (code != 200) LOGT("TG " + name + " HTTP " + String(code));
  else             DBG("TG " + name + " -> 200");
  https.end();
  return resp;
}

// Išvada iš atsiliepimų - rodoma žmonos ekrane ir per /status
String FeedbackConclusion() {
  int total = FbCold + FbHot + FbOk + FbSkip;
  if (total < 2)                                       return "renkuosi patarimus, reikia daugiau atsiliepimų";
  if (FbSkip * 100 / total >= 40)                      return "dažnai nesilaikote patarimų 🙂";
  if (ChillBias <= -3.0 || FbCold > FbHot + FbOk)      return "dažniau jaučiate šaltį — renku šilčiau 🧣";
  if (ChillBias >=  0.5 || FbHot  > FbCold + FbOk)     return "mėgstate lengviau — renku vėsiau 😎";
  return "patarimai gerai pritaikyti 🎯";
}

String FbLabel(const String& data) {
  if (data == "FB_COLD") return "buvo šalta 🥶";
  if (data == "FB_HOT")  return "buvo karšta 🥵";
  if (data == "FB_OK")   return "kaip tik 👍";
  if (data == "FB_SKIP") return "nesilaikė patarimo 🤷";
  return "";
}

// Mygtukai: REPLY klaviatūra (ne inline!). Priežastis: įrenginys miega iki 30 min., o inline
// callback'ą Telegram reikalauja patvirtinti per kelias sekundes - kitaip telefone tiesiog
// nieko neįvyksta (callback_query_id pasensta). Reply mygtukas iškart išsiunčia paprastą
// žinutę - ji MATOMA pokalbyje tą pačią sekundę, o stotelė ją perskaito pabudusi.
void TgSendMessage(const String& chatId, const String& text, bool withButtons) {
  DynamicJsonDocument doc(4096);
  doc["chat_id"] = chatId;
  doc["text"] = text;
  if (withButtons) {
    JsonObject rm = doc.createNestedObject("reply_markup");
    JsonArray kb = rm.createNestedArray("keyboard");
    JsonArray r1 = kb.createNestedArray(); r1.add("🥶 Buvo šalta");  r1.add("👍 Kaip tik");
    JsonArray r2 = kb.createNestedArray(); r2.add("🥵 Buvo karšta"); r2.add("🤷 Nesilaikiau");
    rm["one_time_keyboard"] = true;
    rm["resize_keyboard"]   = true;
  }
  String body;
  serializeJson(doc, body);
  TgApiCall("sendMessage", body);
}

// Reply mygtuko tekstas -> atsiliepimo kodas
String FbCodeFromText(const String& t) {
  if (t.indexOf("šalta") >= 0)       return "FB_COLD";
  if (t.indexOf("karšta") >= 0)      return "FB_HOT";
  if (t.indexOf("Kaip tik") >= 0)    return "FB_OK";
  if (t.indexOf("Nesilaikiau") >= 0) return "FB_SKIP";
  return "";
}

void TgAnswerCallback(const String& cbId, const String& text) { // toast telefone, kad matytųsi jog užskaityta
  DynamicJsonDocument doc(512);
  doc["callback_query_id"] = cbId;
  doc["text"] = text;
  doc["show_alert"] = false;
  String body; serializeJson(doc, body);
  TgApiCall("answerCallbackQuery", body);
}

void TgEditMessage(const String& chatId, long msgId, const String& text) { // pakeičia žinutę (dingsta mygtukai)
  DynamicJsonDocument doc(1024);
  doc["chat_id"] = chatId;
  doc["message_id"] = msgId;
  doc["text"] = text;
  String body; serializeJson(doc, body);
  TgApiCall("editMessageText", body);
}

// Atsiliepimo užskaitymas - veikia ir iš reply mygtuko (paprastos žinutės), ir iš seno inline callback'o
void ApplyFeedback(const String& data, const String& cid) {
  String reply;
  if      (data == "FB_COLD") { ChillBias = constrain(ChillBias - 0.5f, -5.0f, 1.0f); FbCold++; reply = "Užrašiau! 🧣 Nuo šiol renku šilčiau."; }
  else if (data == "FB_HOT")  { ChillBias = constrain(ChillBias + 0.5f, -5.0f, 1.0f); FbHot++;  reply = "Supratau! 😎 Kitąkart siūlysiu lengviau."; }
  else if (data == "FB_OK")   { FbOk++;   reply = "Puiku, vadinasi pataikėm! 🎯"; }
  else if (data == "FB_SKIP") { FbSkip++; reply = "Aišku, užskaičiau, kad nesilaikėte 🙂"; }
  else return;
  FbLastDay = (int)(time(NULL) / 86400);
  prefs.putFloat("chillBias", ChillBias);
  prefs.putInt("fbCold", FbCold); prefs.putInt("fbHot", FbHot);
  prefs.putInt("fbOk", FbOk);     prefs.putInt("fbSkip", FbSkip);
  prefs.putInt("fbLast", FbLastDay);
  LOGT("TG atsiliepimas " + data + " -> korekcija " + String(ChillBias, 1));
  TgSendMessage(cid, reply, false);
  // Kopija adminui, jei atsakė žmona
  String admin = prefs.getString("chatAdmin", prefs.getString("chatId", String(telegramChatID)));
  if (admin.length() && admin != cid)
    TgSendMessage(admin, "👗 Žmona atsakė: " + FbLabel(data) + "\nIšvada: " + FeedbackConclusion(), false);
}

// Senų inline mygtukų apdorojimas (jei tokių dar kabo Telegram eilėje)
void ProcessCallback(const String& data, const String& cid, const String& cbId, long msgId) {
  TgAnswerCallback(cbId, "Užskaityta ✅");   // dažniausiai jau per vėlu (įrenginys miegojo) - nekritiška
  if (msgId) TgEditMessage(cid, msgId, "Atsakyta: " + FbLabel(data) + ". Ačiū! 🙏");
  ApplyFeedback(data, cid);
}

void HandleTgCommand(const String& text, const String& cid) { // /status, /log, /wifireset, /help (tik adminui)
  String cmd = text;
  cmd.toLowerCase();
  if (cmd.startsWith("/status")) {
    String s = "📟 Orų stotelė v" + version + "\n";
    s += "🕐 " + Date_str + " " + Time_str + "\n";
    s += "📍 " + City + "\n";
    s += "🌡 " + String(WxConditions[0].Temperature, 1) + "°C (jaučiasi " + String(WxConditions[0].Feelslike, 1) + "°)\n";
    s += "💧 " + String(WxConditions[0].Humidity, 0) + "%   " + String(hPa_to_mmHg(WxConditions[0].Pressure), 0) + " mmHg\n";
    s += "💨 " + String(WxConditions[0].Windspeed, 1) + " m/s " + WindDegToOrdinalDirection(WxConditions[0].Winddir) + "\n";
    s += "🔋 " + String(BatteryPct) + "% (" + String(BatteryVoltage, 2) + " V)   📶 " + String(WiFi.RSSI()) + " dBm\n";
    s += "🧥 Korekcija " + String(ChillBias, 1) + "°C — " + FeedbackConclusion() + "\n";
    s += "🗳 Atsiliepimai: šalta " + String(FbCold) + " / gerai " + String(FbOk) + " / karšta " + String(FbHot) + " / nesilaikyta " + String(FbSkip) + "\n";
    s += "🖥 Režimas: " + String(WifeMode ? "paprastas" : "pilnas") + "   🧠 " + String(ESP.getFreeHeap() / 1024) + " KB";
    TgSendMessage(cid, s, false);
  }
  else if (cmd.startsWith("/log")) {
    TgSendMessage(cid, RunLog.length() ? ("🧾 Žurnalas:\n" + RunLog) : "Žurnalas tuščias.", false);
  }
  else if (cmd.startsWith("/wifireset")) {
    TgSendMessage(cid, "🔄 WiFi nustatymai išvalyti. Įrenginys pasileidžia iš naujo ir atidaro „OruStotele-Setup\" portalą (192.168.4.1).", false);
    WiFiManager wm;
    wm.resetSettings();
    delay(1500);
    ESP.restart();
  }
  else { // /help, /start ar nežinoma komanda
    TgSendMessage(cid, "Komandos:\n/status – dabartinė būsena ir atsiliepimų suvestinė\n/log – veikimo žurnalas (kaip serial)\n/wifireset – pamiršti WiFi tinklą\n/zmona – užregistruoti žmonos telefoną (klausimams apie aprangą)\n/adminas – užregistruoti admino telefoną (būsena, baterija)\n/help – ši žinutė", false);
  }
}

void TelegramSync() { // Kviečiama kol WiFi dar įjungtas
  if (TgToken.length() == 0) return;
  String admin = prefs.getString("chatAdmin", prefs.getString("chatId", String(telegramChatID)));
  String wife  = prefs.getString("chatWife", "");
  long offset = prefs.getLong("tgOffset", 0);
  // 1. Pasiimti naujus atsakymus/žinutes.
  // SVARBU: limit=3 (ne 20) + didesnis doc. callback_query update'as stambus (jame kartojama visa
  // originali žinutė su klaviatūra), tad 16 KB su 20 update'ų duodavo NoMemory -> parse'as žlugdavo
  // -> tgOffset niekada neišsisaugodavo -> tie patys update'ai grįždavo amžinai (stotelė "negaudavo" nieko).
  String resp = TgApiCall("getUpdates?offset=" + String(offset) + "&limit=3&timeout=0", "");
  if (resp.length() > 0) {
    DynamicJsonDocument doc(32 * 1024);
    DeserializationError err = deserializeJson(doc, resp);
    if (err) LOGT("TG getUpdates parse FAIL: " + String(err.c_str()));
    else if (doc["ok"] != true) LOGT("TG getUpdates ok=false");
    else {
      for (JsonObject upd : doc["result"].as<JsonArray>()) {
        long updId = upd["update_id"].as<long>();
        if (updId >= offset) {
          offset = updId + 1;
          prefs.putLong("tgOffset", offset);  // patvirtinam IŠKART - garantuota pažanga net jei toliau lūžtų
        }
        if (upd.containsKey("callback_query")) { // senas inline mygtukas (jei dar kabo eilėje)
          String data = upd["callback_query"]["data"].as<const char*>();
          String cid;  serializeJson(upd["callback_query"]["message"]["chat"]["id"], cid);
          String cbId = upd["callback_query"]["id"].as<const char*>();
          long msgId  = upd["callback_query"]["message"]["message_id"].as<long>();
          ProcessCallback(data, cid, cbId, msgId);
        }
        else if (upd.containsKey("message")) { // žinutė botui: atsiliepimo mygtukas, registracija ar komanda
          String cid; serializeJson(upd["message"]["chat"]["id"], cid);
          String text = upd["message"]["text"] | "";
          String lc = text; lc.toLowerCase();
          String fb = FbCodeFromText(text);     // reply mygtukas atkeliauja kaip paprastas tekstas
          if (fb.length()) {
            ApplyFeedback(fb, cid);
          }
          else if (lc.startsWith("/zmona")) {
            wife = cid; prefs.putString("chatWife", wife);
            TgSendMessage(cid, "Užregistruota kaip žmona 👗 Vakarais klausiu, ar tiko apranga.", false);
          }
          else if (lc.startsWith("/adminas")) {
            admin = cid; prefs.putString("chatAdmin", admin);
            TgSendMessage(cid, "Užregistruota kaip administratorius 🛠 Gausite būseną ir baterijos perspėjimus.", false);
          }
          else if (admin.length() == 0) { // pirmas parašęs -> adminas
            admin = cid; prefs.putString("chatAdmin", admin);
            TgSendMessage(cid, "Sveiki! Čia jūsų orų stotelė 🌤 Jūs — administratorius.\nŽmona tegul parašo /zmona.\nKomandos: /status /log /wifireset /help", false);
          }
          else if (cid == admin && text.startsWith("/")) HandleTgCommand(text, cid); // komandas priima tik adminas
        }
      }
    }
  }
  int today = (int)(time(NULL) / 86400);
  // 2. Baterijos perspėjimas -> adminui (kartą per dieną)
  if (admin.length() && BatteryPct >= 0 && BatteryPct <= 10 && LastBattAlertDay != today) {
    TgSendMessage(admin, "🪫 Baterija liko " + String(BatteryPct) + "% (" + String(BatteryVoltage, 2) + " V) - laikas pakrauti!", false);
    LastBattAlertDay = today; prefs.putInt("lastBatt", today);
  }
  // 3. Vakarinis klausimas apie aprangą -> žmonai (jei nustatyta), kitaip adminui (kartą per dieną)
  String askTo = wife.length() ? wife : admin;
  if (askTo.length() && CurrentHour == FeedbackHr && LastAskDay != today) {
    TgSendMessage(askTo, "Kaip šiandien tiko apranga pagal mano patarimą? 🙂", true);
    LastAskDay = today; prefs.putInt("lastAsk", today);
  }
}

// Telegram testas iš OTA lango: išsiunčia testinį klausimą ir 90 s gyvai (be miego) laukia
// atsakymo - visa grandinė matoma realiu laiku. Į ChillBias statistiką NEskaičiuojama.
void TelegramTestMode() {
  epd_clear();
  setFont(&OpenSans18B);
  drawStringTop(SCREEN_WIDTH / 2, 60, "Telegram testas", CENTER);
  setFont(&OpenSans12B);
  String admin = prefs.getString("chatAdmin", prefs.getString("chatId", String(telegramChatID)));
  String wife  = prefs.getString("chatWife", "");
  String askTo = wife.length() ? wife : admin;
  if (TgToken.length() == 0 || askTo.length() == 0) {
    drawStringTop(SCREEN_WIDTH / 2, 160, TgToken.length() == 0
        ? "Boto token nenustatytas (ilgas paspaudimas - Setup)"
        : "Nėra registruotų gavėjų - parašykite botui žinutę", CENTER);
    edp_update();
    delay(5000);
    epd_poweroff_all();
    ESP.restart();
  }
  long offset = prefs.getLong("tgOffset", 0);
  TgSendMessage(askTo, "🔧 TESTAS: paspauskite bet kurį mygtuką žemiau", true);
  drawStringTop(SCREEN_WIDTH / 2, 150, String("Klausimas išsiųstas ") + (wife.length() ? "žmonai" : "adminui"), CENTER);
  drawStringTop(SCREEN_WIDTH / 2, 190, "Paspauskite atsakymo mygtuką telefone", CENTER);
  drawStringTop(SCREEN_WIDTH / 2, 230, "Laukiu atsakymo iki 90 s...", CENTER);
  edp_update();
  unsigned long start = millis();
  String got = "";
  while (millis() - start < 90000 && got.length() == 0) {
    delay(3000);                                   // tikrinam kas 3 s - gyvai, be miego ciklo
    String resp = TgApiCall("getUpdates?offset=" + String(offset) + "&limit=3&timeout=0", "");
    if (resp.length() == 0) continue;
    DynamicJsonDocument doc(32 * 1024);
    if (deserializeJson(doc, resp) != DeserializationError::Ok || doc["ok"] != true) continue;
    for (JsonObject upd : doc["result"].as<JsonArray>()) {
      long updId = upd["update_id"].as<long>();
      if (updId >= offset) { offset = updId + 1; prefs.putLong("tgOffset", offset); }
      if (upd.containsKey("message")) {
        String cid; serializeJson(upd["message"]["chat"]["id"], cid);
        String text = upd["message"]["text"] | "";
        String fb = FbCodeFromText(text);
        if (fb.length()) {                         // testo atsakymas - patvirtinam, bet neskaičiuojam
          got = FbLabel(fb);
          TgSendMessage(cid, "Testas pavyko ✅ Ryšys veikia! (į statistiką neįskaičiuota)", false);
          break;
        }
      }
    }
  }
  epd_clear();
  setFont(&OpenSans18B);
  drawStringTop(SCREEN_WIDTH / 2, 200, got.length() ? ("Veikia! Gauta: " + got) : "Atsakymo negauta per 90 s", CENTER);
  setFont(&OpenSans12B);
  drawStringTop(SCREEN_WIDTH / 2, 270, "Perkraunama...", CENTER);
  edp_update();
  epd_poweroff_all();
  delay(3000);
  ESP.restart();
}

//################ PAPRASTASIS ("ŽMONOS") REŽIMAS ########################################
// Perjungiamas plokštės mygtuku (GPIO21). Be grafikų: temperatūra, jutiminė temperatūra,
// šmaikštus patarimas kaip rengtis ir dienos eiga (rytas/diena/vakaras).

struct ClothingAdvice {
  String text;   // bazinis patarimas - rodomas dideliu šriftu (18B)
  String note;   // dienos pokytis / krituliai / vėjas - mažesniu (12B), hierarchija dydžiu
  bool tshirt, sweater, jacket, hat, umbrella;
};

ClothingAdvice GetClothingAdvice() {
  ClothingAdvice a = {"", "", false, false, false, false, false};
  float feels = WxConditions[0].Feelslike + ChillBias; // šalčmyrės korekcija: mokosi iš Telegram atsakymų
  // Dienos jutiminės temperatūros diapazonas (artimiausios dienos valandos) - kad patarimas
  // atspindėtų ne tik dabar, bet ir kaip keisis oras dienos bėgyje
  float feelsMax = feels, feelsMin = feels;
  for (int r = 0; r < 8; r++) {                         // ~24 val. į priekį (3 val. žingsnis)
    time_t ft = WxForecast[r].Dt;
    struct tm *flt = localtime(&ft);
    if (flt->tm_hour < 6 || flt->tm_hour > 22) continue; // tik dienos metas
    float f = WxForecast[r].Feelslike + ChillBias;
    if (f > feelsMax) feelsMax = f;
    if (f < feelsMin) feelsMin = f;
  }
  bool rainy  = (WxConditions[0].Rainfall > 0.1) || (WxForecast[0].Pop >= 0.35) || (WxForecast[1].Pop >= 0.35);
  bool snowy  = (WxConditions[0].Snowfall > 0.05) || (WxForecast[0].Snowfall > 0.1);
  bool windy  = (WxConditions[0].Windspeed >= 8);
  int day = (WxConditions[0].Sunrise / 86400) % 3; // frazė keičiasi kasdien, bet nesikeičia tą pačią dieną
  if (feels >= 22) {
    a.tshirt = true;
    const char* v[3] = {"Vasara kaip reikiant - užteks maikutės",
                        "Karšta! Maikutė ir šalta arbata - daugiau nieko nereikia",
                        "Šilta net šalčiausiam - drąsiai su maikute"};
    a.text = v[day];
  }
  else if (feels >= 15) {
    a.tshirt = true; a.sweater = true;
    const char* v[3] = {"Gražu, bet ne karšta - maikutė, o megztinis ant pečių",
                        "Megztinio diena: nei šalta, nei karšta - kaip tik",
                        "Maikutė drąsiems, megztinis protingiems"};
    a.text = v[day];
  }
  else if (feels >= 8) {
    a.sweater = true; a.jacket = true;
    const char* v[3] = {"Vėsoka - megztinis, o ant viršaus lengva striukė",
                        "Megztinis ir striukė šiandien privalomi",
                        "Oras sako: megztinis. Protas prideda: ir striukė"};
    a.text = v[day];
  }
  else if (feels >= 0) {
    a.jacket = true; a.hat = true;
    const char* v[3] = {"Šalta! Šilta striukė ir kepurė - jokių kompromisų",
                        "Striukė, kepurė ir greitas žingsnis - šiluma pati neateis",
                        "Ausys padėkos už kepurę, o kaklas - už šaliką"};
    a.text = v[day];
  }
  else {
    a.jacket = true; a.hat = true;
    const char* v[3] = {"Speigas! Žieminė striukė, kepurė, pirštinės - visa šarvuotė",
                        "Oras tik pingvinams - renkis kaip į ekspediciją",
                        "Šalčio ataka! Storiausia striukė ir arbata termose"};
    a.text = v[day];
  }
  // Pastabos atskirai nuo bazinio patarimo - rodomos mažesniu šriftu (netrukdo pagrindinei žinutei)
  if (feelsMax - feels >= 6)
    a.note = "Po pietų iki " + String((int)round(feelsMax)) + "° - renkis sluoksniais";
  else if (feels - feelsMin >= 6)
    a.note = "Vakare atvės iki " + String((int)round(feelsMin)) + "° - pasiimk šiltesnį";
  String extra;                                  // krituliai/vėjas (ikonos - nepriklausomai nuo teksto)
  if (snowy)      { extra = "Sninga - neperšlampami batai!"; a.hat = true; }
  else if (rainy) { extra = "Skėtis būtinas!"; a.umbrella = true; }
  else if (windy) { extra = "Vėjas piktas - užsisek!"; }
  if (extra.length()) { if (a.note.length()) a.note += ".  "; a.note += extra; }
  return a;
}

// Apvalaus stačiakampio pagalbinės (e-ink neturi native rounded-rect)
void drawArcCorner(int cx, int cy, int r, float a0, float a1) {
  for (float a = a0; a <= a1; a += 0.05) {
    int px = cx + r * cos(a), py = cy + r * sin(a);
    drawPixel(px, py, Black);
    drawPixel(px + 1, py, Black); // 2 px storis hi-res ekranui
  }
}
void drawRoundRect(int x, int y, int w, int h, int r) {
  drawLine(x + r, y,     x + w - r, y,     Black); // viršus
  drawLine(x + r, y + h, x + w - r, y + h, Black); // apačia
  drawLine(x,     y + r, x,         y + h - r, Black); // kairė
  drawLine(x + w, y + r, x + w,     y + h - r, Black); // dešinė
  drawArcCorner(x + r,     y + r,     r, PI,       1.5 * PI); // v. kairė
  drawArcCorner(x + w - r, y + r,     r, 1.5 * PI, 2 * PI);   // v. dešinė
  drawArcCorner(x + w - r, y + h - r, r, 0,        0.5 * PI); // a. dešinė
  drawArcCorner(x + r,     y + h - r, r, 0.5 * PI, PI);       // a. kairė
}

// Drabužių piktogramos: (x - centras, y - viršus, s - bazinis plotis). Apvalūs kampai.
void DrawTShirtIcon(int x, int y, int s) {
  int bw = s, bh = s * 1.15, sw = s / 3, sh = s / 2, r = s / 7;
  drawRoundRect(x - bw / 2, y, bw, bh, r);
  drawRoundRect(x - bw / 2 - sw, y, sw, sh, r / 2); // trumpos rankovės
  drawRoundRect(x + bw / 2, y, sw, sh, r / 2);
  drawLine(x - s / 5, y, x, y + s / 6, Black);      // kaklo iškirptė
  drawLine(x + s / 5, y, x, y + s / 6, Black);
}

void DrawSweaterIcon(int x, int y, int s) {
  int bw = s, bh = s * 1.15, sw = s / 3, sh = s * 0.9, r = s / 7;
  drawRoundRect(x - bw / 2, y, bw, bh, r);
  drawRoundRect(x - bw / 2 - sw, y, sw, sh, r / 2); // ilgos rankovės
  drawRoundRect(x + bw / 2, y, sw, sh, r / 2);
  drawLine(x - s / 5, y, x, y + s / 6, Black);
  drawLine(x + s / 5, y, x, y + s / 6, Black);
  for (int row = 0; row < 2; row++) {               // mezgimo bangos
    int yy = y + s / 2 + row * s / 4;
    for (int i = 0; i < 3; i++) {
      int xx = x - 3 * s / 8 + i * s / 4;
      drawLine(xx, yy, xx + s / 8, yy - s / 12, Black);
      drawLine(xx + s / 8, yy - s / 12, xx + s / 4, yy, Black);
    }
  }
  for (int i = 1; i < 5; i++)                        // rumbuotas apvadas
    drawLine(x - bw / 2 + i * bw / 5, y + bh - s / 8, x - bw / 2 + i * bw / 5, y + bh, Black);
}

void DrawJacketIcon(int x, int y, int s) {
  int bw = s, bh = s * 1.15, sw = s / 3, sh = s * 0.9, r = s / 7;
  drawRoundRect(x - bw / 2, y, bw, bh, r);
  drawRoundRect(x - bw / 2 - sw, y, sw, sh, r / 2); // ilgos rankovės
  drawRoundRect(x + bw / 2, y, sw, sh, r / 2);
  drawLine(x, y + s / 6, x, y + bh, Black);         // užtrauktukas
  drawLine(x + 1, y + s / 6, x + 1, y + bh, Black);
  fillTriangle(x - s / 4, y, x - 2, y + s / 6, x - s / 8, y + s / 4, Black); // apykaklės atvartai
  fillTriangle(x + s / 4, y, x + 2, y + s / 6, x + s / 8, y + s / 4, Black);
}

void DrawHatIcon(int x, int y, int s) {
  int r = s / 2;
  int cy = y + r + s / 8;
  for (float ang = PI; ang <= 2 * PI; ang += 0.02) { // kupolas
    drawPixel(x + r * cos(ang), cy + r * sin(ang), Black);
    drawPixel(x + r * cos(ang), cy + r * sin(ang) + 1, Black);
  }
  fillRect(x - r - s / 10, cy - s / 14, 2 * r + s / 5, s / 7, Black); // atvartas
  fillCircle(x, cy - r - s / 10, s / 10, Black);                     // bumbulas
}

void DrawUmbrellaIcon(int x, int y, int s) {
  int r = s / 2 + s / 6;
  int cy = y + r;
  for (float ang = PI; ang <= 2 * PI; ang += 0.02) { // kupolas
    drawPixel(x + r * cos(ang), cy + r * sin(ang), Black);
    drawPixel(x + r * cos(ang), cy + r * sin(ang) + 1, Black);
  }
  drawLine(x - r, cy, x + r, cy, Black);             // apačia
  drawLine(x, cy - r - 4, x, cy, Black);             // smaigalys ir kotas per kupolą
  drawLine(x, cy, x, y + s * 1.15, Black);
  drawLine(x + 1, cy, x + 1, y + s * 1.15, Black);
  for (float ang = 0; ang <= PI; ang += 0.05)        // rankenos kabliukas
    drawPixel(x - s / 8 + (s / 8) * cos(ang), y + s * 1.15 + (s / 8) * sin(ang), Black);
}

int FindDayPart(int startHour, int endHour) { // artimiausias prognozės įrašas su vietos valanda intervale
  for (int r = 0; r < 8; r++) {
    time_t t = WxForecast[r].Dt;
    struct tm *lt = localtime(&t);
    if (lt->tm_hour >= startHour && lt->tm_hour <= endHour) return r;
  }
  return -1;
}

// Dienos dalis: antraštė viršuje (12B: yTop..+28), žemiau ikona ir temperatūra šalia (18B: +36..+78)
void DrawDayPart(int x, int yTop, String label, int idx) {
  if (idx < 0) return;
  setFont(&OpenSans12B);
  drawStringTop(x, yTop, label, CENTER);
  DisplayConditionsSection(x - 46, yTop + 58, WxForecast[idx].Icon, SmallIcon); // ikonos centras
  setFont(&OpenSans18B);
  drawStringTop(x + 44, yTop + 36, String(WxForecast[idx].Temperature, 0) + "°", CENTER);
}

// Išskaido tekstą į eilutes pagal IŠMATUOTĄ pikselių plotį (ne pagal spėtą simbolių skaičių)
int WrapMeasured(String text, int maxW, String out[], int maxLines) {
  int n = 0;
  text.trim();
  while (text.length() > 0 && n < maxLines) {
    String line = text;
    while (line.length() > 0 && textWidthOf(line) > maxW) {
      int sp = line.lastIndexOf(' ');
      if (sp <= 0) break;                       // vienas per ilgas žodis - paliekam kaip yra
      line = line.substring(0, sp);
    }
    out[n++] = line;
    if (line.length() >= text.length()) break;
    text = text.substring(line.length());
    text.trim();
  }
  return n;
}

// Viršutinis mygtuko indikatorius: trikampis kampu į viršų ties fiziniu mygtuku
void DrawTopButtonHint(bool wifeMode) {
  fillTriangle(360, 4, 348, 20, 372, 20, Black);
  setFont(&OpenSans12B);
  drawString(384, 4, wifeMode ? "spausk viršutinį mygtuką - PILNA PROGNOZĖ"
                              : "spausk viršutinį mygtuką - PAPRASTAS VAIZDAS", LEFT);
}

// Apatinis baras: miestas + data + WiFi + baterija (abu režimai), linija virš jo
void DisplayBottomBar() {
  drawLine(5, 498, 955, 498, Grey);
  setFont(&OpenSans12B);
  drawString(15, 505, City, LEFT);
  drawString(150, 505, Date_str + "  @  " + Time_str, LEFT);
  DrawBattery(620, 524);          // patraukta kairiau, kad įtampa nesuliptų su WiFi brūkšneliais
  DrawRSSI(878, 532, wifi_signal);
}

// REGIONŲ LENTELĖ (960x540). Aukščiai IŠMATUOTI (get_text_bounds), žingsnis = šrifto advance_y.
// Šriftų advance_y: 8B=22, 10B=28, 12B=33, 18B=50, 48B=133 (48B "17°" realus h=71).
//   R1 Temperatūra     y   6..158   ikona, „jaučiasi kaip"(18B), jutiminė(48B), termometras(12B), dešinys stulpelis(x620)
//   L1 linija          y 164
//   R2 Aprangos pat.   y 168..316   KAIP RENGTIS(8B), ikonos(x 36..284, y 194..249), patarimas(18B x2), pastaba(12B)
//   L2 linija          y 324
//   R3 Adaptacija      y 330..386   Korekcija(10B), išvada(12B)
//   R4 Dienos eiga     y 394..472   antraštė(12B), ikona+temp(18B)
//   L3 + R5 baras      y 498..534   (DisplayBottomBar)
void DisplayWifeMode() {
  // --- R1: temperatūros blokas ---
  DisplayConditionsSection(120, 85, WxConditions[0].Icon, LargeIcon);
  setFont(&OpenSans18B);
  drawStringTop(410, 6, "jaučiasi kaip", CENTER);                                     // 6..48
  setFont(&OpenSans48B);
  drawStringTop(410, 52, String(WxConditions[0].Feelslike, 0) + "°", CENTER);         // 52..123
  setFont(&OpenSans12B);
  drawStringTop(270, 128, "termometras rodo " + String(WxConditions[0].Temperature, 0) + "°", LEFT); // 128..156
  // Dešinys stulpelis: DIENOS temperatūros riba (ne tik dabartinė), vėjas, lietus
  float dMax = WxConditions[0].Temperature, dMin = WxConditions[0].Temperature;
  for (int r = 0; r < 8; r++) {
    time_t ft = WxForecast[r].Dt; struct tm *flt = localtime(&ft);
    if (flt->tm_hour < 6 || flt->tm_hour > 22) continue; // tik dienos metas
    if (WxForecast[r].Temperature > dMax) dMax = WxForecast[r].Temperature;
    if (WxForecast[r].Temperature < dMin) dMin = WxForecast[r].Temperature;
  }
  setFont(&OpenSans18B);
  fillTriangle(632, 12, 622, 32, 642, 32, Black);                                     // ▲ dienos maks
  drawStringTop(654, 6, String(dMax, 0) + "°", LEFT);                                 // 6..48
  fillTriangle(632, 88, 622, 68, 642, 68, Black);                                     // ▼ dienos min
  drawStringTop(654, 52, String(dMin, 0) + "°", LEFT);                                // 52..94
  setFont(&OpenSans12B);
  drawStringTop(622, 100, String(WxConditions[0].Windspeed, 0) + " m/s " + WindDegToOrdinalDirection(WxConditions[0].Winddir), LEFT); // 100..128
  float pop = max(WxForecast[0].Pop, max(WxForecast[1].Pop, WxForecast[2].Pop));
  drawStringTop(622, 130, "lietus " + String((int)round(pop * 100)) + "%", LEFT);     // 130..158
  drawLine(20, 164, 940, 164, Black);                                                 // L1

  // --- R2: aprangos patarimas (be rėmelio; ikonos kairėje, tekstas fiksuotoje zonoje) ---
  ClothingAdvice adv = GetClothingAdvice();
  int iy = 194, is_ = 48, ix = 60;                                                    // ikonos 194..249
  #define DRAW2(FN) do { FN(ix, iy, is_); FN(ix + 1, iy + 1, is_); ix += 92; } while (0) // 2x - storesnis kontūras
  if (adv.tshirt)   DRAW2(DrawTShirtIcon);
  if (adv.sweater)  DRAW2(DrawSweaterIcon);
  if (adv.jacket)   DRAW2(DrawJacketIcon);
  if (adv.hat)      DRAW2(DrawHatIcon);
  if (adv.umbrella) DRAW2(DrawUmbrellaIcon);                                          // skėtis tik kai reikia
  #undef DRAW2
  const int tx = 350, tw = 590;                                                       // teksto zona x 350..940
  setFont(&OpenSans8B);
  drawStringTop(tx, 168, "KAIP RENGTIS", LEFT);                                       // 168..188
  setFont(&OpenSans18B);
  String lines[2];
  int n = WrapMeasured(adv.text, tw, lines, 2);
  for (int i = 0; i < n; i++) drawStringTop(tx, 192 + i * 48, lines[i], LEFT);        // 192..236, 240..284
  if (adv.note.length()) {
    setFont(&OpenSans12B);
    String nl[1];
    if (WrapMeasured(adv.note, tw, nl, 1) > 0) drawStringTop(tx, 288, nl[0], LEFT);   // 288..316
  }
  drawLine(20, 324, 940, 324, Black);                                                 // L2

  // --- R3: adaptacija (korekcija, kada klausta/atsakyta, kada kitas klausimas, išvada) ---
  auto dayToStr = [](int dayNum) -> String {                                          // time/86400 -> "MM-DD"
    if (dayNum <= 0) return "-";
    time_t t = (time_t)dayNum * 86400L;
    struct tm *lt = gmtime(&t);
    char buf[8]; sprintf(buf, "%02d-%02d", lt->tm_mon + 1, lt->tm_mday);
    return String(buf);
  };
  int todayNum = (int)(time(NULL) / 86400);
  String nextAsk = (LastAskDay != todayNum && CurrentHour < FeedbackHr)
                   ? ("šiandien " + String(FeedbackHr) + ":00")
                   : ("rytoj " + String(FeedbackHr) + ":00");
  setFont(&OpenSans10B);
  drawStringTop(30, 330, "Korekcija " + String(ChillBias, 1) + "°    klausta: " + dayToStr(LastAskDay)
                + "    atsakyta: " + dayToStr(FbLastDay) + "    kitas: " + nextAsk, LEFT);  // 330..354
  setFont(&OpenSans12B);
  String concl = FeedbackConclusion();
  if (concl.length()) concl.setCharAt(0, toupper(concl.charAt(0)));
  drawStringTop(30, 358, concl, LEFT);                                                // 358..386

  // --- R4: dienos eiga ---
  DrawDayPart(160, 394, "Rytas",   FindDayPart(6, 10));
  DrawDayPart(480, 394, "Diena",   FindDayPart(11, 16));
  DrawDayPart(800, 394, "Vakaras", FindDayPart(17, 22));
  drawLine(320, 390, 320, 490, LightGrey);
  drawLine(640, 390, 640, 490, LightGrey);

  DisplayBottomBar();                                                                 // L3 + R5
}

void DisplayWeather() {                          // 4.7" e-paper display is 960x540 resolution
  // Tas pats išdėstymas kaip anksčiau, tik viskas pastumta ~30 px aukštyn (atsilaisvino viršus),
  // o info/status juosta perkelta į apačią (DisplayBottomBar). Grafikai - pilno dydžio.
  DisplayDisplayWindSection(137, 120, WxConditions[0].Winddir, WxConditions[0].Windspeed, 100);
  DisplayAstronomySection(5, 225);
  DisplayMainWeatherSection(320, 80);
  DisplayWeatherIcon(810, 100);
  DisplayForecastSection(320, 190);              // 3hr forecast boxes
  DisplayBottomBar();                            // Status baras apačioje (miestas+data+wifi+baterija)
}

//VSMOD
void DisplayGeneralInfoSection() {
  setFont(&OpenSans10B); //10B
  drawString(5, 2, City, LEFT);
  setFont(&OpenSans12B);//8B
  drawString(200, 2, Date_str + "  @   " + Time_str, LEFT); //500
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions[0].Icon, LargeIcon);
}

//VSADD
void DisplayMainWeatherSection(int x, int y) {
  setFont(&OpenSans8B);
  DisplayTemperatureSection(x, y - 40);
  DisplayForecastTextSection(x - 55, y + 25);
  DisplayPressureSection(x - 25, y + 90, WxConditions[0].Pressure, WxConditions[0].Trend);
}

void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, Cradius - 22, angle, 18, 33); // Show wind direction on outer circle of width and length
  setFont(&OpenSans8B);
  int dxo, dyo, dxi, dyi;
  drawCircle(x, y, Cradius, Black);       // Draw compass circle
  drawCircle(x, y, Cradius + 1, Black);   // Draw compass circle
  drawCircle(x, y, Cradius * 0.7, Black); // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 15, dyo + y - 18, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 20, dyo + y - 2,  TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x - 20, dyo + y - 2,  TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 18, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
  }
  drawString(x, y - Cradius - 20,     TXT_N, CENTER);
  drawString(x, y + Cradius + 10,     TXT_S, CENTER);
  drawString(x - Cradius - 15, y - 5, TXT_W, CENTER);
  drawString(x + Cradius + 10, y - 5, TXT_E, CENTER);
  drawString(x + 3, y + 50, String(angle, 0) + "°", CENTER);
  setFont(&OpenSans12B);
  drawString(x, y - 50, WindDegToOrdinalDirection(angle), CENTER);
  setFont(&OpenSans24B);
  drawString(x + 3, y - 18, String(windspeed, 1), CENTER);
  setFont(&OpenSans12B);
  drawString(x, y + 25, (Units == "M" ? "m/s" : "mph"), CENTER);
}

String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
  if (winddirection >=  11.25 && winddirection < 33.75)  return TXT_NNE;
  if (winddirection >=  33.75 && winddirection < 56.25)  return TXT_NE;
  if (winddirection >=  56.25 && winddirection < 78.75)  return TXT_ENE;
  if (winddirection >=  78.75 && winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
  return "?";
}

void DisplayTemperatureSection(int x, int y) {
  setFont(&OpenSans18B);
  drawString(x - 30, y, String(WxConditions[0].Temperature, 1) + "°    " + String(WxConditions[0].Humidity, 0) + "%", LEFT);
  setFont(&OpenSans12B);
  drawString(x + 10, y + 35, String(WxConditions[0].High, 0) + "° | " + String(WxConditions[0].Low, 0) + "°", CENTER); // Show forecast high and Low
}

void DisplayForecastTextSection(int x, int y) {
#define lineWidth 34
  setFont(&OpenSans12B);
  //Wx_Description = WxConditions[0].Main0;          // e.g. typically 'Clouds'
  String Wx_Description = WxConditions[0].Forecast0; // e.g. typically 'overcast clouds' ... you choose which
  Wx_Description.replace(".", ""); // remove any '.'
  int spaceRemaining = 0, p = 0, charCount = 0, Width = lineWidth;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > Width - 1) { // '~' is the end of line marker
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (WxForecast[0].Rainfall > 0) Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((Units == "M" ? "mm" : "in")) + ")";
  //Wx_Description = wordWrap(Wx_Description, lineWidth);
  String Line1 = Wx_Description.substring(0, Wx_Description.indexOf("~"));
  String Line2 = Wx_Description.substring(Wx_Description.indexOf("~") + 1);
  drawString(x + 30, y + 5, TitleCase(Line1), LEFT);
  if (Line1 != Line2) drawString(x + 30, y + 30, Line2, LEFT);
}

//VSMOD PREASURE
void DisplayPressureSection(int x, int y, float pressure, String slope) {
  setFont(&OpenSans12B);
  DrawPressureAndTrend(x - 25, y + 10, pressure, slope);
  if (WxConditions[0].Visibility > 0) {
    Visibility(x + 165, y, String(WxConditions[0].Visibility) + "M"); //145
    x += 150; // Draw the text in the same positions if one is zero, otherwise in-line
  }
  if (WxConditions[0].Cloudcover > 0) CloudCover(x + 175, y, WxConditions[0].Cloudcover);//145
}

void DisplayForecastWeather(int x, int y, int index) {
  int fwidth = 90;
  x = x + fwidth * index;
  DisplayConditionsSection(x + fwidth / 2, y + 90, WxForecast[index].Icon, SmallIcon);
  setFont(&OpenSans10B);
  drawString(x + fwidth / 2, y + 30, String(ConvertUnixTime(WxForecast[index].Dt + WxConditions[0].Timezone).substring(0, 5)), CENTER);
  drawString(x + fwidth / 2, y + 125, String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°", CENTER);
}

void DisplayAstronomySection(int x, int y) {
  setFont(&OpenSans10B);
  drawString(x + 5, y + 30, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5) + " " + TXT_SUNRISE, LEFT);
  drawString(x + 5, y + 50, ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5) + " " + TXT_SUNSET, LEFT);
  time_t now = time(NULL);
  struct tm * now_utc  = gmtime(&now);
  const int day_utc    = now_utc->tm_mday;
  const int month_utc  = now_utc->tm_mon + 1;
  const int year_utc   = now_utc->tm_year + 1900;
  drawString(x + 5, y + 80, MoonPhase(day_utc, month_utc, year_utc, Hemisphere), LEFT);
  DrawMoon(x + 160, y - 15, day_utc, month_utc, year_utc, Hemisphere);
}

void DrawMoon(int x, int y, int dd, int mm, int yy, String hemisphere) {
  const int diameter = 75;
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "south") Phase = 1 - Phase;
  // Draw dark part of moon
  fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, LightGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    drawLine(pW1x, pW1y, pW2x, pW2y, White);
    drawLine(pW3x, pW3y, pW4x, pW4y, White);
  }
  drawCircle(x + diameter - 1, y + diameter, diameter / 2, Black);
}

String MoonPhase(int d, int m, int y, String hemisphere) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d - 694039.09;     /* jd is total days elapsed */
  jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
  b   = jd;                              /* int(jd) -> b, take integer part of jd */
  jd -= b;                               /* subtract integer part to leave fractional part of original jd */
  b   = jd * 8 + 0.5;                /* scale fraction from 0-8 and round by adding 0.5 */
  b   = b & 7;                           /* 0 and 8 are the same phase so modulo 8 for 0 */
  if (hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}

void DisplayForecastSection(int x, int y) {
  int f = 0;
  do {
    DisplayForecastWeather(x, y, f);  //VS MOD
    f++;
  } while (f < max_readings);
  int r = 0;
  do { // Pre-load temporary arrays with with data - because C parses by reference and remember that[1] has already been converted to I units
  //VSMOD
    //if (Units == "I") pressure_readings[r] = WxForecast[r].Pressure * 0.02953;   else pressure_readings[r] = WxForecast[r].Pressure;
    if (Units == "I") pressure_readings[r] = WxForecast[r].Pressure * 0.02953;   else pressure_readings[r] = hPa_to_mmHg(WxForecast[r].Pressure);
    if (Units == "I") rain_readings[r]     = WxForecast[r].Rainfall * 0.0393701; else rain_readings[r]     = WxForecast[r].Rainfall;
    if (Units == "I") snow_readings[r]     = WxForecast[r].Snowfall * 0.0393701; else snow_readings[r]     = WxForecast[r].Snowfall;
    temperature_readings[r]                = WxForecast[r].Temperature;
    humidity_readings[r]                   = WxForecast[r].Humidity;
    r++;
  } while (r < max_readings);
  int gwidth = 175, gheight = 100;                // pilno dydžio grafikai (skaičiai nebesusistumia)
  int gx = (SCREEN_WIDTH - gwidth * 4) / 5 + 8;
  int gy = (SCREEN_HEIGHT - gheight - 65);         // pakelti, kad apačioje tilptų status baras
  int gap = gwidth + gx;
  //VSMOD TO
  //float hPa_to_mmHg(float value_hPa) // return 0.750062 * value_hPa; //675   788
  // (x,y,width,height,MinValue, MaxValue, Title, Data Array, AutoScale, ChartMode)
  DrawGraph(gx + 0 * gap, gy, gwidth, gheight, 675, 788, Units == "M" ? TXT_PRESSURE_HG : TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 1 * gap, gy, gwidth, gheight, 10, 30, Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 2 * gap, gy, gwidth, gheight, 0, 100, Units == "M" ? TXT_RAINFALL_MM : TXT_RAINFALL_IN, rain_readings, max_readings, autoscale_on, barchart_on);

  //humidity_readings
  //snow_readings
  if (SumOfPrecip(snow_readings, max_readings) >2)
    DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, Units == "M" ? TXT_SNOWFALL_MM : TXT_SNOWFALL_IN, snow_readings, max_readings, autoscale_on, barchart_on);
  else    
   DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, 0, 100, TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off);    
}

void DisplayConditionsSection(int x, int y, String IconName, bool IconSize) {
  #ifdef SERIAL_DEBUG 
    DBG("Icon name: " + IconName);
  #endif
  if      (IconName == "01d" || IconName == "01n")  Sunny(x, y, IconSize, IconName);
  else if (IconName == "02d" || IconName == "02n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "03d" || IconName == "03n")  Cloudy(x, y, IconSize, IconName);
  else if (IconName == "04d" || IconName == "04n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "09d" || IconName == "09n")  ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "10d" || IconName == "10n")  Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n")  Tstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n")  Snow(x, y, IconSize, IconName);
  else if (IconName == "50d")                       Haze(x, y, IconSize, IconName);
  else if (IconName == "50n")                       Fog(x, y, IconSize, IconName);
  else                                              Nodata(x, y, IconSize, IconName);
}

void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, Black);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  drawLine(x + o1,  y + o2,  x + o3,  y + o4,  Black);
  drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}
//VSADD draw preasure
void DrawPressureAndTrend(int x, int y, float pressure, String slope) {
 //drawString(x + 25, y - 10, String(pressure, (Units == "M" ? 0 : 1)) + (Units == "M" ? "hPa" : "in"), LEFT);
 drawString(x + 25, y-16, String(hPa_to_mmHg(pressure), (Units == "M" ? 0 : 1)) + (Units == "M" ? "mmHg" : "in"), LEFT);//y-10
  if      (slope == "+") {
    DrawSegment(x, y, 0, 0, 8, -8, 8, -8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, -8, 8, -8, 16, 0);
  }
  else if (slope == "0") {
    DrawSegment(x, y, 8, -8, 16, 0, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 8, -8, 16, 0, 8, 8, 16, 0);
  }
  else if (slope == "-") {
    DrawSegment(x, y, 0, 0, 8, 8, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, 8, 8, 8, 16, 0);
  }
}

//VSMOD
void DisplayStatusSection(int x, int y, int rssi) {
  setFont(&OpenSans12B);
  DrawRSSI(x + 320, y, rssi); //300
  DrawBattery(x+90, y); // 150
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 30; //            <-20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 24; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 18; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 12; //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 6;  // -100dbm to  -81dbm displays 1-bar
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
}

boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
  #ifdef SERIAL_DEBUG 
    DBG("Failed to obtain time");
  #endif
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  #ifdef SERIAL_DEBUG 
  //See http://www.cplusplus.com/reference/ctime/strftime/
    Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  #endif  
  if (Units == "M") {
    sprintf(day_output, "%s, %02u-%s-%04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  return true;
}

void ReadBattery() { // Nuskaito ADC ir užpildo BatteryPct/BatteryVoltage (naudoja ir ekranas, ir Telegram perspėjimas)
  esp_adc_cal_characteristics_t adc_chars;
  //Slopinimas orig DB_11
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 0, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    #ifdef SERIAL_DEBUG
      Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    #endif
    vref = adc_chars.vref;
  }
  //VS MOD
  float voltage = analogRead(14) / 4096.0 * 6.100 * (vref / 1000.0); // VS 14 PIN
  if (voltage > 1) { // Only valid reading
    DBG("\nVoltage = " + String(voltage));
    int pct = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) pct = 100;
    if (voltage <= 3.20) pct = 0;  // orig 3.5
    BatteryPct = constrain(pct, 0, 100); // polinomas gali duoti <0 arba >100
    BatteryVoltage = voltage;
  }
  else BatteryPct = -1;
}

void DrawBattery(int x, int y) {
  if (BatteryPct < 0) return; // nėra korektiško nuskaitymo
  drawRect(x + 25, y - 14, 40, 15, Black);
  fillRect(x + 65, y - 10, 4, 7, Black);
  fillRect(x + 27, y - 12, 36 * BatteryPct / 100.0, 11, Black);
  drawString(x + 85, y - 14, String(BatteryPct) + "%  " + String(BatteryVoltage, 2) + "v", LEFT);
}

// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  fillCircle(x - scale * 3, y, scale, Black);                                                              // Left most circle
  fillCircle(x + scale * 3, y, scale, Black);                                                              // Right most circle
  fillCircle(x - scale, y - scale, scale * 1.4, Black);                                                    // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, Black);                                       // Right middle upper circle
  fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, Black);                                 // Upper and lower lines
  fillCircle(x - scale * 3, y, scale - linesize, White);                                                   // Clear left most circle
  fillCircle(x + scale * 3, y, scale - linesize, White);                                                   // Clear right most circle
  fillCircle(x - scale, y - scale, scale * 1.4 - linesize, White);                                         // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, White);                            // Right middle upper circle
  fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, White); // Upper and lower lines
}

void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(&OpenSans8B);
    drawString(x - 25, y + 12, "///////", LEFT);
  }
  else
  {
    setFont(&OpenSans18B);
    drawString(x - 60, y + 25, "///////", LEFT);
  }
}

void addsnow(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(&OpenSans8B);
    drawString(x - 25, y + 15, "* * * *", LEFT);
  }
  else
  {
    setFont(&OpenSans18B);
    drawString(x - 60, y + 30, "* * * *", LEFT);
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, Black);
    if (scale != Small) {
      drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, Black);
      drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, Black);
    }
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, Black);
    if (scale != Small) {
      drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, Black);
      drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, Black);
    }
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, Black);
    if (scale != Small) {
      drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, Black);
      drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, Black);
    }
  }
}

void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 5;
  fillRect(x - scale * 2, y, scale * 4, linesize, Black);
  fillRect(x, y - scale * 2, linesize, scale * 4, Black);
  drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, Black);
  drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, Black);
  if (IconSize == LargeIcon) {
    drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(2 + x - scale * 1.3, y - scale * 1.3, 2 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(3 + x - scale * 1.3, y - scale * 1.3, 3 + x + scale * 1.3, y + scale * 1.3, Black);
    drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, Black);
    drawLine(2 + x - scale * 1.3, y + scale * 1.3, 2 + x + scale * 1.3, y - scale * 1.3, Black);
    drawLine(3 + x - scale * 1.3, y + scale * 1.3, 3 + x + scale * 1.3, y - scale * 1.3, Black);
  }
  fillCircle(x, y, scale * 1.3, White);
  fillCircle(x, y, scale, Black);
  fillCircle(x, y, scale - linesize, White);
}

void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) {
    y -= 10;
    linesize = 1;
  }
  for (int i = 0; i < 6; i++) {
    fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, Black);
  }
}

void Sunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  else y = y - 3; // Shift up small sun icon
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  scale = scale * 1.6;
  addsun(x, y, scale, IconSize);
}

void MostlySunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
}

void MostlyCloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
}

void Cloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x + 15, y - 22, scale / 2, linesize); // Cloud top right
  addcloud(x - 10, y - 18, scale / 2, linesize); // Cloud top left
  addcloud(x, y, scale, linesize);             // Main cloud
}

void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void ExpectRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}

void Tstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addtstorm(x, y, scale);
}

void Snow(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsnow(x, y, scale, IconSize);
}

void Fog(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addcloud(x, y - 5, scale, linesize);
  addfog(x, y - 5, scale, linesize, IconSize);
}

void Haze(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5, Offset = 10;
  if (IconSize == LargeIcon) {
    scale = Large;
    Offset = 35;
  }
  if (IconName.endsWith("n")) addmoon(x, y + Offset, scale, IconSize);
  addsun(x, y - 5, scale * 1.4, IconSize);
  addfog(x, y - 5, scale * 1.4, linesize, IconSize);
}

void CloudCover(int x, int y, int CCover) {
  addcloud(x - 9, y + 2, Small * 0.3, 2); // Cloud top left
  addcloud(x + 3, y - 2, Small * 0.3, 2); // Cloud top right
  addcloud(x, y + 10, Small * 0.6, 2); // Main cloud
  drawString(x + 20, y, String(CCover) + "%", LEFT);
}

void Visibility(int x, int y, String Visi) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 8;
  int r = 14;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y - r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i) + Offset, Black);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, Black);
  }
  fillCircle(x, y + Offset, r / 4, Black);
  drawString(x + 20, y, Visi, LEFT);
}

void addmoon(int x, int y, int scale, bool IconSize) {
  if (IconSize == LargeIcon) {
    fillCircle(x - 85, y - 100, uint16_t(scale * 0.8), Black);
    fillCircle(x - 57, y - 100, uint16_t(scale * 1.6), White);
  }
  else
  {
    fillCircle(x - 28, y - 37, uint16_t(scale * 1.0), Black);
    fillCircle(x - 20, y - 37, uint16_t(scale * 1.6), White);
  }
}

void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) setFont(&OpenSans24B); else setFont(&OpenSans12B);
  drawString(x - 3, y - 10, "?", CENTER);
}

/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos-the x axis top-left position of the graph
    y_pos-the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width-the width of the graph in pixels
    height-height of the graph in pixels
    Y1_Max-sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale-a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on-a logical value (TRUE or FALSE) that switches the drawing mode between barhcart and line graph
    barchart_colour-a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up fter a change of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
  setFont(&OpenSans10B);
  int maxYscale = -10000;
  int minYscale =  10000;
  int last_x, last_y;
  float x2, y2;
  if (auto_scale == true) {
    for (int i = 1; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  // Draw the graph
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, Grey);
  //VS MOD
 // drawString(x_pos - 20 + gwidth / 2, y_pos - 28, title, CENTER);
  drawString(x_pos + gwidth / 2, y_pos - 28, title, CENTER);
  for (int gx = 0; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1 ; // max_readings is the global variable that sets the maximum data that can be plotted
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      fillRect(last_x + 2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black); // Two lines for hi-res display
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 20
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
      if (spacing < y_minor_axis) drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), Grey);
    }
    if ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) < 5 || title == TXT_PRESSURE_IN) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
    }
    else
    {
      if (Y1Min < 1 && Y1Max < 10) {
        drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
      }
      else {
        drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    drawString(20 + x_pos + gwidth / 3 * i, y_pos + gheight + 10, String(i) + "d", LEFT);
    if (i < 2) drawFastVLine(x_pos + gwidth / 3 * i + gwidth / 3, y_pos, gheight, LightGrey);
  }
}

void drawString(int x, int y, String text, alignment align) {
  // Lietuviškos raidės rodomos tiesiogiai - šriftai sugeneruoti su U+0104-U+017E glifais
  char * data  = const_cast<char*>(text.c_str());
  int  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  int w, h;
  int xx = x, yy = y;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  write_string(&currentFont, data, &x, &cursor_y, framebuffer);
}

// --- Teksto MATAVIMAS ir tikslus pozicionavimas ---------------------------------------
// get_text_bounds su cursor_y = 0 pasako, kad tekstas užims vertikaliai [y1, y1 + h].
// Todėl norint, kad VIRŠUS būtų tiksliai ties yTop, cursor_y = yTop - y1.
// Taip layout'as remiasi bibliotekos matavimu, o ne spėjimu (senasis drawString naudoja
// cursor_y = y + h, tad jo 'y' nėra nei viršus, nei bazinė linija - netinka tiksliam dėliojimui).
int textWidthOf(String text) {
  char *data = const_cast<char*>(text.c_str());
  int x1, y1, w, h, xx = 0, yy = 0;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  return w;
}

int textHeightOf(String text) {
  char *data = const_cast<char*>(text.c_str());
  int x1, y1, w, h, xx = 0, yy = 0;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  return h;
}

void drawStringTop(int x, int yTop, String text, alignment align) {
  char *data = const_cast<char*>(text.c_str());
  int x1, y1, w, h, xx = x, yy = 0;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  // Bibliotekos get_char_bounds skaičiuoja "y aukštyn": y2 = baseline + glifo top.
  // Ekrane (y žemyn) rašalo viršus = baseline - (y1 + h), todėl baseline = yTop + y1 + h.
  // Patikra 48B "20°": y1=-1, h=72 -> baseline = yTop+71, viršus = baseline-71 = yTop. OK.
  int cursor_y = yTop + y1 + h;
  write_string(&currentFont, data, &x, &cursor_y, framebuffer);
}

void fillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_write_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect(x, y, w, h, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect(x, y, w, h, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}

// Šios funkcijos yra „tiltas“ tarp jūsų kodo ir EPD bibliotekos
void setFont(const GFXfont *font) {
    currentFont = *font;
}

void drawPixel(int x, int y, uint16_t color) {
    epd_draw_pixel(x, y, color, framebuffer);
}

/*
   1085 lines of code 28-01-2021
*/
