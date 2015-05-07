/*
* I2S driver for MCHCK
*
* 2014-01-10 @stg, (cc) https://creativecommons.org/licenses/by/3.0/
*
*/

#include <mchck.h>
#include "i2s.h"

// Frame size (as it goes into registers, -1)
#define FRSZ (I2S_FRAME_SIZE   - 1)
// Sync width (as it goes into registers, -1)
#define SYWD (I2S_IO_BIT_DEPTH - 1)
// First bit (as it goes into registers, -1)
#define BIWD (bits_per_sample - 1)
// Bit clock divisor (io depth => mclk divisor)
#define BDIV ((6144 / (I2S_FRAME_SIZE * I2S_IO_BIT_DEPTH * sample_rate_khz)) - 1) // MCLK to BCLK division factor = (DIV + 1) * 2
// DMA size (buffer depth => enum)
#define DMAS (bits_per_sample == 8 ? DMA_SIZE_8BIT : (bits_per_sample == 16 ? DMA_SIZE_16BIT : DMA_SIZE_32BIT))
// DMA transfer length (buffer depth => bytes)
#define DMAL (bits_per_sample >> 3);
// DMA iterations
#define ITER (buffer_size / (bits_per_sample >> 3))
// Sample count per DMA transfer
#define SAMPLE_COUNT (buffer_size / (bits_per_sample >> 2))

// Configuration
static uint8_t sample_rate_khz;
static uint8_t bits_per_sample;
static unsigned int sample_count;
static bool initialized = false;
static size_t buffer_size = 1;
static bool rx_enable = true;

// DMA buffers
static uint8_t *rx_buf_a, *rx_buf_b;
static uint8_t *tx_buf_a, *tx_buf_b;
static volatile bool buf_flip;

// Weak dummy handler
void __audio(void *recv, void *send, unsigned int count) {}
void audio() __attribute__ ((weak, alias ("__audio")));

// Service routines
void DMA0_Handler(void) {
  if(buf_flip) {
    buf_flip = false;
    DMA.tcd[0].saddr = tx_buf_b;
    if(rx_enable) DMA.tcd[1].daddr = rx_buf_b;
    audio(rx_buf_a, tx_buf_a, sample_count);
  } else {
    buf_flip = true;
    DMA.tcd[0].saddr = tx_buf_a;
    if(rx_enable) DMA.tcd[1].daddr = rx_buf_a;
    audio(rx_buf_b, tx_buf_b, sample_count);
  }

  // Clear interrupts
  DMA.cint.cint = 0;
  DMA.cint.cint = 1; // TODO: needed? else only if rx enable
}

void I2S0_TX_Handler() {
  uint32_t recv[2], send[2];
  void *recv_buf = recv, *send_buf = send;

  if(rx_enable) {
    // Copy the data from FIFO into our buffer
    if(bits_per_sample == 8) {
      (((uint8_t *)recv_buf)[0]) = (uint8_t )I2S0.rdr;
      (((uint8_t *)recv_buf)[1]) = (uint8_t )I2S0.rdr;
    } else if(bits_per_sample == 16) {
      (((uint16_t*)recv_buf)[0]) = (uint16_t)I2S0.rdr;
      (((uint16_t*)recv_buf)[1]) = (uint16_t)I2S0.rdr;
    } else {
      (((uint32_t*)recv_buf)[0]) = (uint32_t)I2S0.rdr;
      (((uint32_t*)recv_buf)[1]) = (uint32_t)I2S0.rdr;
    }
  }
  
  audio(recv_buf, send_buf, 1);

  // Copy the data from our buffer into FIFO
  if(bits_per_sample == 8) {
    I2S0.tdr = (uint32_t)(((uint8_t *)send_buf)[0]);
    I2S0.tdr = (uint32_t)(((uint8_t *)send_buf)[1]);
  } else if(bits_per_sample == 16) {
    I2S0.tdr = (uint32_t)(((uint16_t*)send_buf)[0]);
    I2S0.tdr = (uint32_t)(((uint16_t*)send_buf)[1]);
  } else {
    I2S0.tdr = (uint32_t)(((uint32_t*)send_buf)[0]);
    I2S0.tdr = (uint32_t)(((uint32_t*)send_buf)[1]);
  }

  I2S0.tcsr.raw |= ((struct I2S_TCSR_t) {
    .fef = 1, // clear if underrun
    .sef = 1  // clear if frame sync error
  }).raw;
  // TODO: needed? else only if rx enable
  I2S0.rcsr.raw |= ((struct I2S_RCSR_t) {
    .fef = 1, // clear if underrun
    .sef = 1  // clear if frame sync error
  }).raw;
}

static void pin_init() {
  SIM.scgc5.portc = 1;
  pin_mode(PIN_PTC6, PIN_MODE_MUX_ALT6); // MCLK  (master clock)
  pin_mode(PIN_PTC2, PIN_MODE_MUX_ALT6); // LRCLK (frame sync)
  pin_mode(PIN_PTC3, PIN_MODE_MUX_ALT6); // BCLK  (bit clock)
  pin_mode(PIN_PTC1, PIN_MODE_MUX_ALT6); // TX    (tx data)
  pin_mode(PIN_PTC5, PIN_MODE_MUX_ALT4); // RX    (rx data)
}

static void clock_init() {
  // Enable clock gates for I2S, DMAMUX and DMA
  SIM.scgc6.i2s = 1;
  SIM.scgc6.dmamux = 1;
  SIM.scgc7.dma = 1;

  I2S0.mcr.raw = ((struct I2S_MCR_t){
    .mics = 0,
    .moe  = 1
  }).raw;

  // Divide to get the 12.2880 MHz from 48MHz (48*(32/125))
  I2S0.mdr.raw = ((struct I2S_MDR_t){
    .mul = 31,
    .div = 124
  }).raw;
}

static void i2s_deinit() {
  // Interrupts disable
  int_disable(IRQ_I2S0_TX);
  int_disable(IRQ_I2S0_RX);
  // Transmitter disable while we configure everything
  I2S0.tcsr.te = 0;
  while(I2S0.tcsr.te); // Wait
  if(rx_enable) {
    // Receiver disable while we configure everything
    I2S0.rcsr.re = 0;
    while(I2S0.rcsr.re); // Wait
  }
}

static void dma_deinit() {
  // Disable IRQ on the DMA channels 0 and 1
  int_disable(IRQ_DMA0);
  if(rx_enable) int_disable(IRQ_DMA1);
  // Deactivate DMA channels
  DMA.tcd[0].csr.active = 0;
  if(rx_enable) DMA.tcd[1].csr.active = 0;
  // Disable DMA mux 
  DMAMUX.chcfg[0].enbl = 0;
  if(rx_enable) DMAMUX.chcfg[1].enbl = 0;
  // Clear interrupts
  DMA.cint.cint = 0;
  if(rx_enable) DMA.cint.cint = 1;  
}

static void i2s_init() {
  I2S0.tmr.raw = ((struct I2S_TMR_t) {
    .twm = 0                     // Word mask
  }).raw;

  I2S0.tcr1.raw = ((struct I2S_TCR1_t) {
    .tfw = FRSZ                  // FIFO watermark
  }).raw;

  I2S0.tcr2.raw = ((struct I2S_TCR2_t) {
    .sync = I2S_TCR_SYNC_ASYNC,  // use asynchronous mode
    .bcp  = 1,                   // BCLK polarity: active low
    .msel = I2S_MSEL_I2SMCLK,    // use mc1 (notbus clock as BCLK source
    .div  = BDIV,                // divide internal master clock to generate bit
    .bcd  = 1                    // BCLK is generated internally (master mode)
  }).raw;

  I2S0.tcr3.raw = ((struct I2S_TCR3_t) {
    .tce = 1                     // transmit data channel is enabled
  }).raw;

  I2S0.tcr4.raw = ((struct I2S_TCR4_t) {
    .frsz = FRSZ,                // frame size in words (plus one)
    .sywd = SYWD,                // number of bits in frame sync (plus one)
    .mf   = 1,                   // MSB (most significant bit) first
    .fse  = 1,                   // Frame sync one bit before the frame
    .fsd  = 1                    // WCLK is generated internally (master mode)
  }).raw;

  I2S0.tcr5.raw = ((struct I2S_TCR5_t) {
    .w0w = SYWD,                 // bits per word, first frame
    .wnw = SYWD,                 // bits per word, nth frame
    .fbt = BIWD                  // index shifted for FIFO
  }).raw;

  I2S0.rmr.raw = ((struct I2S_RMR_t) {
    .rwm = 0                   // No word mask
  }).raw;

  I2S0.rcr1.raw = ((struct I2S_RCR1_t) {
    .rfw = FRSZ                // FIFO watermark
  }).raw;

  I2S0.rcr2.raw = ((struct I2S_RCR2_t) {
    .sync = I2S_RCR_SYNC_TX,   // synchronous with the transmitter
    .bcp  = 1,                 // BCLK polarity: active low
    .msel = I2S_MSEL_I2SMCLK,  // ???  use MCLK as BCLK source (originally was 0 = bus clock???)
    .div  = BDIV,              // (DIV + 1) * 2, 12.288 MHz / 4 = 3.072 MHz
    .bcd  = 1                  // BCLK is generated internally in Master mode
  }).raw;

  I2S0.rcr3.raw = ((struct I2S_RCR3_t) {
    .rce = 1                   // receive data channel is enabled
  }).raw;

  I2S0.rcr4.raw = ((struct I2S_RCR4_t) {
    .frsz = FRSZ,              // frame size in words (plus one)
    .sywd = SYWD,              // bit width of a word (plus one)
    .mf   = 1,                 // MSB (most significant bit) first
    .fse  = 1,                 // Frame sync one bit before the frame
    .fsd  = 1                  // WCLK is generated internally (master mode)
  }).raw;

  I2S0.rcr5.raw = ((struct I2S_RCR5_t) {
    .w0w = SYWD,               // bits per word, first frame
    .wnw = SYWD,               // bits per word, nth frame
    .fbt = BIWD                // index shifted for FIFO
  }).raw;

  if(buffer_size == 0) int_enable(IRQ_I2S0_TX);
}

static void dma_init() {
  // Configure DMAMUX sources
  DMAMUX.chcfg[0].raw = ((struct DMAMUX_CHCFG_t) {
    .source = DMAMUX_SOURCE_I2S0_TX
  }).raw;
  if(rx_enable) {
    DMAMUX.chcfg[1].raw = ((struct DMAMUX_CHCFG_t) {
      .source = DMAMUX_SOURCE_I2S0_RX
    }).raw;
  }

  // Enable IRQ on the DMA channels 0 and 1
  int_enable(IRQ_DMA0);

  // TODO this should not be done here
#ifndef ROUNDROBIN
  // Set channel priorities (each must be unique)
  DMA.dchpri[0].raw = ((struct DMA_DCHPRI_t) { // Channel 3
    .chpri = 0
  }).raw;
  DMA.dchpri[1].raw = ((struct DMA_DCHPRI_t) { // Channel 2
    .chpri = 1
  }).raw;
  DMA.dchpri[2].raw = ((struct DMA_DCHPRI_t) { // Channel 1
    .chpri = 2
  }).raw;
  DMA.dchpri[3].raw = ((struct DMA_DCHPRI_t) { // Channel 0
    .chpri = 3
  }).raw;
#endif
  // Control register
  DMA.cr.raw = ((struct DMA_CR_t) {
    .emlm = 1, // Enable minor looping
#ifdef ROUNDROBIN
    .erca = 1  // Enable round-robin channel arbitration
#endif
  }).raw;

  // Fill the TCD regs for channel 0 (TX)
  DMA.tcd[0].saddr = tx_buf_a;   // Alternated by isr
  DMA.tcd[0].soff = DMAL;        // Source offset after each xfer
  DMA.tcd[0].attr.raw = ((struct DMA_ATTR_t) {
    .smod = 0,                   // No source modulo
    .ssize = DMAS,               // Source data bits
    .dmod = 0,                   // Destination modulo
    .dsize = DMAS                // Destination data bits
  }).raw;
  DMA.tcd[0].nbytes.mlno = DMAL; // Num bytes in each xfer
  DMA.tcd[0].slast = 0;          //
  DMA.tcd[0].daddr = &I2S0.tdr;  // Destination is I2S.tdr
  DMA.tcd[0].doff = 0;           // No destination offset after each transfer
  DMA.tcd[0].dlastsga = 0;       // No scatter/gather
  DMA.tcd[0].citer.elinkno.raw = ((struct DMA_ELINKNO_t) {
     .iter = ITER                // Major loop count
  }).raw;
  DMA.tcd[0].biter.elinkno.raw = ((struct DMA_ELINKNO_t) {
    .iter = ITER                 // Major loop count, no ch links
  }).raw;
  DMA.tcd[0].csr.raw = ((struct DMA_CSR_t) {
    .intmajor = 1,               // Interrupt on major loop completion
    .bwc      = 3                // DMA bandwidth control
  }).raw;

  if(rx_enable) {
    // Fill the TCD regs for channel 1 (RX)
    DMA.tcd[1].saddr = &I2S0.rdr;  // Source is I2S.rdr
    DMA.tcd[1].soff = 0;           // No source offset after each transfer
    DMA.tcd[1].attr.raw = ((struct DMA_ATTR_t) {
      .smod  = 0,                  // Source modulo
      .ssize = DMAS,               // Source data bits
      .dmod  = 0,                  // No destination modulo
      .dsize = DMAS                // Destination data bits
    }).raw;
    DMA.tcd[1].nbytes.mlno = DMAL; // Num bytes in each xfer
    DMA.tcd[1].slast = 0;          // Source address to be written by isr
    DMA.tcd[1].daddr = rx_buf_a;   // Alternated by isr
    DMA.tcd[1].doff = DMAL;        // Destination offset after each xfer
    DMA.tcd[1].dlastsga = 0;       // No scatter/gather
    DMA.tcd[1].citer.elinkno.raw = ((struct DMA_ELINKNO_t) {
      .iter = ITER                 // Major loop count
    }).raw;
    DMA.tcd[1].biter.elinkno.raw = ((struct DMA_ELINKNO_t) {
      .iter = ITER                 // Major loop count, no ch links
    }).raw;
    DMA.tcd[1].csr.raw = ((struct DMA_CSR_t) {
      .intmajor = 1,               // Interrupt on major loop completion
      .bwc      = 3                // DMA bandwidth control
    }).raw;
  }

  // Enable DMA requests for channels 0 and 1
  DMA.serq.raw = ((struct DMA_SERQ_t) { .serq = 0 }).raw;
  if(rx_enable) DMA.serq.raw = ((struct DMA_SERQ_t) { .serq = 1 }).raw;

  // Enable DMAMUX channels 0 and 1
  DMAMUX.chcfg[0].enbl = 1;
  if(rx_enable) DMAMUX.chcfg[1].enbl = 1;

  // Set DMA active on channels 0 and 1
  DMA.tcd[0].csr.active = 1;
  if(rx_enable) DMA.tcd[1].csr.active = 1;
}

static void buf_init() {
  memset(tx_buf_a, 0, buffer_size);
  memset(tx_buf_b, 0, buffer_size);
  if(rx_enable) {
    memset(rx_buf_a, 0, buffer_size);
    memset(rx_buf_b, 0, buffer_size);
  }
  buf_flip = true;
}

// TODO: bug here - reinitialization crashes sometimes, racing condition?
void i2s_configure(I2S_SR_e sample_rate, I2S_BD_e bit_depth, bool enable_recv, void *dma_buffers, size_t dma_bytes) {
  if(!initialized) {
    pin_init();    // Configure pins
    clock_init();  // Configure and enable clocks
    initialized = true;
  }

  i2s_deinit();
  if(buffer_size) dma_deinit();

  sample_rate_khz = sample_rate;
  bits_per_sample = bit_depth;
  rx_enable = enable_recv;
  buffer_size = dma_bytes >> (rx_enable ? 2 : 1); // TODO: depend also on I2S_FRAME_SIZE
  sample_count = (buffer_size / (bits_per_sample >> 2));
  tx_buf_a = ((uint8_t*)dma_buffers) + (buffer_size * 0);
  tx_buf_b = ((uint8_t*)dma_buffers) + (buffer_size * 1);
  if(rx_enable) {
    rx_buf_a = ((uint8_t*)dma_buffers) + (buffer_size * 2);
    rx_buf_b = ((uint8_t*)dma_buffers) + (buffer_size * 3);
  }

  i2s_init();      // Configure I2S module
  if(buffer_size) {
    buf_init();    // Reset buffers
    dma_init();    // Configure and enable DMA channels
  }
}

void i2s_start() {
  // Transmit enable
  I2S0.tcsr.raw = ((struct I2S_TCSR_t) {
    .te   = 1, // Transmit enable
    .bce  = 1, // Bit clock enable
    .fr   = 1, // FIFO reset
    .frde = buffer_size != 0, // FIFO request DMA enable
    .frie = buffer_size == 0  // FIFO request interrupt enable
  }).raw;

  // Receive enable
  if(rx_enable) {
    // Receive enable
    I2S0.rcsr.raw = ((struct I2S_RCSR_t) {
      .re   = 1, // Receive enable
      .bce  = 1, // Bit clock enable
      .fr   = 1, // FIFO reset
      .frde = buffer_size != 0, // FIFO request DMA enable
      .frie = buffer_size == 0  // FIFO request interrupt enable
    }).raw;
  }
}
