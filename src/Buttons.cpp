#include "Buttons.h"
#include "BoardConfig.h"

void DebouncedButton::begin(uint8_t p) {
  pin = p;
  pinMode(pin, INPUT_PULLUP);
  stable = digitalRead(pin);
  lastRead = stable;
  lastChange = millis();
  pressedEdge = false;
}

void DebouncedButton::update(uint16_t debounceMs) {
  pressedEdge = false;
  uint8_t r = digitalRead(pin);
  if (r != lastRead) {
    lastRead = r;
    lastChange = millis();
  }
  if ((millis() - lastChange) >= debounceMs && stable != lastRead) {
    stable = lastRead;
    if (stable == LOW) pressedEdge = true;
  }
}

void Buttons::begin() {
  click.begin(Pins::BTN_CLICK);
  next.begin(Pins::BTN_NEXT);
  select.begin(Pins::BTN_SELECT);
}

void Buttons::update() {
  click.update();
  next.update();
  select.update();
}