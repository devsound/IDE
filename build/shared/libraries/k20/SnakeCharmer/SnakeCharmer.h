/*
* Arduino device library for SnakeCharmer
*
* 2015-01-23 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*/

#ifndef __UDAD_SNAKECHARMER__
#define __UDAD_SNAKECHARMER__
#include <udad.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RED_LED    GPIO2
#define YELLOW_LED GPIO1
#define GREEN_LED  GPIO0

void snch_begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes);
void snch_volume(uint8_t volume);
void snch_refresh(void);
void snch_calibrate_dc(bool active);

extern struct snakecharmer_t {
  void (*begin)(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes);
  void (*volume)(uint8_t volume);
  void (*refresh)(void);
  void (*calibrate)(bool active);
  uint16_t digital;
  uint16_t analog[12];
} SnakeCharmer;


#ifdef __cplusplus
}
#endif

#endif