#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DFPlayerMini_Fast.h>

#define LCD_CS   18
#define LCD_DC   21
#define LCD_RST   9
#define LCD_SCK  48
#define LCD_MOSI 38
#define LCD_MISO 47
#define RTC_SDA   5
#define RTC_SCL   6
#define DFP_RX    8
#define DFP_TX    7

RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, LCD_DC, LCD_CS, LCD_RST);
HardwareSerial dfpSerial(1);
DFPlayerMini_Fast dfp;

#define PCF8574_ADDRESS 0x20  // DIPスイッチ000の場合

unsigned long playStartTime = 0;
bool isPlaying = false;
unsigned long busyToggleCount = 0;
bool previousBusyState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  dfpSerial.begin(9600, SERIAL_8N1, DFP_TX, DFP_RX);

  if (dfp.begin(dfpSerial, true)) {
    Serial.println("DFPlayer Mini online.");
    dfp.volume(30);
  } else {
    Serial.println("DFPlayer Mini not detected.");
  }

  SPI.begin(LCD_SCK, LCD_MISO, LCD_MOSI, LCD_CS);
  SPI.setFrequency(500000);

  tft.begin();
  tft.setRotation(3);
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

  Wire.beginTransmission(PCF8574_ADDRESS);
  Wire.write(0xFF);
  Wire.endTransmission();
}

void loop() {
  static byte lastInputState = 0xFF;
  static unsigned long lastDisplayTime = 0;

  // PCF8574の入力状態読み取り
  Wire.requestFrom(PCF8574_ADDRESS, 1);
  byte inputState = 0xFF;
  if (Wire.available()) {
    inputState = Wire.read();
  }

  // 7番ピンをビジーモニタとして扱う（Low で再生中）
  bool busyState = !(inputState & (1 << 7));

  // ビジー状態の変化を検出してカウント
  if (busyState != previousBusyState) {
    busyToggleCount++;
    previousBusyState = busyState;
  }

  if (busyState) {
    tft.fillRect(10, 120, 220, 20, ILI9341_BLACK);
    tft.setCursor(10, 120);
    tft.print("Busy...");
  } else {
    tft.fillRect(10, 120, 220, 20, ILI9341_BLACK);
  }

  tft.fillRect(10, 150, 220, 20, ILI9341_BLACK);
  tft.setCursor(10, 150);
  tft.print("Busy Toggles: ");
  tft.print(busyToggleCount);

  if (!isPlaying && busyState) {
    isPlaying = true;
    playStartTime = millis();
  }

  if (isPlaying && !busyState) {
    unsigned long playDuration = (millis() - playStartTime) / 1000;
    tft.fillRect(10, 90, 220, 20, ILI9341_BLACK);
    tft.setCursor(10, 90);
    tft.print("Played: ");
    tft.print(playDuration);
    tft.print(" sec");
    isPlaying = false;
  }

  if (millis() - lastDisplayTime > 500) {
    lastDisplayTime = millis();

    DateTime now = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    tft.fillRect(10, 30, 220, 20, ILI9341_BLACK);
    tft.setCursor(10, 30);
    tft.print(buf);

    if (inputState != lastInputState) {
      lastInputState = inputState;

      tft.fillRect(10, 60, 220, 20, ILI9341_BLACK);
      tft.setCursor(10, 60);

      bool anyPressed = false;
      for (int i = 0; i <= 2; i++) {
        if (!(inputState & (1 << i))) {
          tft.print("Play ");
          tft.print(i);
          dfp.play(i + 1);
          anyPressed = true;
          break;
        }
      }
      if (!anyPressed) {
        tft.print("No button pressed");
      }
    }
  }
}
