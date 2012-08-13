#include "safecast_config.h"
#include "mpr121.h"
#include "i2c.h"
#include "exti.h"
#include <stdio.h>
#include "captouch.h"
#include "GUI.h"

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

GUI *system_gui;
static struct i2c_dev *i2c;
static uint8 touchInit = 0;
static uint16 touchList =  1 << 8 | 1 << 6 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 0;
  
int last_key_state=0;

static void mpr121Write(uint8 addr, uint8 value) {
  struct i2c_msg msg;
  uint8 bytes[2];
  int result;

  bytes[0] = addr;
  bytes[1] = value;

  msg.addr = CAPTOUCH_ADDR;
  msg.flags = 0;
  msg.length = sizeof(bytes);
  msg.xferred = 0;
  msg.data = bytes;

  result = i2c_master_xfer(i2c, &msg, 1, 100);
  return;
}

static uint8 mpr121Read(uint8 addr) {
  struct i2c_msg msgs[2];
  uint8 byte;

  byte = addr;
  msgs[0].addr   = msgs[1].addr   = CAPTOUCH_ADDR;
  msgs[0].length = msgs[1].length = sizeof(byte);
  msgs[0].data   = msgs[1].data   = &byte;
  msgs[0].flags = 0;
  msgs[1].flags = I2C_MSG_READ;
  int result = i2c_master_xfer(i2c, msgs, 2, 100);

  // retry reads
  for(int n=0;(result == -1) && (n<5);n++) {
    result = i2c_master_xfer(i2c, msgs, 2, 100);
  }

  return byte;
}

/*
char c[100];
char *diag_data() {

  int elec2h = mpr121Read(0x09);
  int elec2l = mpr121Read(0x08);

  int elec2 = ((int)elec2h << 8) | (int)elec2l;

  int elec2_base = ((int)mpr121Read(0x20)) << 2;
  int bs = mpr121Read(TCH_STATL);
  sprintf(c,"%d %d %d  ",elec2,elec2_base,bs);

  return c; 
}
*/

static void cap_change(void) {

  // manual beep 
  
  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,1);
  
  gpio_set_mode (GPIOB,9, GPIO_OUTPUT_PP);
  for(int n=0;n<50;n++) {
   // gpio_toggle_bit(GPIOB,9);
    delay_us(1000);
  }
  gpio_write_bit(GPIOB,9,0);
 
  
  int key_state=0;
  key_state  = mpr121Read(TCH_STATL);
  key_state |= mpr121Read(TCH_STATH) << 8;

  // clear unconnected electrodes
  key_state &= touchList;

  // detect keys pressed
  int keys_pressed   = key_state & (~last_key_state); //TODO: ! bitwise NOT

  // detect keys released
  int keys_released  = (~key_state) & last_key_state; //TODO: ! bitwise NOT


  for (int key=0; key<16; key++) {
    if (keys_pressed &(1<<key)) { system_gui->receive_key(key,KEY_PRESSED ); }
    if (keys_released&(1<<key)) { system_gui->receive_key(key,KEY_RELEASED); }
  }

  last_key_state = key_state;

  // gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,0);
}


void cap_init(void) {
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,1);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,1);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_OUTPUT_PP);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,1);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,1);
    delay_us(1000);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_INPUT_PD);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_INPUT_PD);

    i2c = CAPTOUCH_I2C;
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);
    //Serial1.print(".");

    mpr121Write(ELE_CFG, 0x00);   // disable electrodes for config
    delay_us(100);

    // Section A
    // This group controls filtering when data is > baseline.
    mpr121Write(MHD_R, 0x02); // was 0x01 
    mpr121Write(NHD_R, 0x02);
    //    mpr121Write(NCL_R, 0x50);
    //    mpr121Write(FDL_R, 0x50);
    mpr121Write(NCL_R, 0x02); //was 0x00 
    mpr121Write(FDL_R, 0x02);

    // Section B
    // This group controls filtering when data is < baseline.
    mpr121Write(MHD_F, 0x02); // was 0x01
    mpr121Write(NHD_F, 0x02); // was 0x01
    mpr121Write(NCL_F, 0x10); // was 0xFF
    //    mpr121Write(FDL_F, 0x52);
    mpr121Write(FDL_F, 0x02); // was 0x02

    // Section C
    // This group sets touch and release thresholds for each electrode
    mpr121Write(ELE0_T, TOU_THRESH);
    mpr121Write(ELE0_R, REL_THRESH);
    mpr121Write(ELE1_T, TOU_THRESH);
    mpr121Write(ELE1_R, REL_THRESH);
    mpr121Write(ELE2_T, TOU_THRESH);
    mpr121Write(ELE2_R, REL_THRESH);
    mpr121Write(ELE3_T, TOU_THRESH);
    mpr121Write(ELE3_R, REL_THRESH);
    mpr121Write(ELE4_T, TOU_THRESH);
    mpr121Write(ELE4_R, REL_THRESH);
    mpr121Write(ELE5_T, TOU_THRESH);
    mpr121Write(ELE5_R, REL_THRESH);
    mpr121Write(ELE6_T, TOU_THRESH);
    mpr121Write(ELE6_R, REL_THRESH);
    mpr121Write(ELE7_T, TOU_THRESH);
    mpr121Write(ELE7_R, REL_THRESH);
    mpr121Write(ELE8_T, TOU_THRESH);
    mpr121Write(ELE8_R, REL_THRESH);
    mpr121Write(ELE9_T, TOU_THRESH);
    mpr121Write(ELE9_R, REL_THRESH);
    mpr121Write(ELE10_T, TOU_THRESH);
    mpr121Write(ELE10_R, REL_THRESH);
    mpr121Write(ELE11_T, TOU_THRESH);
    mpr121Write(ELE11_R, REL_THRESH);

    // Section D
    // Set the Filter Configuration
    // Set ESI2
    mpr121Write(FIL_CFG, 0xC3);  // set CDT to 32us, ESI (sampling interval) to 8 ms
    //mpr121Write(FIL_CFG, 0x03);  // set CDT to 32us, ESI (sampling interval) to 8 ms
    mpr121Write(AFE_CONF, 0xBF); // 6 samples, 63uA <-- will be overridden by auto-config i think
    //orignialmpr121Write(AFE_CONF, 0x3F); // 6 samples, 63uA <-- will be overridden by auto-config i think

    // Section F
    mpr121Write(ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
    mpr121Write(ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    mpr121Write(ATO_CFGT, 0xB5);    // Target = 0.9*USL = 0xB5 @3.3V

    // mpr121Write(ATO_CFGU, 0xC0);  // VSL = (Vdd-0.7)/vdd*256 = 0xC0 @2.8V
    // mpr121Write(ATO_CFGL, 0x7D);  // LSL = 0.65*USL = 0x7D @2.8V
    // mpr121Write(ATO_CFGT, 0xB2);    // Target = 0.9*USL = 0xB2 @2.8V

    //    mpr121Write(ATO_CFGU, 0x9C);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @1.8V
    //    mpr121Write(ATO_CFGL, 0x65);  // LSL = 0.65*USL = 0x82 @1.8V
    //    mpr121Write(ATO_CFGT, 0x8C);    // Target = 0.9*USL = 0xB5 @1.8V

    // Enable Auto Config and auto Reconfig
//// original   mpr121Write(ATO_CFG0, 0x3B); // must match AFE_CONF setting of 6 samples, retry enabled
    mpr121Write(ATO_CFG0, 0xBF); // must match AFE_CONF setting of 6 samples, retry enabled

    delay_us(100);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
    delay_us(100);

    gpio_set_mode(PIN_MAP[CAPTOUCH_GPIO].gpio_device,PIN_MAP[CAPTOUCH_GPIO].gpio_bit,GPIO_INPUT_PU);
    //gpio_set_mode(PIN_MAP[CAPTOUCH_GPIO].gpio_device,PIN_MAP[CAPTOUCH_GPIO].gpio_bit,GPIO_INPUT_FLOATING);
    
    exti_attach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit), gpio_exti_port(PIN_MAP[CAPTOUCH_GPIO].gpio_device), cap_change, EXTI_FALLING);

    // Clears the first interrupt
    mpr121Read(TCH_STATL);
    mpr121Read(TCH_STATH);

    touchInit = 1;
    return;
}

void cap_deinit(void) {
  exti_detach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit));

  // Disable MPR121 scanning, in case the chip is on
  mpr121Write(ELE_CFG, 0x00);

  return;
}


int cap_setkeydown(void (*new_keydown)(int key)) {
  return 0;
}

int cap_setkeyup(void (*new_keyup)(int key)) {
  return 0;
}
