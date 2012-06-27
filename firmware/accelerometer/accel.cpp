/***
    important note: accel must be AFTER captouch in initialization sequence, as accel
    relies on i2c init by captouch
 ***/

//#include "wirish.h"
#include "device.h"
#include "i2c.h"
//#include "switch.h"

#define ACCEL_I2C I2C1
#define ACCEL_ADDR 0x1D

static struct i2c_dev *i2c = ACCEL_I2C;

static int
accel_write(uint8 addr, uint8 value)
{
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

//    if (switch_state(&back_switch))
    result = i2c_master_xfer(i2c, &msg, 1, 1);

    return result;
}

static uint8
accel_read(uint8 addr)
{
    struct i2c_msg msgs[2];
    uint8 byte;

    byte = addr;
    msgs[0].addr   = msgs[1].addr   = ACCEL_ADDR;
    msgs[0].length = msgs[1].length = sizeof(byte);
    msgs[0].data   = msgs[1].data   = &byte;
    msgs[0].flags = 0;
    msgs[1].flags = I2C_MSG_READ;
    if (switch_state(&back_switch))
        i2c_master_xfer(i2c, msgs, 2, 1);
    return byte;
}


static void
accel_wakeup(void)
{
    /* Set the mode to "measurement", measuring 2g */
    accel_write(0x16, 0x04 | 0x01);
}

static int
accel_ready(void)
{
    return (accel_read(0x09) & 1);
}

static int
accel_sleep(void)
{
    return accel_write(0x16, 0);
}


uint8
accel_read_state(int *x, int *y, int *z)
{
    struct i2c_msg msgs[2];
    signed char values[6];
    uint8 addr = 0x00; /* 10-bits read value */
    uint8 result = 0;

    accel_wakeup();
    while (!accel_ready())
        delay_us(1000);

    msgs[0].addr   = ACCEL_ADDR;
    msgs[0].length = sizeof(byte);
    msgs[0].data   = &addr;
    msgs[0].flags  = 0;

    msgs[1].addr   = ACCEL_ADDR;
    msgs[1].length = sizeof(values);
    msgs[1].data   = (uint8 *)values;
    msgs[1].flags  = I2C_MSG_READ;

    if (switch_state(&back_switch))
        result = i2c_master_xfer(i2c, msgs, 2, 1);

    if (x)
        *x = (values[1]<<2) | (values[0]);
    if (y)
        *y = (values[3]<<2) | (values[2]);
    if (z)
        *z = (values[5]<<2) | (values[4]);

    accel_sleep();

    return result;
}



static int
accel_init(void)
{
    return 0;
}


static int
accel_resume(struct device *dev)
{
    return 0;
}


static int
accel_suspend(struct device *dev) {
    /* Set the "mode" to "Standby" */
//#if WAKEUP_PATCHED
//#warning Need hardware patch to get this working
//    Serial1.println( "Sleeping MMA7455\n" );
    accel_write(0x16, 0);
//#endif
    return 0;
}


static int
accel_deinit(struct device *dev)
{
//    Serial1.println("De-init accelerometer");
    accel_write(0x16, 0);
    return 0;
}


struct device accel = {
    accel_init,
    accel_deinit,
    accel_suspend,
    accel_resume,

    "Accelerometer",
};
