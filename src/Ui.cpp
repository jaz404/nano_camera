#include "Ui.h"
#include "BoardConfig.h"
#include <Adafruit_ST77xx.h>
#include "Storage.h"

extern Storage storage;

void Ui::begin() {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);
  tft_.initR(INITR_BLACKTAB);
  tft_.setRotation(1);
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextWrap(false);
  bus_.tftSelect(false);
  SPI.endTransaction();
}

void Ui::clear() {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);
  tft_.fillScreen(ST77XX_BLACK);
  bus_.tftSelect(false);
  SPI.endTransaction();
}

void Ui::status(const __FlashStringHelper* l1, const __FlashStringHelper* l2) {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.fillRect(0, 0, 160, 28, ST77XX_BLACK);
  tft_.setCursor(0, 0);
  tft_.setTextSize(1);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.println(l1);
  if (l2) tft_.println(l2);

  bus_.tftSelect(false);
  SPI.endTransaction();
}

void Ui::showSaved(const char* fn) {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.fillRect(0, 32, 160, 12, ST77XX_BLACK);
  tft_.setCursor(0, 32);
  tft_.setTextSize(1);
  tft_.setTextColor(ST77XX_CYAN);
  tft_.print(F("Saved: "));
  tft_.print(fn);

  bus_.tftSelect(false);
  SPI.endTransaction();
}

void Ui::drawHome(uint8_t sel) {
  clear();

  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.setCursor(45, 2);
  tft_.setTextSize(1);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.println(F("  Home\n")); 
  
  tft_.setTextColor(sel == 0 ? ST77XX_GREEN : ST77XX_WHITE);
  tft_.println(F(" > Camera"));

  tft_.setTextColor(sel == 1 ? ST77XX_GREEN : ST77XX_WHITE);
  tft_.println(F(" > Gallery"));

  tft_.setTextColor(ST77XX_CYAN);
  tft_.println(F("\n NEXT: move"));
  tft_.println(F(" SELECT: enter"));

  bus_.tftSelect(false);
  SPI.endTransaction();
}

void Ui::drawCameraOverlay() {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.fillRect(0, 0, 160, 12, ST77XX_BLACK);
  tft_.setCursor(0, 0);
  tft_.setTextSize(1);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.print(F("CAM "));
  tft_.setTextColor(ST77XX_CYAN);
  tft_.print(F("CLICK:shot "));
  tft_.print(F("SELECT:home "));

  bus_.tftSelect(false);
  SPI.endTransaction();
}

// void Ui::drawGalleryHeader(uint16_t sel, uint16_t count) {
//   clear();

//   bus_.prepForTft();
//   SPI.beginTransaction(SpiCfg::TFT_SPI);
//   bus_.tftSelect(true);

//   tft_.setCursor(45, 2);
//   tft_.setTextSize(1);
//   tft_.setTextColor(ST77XX_WHITE);
//   tft_.println(F("Gallery"));

//   tft_.setTextColor(ST77XX_CYAN);
//   tft_.print(F(" SEL "));
//   tft_.print(sel);
//   tft_.print(F("/"));
//   tft_.println(count);

//   tft_.setTextColor(ST77XX_CYAN);
//   tft_.println(F(" NEXT: scroll"));
//   tft_.println(F(" CLICK: delete"));
//   tft_.println(F("SEL: home\n"));

//   bus_.tftSelect(false);
//   SPI.endTransaction();
// }
void Ui::drawGallery(uint8_t sel) {
  clear();

  // --- Build list (up to 10) ---
  char items[10][13];
  uint8_t count = 0;

  for (uint16_t i = 0; i < 1000 && count < 10; i++) {
    char fn[13];
    storage.makeFilename(fn, i);

    bus_.prepForSd();
    SPI.beginTransaction(SpiCfg::SD_SPI);
    bool ex = SD.exists(fn);
    SPI.endTransaction();

    if (ex) {
      // copy filename into items[count]
      for (uint8_t k = 0; k < 13; k++) {
        items[count][k] = fn[k];
        if (fn[k] == '\0') break;
      }
      count++;
    }
  }

  if (count == 0) sel = 0;
  else if (sel >= count) sel = count - 1;

  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.setTextWrap(false);
  tft_.setTextSize(1);

  tft_.setCursor(38, 2);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.println(F(" Gallery\n"));

  if (count == 0) {
    tft_.setTextColor(ST77XX_CYAN);
    tft_.println(F(" (none found)"));
    tft_.println(F(" Take a shot first."));
  } else {
    for (uint8_t i = 0; i < count; i++) {
      tft_.setTextColor(i == sel ? ST77XX_GREEN : ST77XX_WHITE);
      tft_.print(F(" > "));
      tft_.println(items[i]);
    }
  }

  tft_.setTextColor(ST77XX_CYAN);
  tft_.println(F("\n NEXT: move"));
  tft_.println(F(" SELECT: open"));
  tft_.println(F(" CLICK: home"));

  bus_.tftSelect(false);
  SPI.endTransaction();
}
void Ui::printLine(const char* s, uint16_t color) {
  bus_.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus_.tftSelect(true);

  tft_.setTextColor(color);
  tft_.println(s);

  bus_.tftSelect(false);
  SPI.endTransaction();
}