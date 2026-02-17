#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "SpiBus.h"
#include "BoardConfig.h" 

class Storage {
public:
  void initIndexFromSD();

  explicit Storage(SpiBus& bus) : bus_(bus) {}

  bool begin();
  bool rebegin();

  void makeFilename(char out[11], uint16_t idx);
  uint16_t findNextImgIndex();

  // Gallery logic (restored)
  // uint16_t galleryCount(bool cache = true);
  // bool galleryGetNameAt(uint16_t k, char out[13]);
  // bool galleryDeleteAt(uint16_t k);

  // For “print all names” style listing
  template<typename Fn>
  void listRootFiles(uint8_t maxLines, Fn onName) {
    bus_.prepForSd();
    SPI.beginTransaction(SpiCfg::SD_SPI);
    File root = SD.open("/");
    SPI.endTransaction();
    if (!root) return;

    uint8_t line = 0;
    while (true) {
      SPI.beginTransaction(SpiCfg::SD_SPI);
      File f = root.openNextFile();
      SPI.endTransaction();
      if (!f) break;

      if (!f.isDirectory()) {
        onName(f.name());
        if (++line >= maxLines) {
          SPI.beginTransaction(SpiCfg::SD_SPI);
          f.close();
          SPI.endTransaction();
          break;
        }
      }

      SPI.beginTransaction(SpiCfg::SD_SPI);
      f.close();
      SPI.endTransaction();
    }

    SPI.beginTransaction(SpiCfg::SD_SPI);
    root.close();
    SPI.endTransaction();
  }

private:
  SpiBus& bus_;

  bool isImgName_(const char* n);

  uint16_t galCountCache_ = 0;
  uint32_t galCountCacheMs_ = 0;
};