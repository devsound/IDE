/*
* Software I2C driver
*
* 2014-01-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*
* Note: EXPERIMENTAL, NOT WELL TESTED!
*
* 1) Does NOT support arbitration
* 2) Does NOT support reads
* 3) Supports limited clock stretching
* 4) Supports repeated start
*
*
* µDAD R004 and above should support soft-i2c via DMA to pin
* toggle register because both pins are now on the same port.
* This would be much faster, but is yet to be implemented.
* Also, responses (including ACK) would be completely ignored.
* In this case - a fair trade-off.
*/

#include <stdint.h>
#include <stdbool.h>
#include "softi2c.h"
//#include "serial.h"

/* == BEGIN: HARDWARE SPECIFIC CODE ================================= */
#include <mchck.h>         // Platform include for MCHCK

/*
// uDAD R001 (DEPRECATED)
#define SCLPORT  PORTA        // Pin configuration for µDAD
#define SCLGPIO  GPIOA        // Currently pins must share port
#define SCLCLOCK scgc5.porta
#define SCL   2
#define SDAPORT  PORTA
#define SDAGPIO  GPIOA
#define SDACLOCK scgc5.porta
#define SDA   4
*/

#if UDAD_REV >= 4
// uDAD R004
#define SCLPORT  PORTC        // Pin configuration for µDAD
#define SCLGPIO  GPIOC        // Currently pins must share port
#define SCLCLOCK scgc5.portc
#define SCL      4
#define SDAPORT  PORTC        // Pin configuration for µDAD
#define SDAGPIO  GPIOC        // Currently pins must share port
#define SDACLOCK scgc5.portc
#define SDA      7
#else
// uDAD R002-R003
#define SCLPORT  PORTA        // Pin configuration for µDAD
#define SCLGPIO  GPIOA        // Currently pins must share port
#define SCLCLOCK scgc5.porta
#define SCL      2
#define SDAPORT  PORTC        // Pin configuration for µDAD
#define SDAGPIO  GPIOC        // Currently pins must share port
#define SDACLOCK scgc5.portc
#define SDA      7
#endif

static void inline pin_config() {
    // Enable port clocks
    SIM.SCLCLOCK = 1;
    SIM.SDACLOCK = 1;
    // Configure GPIO data:low, direction:input/high-z
    SCLGPIO.pcor = (1 << SCL);
    SDAGPIO.pcor = (1 << SDA);
    // Set high impedance
    BITBAND_BIT(SCLGPIO.pddr, SCL) = 0;
    BITBAND_BIT(SDAGPIO.pddr, SDA) = 0;
    // Configure pins
    SCLPORT.pcr[SCL].raw =
    SDAPORT.pcr[SDA].raw = ((struct PCR_t) {
      .pe  = 1,            // pull-xx enable
      .ps  = PCR_PULLUP,   // pull-up
      .mux = PCR_MUX_GPIO  // connect to gpio
    }).raw;
}

static void inline scl_high() { BITBAND_BIT(SCLGPIO.pddr, SCL) = 0; }
static void inline sda_high() { BITBAND_BIT(SDAGPIO.pddr, SDA) = 0; }
static void inline scl_low()  { BITBAND_BIT(SCLGPIO.pddr, SCL) = 1; }
static void inline sda_low()  { BITBAND_BIT(SDAGPIO.pddr, SDA) = 1; }
static bool inline scl_read() { return BITBAND_BIT(SCLGPIO.pdir, SCL); }
static bool inline sda_read() { return BITBAND_BIT(SDAGPIO.pdir, SDA); }
/* == END: HARDWARE SPECIFIC CODE =================================== */

// State machine
       bool    volatile softi2c_en = false;   // enable
static uint8_t volatile sm_st = 0;            // state
static uint8_t volatile sm_da = true;         // data/address
static uint8_t volatile sm_is = SOFTI2C_BUSY; // last isr state

// Configure hardware
void softi2c_begin() {
  pin_config();
  sm_is = SOFTI2C_STOPSENT;
  sm_st = 24;
}

// START a transaction
// Allowed when: SOFTI2C_STOPSENT or SOFTI2C_ACK or SOFTI2C_NACK
// Results in: SOFTI2C_STARTSENT
void softi2c_start() {
  while(softi2c_en); // Block unless taken care of elsewhere
  // Ignore if not allowed
  if(sm_st != 19 && sm_st != 24) return;
  sm_st = sm_st == 19 ? 20 : 0;
  softi2c_en = true; // Resume operation
//  serial_put('S');
}

// Send a WRITE request to specified address
// Allowed when: SOFTI2C_STARTSENT
// Results in: SOFTI2C_ACK or SOFTI2C_NACK
void softi2c_write(uint8_t address) {
  while(softi2c_en); // Block unless taken care of elsewhere
  // Ignore if not allowed
  if(sm_st != 0) return;
  sm_st = 1;
  sm_da = address << 1; // Write request (read is | 1)
  softi2c_en = true; // Resume operation
//  serial_put(address);
//  serial_put('W');
}

// Send DATA to specified address
// Allowed when: SOFTI2C_ACK or SOFTI2C_NACK(not recommended)
// Results in: SOFTI2C_ACK or SOFTI2C_NACK
void softi2c_send(uint8_t data) {
  while(softi2c_en); // Block unless taken care of elsewhere
  // Ignore if not allowed
  if(sm_st != 19) return;
  sm_st = 1;
  sm_da = data;
  softi2c_en = true; // Resume operation
//  serial_put(data);
}

// STOP a transaction
// Allowed when: SOFTI2C_ACK or SOFTI2C_NACK
// Results in: SOFTI2C_STOPSENT
void softi2c_stop() {
  while(softi2c_en); // Block unless taken care of elsewhere
  // Ignore if not allowed
  if(sm_st != 19) return;
  sm_st = 22;
  softi2c_en = true; // Resume operation
//  serial_put('B');
//  serial_put('\n');
}

// Poll state of software i2c, see SOFTI2C_e for return values
enum SOFTI2C_e softi2c_poll() {
  if(softi2c_en) return SOFTI2C_BUSY;
  return sm_is;
}

// Weak dummy softi2c "interrupt" handler
void __softi2c_Handler(uint8_t state) {}
void softi2c_Handler () __attribute__ ((weak, alias ("__softi2c_Handler")));

// Software I2C state machine
// Should be called by systick or equivalent interrupt
// I2C data rate will be call rate / 2
extern uint32_t fetto;
void softi2c_sm() {
  if(!softi2c_en) return;
  
  uint8_t done = 0;
  bool stretch = false;

  /* == STOP processing == */
  if(sm_st == 0) {
    if(!scl_read()) stretch = true; // Clock stretching
    else {
      // Send START condition
      sda_low();
      done = SOFTI2C_STARTSENT;
    }
  /* ===================== */
  
  /* == DATA processing == */
  } else if(sm_st >= 1 && sm_st <= 17) {
    // Shift out DATA
    if(sm_st & 1) {
      if(sm_st != 1 && !scl_read()) stretch = true; // Clock stretching
      else {
        scl_low();
        if(sm_da & 0x80) sda_high(); else sda_low();
          sm_da = (sm_da << 1) | 0x01; // Shift in 1's
        }
    } else {
      scl_high();
    }
  /* ===================== */
  
  /* == ACK processing == */
  } else if(sm_st == 18) {
    scl_high(); // Slave will start driving ACK here
  } else if(sm_st == 19) {
    if(!scl_read()) stretch = true; // Clock stretching
    else {
      scl_low();
      done = sda_read() ? SOFTI2C_NACK : SOFTI2C_ACK;
    }
  /* ==================== */
    
  /* == REPEAT START processing == */
  } else if(sm_st == 20) {
    sda_high(); // Prepare repeated START
  } else if(sm_st == 21) {
    scl_high();
    // Jump to send START
  sm_st = 0;
  stretch = true;
  /* ============================= */
  
  /* == STOP processing == */
  } else if(sm_st == 22) {
    sda_low(); // Prepare STOP
  } else if(sm_st == 23) {
    scl_high();
  } else if(sm_st == 24) {
    if(!scl_read()) stretch = true; // Clock stretching
    else {
      sda_high(); // Send STOP condition
      done = SOFTI2C_STOPSENT;
    }
  }
  /* ===================== */
  
  if(done) {
    sm_is = done;
    softi2c_en = false;
    softi2c_Handler(done);
  } else if(!stretch) {
    sm_st++;
  }
}