#include "Camera.h"
#include "BoardConfig.h"
#include "Debug.h"
#include "memorysaver.h"

bool Camera::probeSpi() {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  cam_.write_reg(ARDUCHIP_TEST1, 0x55);
  uint8_t t = cam_.read_reg(ARDUCHIP_TEST1);
  SPI.endTransaction();
  return (t == 0x55);
}

bool Camera::probeSensor() {
  uint8_t vid = 0, pid = 0;
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  cam_.wrSensorReg8_8(0xff, 0x01);
  cam_.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  cam_.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  SPI.endTransaction();
  return (vid == 0x26 && (pid == 0x41 || pid == 0x42));
}

void Camera::modePreview() {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  cam_.set_format(BMP);
  cam_.InitCAM();
  cam_.OV2640_set_JPEG_size(OV2640_160x120);
  cam_.clear_fifo_flag();
  SPI.endTransaction();
}

void Camera::modeJpeg() {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  cam_.set_format(JPEG);
  cam_.InitCAM();
  cam_.OV2640_set_JPEG_size(OV2640_320x240);
  cam_.clear_fifo_flag();
  SPI.endTransaction();
}

bool Camera::startCapture(uint32_t timeoutMs) {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);

  cam_.flush_fifo();
  cam_.clear_fifo_flag();
  delay(30);

  cam_.clear_bit(ARDUCHIP_TRIG, CAP_DONE_MASK);
  cam_.start_capture();

  delay(5);
  if (!cam_.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) cam_.start_capture();

  const uint32_t t0 = millis();
  while (!cam_.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    if (millis() - t0 > timeoutMs) {
      SPI.endTransaction();
      return false;
    }
  }

  SPI.endTransaction();
  return true;
}

uint32_t Camera::fifoLength() {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  uint32_t len = cam_.read_fifo_length();
  SPI.endTransaction();
  return len;
}

void Camera::clearFifo() {
  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);
  cam_.clear_fifo_flag();
  SPI.endTransaction();
}

bool Camera::writeFifoJpegToFile(File& img, uint32_t fifo_len) {
  const uint16_t BUFSZ = 256;
  uint8_t buf[BUFSZ];
  uint16_t bi = 0;

  uint8_t temp = 0, temp_last = 0;
  bool started = false; // SOI seen
  bool ended = false;   // EOI seen
  uint32_t written = 0;

  bus_.prepForCam();
  SPI.beginTransaction(SpiCfg::CAM_SPI);

  cam_.CS_LOW();
  cam_.set_fifo_burst();

  temp = SPI.transfer(0x00);
  if (fifo_len > 0) fifo_len--;

  while (fifo_len--) {
    temp_last = temp;
    temp = SPI.transfer(0x00);

    // SOI
    if (!started && temp_last == 0xFF && temp == 0xD8) {
      started = true;
      buf[bi++] = 0xFF;
      buf[bi++] = 0xD8;

      if (bi >= BUFSZ) {
        cam_.CS_HIGH();
        SPI.endTransaction();

        bus_.prepForSd();
        SPI.beginTransaction(SpiCfg::SD_SPI);
        img.write(buf, bi);
        SPI.endTransaction();

        bus_.prepForCam();
        SPI.beginTransaction(SpiCfg::CAM_SPI);
        cam_.CS_LOW();
        cam_.set_fifo_burst();

        written += bi;
        bi = 0;
      }
      continue;
    }

    if (!started) continue;

    buf[bi++] = temp;

    if (bi == BUFSZ) {
      cam_.CS_HIGH();
      SPI.endTransaction();

      bus_.prepForSd();
      SPI.beginTransaction(SpiCfg::SD_SPI);
      img.write(buf, bi);
      SPI.endTransaction();

      bus_.prepForCam();
      SPI.beginTransaction(SpiCfg::CAM_SPI);
      cam_.CS_LOW();
      cam_.set_fifo_burst();

      written += bi;
      bi = 0;
    }

    // EOI
    if (temp_last == 0xFF && temp == 0xD9) {
      ended = true;
      break;
    }
  }

  cam_.CS_HIGH();
  SPI.endTransaction();

  if (bi > 0) {
    bus_.prepForSd();
    SPI.beginTransaction(SpiCfg::SD_SPI);
    img.write(buf, bi);
    SPI.endTransaction();
    written += bi;
  }

#if DEBUG
  Serial.print(F("JPEG bytes written: ")); Serial.println(written);
  Serial.print(F("SOI found: ")); Serial.println(started ? F("YES") : F("NO"));
  Serial.print(F("EOI found: ")); Serial.println(ended ? F("YES") : F("NO"));
#endif

  return started && ended && written > 1024;
}