/*
* Arduino wrapper for CSPI peripheral library by MCHCK
*
* Implements all classic Arduino functionality with one exception:
*   usingInterrupt(num), which was deprecated as it is not necessary
*
* Also implements the following extensions:
*   setSpeed(scale, speed, doubleRate), for K20 specific baudrate control
*   transfer([chipSelect, ]rxBuffer, rxCount, txBuffer, txCount[, mode]), for buffer transfers
*   transferAsync([chipSelect, ]rxBuffer, rxCount, txBuffer, txCount[, mode], cb, cbdata), for asynchronous buffer transfers
*
* 2015-04-11 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __K20_SPI__
#define __K20_SPI__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for local C code
void    spi_begin(unsigned pin);
void    spi_setorder(bool order);
void    spi_setspeed(unsigned pbr, unsigned br, bool dbr);
void    spi_setdmode(bool cpol, bool cpha);
//void    spi_io(unsigned pin, void *tx, size_t tx_len, void *rx, size_t rx_len, bool last);
void    spi_io(unsigned pin, void *tx, size_t tx_len, void *rx, size_t rx_len, bool last, void (*cb)(void *cbdata), void *cbdata);
uint8_t spi_iobyte(unsigned pin, uint8_t val, bool last);

#ifdef __cplusplus
}

// Baudrate pre-scaler
typedef enum {
  SPI_SCALE_DIV2 = 0,
  SPI_SCALE_DIV3 = 1,
  SPI_SCALE_DIV5 = 2,
  SPI_SCALE_DIV7 = 3,
} SPI_scale;

// Baudrate scaler
typedef enum {
  SPI_CLOCK_DIV2     =  0,
  SPI_CLOCK_DIV4     =  1,
  SPI_CLOCK_DIV6     =  2,
  SPI_CLOCK_DIV8     =  3,
  SPI_CLOCK_DIV16    =  4,
  SPI_CLOCK_DIV32    =  5,
  SPI_CLOCK_DIV64    =  6,
  SPI_CLOCK_DIV128   =  7,
  SPI_CLOCK_DIV256   =  8,
  SPI_CLOCK_DIV512   =  9,
  SPI_CLOCK_DIV1024  = 10,
  SPI_CLOCK_DIV2048  = 11,
  SPI_CLOCK_DIV4096  = 12,
  SPI_CLOCK_DIV8192  = 13,
  SPI_CLOCK_DIV16384 = 14,
} SPI_speed;

// Data order
typedef enum {
  LSBFIRST = 1,
  MSBFIRST = 0,
} SPI_order;

// Transfer mode (polarity, phase)
typedef enum {
  SPI_MODE0 = 0b00,
  SPI_MODE1 = 0b01,
  SPI_MODE2 = 0b10,
  SPI_MODE3 = 0b11,
} SPI_dmode;

// Data (chip-select) continuation
typedef enum {
  SPI_CONTINUE = 0,
  SPI_LAST     = 1,
} SPI_tmode;

// Configuration class
class SPISettings {
  public:
    SPI_speed speed;
    SPI_order order;
    SPI_dmode dmode;
    inline SPISettings(SPI_speed speed, SPI_order order, SPI_dmode dmode) {
      this->speed = speed;
      this->order = order;
      this->dmode = dmode;
    }
};

// SPI class
class SPIClass {
  public:
    // Classic Arduino functionality
    inline void begin(unsigned cspin) { spi_begin(cspin); }
    inline void begin() { begin(0); }
    inline void end() {}
    inline void setBitOrder(SPI_order order) { spi_setorder(order); }
    inline void setClockDivider(SPI_speed speed) { spi_setspeed(1, speed, 0); }
    inline void setDataMode(SPI_dmode mode) { spi_setdmode(!!(mode & 2), !!(mode & 1)); }
    inline void beginTransaction(SPISettings &settings) {
      setBitOrder(settings.order);
      setClockDivider(settings.speed);
      setDataMode(settings.dmode);
    }
    inline void endTransaction() {}
    inline uint8_t transfer(unsigned cspin, uint8_t val, SPI_tmode mode) { return spi_iobyte(cspin, val, !!mode); }
    inline uint8_t transfer(unsigned cspin, uint8_t val) { return transfer(cspin, val, SPI_LAST); }
    inline uint8_t transfer(uint8_t val) { return transfer(0, val); }
    // Deprecated: usingInterrupt
    // Extensions for K20:
    inline void setSpeed(SPI_scale scale, SPI_speed speed, bool doubleRate) { spi_setspeed(scale, speed, doubleRate); }
    inline void setSpeed(SPI_scale scale, SPI_speed speed) { setSpeed(scale, speed, false); }
    inline void transfer(unsigned cspin, void *rxbuf, size_t rxlen, void *txbuf, size_t txlen, SPI_tmode mode) { spi_io(cspin, txbuf, txlen, rxbuf, rxlen, !!mode, NULL, NULL); }
    inline void transfer(unsigned cspin, void *rxbuf, size_t rxlen, void *txbuf, size_t txlen) { transfer(cspin, rxbuf, rxlen, txbuf, txlen, SPI_LAST); }
    inline void transfer(void *rxbuf, size_t rxlen, void *txbuf, size_t txlen) { transfer(0, rxbuf, rxlen, txbuf, txlen); }
    inline void transferAsync(unsigned cspin, void *rxbuf, size_t rxlen, void *txbuf, size_t txlen, SPI_tmode mode, void (*cb)(void *cbdata), void *cbdata) { spi_io(cspin, txbuf, txlen, rxbuf, rxlen, !!mode, cb, cbdata); }
    inline void transferAsync(unsigned cspin, void *rxbuf, size_t rxlen, void *txbuf, size_t txlen, void (*cb)(void *cbdata), void *cbdata) { transferAsync(cspin, rxbuf, rxlen, txbuf, txlen, SPI_LAST, cb, cbdata); }
    inline void transferAsync(void *rxbuf, size_t rxlen, void *txbuf, size_t txlen, void (*cb)(void *cbdata), void *cbdata) { transferAsync(0, rxbuf, rxlen, txbuf, txlen, cb, cbdata); }
};

extern SPIClass SPI;

#endif//__cplusplus

#endif//__K20_SPI__