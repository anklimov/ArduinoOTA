/*
  Copyright (c) 2017 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 WiFi101OTA version Feb 2017
 by Sandeep Mistry (Arduino)
 modified for ArduinoOTA Dec 2018, Apr 2019
 by Juraj Andrassy
*/

#ifndef _OTA_STORAGE_H_INCLUDED
#define _OTA_STORAGE_H_INCLUDED

#include <Arduino.h>


class OTAStorage {
public:

  OTAStorage();

  virtual int open(int length) = 0;
  virtual int open(int length, int8_t command) {
    (void) command;
    return open(length);
  }
  virtual size_t write(uint8_t) = 0;
  virtual void close() = 0;
  virtual void clear() = 0;
  virtual void apply() = 0;
  virtual int read() = 0;

  virtual long maxSize() {
    Serial.print("amxSize:");Serial.println(MAX_FLASH - SKETCH_START_ADDRESS - bootloaderSize);
    return (MAX_FLASH - SKETCH_START_ADDRESS - bootloaderSize);
  }

protected:
  const uint32_t SKETCH_START_ADDRESS;
  const uint32_t PAGE_SIZE;
  const uint32_t MAX_FLASH;
  uint32_t bootloaderSize;

};

class ExternalOTAStorage : public OTAStorage {

protected:
  const char* updateFileName = "UPDATE.BIN";

public:
  void setUpdateFileName(const char* _updateFileName) {
    updateFileName = _updateFileName;
  }

  virtual void apply();
};

#endif
