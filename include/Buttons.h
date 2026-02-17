#pragma once
#include <Arduino.h>

struct DebouncedButton {
  uint8_t pin = 255;
  uint8_t stable = HIGH;
  uint8_t lastRead = HIGH;
  uint32_t lastChange = 0;
  bool pressedEdge = false;

  void begin(uint8_t p);
  void update(uint16_t debounceMs = 25);
  bool pressed() const { return pressedEdge; }
};

struct Buttons {
  DebouncedButton click, next, select;
  void begin();
  void update();
};