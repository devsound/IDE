/*
* Hardware serial driver for DevSound K20 devices
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/
#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum SERIAL_e {
	SERIAL_8N1,
	SERIAL_8N2,
	SERIAL_8E1,
	SERIAL_8O1
};

void serial_begin(uint32_t divisor, enum SERIAL_e format);
void serial_end(void);
uint8_t serial_get(void);
uint8_t serial_peek(void);
void serial_put(uint8_t c);
int serial_available(void);

void serial1_begin(uint32_t divisor, enum SERIAL_e format);
void serial1_end(void);
void serial1_put(uint8_t c);

#ifdef __cplusplus
}

#include "Stream.h"

static class SerialClass : public Stream {
  public:
    inline SerialClass() {};
    inline void begin(uint32_t divisor, enum SERIAL_e format) { serial_begin(divisor, format); }
    inline void begin(uint32_t divisor) { begin(divisor, SERIAL_8N1); }
    inline void end() {};
    inline virtual int available(void) { return serial_available(); }
    inline virtual int peek(void) { return serial_peek(); }
    inline virtual int read(void) { return serial_get(); }
    inline int availableForWrite(void) { return 0; }
    inline virtual void flush(void) {}
    inline virtual size_t write(uint8_t n) { serial_put((char)n); return 1; }
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool() { return true; }
} Serial;

static class Serial1Class : public Stream {
  public:
    inline Serial1Class() {};
    inline void begin(uint32_t divisor, enum SERIAL_e format) { serial1_begin(divisor, format); }
    inline void begin(uint32_t divisor) { begin(divisor, SERIAL_8N1); }
    inline void end() {};
    inline virtual int available(void) { return 0; } // No read
    inline virtual int peek(void) { return -1; } // No read
    inline virtual int read(void) { return -1; } // No read
    inline int availableForWrite(void) { return 0; }
    inline virtual void flush(void) {}
    inline virtual size_t write(uint8_t n) { serial1_put((char)n); return 1; }
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool() { return true; }
} Serial1;

#endif

#endif// __SERIAL_H__
