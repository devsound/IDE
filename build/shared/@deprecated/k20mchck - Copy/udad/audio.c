#include "audio.h"
#include "i2s.h"

bool initialized = false;

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
    codec_sampling((struct CODEC_SAMPLING_t) {
      .sr = CODEC_SR_FROM_KHZ(samplerate)
    });
    codec_flush();
    i2s_start();
  } else {
    // Initialize codec
    codec_begin(CODEC_CSB_LOW);
    // Configure sampling
    codec_sampling((struct CODEC_SAMPLING_t) {
      .sr = CODEC_SR_FROM_KHZ(samplerate)
    });
    codec_lheadout((struct CODEC_HEADOUT_t) {
      .volume = 100, // set volume
      .sync   = 1,   // sync to rheadout
      .zcen   = 1    // load on zero cross
    });
    codec_control((struct CODEC_CONTROL_t) {
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