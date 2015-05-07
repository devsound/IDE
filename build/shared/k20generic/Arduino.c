#include <mchck.h>
#include "serial.h"
#include "systick.h"
#include "Arduino.h"
#include <math.h>

static uint32_t volatile tickcount = 0;
static volatile bool analog_read_en;
static volatile uint16_t analog_read_data;

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
} pinmap[] = {
  { NULL },                                      //  0
  { NULL },                                      //  1 VBUS
  { NULL },                                      //  2 DM
  { NULL },                                      //  3 DP
  { &PORTD, &GPIOD, 7, 2, 4, &FTM0, 7,  0, 31 }, //  4 TX    PTD7 3:TX       4:FTM0_CH7
  { &PORTA, &GPIOA, 1, 2, 3, &FTM0, 6,  0, 31 }, //  5 RX    PTA1 2:RX       3:FTM0_CH6
  { &PORTD, &GPIOD, 6, 0, 4, &FTM0, 6,  0,  7 }, //  6 AIN0  PTD6 0:AIN0     4:FTM0_CH6  0:ADC0_SE7b
  { &PORTD, &GPIOD, 5, 0, 4, &FTM0, 5,  0,  6 }, //  7 AIN1  PTD5 0:AIN1     4:FTM0_CH5  0:ADC0_SE6b
  { &PORTB, &GPIOB, 1, 2, 3, &FTM1, 1,  0,  9 }, //  8 SDA   PTB1 2:I2C_SDA  3:FRM1_CH1  0:ADC0_SE9
  { &PORTB, &GPIOB, 0, 2, 3, &FTM1, 0,  0,  8 }, //  9 SCL   PTB0 2:I2C_SCL  3:FTM1_CH0  0:ADC0_SE8
  { NULL },                                      // 10 DVDD
  { NULL },                                      // 11 RHP
  { NULL },                                      // 12 LHP
  { NULL },                                      // 13 LIN
  { NULL },                                      // 14 RIN
  { NULL },                                      // 15 GND
  { NULL },                                      // 16 MIC
  { NULL },                                      // 17 BIAS
  { NULL },                                      // 18 VMID
  { NULL },                                      // 19 AVDD
  { NULL },                                      // 20 ROUT
  { NULL },                                      // 21 LOUT
  { NULL },                                      // 22 HPVDD
  { NULL },                                      // 23 RESET
#if UDAD_REV >= 4
  { &PORTA, &GPIOA, 2, 1, 3, &FTM0, 7, 31 },     // 24 GPIO0 PTA2            4:FTM0_CH7
#else
  { &PORTC, &GPIOC, 4, 1, 4, &FTM0, 3, 31 },     // 24 GPIO0 PTC4            4:FTM0_CH3
#endif
  { &PORTA, &GPIOA, 4, 1, 3, &FTM0, 1, 31 },     // 25 GPIO1 PTA4            3:FTM0_CH1
  { &PORTD, &GPIOD, 4, 1, 4, &FTM0, 4, 31 },     // 26 GPIO2 PTD4            4:FTM0_CH4
  { &PORTA, &GPIOA, 3, 7, 3, &FTM0, 0, 31 },     // 27 SWDC  PTA3 7:SWD_DIO  3:FTM0_CH0
  { &PORTA, &GPIOA, 0, 7, 3, &FTM0, 6, 31 },     // 28 SWDD  PTA0 7:SWD_CLK  3:FTM0_CH5
};

// Fix static warnings
#undef BITBAND_BIT
#define BITBAND_BIT(var, bit) *((volatile uint32_t*)(0x42000000 + ((uintptr_t)&(var) - 0x40000000) * 32 + 4 * (bit)))

inline void pinMode(uint8_t pin, int mode) {
  switch(mode) {
    case INPUT:
      BITBAND_BIT(pinmap[pin].gpio->pddr, pinmap[pin].pin) = 0;
      pinmap[pin].port->pcr[pinmap[pin].pin].mux = 1; // raw = ((struct PCR_t) { .mux = 1 }).raw;
      break;
    case OUTPUT:
      BITBAND_BIT(pinmap[pin].gpio->pddr, pinmap[pin].pin) = 1;
      pinmap[pin].port->pcr[pinmap[pin].pin].mux = 1; // raw = ((struct PCR_t) { .mux = 1 }).raw;
      break;
    case ADC:
      pinmap[pin].port->pcr[pinmap[pin].pin].mux = pinmap[pin].adc_mux; // raw = ((struct PCR_t) { .mux = pinmap[pin].adc_mux }).raw;
      break;
    case PWM:
      pinmap[pin].port->pcr[pinmap[pin].pin].mux = pinmap[pin].ftm_mux; // raw = ((struct PCR_t) { .mux = pinmap[pin].ftm_mux }).raw;
      break;
    default:
      pinmap[pin].port->pcr[pinmap[pin].pin].mux = pinmap[pin].std_mux; // raw = ((struct PCR_t) { .mux = pinmap[pin].std_mux }).raw;
  }
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

void __isr_udad_systick() {
  __isr_systick();
}

// Allows overriding default ISR by sketch (without any overhead as calls are optimized away)
void isr_systick() __attribute__ ((weak, alias ("__isr_udad_systick")));

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