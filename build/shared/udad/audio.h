/*
* Audio driver (I2S and codec wrapper) for DevSound µDAD
*
* 2014-02-07 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __AUDIO_H__
#define __AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t  samp8bit;
typedef int16_t samp16bit;
typedef int32_t samp32bit;

#define SampleBuffer uint8_t DMAMEM

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

void audio_hpvolume(uint8_t volume, bool zerocross);
void audio_lhpvolume(uint8_t volume, bool zerocross);
void audio_rhpvolume(uint8_t volume, bool zerocross);
void audio_lineinvolume(uint8_t volume, bool mute);
void audio_llineinvolume(uint8_t volume, bool mute);
void audio_rlineinvolume(uint8_t volume, bool mute);
void audio_selectinput(uint8_t input);
void audio_bypass(uint8_t linein, uint8_t mic);
void audio_microphone(bool mute, bool boost);
void audio_filters(bool highpass, bool highpassmem, bool softmute, uint8_t deemphasis);
void audio_dcfilter(bool active, bool remember);

#ifdef __cplusplus
}

static const class {
  public:
    static inline void begin() { audio_begin(AUDIO_32KHZ, AUDIO_32BIT, true, NULL, 0); }
    static inline void begin(AUDIO_SR_e samplerate) { audio_begin(samplerate, AUDIO_32BIT, true, NULL, 0); }
    static inline void begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth) { audio_begin(samplerate, bitdepth, true, NULL, 0); }
    static inline void begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv) { audio_begin(samplerate, bitdepth, enable_recv, NULL, 0); }
    static inline void begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes) { audio_begin(samplerate, bitdepth, enable_recv, dma_buffers, dma_bytes); }
    static inline void hpVolume(uint8_t volume) { audio_hpvolume(volume, false); }
    static inline void hpVolume(uint8_t volume, bool zerocross) { audio_hpvolume(volume, zerocross); }
    static inline void lHpVolume(uint8_t volume, bool zerocross) { audio_lhpvolume(volume, zerocross); }
    static inline void rHpVolume(uint8_t volume, bool zerocross) { audio_rhpvolume(volume, zerocross); }
    static inline void lineInVolume(uint8_t volume) { audio_lineinvolume(volume, false); }
    static inline void lineInVolume(uint8_t volume, bool mute) { audio_lineinvolume(volume, mute); }
    static inline void lLineInVolume(uint8_t volume, bool mute) { audio_llineinvolume(volume, mute); }
    static inline void rLineInVolume(uint8_t volume, bool mute) { audio_rlineinvolume(volume, mute); }
    static inline void selectInput(uint8_t input) { audio_selectinput(input); }
    static inline void bypass(uint8_t linein, uint8_t mic) { audio_bypass(linein, mic); }
    static inline void microphone(bool mute, bool boost) { audio_microphone(mute, boost); }
    static inline void filters(bool highpass, bool highpassmem, bool softmute, uint8_t deemphasis) { audio_filters(highpass, highpassmem, softmute, deemphasis); }
    static inline void dcFilter(bool active) { audio_dcfilter(active, false); }
    static inline void dcFilter(bool active, bool remember) { audio_dcfilter(active, remember); }
} Audio;

#endif

#endif
