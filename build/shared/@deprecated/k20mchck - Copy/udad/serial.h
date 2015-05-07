#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <stdint.h>
#include <stdlib.h>

enum SERIAL_FMT_e {
	SERIAL_FMT_8N1,
	SERIAL_FMT_8N2,
	SERIAL_FMT_8E1,
	SERIAL_FMT_8O1
};

void serial_begin(uint32_t divisor, enum SERIAL_FMT_e format);
void serial_end(void);
uint8_t serial_get(void);
void serial_put(uint8_t c);
void serial_write(const void *buf, size_t count);
void serial_print(char *str);
int serial_available(void);

void serial1_begin(uint32_t divisor, enum SERIAL_FMT_e format);
void serial1_end(void);
void serial1_put(uint8_t c);
void serial1_write(const void *buf, size_t count);

#endif