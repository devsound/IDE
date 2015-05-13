/*
* Arduino core for DevSound K20 devices
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#include <mchck.h>
#include "serial.h"
#include "systick.h"
#include "Arduino.h"
#include <math.h>

static uint32_t volatile tickcount = 0;
static volatile bool analog_read_en;
static volatile uint16_t analog_read_data;

#define X INVALID_PIN

const struct {
  volatile struct PORT_t *port;
  volatile struct GPIO_t *gpio;
  unsigned int pin;
  unsigned int std_mux;
  unsigned int ftm_mux;
  volatile struct FTM_t *ftm;
  unsigned int ftm_ch;
  unsigned int adc_mux;
  unsigned int adc_ch;
  unsigned int spi_mux;
  unsigned int spi_cs;
} pinmap[] = {
  { NULL },                                          //  0
  { NULL },                                          //  1 VDD
  { NULL },                                          //  2 VSS
  { NULL },                                          //  3 USB0_DP
  { NULL },                                          //  4 USB0_DM
  { NULL },                                          //  5 VOUT33
  { NULL },                                          //  6 VREGIN
  { NULL },                                          //  7 VDDA
  { NULL },                                          //  8 VSSA
  { NULL },                                          //  9 XTAL32
  { NULL },                                          // 10 EXTAL32
  { NULL },                                          // 11 VBAT

                                                     // ## NAME  WIRE         UART         FLEXTIMER   ANALOG
  
  { &PORTA, &GPIOA,  0, 2, 3, &FTM0,  5, 1,  X, X, X }, // 12 PTA0               2:UART0_CTS  3:FTM0_CH5
  { &PORTA, &GPIOA,  1, 2, 3, &FTM0,  6, 1,  X, X, X }, // 13 PTA1               2:UART0_RX   3:FTM0_CH6
  { &PORTA, &GPIOA,  2, 2, 3, &FTM0,  7, 1,  X, X, X }, // 14 PTA2               2:UART0_TX   3:FTM0_CH7
  { &PORTA, &GPIOA,  3, 2, 3, &FTM0,  0, 1,  X, X, X }, // 15 PTA3               2:UART0_RTS  3:FTM0_CH0
  { &PORTA, &GPIOA,  4, 3, 3, &FTM0,  1, 1,  X, X, X }, // 16 PTA4                            3:FTM0_CH1
  { &PORTA, &GPIOA, 18, 1, X,  NULL,  X, 1,  X, X, X }, // 17 PTA18
  { &PORTA, &GPIOA, 19, 1, X,  NULL,  X, 1,  X, X, X }, // 18 PTA19
  
  { NULL },                                          // 19 RESET

  { &PORTB, &GPIOB,  0, 2, 3, &FTM1,  0, 0,  8, X, X }, // 20 PTB0  2:I2C0_SCL                3:FTM1_CH0  0:ADC0_SE8
  { &PORTB, &GPIOB,  1, 2, 3, &FTM1,  1, 0,  9, X, X }, // 21 PTB1  2:I2C0_SDA                3:FTM1_CH1  0:ADC0_SE9

  { &PORTC, &GPIOC,  1, 3, 4, &FTM0,  0, 0, 15, 2, 3 }, // 22 PTC1  2:SPI0_PCS3  3:UART1_RTS  4:FTM0_CH0  0:ADC0_SE15
  { &PORTC, &GPIOC,  2, 3, 4, &FTM0,  1, 0,  4, 2, 2 }, // 23 PTC2  2:SPI0_PCS2  3:UART1_CTS  4:FTM0_CH1  0:ADC0_SE4B
  { &PORTC, &GPIOC,  3, 3, 4, &FTM0,  2, 1,  X, 2, 1 }, // 24 PTC3  2:SPI0_PCS1  3:UART1_RX   4:FTM0_CH2
  { &PORTC, &GPIOC,  4, 3, 4, &FTM0,  3, 1,  X, 2, 0 }, // 25 PTC4  2:SPI0_PCS0  3:UART1_TX   4:FTM0_CH3
  { &PORTC, &GPIOC,  5, 2, X,  NULL,  0, 1,  X, 2, X }, // 26 PTC5  2:SPI0_SCK
  { &PORTC, &GPIOC,  6, 2, X,  NULL,  0, 1,  X, 2, X }, // 27 PTC6  2:SPI0_SOUT
  { &PORTC, &GPIOC,  7, 2, X,  NULL,  0, 1,  X, 2, X }, // 28 PTC7  2:SPI0_SIN

  { &PORTD, &GPIOD,  4, 4, 4, &FTM0,  4, 1, -1, 2, 1 }, // 29 PTD4  2:SPI0_PCS1  3:UART0_RTS  4:FTM0_CH4
  { &PORTD, &GPIOD,  5, 4, 4, &FTM0,  5, 0,  6, 2, 2 }, // 30 PTD5  2:SPI0_PCS2  3:UART0_CTS  4:FTM0_CH5  0:ADC0_SE6b
  { &PORTD, &GPIOD,  6, 4, 4, &FTM0,  6, 0,  7, 2, 3 }, // 31 PTD6  2:SPI0_PCS3  3:UART0_RX   4:FTM0_CH6  0:ADC0_SE7b
  { &PORTD, &GPIOD,  7, 4, 4, &FTM0,  7, 1, -1, X, X }, // 32 PTD7               3:UART0_TX   4:FTM0_CH7
};

#undef X

// Fix static warnings
#undef BITBAND_BIT
#define BITBAND_BIT(var, bit) *((volatile uint32_t*)(0x42000000 + ((uintptr_t)&(var) - 0x40000000) * 32 + 4 * (bit)))

inline uint8_t chipSelectFromPin(uint8_t pin) {
  uint8_t cs = 0;
  if(pin >= 1 && pin <= 32) cs = pinmap[pin].spi_cs;
  if(cs == INVALID_PIN) cs = 0;
  return cs;
}

inline void pinMode(uint8_t pin, int mode) {
  unsigned mux = INVALID_PIN;
  switch(mode) {
    case INPUT:  mux = 1; // GPIO is always 1
      BITBAND_BIT(pinmap[pin].gpio->pddr, pinmap[pin].pin) = 0;
      break;
    case OUTPUT: mux = 1; // GPIO is always 1
      BITBAND_BIT(pinmap[pin].gpio->pddr, pinmap[pin].pin) = 1;
      break;
    case ADC:    mux = pinmap[pin].adc_mux;
      break;
    case PWM:    mux = pinmap[pin].ftm_mux;
      break;
    case DSPI:   mux = pinmap[pin].spi_mux;
      break;
    default:     mux = pinmap[pin].std_mux;
  }
  if(mux != INVALID_PIN) pinmap[pin].port->pcr[pinmap[pin].pin].mux = mux;
}

inline void pinConfig(uint8_t pin, int config) {
  if((config & 0x3) == DRIVE_STRONG) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 6) = 1;
  } else if((config & 0x3) == DRIVE_WEAK) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 6) = 0;
  }

  if((config & 0xC) == OPEN_DRAIN) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 5) = 1;
  } else if((config & 0xC) == PUSH_PULL) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 5) = 0;
  }

  if((config & 0x30) == FILTER_ENABLE) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 4) = 1;
  } else if((config & 0x30) == FILTER_DISABLE) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 4) = 0;
  }

  if((config & 0xC0) == SLEW_SLOW) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 2) = 1;
  } else if((config & 0xC0) == SLEW_FAST) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 2) = 0;
  }

  if((config & 0x300) == PULL_NONE) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 1) = 0;
  } else if((config & 0x300) == PULL_UP) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 1) = 1;
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 0) = 1;
  } else if((config & 0x300) == PULL_DOWN) {
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 1) = 1;
    BITBAND_BIT(pinmap[pin].port->pcr[pinmap[pin].pin], 0) = 0;
  }
}

ALWAYS_INLINE void digitalWrite(uint8_t pin, int value) {
  BITBAND_BIT(pinmap[pin].gpio->pdor, pinmap[pin].pin) = value;
}

ALWAYS_INLINE bool digitalRead(uint8_t pin) {
  return BITBAND_BIT(pinmap[pin].gpio->pdir, pinmap[pin].pin);
}

ALWAYS_INLINE void analogWrite(uint8_t pin, uint16_t duty) {
  pinmap[pin].ftm->channel[pinmap[pin].ftm_ch].cv = duty;
}

ALWAYS_INLINE void analogReference(int ref) {
  ADC0.sc2.refsel = (ref == INTERNAL1V2 ? ADC_REF_ALTERNATE : ADC_REF_DEFAULT);
}

static uint8_t analog_read_shift = 6;

ALWAYS_INLINE void analogReadResolution(int res) {
  analog_read_shift = 16 - res;
}

ALWAYS_INLINE void analogWriteResolution(int res) {
  FTM0.mod = FTM1.mod = (1 << res) - 1;
}

void analogReadAsync_done(uint16_t data, int error, void *cbdata) {
	((void(*)(uint16_t))cbdata)(data);
} 

ALWAYS_INLINE bool analogReadAsync(uint8_t pin, void (*callback)(uint16_t)) {
	return !adc_sample_start(pinmap[pin].adc_ch, analogReadAsync_done, (void*)callback);
}

void analogRead_done(uint16_t data, int error, void *cbdata) {
  ((uint16_t*)cbdata)[0] = 1;
  ((uint16_t*)cbdata)[1] = data;
}

ALWAYS_INLINE uint16_t analogRead(uint8_t pin) {
  uint16_t volatile ctx[2] = {0};
  
  while(adc_sample_start(pinmap[pin].adc_ch, analogRead_done, (void*)ctx));

  while(ctx[0] == 0);
  return ctx[1] >> analog_read_shift;
}

ALWAYS_INLINE void tone(uint8_t pin, uint32_t frequency) {
  uint32_t mod = ((F_CPU / frequency) & 0xFFFFFFFE);
  pinmap[pin].ftm->mod = mod - 1;
  pinmap[pin].ftm->channel[pinmap[pin].ftm_ch].cv = mod >> 1;
}

ALWAYS_INLINE void noTone(uint8_t pin) {
  pinmap[pin].ftm->channel[pinmap[pin].ftm_ch].cv = 0;
}

// Default systick functionality
void __isr_systick() {
  tickcount++;
}

// Allows overriding default ISR by sketch (without any overhead as calls are optimized away)
void isr_systick() __attribute__ ((weak, alias ("__isr_systick")));

// SysTick interrupt entry point
void SysTick_Handler(void) {
  isr_systick();
}

ALWAYS_INLINE void delay(uint32_t ms) {
  uint32_t start = tickcount;
  while(((uint32_t)(tickcount - start)) < ms);
}

ALWAYS_INLINE uint32_t millis() {
  return tickcount;
}

void clk1000ptd4() {
 	const int divisor = 1000;
	ftm_init();
	pin_mode(PIN_PTD4, PIN_MODE_MUX_ALT4);
	FTM0.cntin = 0;
	FTM0.mod = divisor - 1;
	ftm_set_raw(0xFF, divisor >> 1);
  volatile bool x = true;
  while(x);
} 

int main(void) {
  // Port initialization
  SIM.scgc5.porta = 1;
  SIM.scgc5.portb = 1;
  SIM.scgc5.portc = 1;
  SIM.scgc5.portd = 1;
  
  // FlexTimer initialization
  SIM.scgc6.ftm0 = 1; // Clock gating
  SIM.scgc6.ftm1 = 1;
	FTM0.mod = 0xff;    // Modulo (255 conforms to Arduino, but at 187.5kHz)
	FTM1.mod = 0xff;
	FTM0.cntin = 0;     // Counter reset
	FTM1.cntin = 0;
  
  for (int i = 0; i < 8; i++) {
    FTM0.channel[i].csc.msb  = 1; // Edge-aligned PWM
    FTM0.channel[i].csc.elsb = 1;
  }
  for (int i = 0; i < 2; i++) {
    FTM1.channel[i].csc.msb  = 1; // Edge-aligned PWM
    FTM1.channel[i].csc.elsb = 1;
  }
  FTM0.sc.clks = FTM_CLKS_SYSTEM; // Use system clock
  FTM1.sc.clks = FTM_CLKS_SYSTEM;

  // ADC initialization (and calibration)
  adc_init();
  
  // PORT initialization
  for(int n = 1; n <= 28; n++) {
   // if(pinmap[n].port) pinMode(n, DEFAULT);
  }

  // Sketch
  SysTick.begin(1000);
  setup();
  while(1) loop();
}

void exit(int status) {
  while(1);
}