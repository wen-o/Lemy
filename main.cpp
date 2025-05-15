// =============================================
// 【配線ノイズ対策メモ】
// ・SPIクロック(LCD_SCK)だけツイスト化し、他のSPI信号線から離す
// ・MOSI(LCD_MOSI)とMISO(LCD_MISO)は距離を確保
// ・電源ラインは22AWGでキャパシタ追加
// ・この配線構成で80MHz SPI通信が安定
// =============================================

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DFPlayerMini_Fast.h>
#include <SD.h>

#define LCD_CS   18
#define LCD_DC   21
#define LCD_RST   9
#define LCD_SCK  48
#define LCD_MOSI 38
#define LCD_MISO 47
#define RTC_SDA   5
#define RTC_SCL   6
#define DFP_RX    43
#define DFP_TX    44
#define SD_CS     7

RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, LCD_DC, LCD_CS, LCD_RST);
#define dfpSerial Serial1
DFPlayerMini_Fast dfp;

unsigned long lastUpdate = 0;
bool dfpConnected = false;
bool backgroundDrawn = false;

bool drawBufferedBMP(const char *filename) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    Serial.println("BMPファイル開けません");
    return false;
  }

  uint8_t header[54];
  if (bmpFile.read(header, 54) != 54 || header[0] != 'B' || header[1] != 'M') {
    Serial.println("BMPフォーマット不正");
    bmpFile.close();
    return false;
  }

  int32_t bmpWidth  = header[18] | (header[19] << 8);
  int32_t bmpHeight = header[22] | (header[23] << 8);
  uint16_t depth    = header[28] | (header[29] << 8);

  if (depth != 24) {
    Serial.println("24bit BMPのみ対応");
    bmpFile.close();
    return false;
  }

  uint16_t *screenBuffer = (uint16_t *)malloc(bmpWidth * bmpHeight * 2);
  if (!screenBuffer) {
    Serial.println("バッファ確保失敗");
    bmpFile.close();
    return false;
  }

  for (int16_t row = 0; row < bmpHeight; row++) {
    bmpFile.seek(54 + row * ((bmpWidth * 3 + 3) & ~3));
    for (int16_t col = 0; col < bmpWidth; col++) {
      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();
      screenBuffer[(bmpHeight - 1 - row) * bmpWidth + col] = tft.color565(r, g, b);
    }
  }
  bmpFile.close();

  tft.drawRGBBitmap(0, 0, screenBuffer, bmpWidth, bmpHeight);
  free(screenBuffer);
  return true;
}

bool chimeEnabled = true;

void showChimeStatus() {
  tft.fillRect(270, 228, 50, 12, ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(272, 229);
  tft.print(chimeEnabled ? "Chime ON" : "Chime OFF");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  dfpSerial.begin(9600, SERIAL_8N1, DFP_TX, DFP_RX);

  if (dfp.begin(dfpSerial, true)) {
    dfp.volume(15);
    dfpConnected = true;
  }

  SPI.begin(LCD_SCK, LCD_MISO, LCD_MOSI, LCD_CS);
  SPI.setFrequency(80000000);
  tft.begin();
  tft.setRotation(3);

  Wire.begin(RTC_SDA, RTC_SCL);
  if (!rtc.begin()) {
    while (1);
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (SD.begin(SD_CS, SPI)) {
    drawBufferedBMP("/remi_face.bmp");
    backgroundDrawn = true;
  }

  showChimeStatus();
}

#define PCF8574_ADDRESS 0x20

uint8_t lastInputState = 0xFF;

void loop() {
  unsigned long now = millis();
    Wire.requestFrom(PCF8574_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t inputState = Wire.read();
    
    if (!(inputState & 0x01) && (lastInputState & 0x01)) {
      chimeEnabled = !chimeEnabled;
      tft.fillRect(270, 228, 50, 12, ILI9341_WHITE);
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(272, 229);
      tft.print(chimeEnabled ? "Chime ON" : "Chime OFF");
      Serial.println(chimeEnabled ? "Chime ON" : "Chime OFF");
    }

    if (!(inputState & 0x02) && (lastInputState & 0x02)) {
      if (dfpConnected) {
        DateTime currentTime = rtc.now();
        char fileNumber[5];
        snprintf(fileNumber, sizeof(fileNumber), "%04d", currentTime.hour() + 1);
        dfp.play(currentTime.hour() + 1);
      }
    }

    lastInputState = inputState;
  }

  if (now - lastUpdate >= 1000) {
    lastUpdate = now;

    DateTime currentTime = rtc.now();
    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", currentTime.hour(), currentTime.minute(), currentTime.second());

    tft.fillRect(50, 200, 220, 24, ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_BLACK);
    int16_t xCenter = (240 - 18 * 3) / 2;  // 18ピクセル幅×3倍サイズの中央揃え
    tft.setCursor(xCenter, 202);
    tft.print(timeStr);

    if (currentTime.minute() == 0 && currentTime.second() == 0 && chimeEnabled) {
      if (dfpConnected) {
        char fileNumber[5];
        snprintf(fileNumber, sizeof(fileNumber), "%04d", currentTime.hour() + 1);
        dfp.playFolder(1, atoi(fileNumber));
      }
    }
  }
}
