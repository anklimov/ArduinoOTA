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

#ifndef _ARDUINOOTA_H_
#define _ARDUINOOTA_H_

#include "WiFiOTA.h"
#ifdef __AVR__
#if FLASHEND >= 0xFFFF
#include "InternalStorageAVR.h"
#endif
#elif defined(ESP8266) || defined(ESP32)
#include "InternalStorageESP.h"
#else
#include "InternalStorage.h"
#endif
#ifdef __SD_H__
#include "SDStorage.h"
SDStorageClass SDStorage;
#endif
#ifdef SerialFlash_h_
#include "SerialFlashStorage.h"
SerialFlashStorageClass SerialFlashStorage;
#endif

#include "seekablestream.h"

//#define HTTP_TEXT_PLAIN   0x0100
//#define HTTP_TEXT_JSON    0x0200
//#define HTTP_OCTET_STREAM 0x0300


#ifndef OTA_PORT
const uint16_t OTA_PORT = 65280;
#endif

template <class NetServer, class NetClient>
class ArduinoOTAClass : public WiFiOTAClass {

private:
  NetServer server;

public:
  ArduinoOTAClass() : server(OTA_PORT) 
	    {
	    };

  void begin(IPAddress localIP, const char* name, const char* password, OTAStorage& storage, seekableStream& file) {
    WiFiOTAClass::begin(localIP, name, password, storage, file);
    server.begin();
  }

  void begin(IPAddress localIP, const char* name, const char* password, OTAStorage& storage) {
    WiFiOTAClass::begin(localIP, name, password, storage);
    server.begin();
  }  

  void end() {
#if defined(ESP8266) || defined(ESP32)
    server.stop();
#elif defined(_WIFI_ESP_AT_H_)
    server.end();
#else
//#warning "The networking library doesn't have a function to stop the server"
#endif
  }

  void poll() {
    NetClient client = server.available();
    pollServer(client);
  }

  void handle() { // alias
    poll();
  }

};

template <class NetServer, class NetClient, class NetUDP>
class ArduinoOTAMdnsClass : public ArduinoOTAClass<NetServer, NetClient> {

private:
  NetUDP mdnsSocket;

public:
  ArduinoOTAMdnsClass() {};

  void begin(IPAddress localIP, const char* name, const char* password, OTAStorage& storage, seekableStream& file) {
    ArduinoOTAClass<NetServer, NetClient>::begin(localIP, name, password, storage, file);
#if defined(ESP8266) && !(defined(ethernet_h_) || defined(ethernet_h) || defined(UIPETHERNET_H))
    mdnsSocket.beginMulticast(localIP, IPAddress(224, 0, 0, 251), 5353);
#else
    mdnsSocket.beginMulticast(IPAddress(224, 0, 0, 251), 5353);
#endif
  }

  void begin(IPAddress localIP, const char* name, const char* password, OTAStorage& storage) {
    ArduinoOTAClass<NetServer, NetClient>::begin(localIP, name, password, storage);
#if defined(ESP8266) && !(defined(ethernet_h_) || defined(ethernet_h) || defined(UIPETHERNET_H))
    mdnsSocket.beginMulticast(localIP, IPAddress(224, 0, 0, 251), 5353);
#else
    mdnsSocket.beginMulticast(IPAddress(224, 0, 0, 251), 5353);
#endif
  }


  void end() {
    ArduinoOTAClass<NetServer, NetClient>::end();
    mdnsSocket.stop();
  }

  void poll() {
    ArduinoOTAClass<NetServer, NetClient>::poll();
    WiFiOTAClass::pollMdns(mdnsSocket);
  }

};


#if defined(ethernet_h_) || defined(ethernet_h) // Ethernet library

    #if defined (ARDUINO_OTA_MDNS_DISABLE) //Forced disable
    ArduinoOTAClass  <EthernetServer, EthernetClient>   ArduinoOTA;
    #else
    ArduinoOTAMdnsClass  <EthernetServer, EthernetClient, EthernetUDP>   ArduinoOTA;
    #endif

#elif defined(UIPETHERNET_H) // no UDP multicast implementation yet
ArduinoOTAClass  <EthernetServer, EthernetClient>   ArduinoOTA;

#elif defined(WiFiNINA_h) || defined(WIFI_H) || defined(ESP8266) || defined(ESP32) // NINA, WiFi101 and Espressif WiFi

    #if defined (ARDUINO_OTA_MDNS_DISABLE) //Forced disable
    ArduinoOTAClass  <WiFiServer, WiFiClient> ArduinoOTA;
    #else
    #include <WiFiUdp.h>
    ArduinoOTAMdnsClass <WiFiServer, WiFiClient, WiFiUDP> ArduinoOTA;
    #endif
 
#elif defined(WiFi_h) || defined(_WIFI_ESP_AT_H_) // WiFi, WiFiLink and WiFiEspAT lib (no UDP multicast) for WiFiLink the firmware handles mdns
ArduinoOTAClass  <WiFiServer, WiFiClient> ArduinoOTA;

#elif defined(_WIFISPI_H_INCLUDED) // no UDP multicast implementation
ArduinoOTAClass  <WiFiSpiServer, WiFiSpiClient> ArduinoOTA;

#else
#error "Network library not included or not supported"
#endif

#endif
