#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ArduCAM.h>
#include "memorysaver.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST77xx.h>

#include "BoardConfig.h"
#include "Debug.h"
#include "SpiBus.h"
#include "Buttons.h"
#include "Ui.h"
#include "Camera.h"
#include "Storage.h"

#if !(defined OV2640_MINI_2MP)
  #error Please select OV2640_MINI_2MP in memorysaver.h
#endif

static SpiBus bus;

static ArduCAM myCAM(OV2640, Pins::CAM_CS);
static Adafruit_ST7735 tft(Pins::TFT_CS, Pins::TFT_DC, Pins::TFT_RST);

static Buttons buttons;
static Ui ui(bus, tft);
static Camera cam(bus, myCAM);
static Storage storage(bus);

// App state
static View view = View::VIEW_HOME;
static uint8_t homeSel = 0;
static uint16_t galSel = 0;
static char galItems[10][13];
static uint8_t galCount = 0;
static uint16_t imgIndex = 0;

static void enterView(View v);

static volatile bool reqHome = false;
static volatile bool reqGallery = false;
static volatile bool reqCapture = false;

static bool captureAndDisplayFrame() {
  if (!cam.startCapture(8000)) return false;

  uint32_t length = cam.fifoLength();
  if (length < 1000 || length >= MAX_FIFO_SIZE) {
    cam.clearFifo();
    return false;
  }
  // preview mode is OV2640_160x120, need to crop it to 128x120
  constexpr uint16_t SRC_W = 160;
  constexpr uint16_t SRC_H = 120;
  constexpr uint16_t DST_W = 128;
  constexpr uint16_t DST_H = 120;

  // center vertically (128x128 display)
  constexpr uint16_t y0 = (128 - DST_H) / 2;   // (128-120)/2 = 4

  constexpr uint16_t SKIP_L = (SRC_W - DST_W) / 2; // 16
  constexpr uint16_t SKIP_R = SRC_W - DST_W - SKIP_L; // 16

  bus.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus.tftSelect(true);
  tft.fillRect(0, 0, 128, y0, ST77XX_BLACK);
  tft.fillRect(0, y0 + DST_H, 128, 128 - (y0 + DST_H), ST77XX_BLACK);
  bus.tftSelect(false);
  SPI.endTransaction();

  uint16_t line[DST_W];
  uint32_t remaining = length;

  // Set frame window once (128x120)
  bus.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus.tftSelect(true);
  tft.setAddrWindow(0, y0, DST_W, DST_H);
  bus.tftSelect(false);
  SPI.endTransaction();

  for (uint16_t y = 0; y < SRC_H; y++) {
    bus.prepForCam();
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();

    // discard left pixels
    for (uint16_t x = 0; x < SKIP_L; x++) {
      SPI.transfer(0x00);
      SPI.transfer(0x00);
      remaining = (remaining >= 2) ? (remaining - 2) : 0;
    }

    // read center pixels into line[]
    for (uint16_t x = 0; x < DST_W; x++) {
      uint8_t hi = SPI.transfer(0x00);
      uint8_t lo = SPI.transfer(0x00);
      remaining = (remaining >= 2) ? (remaining - 2) : 0;
      line[x] = (uint16_t(hi) << 8) | uint16_t(lo); // if colors look wrong, swap bytes
    }

    // discard right pixels
    for (uint16_t x = 0; x < SKIP_R; x++) {
      SPI.transfer(0x00);
      SPI.transfer(0x00);
      remaining = (remaining >= 2) ? (remaining - 2) : 0;
    }

    myCAM.CS_HIGH();
    SPI.endTransaction();

    // Draw this cropped line to TFT at y0+y (DST_W pixels)
    bus.prepForTft();
    SPI.beginTransaction(SpiCfg::TFT_SPI);
    bus.tftSelect(true);
    tft.startWrite();
    tft.setAddrWindow(0, y0 + y, DST_W, 1);
    tft.writePixels(line, DST_W, true);
    tft.endWrite();
    bus.tftSelect(false);
    SPI.endTransaction();
    if ((y & 0x03) == 0) {
      buttons.update();
      if (buttons.select.pressed()) reqHome = true;
      if (buttons.next.pressed())   reqGallery = true;
      if (buttons.click.pressed())  reqCapture = true;
      if (reqHome || reqGallery || reqCapture) break;
    }

    if (remaining < 2) break;
  }

  cam.clearFifo();
  return true;
}
static bool forceSDReinit() {
  bus.deselectAll();
  cam.cameraDeselectHard();
  bus.spiSoftReset();
  SPI.beginTransaction(SpiCfg::SD_SPI);
  bool ok = SD.begin(Pins::SD_CS);
  SPI.endTransaction();
  #if DEBUG
  Serial.print(F("SD.begin:")); Serial.println(ok);
  #endif
  return ok;
}

bool captureAndSaveToSD() {

  if (!cam.startCapture(8000)) {
    ui.status(F("Timeout"));
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.clear_fifo_flag();
    SPI.endTransaction();
    return false;
  }

  SPI.beginTransaction(SpiCfg::CAM_SPI);
  uint32_t length = myCAM.read_fifo_length();
  SPI.endTransaction();

  if (length == 0 || length >= MAX_FIFO_SIZE) {
    ui.status(F("Bad FIFO"));
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.clear_fifo_flag();
    SPI.endTransaction();
    return false;
  }

  char filename[13];
  storage.makeFilename(filename, imgIndex);

  if (!forceSDReinit()) {
    ui.status(F("SD reinit!"));
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.clear_fifo_flag();
    SPI.endTransaction();
    return false;
  }

  bus.deselectAll();
  cam.cameraDeselectHard();
  bus.spiSoftReset();

  SPI.beginTransaction(SpiCfg::SD_SPI);
  File img = SD.open(filename, FILE_WRITE);
  SPI.endTransaction();
  #if DEBUG
  Serial.print(F("open:")); Serial.println(img ? 1 : 0);
  #endif
  if (!img) {
    ui.status(F("SD open failed"));
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.clear_fifo_flag();
    SPI.endTransaction();
    return false;
  }

  bool ok = cam.writeFifoJpegToFile(img, length);

  SPI.beginTransaction(SpiCfg::SD_SPI);
  img.flush();
  img.close();
  SPI.endTransaction();

  SPI.beginTransaction(SpiCfg::CAM_SPI);
  myCAM.clear_fifo_flag();
  SPI.endTransaction();

  if (!ok) {
    ui.status(F("JPEG invalid"));
    return false;
  }
  ui.status(F("Saved OK"));
}

static void scanGallery() {
  galCount = 0;
  for (uint16_t i = 0; i < 1000 && galCount < 10; i++) {
    char fn[13];
    storage.makeFilename(fn, i);
    bus.prepForSd();
    SPI.beginTransaction(SpiCfg::SD_SPI);
    bool ex = SD.exists(fn);
    SPI.endTransaction();
    if (ex) {
      for (uint8_t k = 0; k < 13; k++) {
        galItems[galCount][k] = fn[k];
        if (fn[k] == '\0') break;
      }
      galCount++;
    }
  }
}

static void enterView(View v) {
  view = v;

  if (view == View::VIEW_HOME) {
    ui.drawHome(homeSel);
    return;
  }

  if (view == View::VIEW_CAMERA) {
    ui.drawCameraOverlay();
    return;
  }

  if (view == View::VIEW_GALLERY) {
    galSel = 0;
    scanGallery();
    ui.drawGallery(galSel, galItems, galCount);
    return;
  }
}

void setup() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  #if DEBUG
  Serial.begin(115200);
  delay(300);
  #endif
  Wire.begin();
  SPI.begin();

  bus.begin();
  buttons.begin();
  ui.begin();

  ui.status(F("Booting..."));

  if (!storage.begin()) {
    ui.status(F("SD init FAILED"));
    while (1) {}
  }

  imgIndex = storage.findNextImgIndex();

  while (!cam.probeSpi()) {
    ui.status(F("CAM SPI ERROR"));
    delay(500);
  }

  while (!cam.probeSensor()) {
    ui.status(F("OV2640 not found"));
    delay(500);
  }

  cam.modePreview();
  enterView(View::VIEW_HOME);
}

void loop() {
  buttons.update();

#if TEST_ALL
  static uint32_t lastSwitch = 0;
  static uint8_t phase = 0;

  if (millis() - lastSwitch >= 5000) {
    lastSwitch = millis();
    phase = (phase + 1) % 3;

    if (phase == 0) enterView(View::VIEW_HOME);
    else if (phase == 1) enterView(View::VIEW_CAMERA);
    else enterView(View::VIEW_GALLERY);
  }
#endif

  const bool clickEvt = buttons.click.pressed();
  const bool nextEvt  = buttons.next.pressed();
  const bool selEvt   = buttons.select.pressed();

  if (view == View::VIEW_HOME) {
    if (nextEvt) {
      homeSel = (homeSel + 1) & 1;
      ui.drawHome(homeSel);
    }
    if (selEvt) {
      enterView(homeSel == 0 ? View::VIEW_CAMERA : View::VIEW_GALLERY);
    }
    return;
  }

  if (view == View::VIEW_CAMERA) {
    if (reqHome) {
      reqHome = reqGallery = reqCapture = false;
      enterView(View::VIEW_HOME);
      return;
    }
    if (reqGallery) {
      reqHome = reqGallery = reqCapture = false;
      enterView(View::VIEW_GALLERY);
      return;
    }
    if (selEvt) {
      enterView(View::VIEW_HOME);
      return;
    }
    if (nextEvt) {
      enterView(View::VIEW_GALLERY);
      return;
    }
    if (clickEvt || reqCapture) {
      reqCapture = false;
      ui.status(F("Saving JPEG..."));
      cam.modeJpeg();
      if (captureAndSaveToSD()) imgIndex++;
      cam.modePreview();
      ui.drawCameraOverlay();
      return;
    }

    static uint32_t lastPreview = 0;
    if (millis() - lastPreview >= 100) {
      lastPreview = millis();
      captureAndDisplayFrame();
      ui.drawCameraOverlay();
    }
    return;
  }
  if (view == View::VIEW_GALLERY) {
    if (clickEvt) {
      enterView(View::VIEW_HOME);
      return;
    }
    if (nextEvt) {
      if (galCount > 0) galSel = (galSel + 1) % galCount;
      ui.drawGallery(galSel, galItems, galCount);
    }
    if (selEvt) {
      ui.status(F("SELECT pressed"));
    }
    return;
  }
  enterView(View::VIEW_HOME);
}