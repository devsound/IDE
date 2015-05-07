#include <mchck.h>
#include <Arduino.h>
#include <codec.h>
#include "SnakeCharmer.h"
/*
SnakeCharmer library
Thanks to Jim Lindblom for the code on which this was based
*/

struct snakecharmer_t SnakeCharmer = {snch_begin, snch_volume, snch_refresh, snch_calibrate_dc};

// MPR121 Register Defines
#define MHD_R 0x2B
#define NHD_R 0x2C
#define NCL_R 0x2D
#define FDL_R 0x2E
#define MHD_F 0x2F
#define NHD_F 0x30
#define NCL_F 0x31
#define FDL_F 0x32
#define ELE0_T 0x41
#define ELE0_R 0x42
#define ELE1_T 0x43
#define ELE1_R 0x44
#define ELE2_T 0x45
#define ELE2_R 0x46
#define ELE3_T 0x47
#define ELE3_R 0x48
#define ELE4_T 0x49
#define ELE4_R 0x4A
#define ELE5_T 0x4B
#define ELE5_R 0x4C
#define ELE6_T 0x4D
#define ELE6_R 0x4E
#define ELE7_T 0x4F
#define ELE7_R 0x50
#define ELE8_T 0x51
#define ELE8_R 0x52
#define ELE9_T 0x53
#define ELE9_R 0x54
#define ELE10_T 0x55
#define ELE10_R 0x56
#define ELE11_T 0x57
#define ELE11_R 0x58
#define FIL_CFG 0x5D
#define ELE_CFG 0x5E
#define GPIO_CTRL0 0x73
#define GPIO_CTRL1 0x74
#define GPIO_DATA 0x75
#define GPIO_DIR 0x76
#define GPIO_EN 0x77
#define GPIO_SET 0x78
#define GPIO_CLEAR 0x79
#define GPIO_TOGGLE 0x7A
#define ATO_CFG0 0x7B
#define ATO_CFGU 0x7D
#define ATO_CFGL 0x7E
#define ATO_CFGT 0x7F

// MPR121 Global Constants
#define TOU_THRESH 0x05
#define REL_THRESH 0x02


void snch_send_complete(enum i2c_result result, struct i2c_transaction *transaction) {
  *((bool*)transaction->cbdata) = true;
}

void snch_mpr121_read(unsigned char address, void *data, size_t len) {
  struct i2c_transaction reader;

//  char volatile data;
  bool volatile done = false;

  delay(1);
  reader.address = 0x5A;
  reader.direction = I2C_WRITE;
  reader.buffer = &address;
  reader.length = 1;
  reader.cb = snch_send_complete;
  reader.cbdata = (void*)&done;
  reader.stop = I2C_NOSTOP;
  i2c_queue(&reader);
  while(!done);
  delay(1);
  reader.address = 0x5A;
  reader.direction = I2C_READ;
  reader.buffer = data;
  reader.length = len;
  reader.cb = snch_send_complete;
  reader.cbdata = (void*)&done;
  reader.stop = I2C_STOP;
  done = false;
  i2c_queue(&reader);
  while(!done);
}

void snch_mpr121_write(unsigned char address, unsigned char data) {
  struct i2c_transaction writer;
  unsigned char buf[2] = {address, data};

  bool volatile done = false;

  delay(1);
  writer.address = 0x5A;
  writer.direction = I2C_WRITE;
  writer.buffer = buf;
  writer.length = 2;
  writer.cb = snch_send_complete;
  writer.cbdata = (void*)&done;
  writer.stop = I2C_STOP;
  i2c_queue(&writer);
  while(!done);


}

void snch_mpr121_config(void) {
  // Section A - above baseline filtering
  snch_mpr121_write(MHD_R, 0x01);
  snch_mpr121_write(NHD_R, 0x01);
  snch_mpr121_write(NCL_R, 0x00);
  snch_mpr121_write(FDL_R, 0x00);

  // Section B - below baseline filtering
  snch_mpr121_write(MHD_F, 0x01);
  snch_mpr121_write(NHD_F, 0x01);
  snch_mpr121_write(NCL_F, 0xFF);
  snch_mpr121_write(FDL_F, 0x02);

  // Section C - touch/release thresholds
  snch_mpr121_write(ELE0_T, TOU_THRESH);
  snch_mpr121_write(ELE0_R, REL_THRESH);
  snch_mpr121_write(ELE1_T, TOU_THRESH);
  snch_mpr121_write(ELE1_R, REL_THRESH);
  snch_mpr121_write(ELE2_T, TOU_THRESH);
  snch_mpr121_write(ELE2_R, REL_THRESH);
  snch_mpr121_write(ELE3_T, TOU_THRESH);
  snch_mpr121_write(ELE3_R, REL_THRESH);
  snch_mpr121_write(ELE4_T, TOU_THRESH);
  snch_mpr121_write(ELE4_R, REL_THRESH);
  snch_mpr121_write(ELE5_T, TOU_THRESH);
  snch_mpr121_write(ELE5_R, REL_THRESH);

  snch_mpr121_write(ELE6_T, TOU_THRESH);
  snch_mpr121_write(ELE6_R, REL_THRESH);
  snch_mpr121_write(ELE7_T, TOU_THRESH);
  snch_mpr121_write(ELE7_R, REL_THRESH);
  snch_mpr121_write(ELE8_T, TOU_THRESH);
  snch_mpr121_write(ELE8_R, REL_THRESH);
  snch_mpr121_write(ELE9_T, TOU_THRESH);
  snch_mpr121_write(ELE9_R, REL_THRESH);
  snch_mpr121_write(ELE10_T, TOU_THRESH);
  snch_mpr121_write(ELE10_R, REL_THRESH);
  snch_mpr121_write(ELE11_T, TOU_THRESH);
  snch_mpr121_write(ELE11_R, REL_THRESH);

  // Section D - filter configuration
  snch_mpr121_write(FIL_CFG, 0x00);

  // Section E - electrode configuration
  snch_mpr121_write(ELE_CFG, 0x0C);	// Enable all 12 electrodes

  // Section F - auto config
  // Enable Auto Config and auto Reconfig
  //snch_mpr121_write(ATO_CFG0, 0x0B);
  //snch_mpr121_write(ATO_CFGU, 0xC9); // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
  //snch_mpr121_write(ATO_CFGL, 0x82); // LSL = 0.65*USL = 0x82 @3.3V
  //snch_mpr121_write(ATO_CFGT, 0xB5); // Target = 0.9*USL = 0xB5 @3.3V

}

void snch_begin(AUDIO_SR_e samplerate, AUDIO_BD_e bitdepth, bool enable_recv, void *dma_buffers, size_t dma_bytes) {
  // Initialize I2C
  i2c_init(I2C_RATE_100);
  
  // LED configuration
  pinMode(RED_LED, OUTPUT);
  pinConfig(RED_LED, DRIVE_WEAK);
  pinMode(YELLOW_LED, OUTPUT);
  pinConfig(YELLOW_LED, DRIVE_WEAK);
  pinMode(GREEN_LED, OUTPUT);
  pinConfig(GREEN_LED, DRIVE_WEAK);  

  // Settling time
  delay(333);

  // Configure MPR121
  snch_mpr121_config();

  // Initialize audio system
  audio_begin(samplerate, bitdepth, enable_recv, dma_buffers, dma_bytes);
  // Select line input
  audio_selectinput(LINE_IN);
  // Set default input gain
  audio_lineinvolume(26, false);
  // Disable DC filter
  audio_dcfilter(false, false);
  // Set default volume
  audio_hpvolume(127, true);
}

void snch_calibrate_dc(bool active) {
  audio_dcfilter(active, true);
}

void snch_volume(uint8_t volume) {
  audio_dcfilter(volume, true);
}

void snch_refresh(void){
  char regs[28];
  snch_mpr121_read(0, regs, 28);
  SnakeCharmer.digital = (regs[0] << 8) | regs[1];
  unsigned int n;
  for(n = 0; n < 12; n++) SnakeCharmer.analog[n] = (regs[n * 2 + 2] << 8) | regs[n * 2 + 3];
}
