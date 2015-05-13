#ifndef __SYSTICK_H__
#define __SYSTICK_H__
#include <mchck.h>


void systick_init(uint32_t frequency);
void systick_attach(void (*fp)());

static const struct {
  void (*begin)(uint32_t frequency);
  void (*attachInterrupt)(void (*fp)());
} SysTick = {
  systick_init,
  systick_attach
};
#endif