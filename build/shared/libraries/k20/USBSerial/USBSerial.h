/*
* Arduino wrapper for the USB and CDC VCP library by MCHCK
*
* Implements all classic Arduino functionality
*
* 2015-01-23 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __K20_USBSERIAL__
#define __K20_USBSERIAL__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void   usbserial_begin();
void   usbserial_write(void *data, size_t len);
void   usbserial_put(char data);
size_t usbserial_available();
char   usbserial_peek();
char   usbserial_get();

#ifdef __cplusplus
}

#include "Stream.h"

class USBSerialClass : public Stream {
  public:
    inline USBSerialClass() {};
    void begin();
    void end() {};
    virtual int available(void) { return usbserial_available(); }
    virtual int peek(void) { return usbserial_peek(); }
    virtual int read(void) { return usbserial_get(); }
    int availableForWrite(void) { return 0; }
    virtual void flush(void) {}
    virtual size_t write(uint8_t n) { usbserial_put((char)n); return 1; }
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool() { return true; }
};

extern USBSerialClass USBSerial;

#endif

#endif