// ŠABLONAS: nukopijuokite šį failą kaip owm_credentials.h ir įrašykite savo duomenis.
// owm_credentials.h yra .gitignore sąraše - jis niekada nekeliaus į git.

const bool DebugDisplayUpdate = false;

// WiFi nustatomas per WiFiManager portalą (AP "OruStotele-Setup", 192.168.4.1).
// Šie duomenys naudojami tik kaip pradinis užpildymas pirmam paleidimui; galima palikti tuščius "".
const char* ssid     = "";
const char* password = "";

// Use your own API key by signing up for a free developer account at https://openweathermap.org/
String apikey       = "JUSU_OWM_API_RAKTAS";
const char server[] = "api.openweathermap.org";

//Set your location according to OWM locations
String City             = "Vilnius";
String Country          = "LT";
String Language         = "LT";                            // NOTE: Only the weather description is translated by OWM
String Hemisphere       = "north";                         // or "south"
String Units            = "M";                             // Use 'M' for Metric or I for Imperial
const char* Timezone    = "EET-2EEST,M3.5.0/3,M10.5.0/4";  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* ntpServer   = "0.europe.pool.ntp.org";
int   gmtOffset_sec     = 0;
int  daylightOffset_sec = 3600;

// --- Telegram (grįžtamasis ryšys ir baterijos perspėjimas) ---
// Token gaunamas telefone per @BotFather (komanda /newbot). Kol tuščias - Telegram funkcijos išjungtos.
const char* telegramBotToken = "";
// Chat ID galima palikti tuščią: įrenginys pats įsimins pirmą parašiusį žmogų (parašykite botui bet ką).
const char* telegramChatID   = "";
const int   FeedbackHour     = 20;  // kurią valandą klausti "ar tiko apranga?"
