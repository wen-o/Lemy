// Remi Clock - Arduino Nano ESP32 (S3) Clock & Chime Sketch (点滅防止・秒変化時のみ更新・GPIO9リセット対応)

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <DFPlayerMini_Fast.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSans9pt7b.h>
#include "secrets.h"

#define LCD_CS     18
#define LCD_DC     21
#define LCD_RST     9  // GPIO9をリセットに指定
#define LCD_SCK    48
#define LCD_MOSI   38
#define LCD_MISO   47
#define DFPLAYER_RX  8
#define DFPLAYER_TX  7
#define RTC_SDA     5
#define RTC_SCL     6
#define BUTTON1_PIN  1
#define BUTTON2_PIN  2
#define BUTTON3_PIN 42
#define BUTTON4_PIN 39
#define SD_CS       10

HardwareSerial mySerial(1);
DFPlayerMini_Fast dfp;
RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, LCD_DC, LCD_CS, LCD_RST);

int lastHour = -1;
int lastSecond = -1;
bool settingMode = false;
bool chimeEnabled = true;
unsigned long aButtonPressedTime = 0;

void showClock() {
  DateTime now = rtc.now();
  char buf[64];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d\n時報:%s", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), chimeEnabled ? "ON" : "OFF");

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 30);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println(buf);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(RTC_SDA, RTC_SCL);
  rtc.begin();

  mySerial.begin(9600, SERIAL_8N1, DFPLAYER_TX, DFPLAYER_RX);
  dfp.begin(mySerial, true);
  dfp.volume(20);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  SPI.begin(LCD_SCK, LCD_MISO, LCD_MOSI, SD_CS);
  SPI.setFrequency(1000000);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  showClock();
}

void loop() {
  bool btnA = digitalRead(BUTTON1_PIN) == LOW;
  bool btnB = digitalRead(BUTTON2_PIN) == LOW;

  if (btnA) {
    if (aButtonPressedTime == 0) aButtonPressedTime = millis();
    if (!settingMode && (millis() - aButtonPressedTime > 2000)) {
      settingMode = true;
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(10, 30);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE);
      tft.println("[時刻設定モード]\nシリアルに yyyy/mm/dd hh:mm:ss 入力\n");
    }
  } else {
    aButtonPressedTime = 0;
  }

  if (btnB) {
    chimeEnabled = !chimeEnabled;
    lastSecond = -1;  // 強制再描画
    delay(1000);
  }

  if (settingMode) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      int y, M, d, h, m, s;
      if (sscanf(input.c_str(), "%d/%d/%d %d:%d:%d", &y, &M, &d, &h, &m, &s) == 6) {
        rtc.adjust(DateTime(y, M, d, h, m, s));
        settingMode = false;
        lastSecond = -1;  // 強制再描画
        tft.fillScreen(ILI9341_BLACK);
      }
    }
    delay(100);
    return;
  }

  DateTime now = rtc.now();
  if (now.second() != lastSecond) {
    lastSecond = now.second();
    showClock();
  }

  if (chimeEnabled && now.minute() == 0 && now.second() < 5 && now.hour() != lastHour) {
    lastHour = now.hour();
    dfp.play(now.hour() + 1);
  }

  if (btnA) {
    dfp.play(now.hour() + 1);
  }

  delay(50);
}
