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

void setup() {
  Serial.begin(115200);
  delay(1000);  // PC接続待機
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
  DateTime now = rtc.now();
  char buf[32];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());

  tft.fillRect(10, 30, 220, 20, ILI9341_BLACK);
  tft.setCursor(10, 30);
  tft.print(buf);

  Wire.requestFrom(PCF8574_ADDRESS, 1);
  if (Wire.available()) {
    byte inputState = Wire.read();

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

  delay(100);
}
