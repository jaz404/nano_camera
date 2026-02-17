#pragma once
#include <Arduino.h>
#include <ArduCAM.h>
#include "SpiBus.h"
#include <SD.h>  
class Camera {
public:
  Camera(SpiBus& bus, ArduCAM& cam) : bus_(bus), cam_(cam) {}

  bool probeSpi();
  bool probeSensor();

  void modePreview();
  void modeJpeg();

  bool startCapture(uint32_t timeoutMs);
  uint32_t fifoLength();
  void clearFifo();

  // Restored: actual FIFO burst -> SD file write (JPEG SOI/EOI)
  bool writeFifoJpegToFile(File& img, uint32_t fifo_len);

  ArduCAM& raw() { return cam_; }

private:
  SpiBus& bus_;
  ArduCAM& cam_;
};