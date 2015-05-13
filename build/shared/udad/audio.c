#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <mchck.h>
#include "audio.h"
#include "codec.h"
#include "i2s.h"

static bool initialized = false;

// 1024 bytes buffers
//
// DMA uses page flipping with two pages, so 512 bytes per page
// We have two channels (stereo), so 256 bytes per channel
// Sample size is configurable:
// 8BIT:  1 byte  per sample, so 256 samples
// 16BIT: 2 bytes per sample, so 128 samples 
// 32BIT: 4 bytes per sample, so  64 samples
// If receiver is enabled, divide by two
//
// For 32bit audio, receiver enabled this means:
// 1024/2/2/4/2=32 samples per call to handler
// Also, 32 sample minimum latency

void audio_begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes) {
  if(initialized) {
    // Re-initialization sequence
    i2s_configure(samplerate, bitdepth, enable_recv, dma_buffers, dma_bytes);
    codec_sampling_set((struct CODEC_SAMPLING_t) {
      .sr = CODEC_SR_FROM_KHZ(samplerate)
    });
    codec_flush();
    i2s_start();
  } else {
    // Initialize codec
    codec_begin(CODEC_CSB_LOW);
    // Configure sampling
    codec_sampling_set((struct CODEC_SAMPLING_t) {
      .sr = CODEC_SR_FROM_KHZ(samplerate)
    });
    codec_lheadout_set((struct CODEC_HEADOUT_t) {
      .volume = 100, // set volume
      .sync   = 1,   // sync to rheadout
      .zcen   = 1    // load on zero cross
    });
    codec_control_set((struct CODEC_CONTROL_t) {
      .active = 1    // activate codec
    });
    // Wait for registers to update
    codec_flush();
    // Initialize I2S
    //i2s_configure(samplerate, bitdepth, true, NULL, 0);//, audio_bufs, sizeof(audio_bufs));
    i2s_configure(samplerate, bitdepth, enable_recv, dma_buffers, dma_bytes);
    i2s_start();
    // All done
    initialized = true;
  }
}

__attribute__((__always_inline__))
inline void audio_hpvolume(uint8_t volume, bool zerocross) {
  codec_lheadout_set((struct CODEC_HEADOUT_t) {
    .volume = volume,    // set volume
    .sync   = 1,         // sync to rheadout
    .zcen   = zerocross  // load on zero cross
  });
}

__attribute__((__always_inline__))
inline void audio_lhpvolume(uint8_t volume, bool zerocross) {
  codec_lheadout_set((struct CODEC_HEADOUT_t) {
    .volume = volume,    // set volume
    .sync   = 0,         // no sync
    .zcen   = zerocross  // load on zero cross
  });
}

__attribute__((__always_inline__))
inline void audio_rhpvolume(uint8_t volume, bool zerocross) {
  codec_rheadout_set((struct CODEC_HEADOUT_t) {
    .volume = volume,    // set volume
    .sync   = 0,         // no sync
    .zcen   = zerocross  // load on zero cross
  });
}

__attribute__((__always_inline__))
inline void audio_lineinvolume(uint8_t volume, bool mute) {
  codec_llinein_set((struct CODEC_LINEIN_t) {
    .volume = volume, // set volume
    .sync   = 1,      // sync to rlinein 
    .mute   = mute    // mute
  });
}

__attribute__((__always_inline__))
inline void audio_llineinvolume(uint8_t volume, bool mute) {
  codec_llinein_set((struct CODEC_LINEIN_t) {
    .volume = volume, // set volume
    .sync   = 0,      // no sync
    .mute   = mute    // mute
  });
}

__attribute__((__always_inline__))
inline void audio_rlineinvolume(uint8_t volume, bool mute) {
  codec_rlinein_set((struct CODEC_LINEIN_t) {
    .volume = volume, // set volume
    .sync   = 0,      // no sync
    .mute   = mute    // mute
  });
}

__attribute__((__always_inline__))
inline void audio_selectinput(uint8_t input) {
  codec_apath((struct CODEC_APATH_t) {
    .insel = (input == MICROPHONE), // ADC source
    .dacsel = 1 // 
  }, (struct CODEC_APATH_t) {
    .insel = ~0,
    .dacsel = ~0
  });
}

__attribute__((__always_inline__))
inline void audio_bypass(uint8_t linein, uint8_t mic) {
  // TODO: need memory
  codec_apath((struct CODEC_APATH_t) {
    .sidetone = !!(mic & 4), // sitetone (mic bypass)
    .sideatt = mic & 3,      // attenuation for sidetone
    .bypass = linein         // bypass (linein bypass)
  }, (struct CODEC_APATH_t) {
    .sidetone = ~0,
    .sideatt = ~0,
    .bypass = ~0
  });
}

__attribute__((__always_inline__))
inline void audio_microphone(bool mute, bool boost) {
  codec_apath((struct CODEC_APATH_t) {
    .micmute = mute,  // mute mic
    .micboost = boost // boost mic
  }, (struct CODEC_APATH_t) {
    .micmute = ~0,
    .micboost = ~0
  });
}

__attribute__((__always_inline__))
inline void audio_filters(bool dc, bool highpassmem, bool softmute, uint8_t deemphasis) {
  codec_dpath_set((struct CODEC_DPATH_t) {
    .adchpd = !dc,       // high pass disable
    .hpor = highpassmem, // memorize dc-offset on high pass disable
    .dacmu = softmute,   // enable soft mute
    .deemph = deemphasis // de-emphasis center frequency
  });
}

__attribute__((__always_inline__))
inline void audio_dcfilter(bool active, bool remember) {
  codec_dpath((struct CODEC_DPATH_t) {
    .adchpd = !active, // high pass disable
    .hpor = remember   // memorize dc-offset on high pass disable
  }, (struct CODEC_DPATH_t) {
    .adchpd = ~0,
    .hpor = ~0
  });
}