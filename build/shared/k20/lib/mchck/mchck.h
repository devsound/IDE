#ifdef __cplusplus

#error "mchck libraries are not C++ compatible"

#else

#include <mchck/system.h>
#include <mchck/pin.h>
#include <mchck/gpio.h>
#include <mchck/flash.h>
#include <mchck/onboard-led.h>
#include <mchck/adc.h>
#include <mchck/dspi.h>
#include <mchck/timeout.h>
#include <mchck/stdio.h>
#include <mchck/ftm.h>
#include <mchck/crc.h>
#include <mchck/pit.h>
#include <mchck/rtc.h>
#include <mchck/spiflash.h>
#include <mchck/i2c.h>

#define __disable_irq() asm volatile("CPSID i");
#define __enable_irq()  asm volatile("CPSIE i");

#endif