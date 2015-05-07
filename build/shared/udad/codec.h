/*
* MCHCK driver for Wolfson WM8731 codec control interface
*
* NOTE: This library only controls the codec operation.
*
* 2014-01-07 @stg
*/

#ifndef __CODEC_H__
#define __CODEC_H__

// Codec sample rates
typedef enum {
    CODEC_SR_HZ8K  = 3,
    CODEC_SR_HZ32K = 6,
    CODEC_SR_HZ48K = 0,
    CODEC_SR_HZ96K = 7, // TODO: test
    // Asynchronous DAC/ADC modes are not supported
} CODEC_SR_e;

#define CODEC_SR_FROM_KHZ(KHZ) (KHZ == 8 ? CODEC_SR_HZ8K : (KHZ == 32 ? CODEC_SR_HZ32K : (KHZ == 48 ? CODEC_SR_HZ48K : CODEC_SR_HZ96K)))

// Codec addresses, as defined by hardware pin CSB
typedef enum {
  CODEC_CSB_LOW  = 0x1a,
  CODEC_CSB_HIGH = 0x1b,
} CODEC_CSB_e;

enum CODEC_SIDEATT_e {
  CODEC_SIDEATT_6DB = 0,
  CODEC_SIDEATT_9DB = 1,
  CODEC_SIDEATT_12DB = 2,
  CODEC_SIDEATT_15DB = 3,
};

enum CODEC_DEEMPH_e {
  CODEC_DEEMPH_DISABLE = 0,
  CODEC_DEEMPH_HZ32K = 1,
  CODEC_DEEMPH_HZ44K1 = 2,
  CODEC_DEEMPH_HZ48 = 3,
};

enum CODEC_FORMAT_e {
  CODEC_FORMAT_RIGHTJUSTIFIED = 0,
  CODEC_FORMAT_LEFTJUSTIFIED = 1,
  CODEC_FORMAT_I2S = 2,
  CODEC_FORMAT_DSP = 3,
};

enum CODEC_IWL_e {
  CODEC_IWL_16BIT = 0,
  CODEC_IWL_20BIT = 1,
  CODEC_IWL_24BIT = 2,
  CODEC_IWL_32BIT = 3,
};

enum CODEC_MODE_e {
  CODEC_MODE_NORMAL = 0,
  CODEC_MODE_USB = 1,
};

enum CODEC_BOSR_e {
  CODEC_BOSR_USB_250FS = 0,
  CODEC_BOSR_USB_272FS = 1,
  CODEC_BOSR_NORMAL_256FS = 0,
  CODEC_BOSR_NORMAL_384FS = 1,
};

// Register values
struct CODEC_LINEIN_t {
  UNION_STRUCT_START(16);
  uint16_t volume : 5;
  uint16_t _zero0 : 2;
  uint16_t mute   : 1;
  uint16_t sync   : 1;
  UNION_STRUCT_END;
};
struct CODEC_HEADOUT_t {
  UNION_STRUCT_START(16);
  uint16_t volume : 7;
  uint16_t zcen   : 1;
  uint16_t sync   : 1;
  UNION_STRUCT_END;
};
struct CODEC_APATH_t {
  UNION_STRUCT_START(16);
  uint16_t micboost : 1;
  uint16_t micmute  : 1;
  uint16_t insel    : 1;
  uint16_t bypass   : 1;
  uint16_t dacsel   : 1;
  uint16_t sidetone : 1;
  enum CODEC_SIDEATT_e
           sideatt  : 2;
  uint16_t _zero0   : 1;
  UNION_STRUCT_END;
};
struct CODEC_DPATH_t {
  UNION_STRUCT_START(16);
  uint16_t adchpd  : 1;
  enum CODEC_DEEMPH_e 
           deemph  : 2;
  uint16_t dacmu   : 1;
  uint16_t hpor    : 1;
  uint16_t _zero0  : 4;
  UNION_STRUCT_END;
};
struct CODEC_POWERDOWN_t {
  UNION_STRUCT_START(16);
  uint16_t linein  : 1;
  uint16_t mic     : 1;
  uint16_t adc     : 1;
  uint16_t dac     : 1;
  uint16_t out     : 1;
  uint16_t osc     : 1;
  uint16_t clkout  : 1;
  uint16_t power   : 1;
  uint16_t _zero0  : 1;
  UNION_STRUCT_END;
};
struct CODEC_INTERFACE_t {
  UNION_STRUCT_START(16);
  enum CODEC_FORMAT_e 
           format  : 2;
  enum CODEC_IWL_e
           iwl     : 2;
  uint16_t lrp     : 1;
  uint16_t lrswap  : 1;
  uint16_t ms      : 1;
  uint16_t bclkinv : 1;
  uint16_t _zero0  : 1;
  UNION_STRUCT_END;
};
struct CODEC_SAMPLING_t {
  UNION_STRUCT_START(16);
  enum CODEC_MODE_e
           mode     : 1;
  enum CODEC_BOSR_e
           bosr    : 1;
  CODEC_SR_e
           sr      : 4;
  uint16_t clkidiv2: 1;
  uint16_t clkodiv2: 1;
  uint16_t _zero0  : 1;
  UNION_STRUCT_END;
};
struct CODEC_CONTROL_t {
  UNION_STRUCT_START(16);
  uint16_t active  : 1;
  uint16_t _zero0  : 8;
  UNION_STRUCT_END;
};

// Function prototypes
void codec_begin(CODEC_CSB_e address);
void codec_q(uint8_t reg, uint16_t value, uint16_t mask);
uint16_t codec_r(uint8_t reg);
void codec_flush();

// Register addresses
#define CODEC_LLINEIN   0x00
#define CODEC_RLINEIN   0x01
#define CODEC_LHEADOUT  0x02
#define CODEC_RHEADOUT  0x03
#define CODEC_APATH     0x04
#define CODEC_DPATH     0x05
#define CODEC_POWERDOWN 0x06
#define CODEC_INTERFACE 0x07
#define CODEC_SAMPLING  0x08
#define CODEC_CONTROL   0x09
#define CODEC_RESET     0x0F

// Register access
__attribute__((__always_inline__))
inline void codec_reset    () { codec_q(CODEC_RESET, 0, ~0); }

__attribute__((__always_inline__))
inline void codec_llinein  (struct CODEC_LINEIN_t    value, struct CODEC_LINEIN_t    mask) { codec_q(CODEC_LLINEIN,   value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_rlinein  (struct CODEC_LINEIN_t    value, struct CODEC_LINEIN_t    mask) { codec_q(CODEC_RLINEIN,   value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_lheadout (struct CODEC_HEADOUT_t   value, struct CODEC_HEADOUT_t   mask) { codec_q(CODEC_LHEADOUT,  value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_rheadout (struct CODEC_HEADOUT_t   value, struct CODEC_HEADOUT_t   mask) { codec_q(CODEC_RHEADOUT,  value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_apath    (struct CODEC_APATH_t     value, struct CODEC_APATH_t     mask) { codec_q(CODEC_APATH,     value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_dpath    (struct CODEC_DPATH_t     value, struct CODEC_DPATH_t     mask) { codec_q(CODEC_DPATH,     value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_powerdown(struct CODEC_POWERDOWN_t value, struct CODEC_POWERDOWN_t mask) { codec_q(CODEC_POWERDOWN, value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_interface(struct CODEC_INTERFACE_t value, struct CODEC_INTERFACE_t mask) { codec_q(CODEC_INTERFACE, value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_sampling (struct CODEC_SAMPLING_t  value, struct CODEC_SAMPLING_t  mask) { codec_q(CODEC_SAMPLING,  value.raw, mask.raw); }
__attribute__((__always_inline__))
inline void codec_control  (struct CODEC_CONTROL_t   value, struct CODEC_CONTROL_t   mask) { codec_q(CODEC_CONTROL,   value.raw, mask.raw); }

__attribute__((__always_inline__))
inline void codec_llinein_set  (struct CODEC_LINEIN_t    value) { codec_q(CODEC_LLINEIN,   value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_rlinein_set  (struct CODEC_LINEIN_t    value) { codec_q(CODEC_RLINEIN,   value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_lheadout_set (struct CODEC_HEADOUT_t   value) { codec_q(CODEC_LHEADOUT,  value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_rheadout_set (struct CODEC_HEADOUT_t   value) { codec_q(CODEC_RHEADOUT,  value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_apath_set    (struct CODEC_APATH_t     value) { codec_q(CODEC_APATH,     value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_dpath_set    (struct CODEC_DPATH_t     value) { codec_q(CODEC_DPATH,     value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_powerdown_set(struct CODEC_POWERDOWN_t value) { codec_q(CODEC_POWERDOWN, value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_interface_set(struct CODEC_INTERFACE_t value) { codec_q(CODEC_INTERFACE, value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_sampling_set (struct CODEC_SAMPLING_t  value) { codec_q(CODEC_SAMPLING,  value.raw, ~0); }
__attribute__((__always_inline__))
inline void codec_control_set  (struct CODEC_CONTROL_t   value) { codec_q(CODEC_CONTROL,   value.raw, ~0); }

__attribute__((__always_inline__))
inline struct CODEC_LINEIN_t    codec_llinein_get  () { return (struct CODEC_LINEIN_t   ){.raw = codec_r(CODEC_LLINEIN  )}; }
__attribute__((__always_inline__))
inline struct CODEC_LINEIN_t    codec_rlinein_get  () { return (struct CODEC_LINEIN_t   ){.raw = codec_r(CODEC_RLINEIN  )}; }
__attribute__((__always_inline__))
inline struct CODEC_HEADOUT_t   codec_lheadout_get () { return (struct CODEC_HEADOUT_t  ){.raw = codec_r(CODEC_LHEADOUT )}; }
__attribute__((__always_inline__))
inline struct CODEC_HEADOUT_t   codec_rheadout_get () { return (struct CODEC_HEADOUT_t  ){.raw = codec_r(CODEC_RHEADOUT )}; }
__attribute__((__always_inline__))
inline struct CODEC_APATH_t     codec_apath_get    () { return (struct CODEC_APATH_t    ){.raw = codec_r(CODEC_APATH    )}; }
__attribute__((__always_inline__))
inline struct CODEC_DPATH_t     codec_dpath_get    () { return (struct CODEC_DPATH_t    ){.raw = codec_r(CODEC_DPATH    )}; }
__attribute__((__always_inline__))
inline struct CODEC_POWERDOWN_t codec_powerdown_get() { return (struct CODEC_POWERDOWN_t){.raw = codec_r(CODEC_POWERDOWN)}; }
__attribute__((__always_inline__))
inline struct CODEC_INTERFACE_t codec_interface_get() { return (struct CODEC_INTERFACE_t){.raw = codec_r(CODEC_INTERFACE)}; }
__attribute__((__always_inline__))
inline struct CODEC_SAMPLING_t  codec_sampling_get () { return (struct CODEC_SAMPLING_t ){.raw = codec_r(CODEC_SAMPLING )}; }
__attribute__((__always_inline__))
inline struct CODEC_CONTROL_t   codec_control_get  () { return (struct CODEC_CONTROL_t  ){.raw = codec_r(CODEC_CONTROL  )}; }

#endif
