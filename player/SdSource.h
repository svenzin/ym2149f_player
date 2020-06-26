#ifndef _SDSOURCE_H_
#define _SDSOURCE_H_

#include "Logger.h"
#include <SdFat.h>

class SdSource {
  uint16_t file_index[4096];
  uint32_t file_count;
  uint32_t current;

  uint32_t pin_mosi;
  uint32_t pin_miso;
  uint32_t pin_sclk;
  uint32_t pin_ssel;
  
  SdFat sd;
  SdFile root;
public:
  
  explicit SdSource(uint32_t mosi, uint32_t miso, uint32_t sclk, uint32_t ssel)
  : pin_mosi(mosi), pin_miso(miso), pin_sclk(sclk), pin_ssel(ssel)
  {}

  void get_index(SdFile &file, uint32_t i) {
    current = i % file_count;
    file.open(&root, file_index[current], O_RDONLY);
  }

  void get_random(SdFile &file) {
    get_index(file, random(0, file_count));
  }

  void get_next(SdFile &file) {
    get_index(file, current + 1);
  }

  bool setup() {
    logger::debug("Opening SD card");
    SPI.setMOSI(pin_mosi);
    SPI.setMISO(pin_miso);
    SPI.setSCLK(pin_sclk);
    if (!sd.begin(pin_ssel, SPI_FULL_SPEED)) return false;
    if (!root.open("/"))                     return false;

    logger::debug("Parsing SD card");
    SdFile file;
    file_count = 0;
    root.rewind();
    while (file.openNext(&root)) {
      file_index[file_count] = file.dirIndex();
      ++file_count;
      file.close();
    }

    return true;
  }
};

#endif // _SDSOURCE_H_
