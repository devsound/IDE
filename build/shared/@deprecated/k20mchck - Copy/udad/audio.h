#ifndef __AUDIO_H__
#define __AUDIO_H__
#include <mchck.h>
#include "codec.h"

typedef enum {
  AUDIO_8KHZ  =  8,
  AUDIO_32KHZ = 32,
  AUDIO_48KHZ = 48,
} AUDIO_SR_e;

typedef enum {
  AUDIO_8BIT  =  8,
  AUDIO_16BIT = 16,
  AUDIO_32BIT = 32,
} AUDIO_BD_e;

typedef enum {
  MICROPHONE = 0,
  LINE_IN = 1,
} AUDIO_INPUT_e;

typedef enum {
  LINEIN_OFF = 0,
  LINEIN_0DB = 1,
} AUDIO_LINEIN_BYPASS_e;

typedef enum {
  MIC_OFF  = 0,
  MIC_6DB  = 4,
  MIC_9DB  = 5,
  MIC_12DB = 6,
  MIC_15DB = 7,
} AUDIO_MIC_BYPASS_e;

typedef enum {
  DEEMPHASIS_NONE   = 0,
  DEEMPHASIS_32KHZ  = 1,
  DEEMPHASIS_44K1HZ = 2,
  DEEMPHASIS_48KHZ  = 3,
} AUDIO_DEEMPHASIS_e;

void audio_begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes);

__attribute__((__always_inline__))
inline void audio_hpvolume(uint8_t volume, bool zerocross) {
  codec_lheadout((struct CODEC_HEADOUT_t) {
    .volume = volume,    // set volume
    .sync   = 1,         // sync to rheadout
    .zcen   = zerocross  // load on zero cross
  });
}

__attribute__((__always_inline__))
inline void audio_lineinvolume(uint8_t volume, bool mute) {
  codec_llinein((struct CODEC_LINEIN_t) {
    .volume = volume, // set volume
    .sync   = 1,      // sync to rlinein 
    .mute   = mute    // mute
  });
}

__attribute__((__always_inline__))
inline void audio_selectinput(uint8_t input) {
  // TODO: need memory
  codec_apath((struct CODEC_APATH_t) {
    .insel = (input == MICROPHONE), // ADC source
    .dacsel = 1 // 
  });
}

__attribute__((__always_inline__))
inline void audio_bypass(uint8_t linein, uint8_t mic) {
  // TODO: need memory
  codec_apath((struct CODEC_APATH_t) {
    .sidetone = !!(mic & 4), // sitetone (mic bypass)
    .sideatt = mic & 3,      // attenuation for sidetone
    .bypass = linein         // bypass (linein bypass)
  });
}

__attribute__((__always_inline__))
inline void audio_microphone(bool mute, bool boost) {
  // TODO: need memory
  codec_apath((struct CODEC_APATH_t) {
    .micmute = mute,  // mute mic
    .micboost = boost // boost mic
  });
}

__attribute__((__always_inline__))
inline void audio_filters(bool highpass, bool highpassmem, bool softmute, uint8_t deemphasis) {
  codec_dpath((struct CODEC_DPATH_t) {
    .adchpd = !highpass, // high pass diable
    .hpor = highpassmem, // memorize dc-offset on high pass disable
    .dacmu = softmute,   // enable soft mute
    .deemph = deemphasis  // de-emphasis center frequency
  });
}

// Arduino object emulation (for now)
static const struct {
  void (*begin)(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes);
  void (*hpvolume)(uint8_t volume, bool zerocross);
  void (*lineinvolume)(uint8_t volume, bool mute);
  void (*selectinput)(uint8_t input);
  void (*bypass)(uint8_t linein, uint8_t mic);
  void (*microphone)(bool mute, bool boost);
  void (*filters)(bool highpass, bool highpassmem, bool softmute, uint8_t deemphasis);
} Audio = {
  audio_begin,
  audio_hpvolume,
  audio_lineinvolume,
  audio_selectinput,
  audio_bypass,
  audio_microphone,
  audio_filters,
};

typedef int8_t  samp8bit;
typedef int16_t samp16bit;
typedef int32_t samp32bit;

#define SampleBuffer uint8_t DMAMEM

#endif