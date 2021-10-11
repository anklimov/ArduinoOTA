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

#include <Arduino.h>

#include "WiFiOTA.h"

#define BOARD "arduino"
#define BOARD_LENGTH (sizeof(BOARD) - 1)
const char CODES[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static String base64Encode(const String& in)
{
  int b;
  String out;
  out.reserve((in.length()) * 4 / 3);
  
  for (unsigned int i = 0; i < in.length(); i += 3) {
    b = (in.charAt(i) & 0xFC) >> 2;
    //out += CODES[b];
    out += (char) pgm_read_byte_near(&CODES[b]);

    b = (in.charAt(i) & 0x03) << 4;
    if (i + 1 < in.length()) {
      b |= (in.charAt(i + 1) & 0xF0) >> 4;
      //out += CODES[b];
      out += (char) pgm_read_byte_near(&CODES[b]);

      b = (in.charAt(i + 1) & 0x0F) << 2;
      if (i + 2 < in.length()) {
         b |= (in.charAt(i + 2) & 0xC0) >> 6;
         //out += CODES[b];
         out += (char) pgm_read_byte_near(&CODES[b]);

         b = in.charAt(i + 2) & 0x3F;
         //out += CODES[b];
         out += (char) pgm_read_byte_near(&CODES[b]);

      } else {
        //out += CODES[b];
        out += (char) pgm_read_byte_near(&CODES[b]);
        out += '=';
      }
    } else {
      //out += CODES[b];
      out += (char) pgm_read_byte_near(&CODES[b]);
      out += "==";
    }
  }
  return out;
}

WiFiOTAClass::WiFiOTAClass() :
  _storage(NULL),
  localIp(0),
  _lastMdnsResponseTime(0),
  beforeApplyCallback(nullptr),
  processCustomRequest(nullptr)
{
}

void WiFiOTAClass::begin(IPAddress& localIP, const char* name, const char* password, OTAStorage& storage, seekableStream& cstream)
{
  localIp = localIP;
  _name = name;
  _expectedAuthorization = String(F("Basic ")) + base64Encode(String(F("arduino:")) + String(password));
  _storage = &storage;
  _file=&cstream;
}

#ifndef ARDUINO_OTA_MDNS_DISABLE
void WiFiOTAClass::pollMdns(UDP &_mdnsSocket)
{
  int packetLength = _mdnsSocket.parsePacket();

  if (packetLength <= 0) {
    return;
  }

  const byte ARDUINO_SERVICE_REQUEST[37] = {
    0x00, 0x00, // transaction id
    0x00, 0x00, // flags
    0x00, 0x01, // questions
    0x00, 0x00, // answer RRs
    0x00, 0x00, // authority RRs
    0x00, 0x00, // additional RRs
    0x08,
    0x5f, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e, 0x6f, // _arduino
    0x04, 
    0x5f, 0x74, 0x63, 0x70, // _tcp
    0x05,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, // local
    0x00, 0x0c, // PTR
    0x00, 0x01 // Class IN
  };

  if (packetLength != sizeof(ARDUINO_SERVICE_REQUEST)) {
    while (packetLength) {
      if (_mdnsSocket.available()) {
        packetLength--;
      }
    }
    return;
  }

  byte request[packetLength];

  _mdnsSocket.read(request, sizeof(request));

  if (memcmp(&request[2], &ARDUINO_SERVICE_REQUEST[2], packetLength - 2) != 0) {
    return;
  }

  if ((millis() - _lastMdnsResponseTime) < 1000) {
    // ignore request
    return;
  }
  _lastMdnsResponseTime = millis();

  _mdnsSocket.beginPacket(IPAddress(224, 0, 0, 251), 5353);

  const byte responseHeader[] = {
    0x00, 0x00, // transaction id
    0x84, 0x00, // flags
    0x00, 0x00, // questions
    0x00, 0x04, // answers RRs
    0x00, 0x00, // authority RRs
    0x00, 0x00  // additional RRS
  };
  _mdnsSocket.write(responseHeader, sizeof(responseHeader));

  const byte ptrRecordStart[] = {
    0x08,
    '_', 'a', 'r', 'd', 'u', 'i', 'n', 'o',
    
    0x04,
    '_', 't', 'c', 'p',

    0x05,
    'l', 'o', 'c', 'a', 'l',
    0x00,

    0x00, 0x0c, // PTR
    0x00, 0x01, // class IN
    0x00, 0x00, 0x11, 0x94, // TTL

    0x00, (byte)(_name.length() + 3), // length
    (byte)_name.length()
  };

  const byte ptrRecordEnd[] = {
    0xc0, 0x0c
  };

  _mdnsSocket.write(ptrRecordStart, sizeof(ptrRecordStart));
  _mdnsSocket.write((const byte*) _name.c_str(), _name.length());
  _mdnsSocket.write(ptrRecordEnd, sizeof(ptrRecordEnd));

  const byte txtRecord[] = {
    0xc0, 0x2b,
    0x00, 0x10, // TXT strings
    0x80, 0x01, // class
    0x00, 0x00, 0x11, 0x94, // TTL
    0x00, (50 + BOARD_LENGTH),
    13,
    's', 's', 'h', '_', 'u', 'p', 'l', 'o', 'a', 'd', '=', 'n', 'o',
    12,
    't', 'c', 'p', '_', 'c', 'h', 'e', 'c', 'k', '=', 'n', 'o',
    15,
    'a', 'u', 't', 'h', '_', 'u', 'p', 'l', 'o', 'a', 'd', '=', 'y', 'e', 's',
    (6 + BOARD_LENGTH),
    'b', 'o', 'a', 'r', 'd', '=',
  };
  _mdnsSocket.write(txtRecord, sizeof(txtRecord));
  _mdnsSocket.write((byte*)BOARD, BOARD_LENGTH);

  const byte srvRecordStart[] = {
    0xc0, 0x2b, 
    0x00, 0x21, // SRV
    0x80, 0x01, // class
    0x00, 0x00, 0x00, 0x78, // TTL
    0x00, (byte)(_name.length() + 9), // length
    0x00, 0x00,
    0x00, 0x00,
    0xff, 0x00, // port
    (byte)_name.length()
  };

  const byte srvRecordEnd[] = {
    0xc0, 0x1a
  };

  _mdnsSocket.write(srvRecordStart, sizeof(srvRecordStart));
  _mdnsSocket.write((const byte*) _name.c_str(), _name.length());
  _mdnsSocket.write(srvRecordEnd, sizeof(srvRecordEnd));

  byte aRecordNameOffset = sizeof(responseHeader) +
                            sizeof(ptrRecordStart) + _name.length() + sizeof(ptrRecordEnd) + 
                            sizeof(txtRecord) + BOARD_LENGTH +
                            sizeof(srvRecordStart) - 1;

  byte aRecord[] = {
    0xc0, aRecordNameOffset,

    0x00, 0x01, // A record
    0x80, 0x01, // class
    0x00, 0x00, 0x00, 0x78, // TTL
    0x00, 0x04,
    0xff, 0xff, 0xff, 0xff // IP
  };
  memcpy(&aRecord[sizeof(aRecord) - 4], &localIp, sizeof(localIp));
  _mdnsSocket.write(aRecord, sizeof(aRecord));

  _mdnsSocket.endPacket();
}
#endif

long  WiFiOTAClass::openStorage(String fileName, long  contentLength, char mode, uint16_t * dataType, bool isAuthorized)
{        
         if (!isAuthorized && mode =='w') return -2;
         if (fileName == String(F("/sketch")) && mode == 'w')
              { 
                if (dataType) *dataType = DATA_SKETCH;
                if (_storage && _storage->open(contentLength, DATA_SKETCH)) 
                        return _storage->maxSize(); else return 0;
              }

         else if (fileName == String(F("/data")) && mode == 'w')    
              {
                if (dataType) *dataType = DATA_FS;
                if (_storage && _storage->open(contentLength, DATA_FS)) 
                        return _storage->maxSize(); else return 0;
                
              } 

         else {
               if (_file && !_file->checkPermissions(fileName,mode,isAuthorized)) return -2;
               if (_file && _file->open(fileName,mode)) 
                        {
                        if (dataType) *dataType = DATA_FILE | _file->getContentType();   
                        return _file->getSize();
                         } else return -1;
              } 
        
}

void WiFiOTAClass::closeStorage(uint16_t dataType)
  {
  switch (dataType & 0xF)
        {
        case DATA_SKETCH: 
        case DATA_FS:
          _storage->close();
        break;
        case DATA_FILE:
          _file->putEOF();
          _file->close();
        break;                  
        }  
  }


void WiFiOTAClass::pollServer(Client& client)
{

  if (client) {
    String request = client.readStringUntil('\n');
    request.trim();

    String header;
    long contentLength = -1;
    String authorization;
    String response;
    uint8_t method = 0;
    char mode = '\0';
    //String URI;

    do {
      header = client.readStringUntil('\n');
      header.trim();

      if (header.startsWith(F("Content-Length: "))) {
        header.remove(0, 16);

        contentLength = header.toInt();
      } else if (header.startsWith(F("Authorization: ")) || header.startsWith(F("authorization: "))) {
        header.remove(0, 15);

        authorization = header;
      }
    } while (header != "");

    uint16_t  retCode;
    bool authorized = (_expectedAuthorization == authorization);

    //remove HTTP/1.1
        int pos=request.lastIndexOf(' ');
        if (pos)
            request.remove(pos,request.length()-pos);    

   if (request.startsWith(F("POST"))) 
                  {
                    method = HTTP_POST;
                    //URI=request;
                    request.remove(0,5);
                    mode = 'w';
                  }
    else if (request.startsWith(F("GET")))       
                  {
                    method = HTTP_GET; 
                    //URI=request;
                    request.remove(0,4);
                    mode = 'r';
                  } 
    else if (request.startsWith(F("PUT")))       
                  {
                    method = HTTP_PUT; 
                    //URI=request;
                    request.remove(0,4);
                    mode = 'w';
                  }   
     else if (request.startsWith(F("DELETE")))       
                  {
                    method = HTTP_DELETE; 
                    //URI=request;
                    request.remove(0,7);
                  }     
      else if (request.startsWith(F("OPTIONS")))       
                  {
                    method = HTTP_OPTIONS; 
                    //URI=request;
                    request.remove(0,8);
                  }                         

    else 
                  {
                  flushRequestBody(client, contentLength);
                  sendHttpResponse(client, 400);
                  return;
                  } 

   // Check custom handler
   if (processCustomRequest && (retCode = processCustomRequest(client,request,method,contentLength,authorized, response)))
    {
      if (retCode == 1)
          {
          client.stop();
          return;
          }
      else 
          {
          sendHttpResponseWithText(client, retCode, true, response);    
          return;
          }  
    }
    else // local operation
    { 

    #ifdef CORS  
    if (method == HTTP_OPTIONS)
    {
      sendHttpResponse(client, 200, false); 
      client.println(F("Access-Control-Allow-Credentials: true"));
      client.println(F("Access-Control-Allow-Methods: POST,GET,OPTIONS"));
      client.println(F("Access-Control-Allow-Headers: Content-type,Connection,Authorization,Pragma"));
      delay(100);
      client.stop();
      return;
    }
    #endif

    uint16_t  dataType = DATA_UNKNOWN;
    long maxSize;

    if (!(maxSize=openStorage(request, contentLength,mode,&dataType,authorized))) 
          {
            flushRequestBody(client, contentLength);
            sendHttpResponse(client, 500);
            return;
          }       

    else if (maxSize == -1)
          {
            flushRequestBody(client, contentLength);
            sendHttpResponse(client, 404);
            return;
          }
          
    else if (maxSize == -2) {
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 401);
      return;   
    }   

    //Serial.print("Opened DT ");
    //Serial.println(dataType,HEX);

    if ((method == HTTP_POST) && contentLength <= 0) {
      sendHttpResponse(client, 400);
      return;
    }


    if (contentLength > maxSize) {
      closeStorage(dataType);
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 413);
      return;
    }

    long read = 0;
    #if defined(__SAM3X8E__)
    uint8_t buff[1024];
    #else
    byte buff[64];
    #endif
    
    if (method == HTTP_POST)
    {
          while (client.connected() && read < contentLength) {
            while (client.available()) {
              size_t l = client.read(buff, sizeof(buff));
              
               switch (dataType  & 0xF)
                {
                case DATA_SKETCH: 
                case DATA_FS:
                      for (int i = 0; i < l; i++) 
                                _storage->write(buff[i]);
                      read += l;
 
                break;
                case DATA_FILE:
                      _file->write(buff,l);
             //         Serial.write(buff,l);
                      read += l;
                break;

                }
            }
          }
          closeStorage(dataType);

          if (read == contentLength) 
          {
            sendHttpResponse(client, 200);

            delay(500);

            switch (dataType & 0xF)
                {
                case DATA_SKETCH: 
                case DATA_FS:
                if (beforeApplyCallback) {
                beforeApplyCallback();
                 }

                // apply the update
                _storage->apply();
                
                while (true);
                } 
          } 
          else 
          {
          sendHttpResponse(client, 414);
          _storage->clear();
          delay(100);
          client.stop();
          }        
     }
    else if (method == HTTP_GET)//Download
     { 
       int16_t ch;
       uint32_t counter=0;
       uint16_t i=0;
       
       switch (dataType & 0xF)
          {
          case DATA_SKETCH: 
          case DATA_FS:  
            sendHttpResponse(client, 400);
            return; 
          case DATA_FILE: 
            sendHttpResponse(client,200 | (dataType & 0xF000),false);
            client.println();
           // if (!_file) break;
           // _file->seek();
              while ( client.connected()  && _file->available() ) 
                {
                  ch = _file->read();

                  counter++;
                  buff[i++]=ch;
                  if (i==sizeof(buff))
                              {
                                client.write(buff,i);
             //                   Serial.write(buff,i);
                                i=0;
                              }
                  
                }
             if (client.connected() && i) client.write(buff,i);
             //Serial.write(buff,i);
            break;
          
        }
      client.stop();   

     } //download
     else sendHttpResponse(client, 400);
    } //Authorized local operation
  }
}

void WiFiOTAClass::sendHttpResponse(Client& client, uint16_t code,  bool closeSocket)
{
String nullstr="";  
sendHttpResponseWithText(client,code,closeSocket,nullstr);
}

void WiFiOTAClass::sendHttpResponseWithText(Client& client, uint16_t code,  bool closeSocket, String& response)
{
  while (client.available()) {
    client.read();
  }
  uint16_t  httpRetCode     = code & 0xFFF;
  uint16_t contentTypeCode = code & 0xF000; 
  
  client.print(String(F("HTTP/1.1 ")) + String(httpRetCode) + String F((" ")));

  switch (httpRetCode)
  {
      case 400: client.println(F("Bad request"));
      break;
      case 414: client.println(F("Payload size wrong"));
      break;
      case 401: client.println(F("Unauthorized"));
                client.println(F("WWW-Authenticate: Basic realm=\"Access restricted\""));
      break;
      case 404: client.println(F("Not found"));
      break;
      case 413: client.println(F("Payload Too Large"));
      break;
      case 500: client.println(F("Internal Server Error"));
      break;
      case 301: client.println(F("Moved permanently"));
      break;
      case 200: client.println(F("OK"));
      break;
      default:
      client.println(F("Unknown"));
  }
         
  #ifdef CORS
  client.println(F("Access-Control-Allow-Origin: " CORS));
  #endif

    if (contentTypeCode)
    {
    client.println(F("Connection: close"));  
    client.print(F("Content-Type: "));
    switch (contentTypeCode) {
      case HTTP_TEXT_PLAIN: client.println(F("text/plain"));
      break;
      case HTTP_TEXT_JSON:  client.println(F("text/json"));
      break;
      case HTTP_TEXT_HTML: client.println(F("text/html"));
      break;
      case HTTP_IMAGE_GIF:  client.println(F("image/gif"));
      break;
      case HTTP_IMAGE_JPEG:  client.println(F("image/jpeg"));
      break;
      default:
      client.println(F("octet/stream"));
    }
  }

  if (response!="") 
          { 
          client.print("\n"+response);
          if (closeSocket) {delay(100);client.stop();}
          }
  else  
  if (closeSocket) 
          {
          client.println();
          delay(100);
          client.stop();
          }
}

void WiFiOTAClass::flushRequestBody(Client& client, long contentLength)
{
  long read = 0;

  while (client.connected() && read < contentLength) {
    if (client.available()) {
      read++;

      client.read();
    }
  }
}

