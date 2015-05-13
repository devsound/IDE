/*
* Quadrature decoder driver for MCHCK
* Code is specifically designed for ROTARY ENCODERS WITH DETENTS and is not suitable for other purposes
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*
*/

#include <mchck.h>
#include "quadrature.h"

static uint32_t cntmem;
static uint16_t valmem;

void quadrature_init() {
  // Configure pins for QD_P, pull-up and filter
  PORTB.pcr[0].raw = ((struct PCR_t) { .mux = 6, .pe = 1, .ps = 1, .pfe = 1 }).raw; // FTM1_QD_P_HA
  PORTB.pcr[1].raw = ((struct PCR_t) { .mux = 6, .pe = 1, .ps = 1, .pfe = 1 }).raw; // FTM1_QD_P_HB
  // Configure FTM1 for quadrature decoding
  FTM1.sc.raw = ((struct FTM_SC_t) {
    .clks = FTM_CLKS_NONE,        // Disable FTM
  }).raw;
  FTM1.mode.wpdis = 1;            // Enable writes
  FTM1.mod = 0xFFFF;              // Maximum modulo
  FTM1.qdctrl.raw = ((struct FTM_QDCTRL_t) {
    .quaden = 1,                  // Enable quadrature decoder
    .quadmode = 0,                // Quadrature mode
    .phafltren = 1,               // Filter phase A
    .phbfltren = 1,               // Filter phase B
  }).raw;
  FTM1.filter.ch0fval = 15;       // Maximum filtration
  FTM1.filter.ch1fval = 15;       
  FTM1.cntin = 0;                 // Reset counter
  FTM1.cnt = 0;
  FTM1.sc.clks = FTM_CLKS_SYSTEM; // Enable FTM
  FTM1.mode.wpdis = 0;            // Disable writes
  cntmem = valmem = 0;
}

// Returns value(position) of quadrature decoder 0x0000...0xFFFF
// Performs hysteresis for optimal stability
// Increases one count per detent (one full quadrature cycle)
uint16_t quadrature_value() {
  uint16_t count = FTM1.cnt;
  int16_t delta;
  delta = count - (uint16_t)cntmem;
  if(delta >  1) delta -= 1;
  else
  if(delta < -1) delta += 1;
  else delta = 0;
  cntmem += delta;
  return (cntmem + 2) >> 2;
}

// Returns the change in value since last call as signed 16-bit integer
int16_t quadrature_delta() {
  uint16_t value = quadrature_value();
  int16_t delta = (int16_t)(value - valmem);
  valmem = value;
  return delta;
}