#include "SPI.h"
#include <mchck.h>
#include <Arduino.h>

static struct spi_ctx ctx;
static bool initialized;

void spi_setorder(bool lsbfe) {
  SPI0.ctar[0].lsbfe = lsbfe;
}

void spi_setspeed(unsigned pbr, unsigned br, bool dbr) {
  SPI0.ctar[0].pbr = pbr; // Configure PBR (baud pre-scaler)
  SPI0.ctar[0].br  = br;  // Configure BR  (baud scaler)
  SPI0.ctar[0].dbr = dbr; // Configure DBR (baud doubler may skew duty)
}

void spi_setdmode(bool cpol, bool cpha) {
  SPI0.ctar[0].cpol = cpol; // Configure CPOL (clock polarity)
  SPI0.ctar[0].cpha = cpha; // Configure CPHA (clock phase)
}

void spi_begin(unsigned pin) {
  if(!initialized) {
    pin_mode(PIN_PTC5, PIN_MODE_MUX_ALT2); // PTC5 = SPI0_SCK  = SCLK (pin name = K20 name = SPI name)
    pin_mode(PIN_PTC6, PIN_MODE_MUX_ALT2); // PTC6 = SPI0_SOUT = MOSI
    pin_mode(PIN_PTC7, PIN_MODE_MUX_ALT2); // PTC7 = SPI0_SIN  = MISO
    spi_init();
  }
  if(pin > 0) pinMode(pin, DSPI);
}

void spi_io(unsigned pin, void *tx, size_t tx_len, void *rx, size_t rx_len, bool last, void (*cb)(void *cbdata), void *cbdata) {
  uint8_t pcs = 1 << chipSelectFromPin(pin);
  spi_queue_xfer(&ctx, pcs, (const uint8_t *)tx, (uint16_t)tx_len, (uint8_t *)rx, (uint16_t)rx_len, cb, cbdata, !last);
  if(!cb) while(spi_is_xfer_active());
}

uint8_t spi_iobyte(unsigned pin, uint8_t val, bool last) {
  uint8_t volatile rx;
  spi_io(pin, &val, 1, (uint8_t*)&rx, 1, last, NULL, NULL);
  return rx;
}