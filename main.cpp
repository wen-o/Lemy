#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define LCD_CS   18
#define LCD_DC   21
#define LCD_RST   9
#define LCD_SCK  48
#define LCD_MOSI 38
#define LCD_MISO 47
#define RTC_SDA   5
#define RTC_SCL   6

RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, LCD_DC, LCD_CS, LCD_RST);

void setup() {
  Serial.begin(115200);

  // SPI速度を落として初期化
  SPI.begin(LCD_SCK, LCD_MISO, LCD_MOSI, LCD_CS);
  SPI.setFrequency(1000000); // 1MHzに設定

  // LCD初期化を先に実行
  tft.begin();
  tft.setRotation(3); // 上下反転
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10);
  tft.println("LCD Ready");

  Wire.begin(RTC_SDA, RTC_SCL);

  if (!rtc.begin()) {
    Serial.println("RTCが見つかりません");
    tft.println("RTC not found");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTCが動いていません。初期化中…");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();
  char buf[32];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());

  tft.setCursor(10, 30);
  tft.print(buf);

  Serial.println(buf);  // シリアルにも出力

  delay(1000);
}
