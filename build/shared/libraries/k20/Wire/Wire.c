#include "Wire.h"
#include <mchck.h>
#include <Arduino.h>

static struct spi_ctx ctx;

void wire_begin() {
  i2c_init(I2C_RATE_100);
}