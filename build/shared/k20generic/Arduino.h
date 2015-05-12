/*
* Arduino core for DevSound K20 devices
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __ARDUINO_H__
#define __ARDUINO_H__

// Compiler includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// K20 includes
#include "serial.h"
#include "quadrature.h"

#ifdef __cplusplus
extern "C" {
#endif

// Generic
#define ALWAYS_INLINE __attribute__((__always_inline__)) inline
#define DMAMEM __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)))

// Named aliases for pin numbers
#define PTA0  12
#define PTA1  13
#define PTA2  14
#define PTA3  15
#define PTA4  16
#define PTA18 17
#define PTA19 18
#define PTB0  20
#define PTB1  21
#define PTC1  22
#define PTC2  23
#define PTC3  24
#define PTC4  25
#define PTC5  26
#define PTC6  27
#define PTC7  28
#define PTD4  29
#define PTD5  30
#define PTD6  31
#define PTD7  32

// Invalid MUX number for PORT peripheral
// Also used for invalid chip select number for DSPI peripheral
#define INVALID_PIN      9

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
#define DSPI             5

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
uint8_t chipSelectFromPin(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif//__ARDUINO_H__