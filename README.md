# A Very Crude Camera

Camera built on an Arduino Nano (ATmega328P) — ArduCAM OV2640 for capture, 128×128 ST7735 TFT for preview, SPI microSD for storage. Three SPI devices on one bus, 2KB SRAM, 32KB flash.

<p align="center">
  <img src="assets/vid1.gif" width="400" height="400"/>
  <br/>
  <em>TEST_ALL mode cycling through views</em>
</p>

## What it does

- RGB565 live preview from OV2640 to TFT at ~160×120, center-cropped to 128×120
- JPEG snapshot capture to SD (IMG000.JPG, IMG001.JPG, ...)
- Three-view UI: Home, Camera, Gallery
- Single shared SPI bus — CS arbitration handled manually per transaction
- Powered from 1S LiPo via TP4056 + boost converter

<p align="center">
  <img src="assets/vid2.gif" width="400" />
  <br/>
  <em>UI navigation</em>
</p>

## Hardware

<p align="center">
  <img src="assets/components.jpeg" width="400" />
</p>

- Arduino Nano (ATmega328P, 16 MHz, 2 KB SRAM, 32 KB flash)
- ArduCAM Mini 2MP (OV2640)
- ST7735 128×128 TFT
- SPI microSD breakout
- 3× tactile buttons (INPUT_PULLUP)
- TP4056 LiPo charger + 5V boost converter

<p align="center">
  <img src="assets/split_view.jpeg" width="400" />
</p>

## Wiring

<p align="center">
  <img src="assets/breadboard.jpeg" width="400" />
</p>

### Shared SPI bus

Signal | Nano pin | Devices
--- | --- | ---
MOSI | D11 | ArduCAM, TFT, SD
MISO | D12 | ArduCAM, SD
SCK  | D13 | ArduCAM, TFT, SD

### ArduCAM (OV2640)

Signal | Nano pin
--- | ---
CS   | D7
MOSI | D11
MISO | D12
SCK  | D13

### ST7735 TFT

Signal | Nano pin
--- | ---
CS   | D8
DC   | D9
RST  | D6
MOSI | D11
SCK  | D13

### SD card

Signal | Nano pin
--- | ---
CS   | D4
MOSI | D11
MISO | D12
SCK  | D13

### Buttons

Button | Pin
--- | ---
CLICK  | D2
SELECT | D3
NEXT   | D5

All active-low via internal pull-ups. Only one CS line is ever asserted at a time.

## Architecture

### SPI arbitration

All three devices share MOSI/MISO/SCK. `SpiBus` handles CS toggling and transaction boundaries. Before any device access, all CS lines are driven HIGH, then `SPI.begin()` is called to reset the hardware SPI peripheral state. This was necessary to prevent the OV2640's ArduCAM CS behavior from corrupting subsequent SD transactions — the ArduCAM library manages its own CS internally via `CS_HIGH()`/`CS_LOW()`, so `cameraDeselectHard()` explicitly drives the GPIO pin HIGH on top of the library call.

SPI clock speeds: CAM at 2 MHz, SD at 250 kHz, TFT at 8 MHz. Transaction settings are applied per-device via `SPISettings`.

### Preview pipeline

OV2640 is configured in BMP mode at 160×120. Each frame is read back as raw RGB565 over FIFO burst. Pixels are assembled as `(hi << 8) | lo` and written to the TFT line-by-line using `writePixels()`. Left and right 16-pixel columns are discarded to fit the 128-wide display. Button state is sampled every 4 rows during the frame loop to stay responsive mid-frame.

### JPEG capture

On capture trigger, the sensor is reconfigured to JPEG mode at 800×600 via `InitCAM()`. After `CAP_DONE` is asserted, the FIFO is burst-read byte-by-byte. SOI (`0xFF 0xD8`) and EOI (`0xFF 0xD9`) markers are detected inline; only the bytes between them are written to the SD file. Writes are buffered in a 256-byte stack buffer and flushed to SD when full, switching CS between the camera and SD card mid-stream.

SD is reinitialised with `SD.begin()` before every capture. This was needed because preview frames leave the SPI bus in a state that breaks subsequent SD access without a full reinit.

### Gallery

File list is scanned once on gallery entry (`scanGallery()`) and cached in `galItems[10][13]`. Navigation redraws from the cache — no SD access during scrolling.

### Memory

No heap allocation anywhere. The 256-byte JPEG write buffer is on the stack inside `writeFifoJpegToFile()`. The gallery cache (`galItems[10][13]` = 130 bytes) and the preview line buffer (`uint16_t line[128]` = 256 bytes) are the largest stack consumers. SRAM is tight; the ArduCAM and SD libraries both have internal buffers that leave roughly 200–300 bytes of headroom at runtime.

## Build flags

**`DEBUG 1`** — enables `Serial` output: JPEG byte count, SOI/EOI detection, SD init results. Costs ~800 bytes of flash. Leave off for release.

**`TEST_ALL`** — auto-cycles through views on a 5-second timer. For display testing without button input.

## Known constraints

- SD writes block the entire pipeline — no buffering or DMA
- Preview is ~3–5 FPS, bottlenecked by SPI transaction switching overhead per scanline
- `findNextImgIndex()` on startup does up to 1000 `SD.exists()` calls sequentially
- Flash is near capacity on the Nano; adding features requires removing others

## Possible next steps

- [ ] RP2040 or ESP32 port — DMA, faster SPI, more RAM
- [ ] Continuous FIFO burst read across scanlines to improve preview FPS  
- [ ] OV2640 compression ratio register tuning
- [ ] In-gallery image viewer
- [ ] Enclosure