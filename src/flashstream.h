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
};
#endif