#include "safecast_config.h"
#include "mpr121.h"
#include "i2c.h"
#include "exti.h"
#include <stdio.h>
#include "captouch.h"
#include "realtime.h"
#include "GUI.h"

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

GUI *system_gui;
static struct i2c_dev *i2c;
static uint8 touchInit = 0;
static uint16 touchList =  1 << 8 | 1 << 6 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 0;
  
int last_key_state=0;
bool captouch_disable_messages=false;

void cap_set_disable_messages(bool b) {
 captouch_disable_messages=b;
}

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
  if(result == -1) return 255;

  return byte;
}

char c[100];
char *diag_data(int e) {

  int elech;
  int elecl;
  if(e==0)  elecl = mpr121Read(ELE0_DATAL);
  if(e==0)  elech = mpr121Read(ELE0_DATAH);
  if(e==1)  elecl = mpr121Read(ELE1_DATAL);
  if(e==1)  elech = mpr121Read(ELE1_DATAH);
  if(e==2)  elecl = mpr121Read(ELE2_DATAL);
  if(e==2)  elech = mpr121Read(ELE2_DATAH);
  if(e==3)  elecl = mpr121Read(ELE3_DATAL);
  if(e==3)  elech = mpr121Read(ELE3_DATAH);
  if(e==4)  elecl = mpr121Read(ELE4_DATAL);
  if(e==4)  elech = mpr121Read(ELE4_DATAH);
  if(e==5)  elecl = mpr121Read(ELE5_DATAL);
  if(e==5)  elech = mpr121Read(ELE5_DATAH);
  if(e==6)  elecl = mpr121Read(ELE6_DATAL);
  if(e==6)  elech = mpr121Read(ELE6_DATAH);
  if(e==7)  elecl = mpr121Read(ELE7_DATAL);
  if(e==7)  elech = mpr121Read(ELE7_DATAH);
  if(e==8)  elecl = mpr121Read(ELE8_DATAL);
  if(e==8)  elech = mpr121Read(ELE8_DATAH);
  if(e==9)  elecl = mpr121Read(ELE9_DATAL);
  if(e==9)  elech = mpr121Read(ELE9_DATAH);
  if(e==10) elecl = mpr121Read(ELE10_DATAL);
  if(e==10) elech = mpr121Read(ELE10_DATAH);

  elech = elech & 0x3;
  uint32_t elecv = ((uint32_t)elech << 8) | (uint32_t)elecl;

  uint32_t elec_base;
  if(e==0)  elec_base = ((uint32_t)((uint8_t)mpr121Read(ELE0_BASE)))  << 2;
  if(e==1)  elec_base = ((uint32_t)((uint8_t)mpr121Read(ELE1_BASE)))  << 2;
  if(e==2)  elec_base = ((int)mpr121Read(ELE2_BASE))  << 2;
  if(e==3)  elec_base = ((int)mpr121Read(ELE3_BASE))  << 2;
  if(e==4)  elec_base = ((int)mpr121Read(ELE4_BASE))  << 2;
  if(e==5)  elec_base = ((int)mpr121Read(ELE5_BASE))  << 2;
  if(e==6)  elec_base = ((int)mpr121Read(ELE6_BASE))  << 2;
  if(e==7)  elec_base = ((int)mpr121Read(ELE7_BASE))  << 2;
  if(e==8)  elec_base = ((int)mpr121Read(ELE8_BASE))  << 2;
  if(e==9)  elec_base = ((int)mpr121Read(ELE9_BASE))  << 2;
  if(e==10) elec_base = ((int)mpr121Read(ELE10_BASE)) << 2;

  int bs = mpr121Read(TCH_STATL);
      bs = bs | ((0x1F & mpr121Read(TCH_STATH)) << 8);
  sprintf(c,"%d %d %d  ",elecv,elec_base,bs);

  return c; 
}

bool cap_check() {
  uint8 d = mpr121Read(ELE0_DATAL);
  if(d == 255) return false;
  return true;
}

uint32_t press_time[16];
uint32_t release_time[16];
uint32_t press_time_any;
uint32_t release_time_any;

uint32_t cap_last_press(int key) {
  return press_time[key];
}

uint32_t cap_last_release(int key) {
  return release_time[key];
}

uint32_t cap_last_press_any() {
  return press_time_any;
}

uint32_t cap_last_release_any() {
  return release_time_any;
}

void cap_clear_press() {
  for(int n=0;n<16;n++) press_time[n] = 0;
  for(int n=0;n<16;n++) release_time[n] = 0;
  press_time_any=0;
  release_time_any=0;
}

static void cap_change(void) {

  int key_state=0;
  key_state  = mpr121Read(TCH_STATL);
  key_state |= mpr121Read(TCH_STATH) << 8;

  // clear unconnected electrodes
  key_state &= touchList;

  // detect keys pressed
  int keys_pressed = key_state & (~last_key_state); //TODO: ! bitwise NOT

  // detect keys released
  int keys_released  = (~key_state) & last_key_state; //TODO: ! bitwise NOT


  if(!captouch_disable_messages) {
    for (int key=0; key<16; key++) {
      if (keys_pressed &(1<<key)) { system_gui->receive_key(key,KEY_PRESSED );   press_time[key] = realtime_get_unixtime(); press_time_any=realtime_get_unixtime(); }
      if (keys_released&(1<<key)) { system_gui->receive_key(key,KEY_RELEASED); release_time[key] = realtime_get_unixtime(); release_time_any = realtime_get_unixtime(); }
    }
  }


  last_key_state = key_state;
}

bool cap_ispressed(int key) {
  int key_state=0;
  key_state  = mpr121Read(TCH_STATL);
  key_state |= mpr121Read(TCH_STATH) << 8;

  if (key_state & (1<<key)) { return true; }
                       else { return false;}
}

int cap_lastkey() {
  return last_key_state;
}


void cap_init(void) {
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,GPIO_OUTPUT_PP);
    gpio_set_mode(PIN_MAP[5].gpio_device,PIN_MAP[5].gpio_bit,GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,1);
    gpio_write_bit(PIN_MAP[5].gpio_device,PIN_MAP[5].gpio_bit,1);
    delay_us(1000);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,GPIO_INPUT_PD); // Can also be floating, but PD is safer if components misplaced.
    gpio_set_mode(PIN_MAP[5].gpio_device,PIN_MAP[5].gpio_bit,GPIO_INPUT_PD);

    i2c = CAPTOUCH_I2C;
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);

    mpr121Write(0x80,0x63); // soft reset
    delay_us(1000);
    mpr121Write(ELE_CFG, 0x00);   // disable electrodes for config
    delay_us(100);

    // Section A
    mpr121Write(MHD_R, 0x3F); // was 0x01 
    mpr121Write(NHD_R, 0x02); // was 0x02
    mpr121Write(NCL_R, 0xFF); // was 0x00 
    mpr121Write(FDL_R, 0x00); // was 0x08

    // Section B
    mpr121Write(MHD_F, 0x3F); // was 0x01 , largest value to pass through filer
    mpr121Write(NHD_F, 0x02); // was 0x02 , maximum change allowed
    mpr121Write(NCL_F, 0xFF); // was 0xFF
    mpr121Write(FDL_F, 0x00); // was 0x08

    // Section D
    // Set the Filter Configuration
    // Set ESI2

    // was 0x01, 0x25
    mpr121Write(AFE_CONF, 0x01); //AFE_CONF  0x5C
    mpr121Write(FIL_CFG , 0x25); //FIL_CFG   0x5D

    // Section F
    //mpr121Write(ATO_CFG0, 0x05); // ATO_CFG0 0x7B
    mpr121Write(ATO_CFG0, 0x05); // ATO_CFG0 0x7B

    // limits
    // was0xFF,0x00,0x0E
    mpr121Write(ATO_CFGU, 0x9C); // ATO_CFGU 0x7D
    mpr121Write(ATO_CFGL, 0x65); // ATO_CFGL 0x7E
    mpr121Write(ATO_CFGT, 0x8C); // ATO_CFGT 0x7F

    // Section C
    // This group sets touch and release thresholds for each electrode
    mpr121Write(ELE0_T , TOU_THRESH);
    mpr121Write(ELE0_R , REL_THRESH);
    mpr121Write(ELE1_T , TOU_THRESH);
    mpr121Write(ELE1_R , REL_THRESH);
    mpr121Write(ELE2_T , TOU_THRESH);
    mpr121Write(ELE2_R , REL_THRESH);
    mpr121Write(ELE3_T , TOU_THRESH);
    mpr121Write(ELE3_R , REL_THRESH);
    mpr121Write(ELE4_T , TOU_THRESH);
    mpr121Write(ELE4_R , REL_THRESH);
    mpr121Write(ELE5_T , TOU_THRESH);
    mpr121Write(ELE5_R , REL_THRESH);
    mpr121Write(ELE6_T , TOU_THRESH);
    mpr121Write(ELE6_R , REL_THRESH);
    mpr121Write(ELE7_T , TOU_THRESH);
    mpr121Write(ELE7_R , REL_THRESH);
    mpr121Write(ELE8_T , TOU_THRESH);
    mpr121Write(ELE8_R , REL_THRESH);
    mpr121Write(ELE9_T , TOU_THRESH);
    mpr121Write(ELE9_R , REL_THRESH);
    mpr121Write(ELE10_T, TOU_THRESH);
    mpr121Write(ELE10_R, REL_THRESH);
    mpr121Write(ELE11_T, TOU_THRESH);
    mpr121Write(ELE11_R, REL_THRESH);

    delay_us(100);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
    delay_us(100);

    // This can also be FLOATING, but PU is safer if components misplaced.
    gpio_set_mode(PIN_MAP[CAPTOUCH_GPIO].gpio_device,PIN_MAP[CAPTOUCH_GPIO].gpio_bit,GPIO_INPUT_PU);
    //gpio_set_mode(PIN_MAP[CAPTOUCH_GPIO].gpio_device,PIN_MAP[CAPTOUCH_GPIO].gpio_bit,GPIO_INPUT_FLOATING);
    
    exti_attach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit), gpio_exti_port(PIN_MAP[CAPTOUCH_GPIO].gpio_device), cap_change, EXTI_FALLING);

    // Clears the first interrupt
 //   mpr121Read(TCH_STATL);
 //   mpr121Read(TCH_STATH);
    for(int n=0;n<16;n++) press_time[n]   = realtime_get_unixtime();
    for(int n=0;n<16;n++) release_time[n] = realtime_get_unixtime();
    press_time_any   = realtime_get_unixtime();
    release_time_any = realtime_get_unixtime();

    return;
}

void cap_deinit(void) {
  exti_detach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit));

  // Disable MPR121 scanning, in case the chip is on
  mpr121Write(ELE_CFG, 0x00);

  return;
}
