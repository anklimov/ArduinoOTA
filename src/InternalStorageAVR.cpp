/*
 Copyright (c) 2018 Juraj Andrassy

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
 */

#include <Arduino.h>

#if defined(__AVR__) && FLASHEND >= 0xFFFF

#include "InternalStorageAVR.h"
#include "utility/optiboot.h"
#include "WiFiOTA.h"
#include <EEPROM.h>

InternalStorageAVRClass::InternalStorageAVRClass(uint32_t dataAddress) {
  maxSketchSize = (MAX_FLASH - bootloaderSize) / 2;
  maxSketchSize = (maxSketchSize / SPM_PAGESIZE) * SPM_PAGESIZE; // align to page
  pageAddress = maxSketchSize;
  pageIndex = 0;
  DATA_START_ADDRESS = dataAddress;
  command = -1;
}

int InternalStorageAVRClass::open(int length , int8_t _command) {
  (void)length;

switch (_command)
{
case DATA_SKETCH:   
  pageAddress = maxSketchSize;
  break;
case DATA_FS:
  pageAddress=0;
  break;
case DATA_CONFIG:
  pageAddress=DATA_START_ADDRESS;
}
      
  pageIndex = 0;
  command = _command;
  return 1;
}

size_t InternalStorageAVRClass::write(uint8_t b) {
switch (command)
{
case DATA_SKETCH: 
  if (pageIndex == 0) {
    optiboot_page_erase(pageAddress);
  }
  dataWord.u8[pageIndex % 2] = b;
  if (pageIndex % 2) {
    optiboot_page_fill(pageAddress + pageIndex, dataWord.u16);
  }
  pageIndex++;
  if (pageIndex == SPM_PAGESIZE) {
    optiboot_page_write(pageAddress);
    pageIndex = 0;
    pageAddress += SPM_PAGESIZE;
  }
break;

case DATA_FS:
case DATA_CONFIG:
    EEPROM.write(pageAddress++,b);
break;  
}  
  return 1;
}

void InternalStorageAVRClass::close() {
  if (pageIndex && command == DATA_SKETCH) {
    optiboot_page_write(pageAddress);
  }
  pageIndex = 0;
}

void InternalStorageAVRClass::clear() {
}

void (*RebootFunc)(void) = 0;

void InternalStorageAVRClass::apply() {
switch (command)
{
case DATA_SKETCH: 
  copy_flash_pages_cli(SKETCH_START_ADDRESS, maxSketchSize, (pageAddress - maxSketchSize) / SPM_PAGESIZE + 1, true);
  break;
case DATA_FS:
case DATA_CONFIG:
RebootFunc();  
}
}

long InternalStorageAVRClass::maxSize() {
  return maxSketchSize;
}

int InternalStorageAVRClass::read()
{
int ch;
switch (command)
{
case DATA_SKETCH:   
   return -1;
   
case DATA_CONFIG:
case DATA_FS:
    return EEPROM.read(pageAddress++);
    
}  
};

InternalStorageAVRClass InternalStorage;

#endif
