/***
    important note: accel must be AFTER captouch in initialization sequence, as accel
    relies on i2c init by captouch
 ***/

//#include "device.h"
#include "i2c.h"
#include <stdint.h>
#define ACCEL_I2C I2C1
#define ACCEL_ADDR 0x1D

static struct i2c_dev *i2c = ACCEL_I2C;

static int accel_write(uint8 addr, uint8 value) {
    struct i2c_msg msg;
    uint8 bytes[2];
    int result;

    bytes[0] = addr;
    bytes[1] = value;

    msg.addr    = ACCEL_ADDR;
    msg.flags   = 0;
    msg.length  = sizeof(bytes);
    msg.xferred = 0;
    msg.data    = bytes;

    result = i2c_master_xfer(i2c, &msg, 1, 1);

    return result;
}

static uint8 accel_read(uint8 addr) {
    struct i2c_msg msgs[2];
    uint8 byte;

    byte = addr;
    msgs[0].addr   = msgs[1].addr   = ACCEL_ADDR;
    msgs[0].length = msgs[1].length = sizeof(byte);
    msgs[0].data   = msgs[1].data   = &byte;
    msgs[0].flags = 0;
    msgs[1].flags = I2C_MSG_READ;
    i2c_master_xfer(i2c, msgs, 2, 1);
    return byte;
}


static void accel_wakeup(void) {
    /* Set the mode to "measurement", measuring 2g */
    accel_write(0x16, 0x04 | 0x01);
}

static int accel_ready(void) {
    return (accel_read(0x09) & 1);
}

static int accel_sleep(void) {
    return accel_write(0x16, 0);
}


uint8 accel_read_state(int16 *x, int16 *y, int16 *z) {
  struct i2c_msg msgs[2];
  signed char values[6];
  uint8 addr = 0x00; /* 10-bits read value */
  int32 result = 0;

  accel_wakeup();
  for(int n=0;(!accel_ready()) && (n < 10);n++) delay_us(1000);
  if(!accel_ready()) return 200;

  msgs[0].addr   = ACCEL_ADDR;
  msgs[0].length = sizeof(uint8_t);
  msgs[0].data   = &addr;
  msgs[0].flags  = 0;

  msgs[1].addr   = ACCEL_ADDR;
  msgs[1].length = sizeof(values);
  msgs[1].data   = (uint8 *)values;
  msgs[1].flags  = I2C_MSG_READ;

  result = i2c_master_xfer(i2c, msgs, 2, 1);

  if(result == I2C_STATE_ERROR  ) return 100;
  if(result == I2C_ERROR_TIMEOUT) return 10;

  if (x)
      *x = (values[1]<<2) | (values[0]);
  if (y)
      *y = (values[3]<<2) | (values[2]);
  if (z)
      *z = (values[5]<<2) | (values[4]);

  accel_sleep();

  return 0;
}

int accel_init(void) {
  return 0;
}

int accel_deinit() {
    // suspends accel
    accel_write(0x16, 0);
    return 0;
}
