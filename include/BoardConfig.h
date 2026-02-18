#pragma once
#include <Arduino.h>
#include <SPI.h>

// Pins 
namespace Pins {
  static constexpr uint8_t CAM_CS = 7;
  static constexpr uint8_t SD_CS  = 4;

  static constexpr uint8_t TFT_CS  = 8;
  static constexpr uint8_t TFT_DC  = 9;
  static constexpr uint8_t TFT_RST = 6;

  // Buttons (wired to GND, INPUT_PULLUP)
  static constexpr uint8_t BTN_CLICK  = 2;
  static constexpr uint8_t BTN_NEXT   = 5;
  static constexpr uint8_t BTN_SELECT = 3;
}

// SPI settings 
namespace SpiCfg {
  static const SPISettings CAM_SPI(2000000, MSBFIRST, SPI_MODE0);
  static const SPISettings SD_SPI (250000,  MSBFIRST, SPI_MODE0);
  static const SPISettings TFT_SPI(8000000, MSBFIRST, SPI_MODE0);
}