#include "Storage.h"
#include "BoardConfig.h"
#include "Debug.h"

bool Storage::begin() {
  bus_.prepForSd();
  SPI.beginTransaction(SpiCfg::SD_SPI);
  bool ok = SD.begin(Pins::SD_CS);
  SPI.endTransaction();
  return ok;
}

void Storage::makeFilename(char out[13], uint16_t idx) {
  out[0] = 'I'; out[1] = 'M'; out[2] = 'G';
  out[3] = '0' + (idx / 100) % 10;
  out[4] = '0' + (idx / 10)  % 10;
  out[5] = '0' + (idx % 10);
  out[6] = '.';
  out[7] = 'J'; out[8] = 'P'; out[9] = 'G';
  out[10] = '\0';
}

uint16_t Storage::findNextImgIndex() {
  char fn[13];

  for (uint16_t i = 0; i < 1000; i++) {
    makeFilename(fn, i);

    bus_.prepForSd();
    SPI.beginTransaction(SpiCfg::SD_SPI);
    bool ex = SD.exists(fn);
    SPI.endTransaction();

    if (!ex) return i;
  }
  return 999;
}