#include "SpiBus.h"
#include "BoardConfig.h"

void SpiBus::begin() {
  pinMode(Pins::CAM_CS, OUTPUT);
  pinMode(Pins::SD_CS, OUTPUT);
  pinMode(Pins::TFT_CS, OUTPUT);
  pinMode(Pins::TFT_DC, OUTPUT);
  pinMode(Pins::TFT_RST, OUTPUT);
  setCsHigh_();
}

void SpiBus::setCsHigh_() {
  digitalWrite(Pins::CAM_CS, HIGH);
  digitalWrite(Pins::SD_CS, HIGH);
  digitalWrite(Pins::TFT_CS, HIGH);
}

void SpiBus::deselectAll() { setCsHigh_(); }

void SpiBus::spiSoftReset() {
  SPI.endTransaction();
  SPI.begin();
  delayMicroseconds(5);
}

void SpiBus::tftSelect(bool en) { digitalWrite(Pins::TFT_CS, en ? LOW : HIGH); }
void SpiBus::sdSelect(bool en)  { digitalWrite(Pins::SD_CS,  en ? LOW : HIGH); }
void SpiBus::camSelect(bool en) { digitalWrite(Pins::CAM_CS, en ? LOW : HIGH); }

void SpiBus::prepForTft() { deselectAll(); spiSoftReset(); }
void SpiBus::prepForSd()  { deselectAll(); spiSoftReset(); }
void SpiBus::prepForCam() { deselectAll(); spiSoftReset(); }