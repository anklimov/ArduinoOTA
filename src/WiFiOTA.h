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
 modified for ArduinoOTA Dec 2018
 by Juraj Andrassy
*/


#define DATA_SKETCH      0
#define DATA_FS          1 // JFFS ESP only 
#define DATA_FILE        2
#define DATA_UNKNOWN     0xF

#define HTTP_TEXT_PLAIN   0x1000
#define HTTP_TEXT_JSON    0x2000
#define HTTP_OCTET_STREAM 0x3000
#define HTTP_TEXT_HTML    0x4000
#define HTTP_IMAGE_GIF    0x5000
#define HTTP_IMAGE_JPEG   0x6000

#define HTTP_POST   1
#define HTTP_GET    2
#define HTTP_DELETE 3
#define HTTP_PUT    4
#define HTTP_OPTIONS 5



#ifndef _WIFI_OTA_H_INCLUDED
#define _WIFI_OTA_H_INCLUDED

#include <Arduino.h>
#include <Server.h>
#include <Client.h>
#include <IPAddress.h>
#include <Udp.h>
#include "seekablestream.h"

#include "OTAStorage.h"

class WiFiOTAClass {
protected:
  WiFiOTAClass();

  void begin(IPAddress& localIP, const char* name, const char* password, OTAStorage& storage, seekableStream& cstream);
  void begin(IPAddress& localIP, const char* name, const char* password, OTAStorage& storage);
#ifndef DISABLE_ARDUINO_OTA_MDNS
  void pollMdns(UDP &mdnsSocket);
#endif
  
  void pollServer(Client& client);

public:
  void setCustomHandler(uint16_t (*fn)(Client& client, String request, uint8_t method, long contentLength, bool authorized, String& response))
  {
    processCustomRequest = fn;
  }

  void beforeApply(void (*fn)(void)) {
    beforeApplyCallback = fn;
  }
  
  void setDeviceName(char * name)
  {
    _name = name;
  }
  void sendHttpResponse(Client& client, uint16_t code, bool closeSocket = true);
  void sendHttpResponseWithText(Client& client, uint16_t code, bool closeSocket, String& response);

private:
  void flushRequestBody(Client& client, long contentLength);
  long openStorage(String fileName,long contentLength, char mode, uint16_t * dataType, bool isAuthorized);
  void closeStorage(uint16_t dataType);

private:
  String _name;
  String _expectedAuthorization;
  OTAStorage* _storage;
 
  seekableStream* _file;
  
  uint32_t localIp;
  uint32_t _lastMdnsResponseTime;

  void (*beforeApplyCallback)(void);
  uint16_t (*processCustomRequest)(Client& client,  String request, uint8_t method, long contentLength, bool authorized, String& response);
};

#endif
