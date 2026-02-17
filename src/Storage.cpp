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

bool Storage::rebegin() {
  bus_.prepForSd();
  SPI.beginTransaction(SpiCfg::SD_SPI);
  bool ok = SD.begin(Pins::SD_CS);
  SPI.endTransaction();
  DPRINT(F("SD.rebegin: ")); DPRINTLN(ok ? F("OK") : F("FAIL"));
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

bool Storage::isImgName_(const char* n) {
  if (!n) return false;

  int len = 0;
  while (n[len]) len++;
  if (len < 4) return false;

  char c1 = n[len - 4];
  char c2 = n[len - 3];
  char c3 = n[len - 2];
  char c4 = n[len - 1];

  if (c1 != '.') return false;

  return ((c2 == 'J' || c2 == 'j') &&
          (c3 == 'P' || c3 == 'p') &&
          (c4 == 'G' || c4 == 'g'));
}


// cache ~2s (same as your original)
// uint16_t Storage::galleryCount(bool cache) {
//   if (cache && (millis() - galCountCacheMs_ < 2000)) return galCountCache_;
//   galCountCacheMs_ = millis();

//   uint16_t count = 0;

//   bus_.prepForSd();
//   SPI.beginTransaction(SpiCfg::SD_SPI);
//   File root = SD.open("/");
//   SPI.endTransaction();
//   if (!root) { galCountCache_ = 0; return 0; }

//   while (true) {
//     SPI.beginTransaction(SpiCfg::SD_SPI);
//     File f = root.openNextFile();
//     SPI.endTransaction();
//     if (!f) break;

//     if (!f.isDirectory()) {
//       const char* nm = f.name();
//       if (isImgName_(nm)) count++;
//     }

//     SPI.beginTransaction(SpiCfg::SD_SPI);
//     f.close();
//     SPI.endTransaction();
//   }

//   SPI.beginTransaction(SpiCfg::SD_SPI);
//   root.close();
//   SPI.endTransaction();

//   galCountCache_ = count;
//   return count;
// }

// bool Storage::galleryGetNameAt(uint16_t k, char out[13]) {
//   bus_.prepForSd();
//   SPI.beginTransaction(SpiCfg::SD_SPI);
//   File root = SD.open("/");
//   SPI.endTransaction();
//   if (!root) return false;

//   uint16_t idx = 0;
//   bool ok = false;

//   while (true) {
//     SPI.beginTransaction(SpiCfg::SD_SPI);
//     File f = root.openNextFile();
//     SPI.endTransaction();
//     if (!f) break;

//     if (!f.isDirectory()) {
//       const char* nm = f.name();
//       if (isImgName_(nm)) {
//         if (idx == k) {
//           uint8_t i = 0;
//           for (; i < 12 && nm[i]; i++) out[i] = nm[i];
//           out[i] = '\0';
//           ok = true;

//           SPI.beginTransaction(SpiCfg::SD_SPI);
//           f.close();
//           SPI.endTransaction();
//           break;
//         }
//         idx++;
//       }
//     }

//     SPI.beginTransaction(SpiCfg::SD_SPI);
//     f.close();
//     SPI.endTransaction();
//   }

//   SPI.beginTransaction(SpiCfg::SD_SPI);
//   root.close();
//   SPI.endTransaction();

//   return ok;
// }
