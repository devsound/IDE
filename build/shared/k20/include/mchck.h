#ifndef __MCHCK_H
#define __MCHCK_H

#include <sys/types.h>
#include <sys/cdefs.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdfix.h>
#include <stdarg.h>

#include <mchck-cdefs.h>

#ifdef TARGET_HOST

#include <host/host.h>

#else

#ifdef __cplusplus
 extern "C" {
#if 0                           /* to make emacs indent properly */
 }
#endif
#endif

#include <MK20DZ10.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _app_rom;

#include <intnums.h>

#include <kinetis/ftfl.h>
#include <kinetis/usbotg.h>
#include <kinetis/sim.h>
#include <kinetis/mcg.h>
#include <kinetis/rcm.h>
#include <kinetis/port.h>
#include <kinetis/gpio.h>
#include <kinetis/pmc.h>
#include <kinetis/adc.h>
#include <kinetis/spi.h>
#include <kinetis/lptmr.h>
#include <kinetis/ftm.h>
#include <kinetis/i2c.h>
#include <kinetis/pit.h>
#include <kinetis/tsi.h>
#include <kinetis/uart.h>
#include <kinetis/rtc.h>
#include <kinetis/crc.h>
#include <kinetis/llwu.h>
#include <kinetis/smc.h>
#include <kinetis/i2s.h>
#include <kinetis/dma.h>

#include <arm/scb.h>
#include <arm/nvic.h>
#include <arm/systick.h>

#include <mchck/mchck.h>

#include <kinetis/simd.h>

#ifdef __cplusplus
}
#endif

#endif  /* !host */

#ifdef __cplusplus
extern "C" {
#if 0                           /* to make emacs indent properly */
}
#endif
#endif

#include <usb/usb.h>

#include <nrf/nrf.h>

#ifdef __cplusplus
}
#endif

#endif
