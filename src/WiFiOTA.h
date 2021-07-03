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


#define DATA_SKETCH 0
#define DATA_FS 1 // JFFS ESP only 
#define DATA_JSON_CONFIG 2
#define DATA_BIN_CONFIG  3

#ifndef _WIFI_OTA_H_INCLUDED
#define _WIFI_OTA_H_INCLUDED

#include <Arduino.h>
#include <Server.h>
#include <Client.h>
#include <IPAddress.h>
#include <Udp.h>
#include "flashstream.h"

#include "OTAStorage.h"

class WiFiOTAClass {
protected:
  WiFiOTAClass();

  void begin(IPAddress& localIP, const char* name, const char* password, OTAStorage& storage, seekableStream& BINConfig, seekableStream& JSONConfig);

#ifndef DISABLE_ARDUINO_OTA_MDNS
  void pollMdns(UDP &mdnsSocket);
#endif
  
  void pollServer(Client& client);

public:
  void setCustomHandler(int (*fn)(Client& client, String request, long contentLength))
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

private:
  void sendHttpResponse(Client& client, int code, const char* status);
  void sendHttpContentHeader(Client& client, const char* content);
  void flushRequestBody(Client& client, long contentLength);
  long  openStorage(Client& client, unsigned int contentLength, short dataType);

private:
  String _name;
  String _expectedAuthorization;
  OTAStorage* _storage;
  seekableStream* _JSONConfig;
  seekableStream* _BINConfig;
  
  uint32_t localIp;
  uint32_t _lastMdnsResponseTime;
  
  void (*beforeApplyCallback)(void);
  int (*processCustomRequest)(Client& client, String request, long contentLength);
};

#endif
