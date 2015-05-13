/*
* I2S driver for DevSound K20 devices
*
* 2014-01-10 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*
*/

#ifndef __I2S_H__
#define __I2S_H__
#include <mchck.h>

#define I2S_IO_BIT_DEPTH     32       // Number of bits per sample in the physical data (8, 16 or 32)
#define I2S_FRAME_SIZE        2       // Number of frames, 2=stereo

typedef enum {
  I2S_SR_8KHZ  =  8,
  I2S_SR_32KHZ = 32,
  I2S_SR_48KHZ = 48,
} I2S_SR_e;

typedef enum {
  I2S_BD_8BIT  =  8,
  I2S_BD_16BIT = 16,
  I2S_BD_32BIT = 32,
} I2S_BD_e;

// Assert
CTASSERT(I2S_FRAME_SIZE == 2); // Only support stereo at this point
CTASSERT(I2S_IO_BIT_DEPTH == 8 || I2S_IO_BIT_DEPTH == 16 || I2S_IO_BIT_DEPTH == 32);

#define ROUNDROBIN                    // DMA round-robin scheduling

void i2s_configure(I2S_SR_e sample_rate, I2S_BD_e bit_depth, bool enable_recv, void *dma_buffers, size_t dma_bytes);
void i2s_start();

#endif