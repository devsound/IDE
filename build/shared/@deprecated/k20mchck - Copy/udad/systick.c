#include <mchck.h>
#include "systick.h"

void systick_init(uint32_t frequency) {
  SYSTICK.load.raw = ((struct SYSTICK_LOAD) {
#if EXTERNAL_XTAL==32768
    .reload = ((F_CPU << 1) / frequency) - 1
#else
    .reload = (F_CPU / frequency) - 1
#endif
	}).raw;
  SYSTICK.val.raw  = ((struct SYSTICK_VAL) {
		.current = 0
	}).raw;
  SYSTICK.ctrl.raw = ((struct SYSTICK_CTRL) {
		.enable = 1,
		.tickint = 1,
		.clksource = 1,
	}).raw;
}