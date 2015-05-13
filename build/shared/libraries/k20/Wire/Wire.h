/*
* Arduino wrapper for I2C peripheral library by MCHCK
*
* Implements classic Arduino functionality with exceptions:
*   TODO: No slave support
*
* 2015-05-13 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __K20_WIRE__
#define __K20_WIRE__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes for library C code
void    wire_begin();

#ifdef __cplusplus
}

// Baudrate pre-scaler
typedef enum {
  SPI_SCALE_DIV2 = 0,
  SPI_SCALE_DIV3 = 1,
  SPI_SCALE_DIV5 = 2,
  SPI_SCALE_DIV7 = 3,
} SPI_scale;

// SPI class
class WireClass {
  public:
    // Classic Arduino functionality
    inline void begin() { wire_begin(); }
};

extern WirClass Wire;

#endif//__cplusplus

#endif//__K20_WIRE__