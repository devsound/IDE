/*
* Software I2C driver for DevSound K20 devices
*
* 2014-01-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __SOFTI2C_H__
#define __SOFTI2C_H__

enum SOFTI2C_e {
  SOFTI2C_BUSY      = 0, // Working
  SOFTI2C_STARTSENT = 1, // Start sent, ready for "write" or "stop"
  SOFTI2C_ACK       = 2, // Data sent, ACK received, ready for "write", "start"(repeat) or "stop"
  SOFTI2C_NACK      = 3, // Data sent, ACK not received, ready for "start"(repeat) or "stop"
  SOFTI2C_STOPSENT  = 4, // Stop sent, ready for "start"
};

extern bool volatile softi2c_en;

void softi2c_sm();
void softi2c_start();
void softi2c_write(uint8_t address);
void softi2c_send(uint8_t data);
void softi2c_stop();
void softi2c_begin();
void softi2c_sm();

enum SOFTI2C_e softi2c_poll();

#endif

