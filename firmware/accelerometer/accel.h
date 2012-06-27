#ifndef __ACCEL_H__
#define __ACCEL_H__

#include "device.h"

uint8 accel_read_state(int *x, int *y, int *z);

extern struct device accel;

#endif /* __ACCEL_H__ */
