#ifndef _FLASHSTREAM_H_
#define _FLASHSTREAM_H_

#include <Stream.h>
#include <Arduino.h>

class seekableStream : public Stream 
{
unsigned int streamSize;       
public:
seekableStream(unsigned int size):Stream(),streamSize(size) {};
unsigned int getSize() {return streamSize;}
virtual unsigned int seek(unsigned int _pos = 0) = 0;
   // virtual int available() { return 1;};
   // virtual int read() {};
   // virtual int peek() {};
   // virtual void flush() {};
   // virtual size_t write(uint8_t) {};
};
#endif