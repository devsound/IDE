/*
* MCHCK driver for Wolfson WM8731 codec control interface
*
* NOTE: This library only controls the codec operation.
*
* 2014-01-07 @stg
*/
#include <mchck.h>
#include "codec.h"
#include "softi2c.h"

// Selected device address
static uint8_t  address;

#define Q_SIZE 10 // Register write queue size

union Q_u {
  uint16_t raw;
  struct {
    uint8_t msb;
    uint8_t lsb;
  };
  struct {
    uint16_t value : 9;
    uint16_t reg   : 7;
  };
} static volatile q[Q_SIZE]; // Register write queue

static unsigned volatile int q_head = 0, q_tail = 0; // Register write queue pointers
static bool volatile writing = false; // Write queue running
static uint16_t codec_reg_shadow[10]; // Register shadowing

CTASSERT_SIZE_BYTE(union Q_u, 2);

// Return shadow register
uint16_t codec_r(uint8_t reg) {
  return codec_reg_shadow[reg];
}

// Insert register write into queue
void codec_q(uint8_t reg, uint16_t value, uint16_t mask) {
  unsigned int head = q_head;
  if(++head >= Q_SIZE) head = 0;
  while(q_tail == head);
  value &= mask;
  if(reg < 10) {
    value = (codec_reg_shadow[reg] & ~mask) | value;
    codec_reg_shadow[reg] = value;
  } else {
    // Default reset values (from datasheet)
    reg = CODEC_RESET; value = 0;
    codec_reg_shadow[0] = codec_reg_shadow[1] = 0b010010111;
    codec_reg_shadow[2] = codec_reg_shadow[3] = 0b001111001;
    codec_reg_shadow[4] = 0b000001010;
    codec_reg_shadow[5] = 0b000001000;
    codec_reg_shadow[6] = codec_reg_shadow[7] = 0b010011111;
    codec_reg_shadow[8] = codec_reg_shadow[9] = 0b000000000;
  }
  q[q_head].raw = ((union Q_u) {
    .reg = reg,
    .value = value
  }).raw;
  q_head = head;
  if(!writing) {
    writing = true;
    softi2c_start();
  }
}

void codec_flush() {
  while(writing);
}

// Perform queued register writes
void softi2c_Handler(uint8_t state) {
  static uint8_t stage;
  unsigned int head, tail;
  switch(state) {
    case SOFTI2C_STARTSENT:
      softi2c_write(address);
      stage = 0;
      break;
    case SOFTI2C_ACK:
    case SOFTI2C_NACK: // Ignore NACK
           if(stage == 0)   softi2c_send(q[q_tail].lsb);
      else if(stage == 1)   softi2c_send(q[q_tail].msb);
      else /* stage == 2 */ softi2c_stop();
      stage++;
      break;
    case SOFTI2C_STOPSENT:
      head = q_head;
      tail = q_tail;
      if(++tail >= Q_SIZE) tail = 0;
      if(head != tail) softi2c_start();
      else writing = false;
      q_tail = tail;
      break;
  }
}

void codec_begin(CODEC_CSB_e codec_address) {
  address = codec_address;
  // Initialize I2C communication
  softi2c_begin();
  // Reset codec
  codec_reset();
  // Configure digital audio interface
  codec_interface_set((struct CODEC_INTERFACE_t) {
    .format = CODEC_FORMAT_I2S,
    .iwl    = CODEC_IWL_32BIT   // Lets always use 32-bit
  });
  // Default all volumes to zero
  codec_llinein_set((struct CODEC_LINEIN_t) { .volume = 0 });
  codec_rlinein_set((struct CODEC_LINEIN_t) { .volume = 0 });
  codec_lheadout_set((struct CODEC_HEADOUT_t) { .volume = 0 });
  codec_rheadout_set((struct CODEC_HEADOUT_t) { .volume = 0 });
  // Select output from DAC
  codec_apath_set((struct CODEC_APATH_t) { .dacsel = 1 });
  // Enable ADC
  codec_dpath_set((struct CODEC_DPATH_t) { });
  // Power down nothing (enable everything)
  codec_powerdown_set((struct CODEC_POWERDOWN_t) { });
  // Wait for all registers to update
  codec_flush();
}

