#ifndef __ACCEL_H__
#define __ACCEL_H__

//#include "device.h"

uint8 accel_read_state(int16 *x, int16 *y, int16 *z);

void accel_init();

//extern struct device accel;

#endif /* __ACCEL_H__ */
