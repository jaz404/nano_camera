#pragma once
#include <Arduino.h>
#include <SPI.h>

class SpiBus {
public:
  void begin();

  void deselectAll();
  void spiSoftReset();

  void prepForTft();
  void prepForSd();
  void prepForCam();

  void tftSelect(bool en);
  void sdSelect(bool en);
  void camSelect(bool en);

private:
  void setCsHigh_();
};