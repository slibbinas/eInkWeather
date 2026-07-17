#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>
// Pridėkite šią eilutę, kad atpažintų alignment tipą
typedef enum { LEFT, RIGHT, CENTER } alignment;

// --- SISTEMOS IR TINKLO ---
void BeginSleep();
boolean SetupTime();
uint8_t StartWiFi();
void StopWiFi();
void InitialiseSystem();
bool obtainWeatherData(WiFiClient & client, const String & RequestType);
bool DecodeWeather(WiFiClient& json, String Type);
boolean UpdateLocalTime();

// --- EKRANO VALDYMO IR PIEŠIMO (pagalbinės) ---
void edp_update();
void setFont(const GFXfont *font);
void write_string(const GFXfont* font, String data, int* x, int* y, uint8_t* framebuffer);
void fillCircle(int x, int y, int r, uint8_t color);
void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void drawCircle(int x0, int y0, int r, uint8_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawString(int x, int y, String text, alignment align);
// Matavimu paremtas pozicionavimas (layout'as iš faktų, ne iš akies)
void drawStringTop(int x, int yTop, String text, alignment align);
int  textWidthOf(String text);
int  textHeightOf(String text);
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void drawPixel(int x, int y, uint16_t color);
void drawStringAuto(int x, int y, String text, alignment align);
void DrawIcon(int x, int y, const uint8_t* bmp, int w, int h);   // nespalvoto drabužio bitmapo piešimas (adaptyvus dydis)

// --- PAGRINDINIO EKRANO PIEŠIMO BLOKAI ---
void DisplayWeather();
void DisplayWifeMode();
void DrawVersionTag();
void DisplayGeneralInfoSection();
void DisplayConditionsSection(int x, int y, String IconName, bool IconSize);
void DisplayWeatherIcon(int x, int y);
void DisplayMainWeatherSection(int x, int y);
void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius);
void DisplayStatusSection(int x, int y, int rssi);
void DisplayTemperatureSection(int x, int y);
void DisplayForecastTextSection(int x, int y);
void DisplayPressureSection(int x, int y, float pressure, String slope);
void DrawRSSI(int x, int y, int rssi);
void DrawBattery(int x, int y);
void ReadBattery();
void TelegramSync();
void StartOtaMode();
void TelegramTestMode();
void SaveDailyAdvice();
void SelfUpdateCheck(bool force);
void DisplayForecastWeather(int x, int y, int index);
void DisplayAstronomySection(int x, int y);
void DisplayForecastSection(int x, int y);
void DrawPressureAndTrend(int x, int y, float pressure, String slope);
void DrawMoon(int x, int y, int day, int month, int year, String hemisphere);

// --- IKONŲ PIEŠIMAS ---
void addmoon(int x, int y, int scale, bool IconSize);
void addsun(int x, int y, int scale, bool IconSize);
void addcloud(int x, int y, int scale, int linesize);
void addrain(int x, int y, int scale, bool IconSize);
void addsnow(int x, int y, int scale, bool IconSize);
void addtstorm(int x, int y, int scale);
void addfog(int x, int y, int scale, int linesize, bool IconSize);

// --- ORO SĄLYGŲ FUNKCIJOS ---
void Nodata(int x, int y, bool IconSize, String IconName);
void Sunny(int x, int y, bool IconSize, String IconName);
void MostlySunny(int x, int y, bool IconSize, String IconName);
void MostlyCloudy(int x, int y, bool IconSize, String IconName);
void Cloudy(int x, int y, bool IconSize, String IconName);
void Rain(int x, int y, bool IconSize, String IconName);
void ExpectRain(int x, int y, bool IconSize, String IconName);
void ChanceRain(int x, int y, bool IconSize, String IconName);
void Tstorms(int x, int y, bool IconSize, String IconName);
void Snow(int x, int y, bool IconSize, String IconName);
void Fog(int x, int y, bool IconSize, String IconName);
void Haze(int x, int y, bool IconSize, String IconName);
void Visibility(int x, int y, String Visi);
void CloudCover(int x, int y, int CCover);

// --- MATEMATIKA IR PAGALBINĖS ---
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength);
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode);
void Convert_Readings_to_Imperial();
String ConvertUnixTime(int unix_time);
float mm_to_inches(float value_mm);
float hPa_to_inHg(float value_hPa);
float hPa_to_mmHg(float value_hPa);
int JulianDate(int d, int m, int y);
float SumOfPrecip(float DataArray[], int readings);
String TitleCase(String text);
double NormalizedMoonPhase(int d, int m, int y);
String MoonPhase(int d, int m, int y, String hemisphere);
String WindDegToOrdinalDirection(float winddirection);

#endif