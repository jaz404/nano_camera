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
static uint16_t imgIndex = 0;

static void enterView(View v);

static bool captureAndDisplayFrame() {
  if (!cam.startCapture(8000)) return false;

  uint32_t length = cam.fifoLength();
  if (length < 1000 || length >= MAX_FIFO_SIZE) {
    cam.clearFifo();
    return false;
  }

  constexpr uint16_t W = 160;
  constexpr uint16_t H = 120;
  constexpr uint16_t y0 = (128 - H) / 2;

  // clear bars
  bus.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus.tftSelect(true);
  tft.fillRect(0, 0, 160, y0, ST77XX_BLACK);
  tft.fillRect(0, y0 + H, 160, 128 - (y0 + H), ST77XX_BLACK);
  bus.tftSelect(false);
  SPI.endTransaction();

  uint16_t line[W];
  uint32_t remaining = length;

  // set window once
  bus.prepForTft();
  SPI.beginTransaction(SpiCfg::TFT_SPI);
  bus.tftSelect(true);
  tft.setAddrWindow(0, y0, W, H);
  bus.tftSelect(false);
  SPI.endTransaction();

  for (uint16_t y = 0; y < H; y++) {
    // read scanline from CAM FIFO
    bus.prepForCam();
    SPI.beginTransaction(SpiCfg::CAM_SPI);
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();

    for (uint16_t x = 0; x < W; x++) {
      uint8_t hi = SPI.transfer(0x00);
      uint8_t lo = SPI.transfer(0x00);
      remaining = (remaining >= 2) ? (remaining - 2) : 0;
      line[x] = (uint16_t(hi) << 8) | uint16_t(lo); // swap if needed
    }

    myCAM.CS_HIGH();
    SPI.endTransaction();

    // write scanline to TFT
    bus.prepForTft();
    SPI.beginTransaction(SpiCfg::TFT_SPI);
    tft.startWrite();
    tft.setAddrWindow(0, y0 + y, W, 1);
    tft.writePixels(line, W, true);
    tft.endWrite();
    SPI.endTransaction();

    if (remaining == 0) break;
  }

  cam.clearFifo();
  return true;
}

static bool captureAndSaveToSD() {
  ui.status(F("Capturing JPEG..."));

  if (!cam.startCapture(8000)) {
    ui.status(F("Capture timeout"));
    cam.clearFifo();
    return false;
  }

  uint32_t len = cam.fifoLength();
#if DEBUG
  Serial.print(F("FIFO length: ")); Serial.println(len);
#endif

  if (len == 0 || len >= MAX_FIFO_SIZE) {
    ui.status(F("Bad FIFO length"));
    cam.clearFifo();
    return false;
  }

  char filename[11];
  storage.makeFilename(filename, imgIndex);

  if (!storage.rebegin()) {
    ui.status(F("SD reinit failed"));
    cam.clearFifo();
    return false;
  }

  bus.prepForSd();
  SPI.beginTransaction(SpiCfg::SD_SPI);
  File img = SD.open(filename, FILE_WRITE);
  SPI.endTransaction();

  if (!img) {
    ui.status(F("SD open failed"));
    cam.clearFifo();
    return false;
  }

#if DEBUG
  Serial.print(F("Saving to SD as ")); Serial.println(filename);
#endif

  bool ok = cam.writeFifoJpegToFile(img, len);

  bus.prepForSd();
  SPI.beginTransaction(SpiCfg::SD_SPI);
  img.flush();
  img.close();
  SPI.endTransaction();

  cam.clearFifo();

  if (!ok) {
    ui.status(F("JPEG invalid"));
    return false;
  }

  ui.status(F("Saved OK"));
  ui.showSaved(filename);
  return true;
}

static void enterView(View v) {
  view = v;

  if (view == View::VIEW_HOME) {
    ui.drawHome(homeSel);
    return;
  }

  if (view == View::VIEW_CAMERA) {
    ui.status(F("Camera"));
    ui.drawCameraOverlay();
    return;
  }

  if (view == View::VIEW_GALLERY) {
    // ui.status(F("Gallery"));
    // delay(150);
    ui.drawGallery(galSel);
    return;
  }
}

void setup() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

#if DEBUG
  Serial.begin(115200);
  delay(300);
  Serial.println(F("Boot: ArduCAM + SD + TFT"));
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
  ui.status(F("SD init OK"));

  imgIndex = storage.findNextImgIndex();

  while (!cam.probeSpi()) {
    ui.status(F("CAM SPI ERROR"));
    delay(500);
  }
  ui.status(F("CAM SPI OK"));

  while (!cam.probeSensor()) {
    ui.status(F("OV2640 not found"));
    delay(500);
  }
  ui.status(F("OV2640 detected"));

  cam.modePreview();
  enterView(View::VIEW_HOME);

  imgIndex = storage.findNextImgIndex();

#if DEBUG
  Serial.println(F("Camera ready (preview). Press 's' to save JPEG."));
#endif
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

  if (view == View::VIEW_HOME) {
    if (buttons.next.pressed()) {
      homeSel = (homeSel + 1) & 1;     
      ui.drawHome(homeSel);
    }

    if (buttons.select.pressed()) {
      enterView(homeSel == 0 ? View::VIEW_CAMERA : View::VIEW_GALLERY);
    }

    return; // HOME handled
  }

  if (view == View::VIEW_CAMERA) {
    static uint32_t lastPreview = 0;

    // preview refresh
    if (millis() - lastPreview >= 500) {
      lastPreview = millis();
      captureAndDisplayFrame();
      ui.drawCameraOverlay();
    }

    // take photo
    if (buttons.click.pressed()) {
      ui.status(F("Saving JPEG..."));
      cam.modeJpeg();
      if (captureAndSaveToSD()) imgIndex++;
      cam.modePreview();
      ui.status(F("Preview resumed"));
      ui.drawCameraOverlay();
    }

    // navigation
    if (buttons.select.pressed()) {         
      enterView(View::VIEW_HOME);
    } else if (buttons.next.pressed()) {    
      galSel = 0;
      enterView(View::VIEW_GALLERY);
    }

#if DEBUG
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == 's' || c == 'S') {
        ui.status(F("Saving JPEG..."));
        cam.modeJpeg();
        if (captureAndSaveToSD()) imgIndex++;
        cam.modePreview();
        ui.status(F("Preview resumed"));
        ui.drawCameraOverlay();
      }
    }
#endif

    return; // CAMERA handled
  }

  if (view == View::VIEW_GALLERY) {
    // back to home
    if (buttons.click.pressed()) {           
      enterView(View::VIEW_HOME);
      return;
    }

    // move selection
    if (buttons.next.pressed()) {
      galSel = (galSel + 1) % 10;            
      ui.drawGallery(galSel);
    }

    // select action 
    if (buttons.select.pressed()) {
      ui.status(F("SELECT pressed"));
    }

    return; 
  }

  enterView(View::VIEW_HOME);
}