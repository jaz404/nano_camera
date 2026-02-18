// include/Ui.h
#pragma once
#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include "SpiBus.h"

enum class View : uint8_t { VIEW_HOME, VIEW_CAMERA, VIEW_GALLERY };

class Ui {
public:
  Ui(SpiBus& bus, Adafruit_ST7735& tft) : bus_(bus), tft_(tft) {}

  void begin();

  void clear();
  void status(const __FlashStringHelper* l1, const __FlashStringHelper* l2 = nullptr);
  void drawHome(uint8_t sel);
  void drawCameraOverlay();
  // void drawGalleryHeader(uint16_t sel, uint16_t count);
  void drawGallery(uint8_t sel);

private:
  SpiBus& bus_;
  Adafruit_ST7735& tft_;
};