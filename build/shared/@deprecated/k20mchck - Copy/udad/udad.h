#ifndef __UDAD_H__
#define __UDAD_H__

// Targets
#define UDAD 1

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mchck.h>
#include "audio.h"
#include "serial.h"
#include "quadrature.h"

#define ALWAYS_INLINE __attribute__((__always_inline__)) inline
#define DMAMEM __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)))

void setup();
void loop();

extern uint32_t millis();
extern void delay(uint32_t ms);

typedef uint8_t byte;
typedef uint16_t word;

// Arduino style object emulation for serial driver
static const struct {
  void (*begin)(uint32_t divisor, enum SERIAL_FMT_e format);
  void (*end)(void);
  uint8_t (*get)(void);
  void (*put)(uint8_t c);
  void (*write)(const void *buf, size_t count);
  int (*available)(void);
  void (*print)(char *str);
} Serial = {
  serial_begin,
  serial_end,
  serial_get,
  serial_put,
  serial_write,
  serial_available,
  serial_print,
};

// Arduino style object emulation for serial1 tx-only driver
static const struct {
  void (*begin)(uint32_t divisor, enum SERIAL_FMT_e format);
  void (*end)(void);
  void (*put)(uint8_t c);
  void (*write)(const void *buf, size_t count);
} Serial1 = {
  serial1_begin,
  serial1_end,
  serial1_put,
  serial1_write,
};

// Arduino style object emulation for quadrature driver
static const struct {
  void (*begin)();
  uint16_t (*count)();
  int16_t (*delta)();
} Encoder = {
  quadrature_init,
  quadrature_value,
  quadrature_delta,
};

#define DEFAULT       0 // pin mode/adc reference
#define INPUT         1
#define OUTPUT        2
#define ANALOG_IN     3
#define ANALOG_INPUT  3
#define ADC           3
#define ANALOG_OUT    4
#define ANALOG_OUTPUT 4
#define PWM           4

// Configure bits 0-1
#define DRIVE_WEAK      (1 << 0)
#define DRIVE_STRONG    (1 << 1)

// Configure bits 2-3
#define OPEN_DRAIN      (1 << 2)
#define PUSH_PULL       (1 << 3)

// Configure bits 4-5
#define FILTER_ENABLE   (1 << 4)
#define FILTER_DISABLE  (1 << 5)

// Configure bits 5-6
#define SLEW_FAST      (1 << 6)
#define SLEW_SLOW      (1 << 7)

// Configure bits 7-8
#define PULL_NONE      ((1 << 8) | (1 << 9))
#define PULL_UP        (1 << 8)
#define PULL_DOWN      (1 << 9)

#define LOW         0
#define HIGH        1

#define INTERNAL    1
#define INTERNAL1V2 1

void pinMode(uint8_t pin, int mode);
void pinConfig(uint8_t pin, int config);
void digitalWrite(uint8_t pin, int value);
bool digitalRead(uint8_t pin);
void analogReference(int ref);
void analogWrite(uint8_t pin, uint16_t duty);
void analogReadResolution(int res);
void analogWriteResolution(int res);
uint16_t analogRead(uint8_t pin);
bool analogReadAsync(uint8_t pin, void (*callback)(uint16_t));
void tone(uint8_t pin, uint32_t frequency);
void noTone(uint8_t pin);
void __isr_systick();
void __isr_udad_systick();

// Define pin names
#define GPIO0   24
#define GPIO1   25
#define GPIO2   26
#define SDA_SWD 27
#define SCL_SWD 28
#define SCL0     9
#define SDA0     8
#define A0       6
#define A1       7
#define TX0      4
#define RX0      5

#endif