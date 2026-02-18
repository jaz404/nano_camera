#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "SpiBus.h"
#include "BoardConfig.h" 

class Storage {
public:
  explicit Storage(SpiBus& bus) : bus_(bus) {}

  bool begin();
  void makeFilename(char out[13], uint16_t idx);
  uint16_t findNextImgIndex();

private:
  SpiBus& bus_;

};