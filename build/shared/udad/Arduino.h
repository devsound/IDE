/*
* Arduino core for DevSound µDAD
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __UDAD_H__
#define __UDAD_H__

// Targets
#define UDAD 1

// Compiler includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// uDAD includes
#include "audio.h"
#include "serial.h"
#include "quadrature.h"

#ifdef __cplusplus
extern "C" {
#endif

// Generic
#define ALWAYS_INLINE __attribute__((__always_inline__)) inline
#define DMAMEM __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)))

// Named aliases for pin numbers
#define GPIO0           24
#define GPIO1           25
#define GPIO2           26
#define SDA_SWD         27
#define SCL_SWD         28
#define SCL0             9
#define SDA0             8
#define A0               6
#define A1               7
#define TX0              4
#define RX0              5

// Pin mode
#define DEFAULT          0 // pin mode/adc reference
#define INPUT            1
#define OUTPUT           2
#define ANALOG_IN        3
#define ANALOG_INPUT     3
#define ADC              3
#define ANALOG_OUT       4
#define ANALOG_OUTPUT    4
#define PWM              4
#define PWM_OUT          4
#define PWM_OUTPUT       4

// Digital values
#define LOW              0
#define HIGH             1

// Analog reference
#define INTERNAL         1
#define INTERNAL1V2      1

// Pin configuration bits 0-1 (drive strength)
#define DRIVE_WEAK       (1 << 0)
#define DRIVE_STRONG     (1 << 1)

// Pin configuration bits 2-3 (output type)
#define OPEN_DRAIN       (1 << 2)
#define PUSH_PULL        (1 << 3)

// Pin configuration bits 4-5 (input filtering)
#define FILTER_ENABLE    (1 << 4)
#define FILTER_DISABLE   (1 << 5)

// Pin configuration bits 5-6 (slew rate)
#define SLEW_FAST        (1 << 6)
#define SLEW_SLOW        (1 << 7)

// Pin configuration bits 7-8 (pull up/down)
#define PULL_NONE        ((1 << 8) | (1 << 9))
#define PULL_UP          (1 << 8)
#define PULL_DOWN        (1 << 9)

// Arduino type names
typedef uint8_t  byte;
typedef uint16_t word;
typedef bool     boolean;

// Arduino (AVR) style global interrupts enable/disable
ALWAYS_INLINE static void cli() { asm volatile("CPSID i"); }
ALWAYS_INLINE static void sei() { asm volatile("CPSIE i"); }

// Sketch prototypes
void setup();
void loop();

// Arduino core function prototypes
uint32_t millis();
void delay(uint32_t ms);
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
void exit(int status);

#ifdef __cplusplus
}
#endif

#endif