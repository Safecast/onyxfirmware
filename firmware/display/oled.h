/******************************************************************************
 *
 * $RCSfile$
 * $Revision: 124 $
 *
 * This module provides the interface definitions for setting up and
 * controlling the various interrupt modes present on the ARM processor.
 * Copyright 2004, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 *
 *****************************************************************************/
#ifndef OLED_H
#define OLED_H

#include "spi.h"
#include <stdint.h>

//#define SSD1339
#define SSD1351		// for newer screens with 1351 gdd

#ifdef SSD1339
// data bus for LCD, pins on port 0
#define D0 16
#define D1 17
#define D2 18
#define D3 19
#define D4 20
#define D5 21
#define D6 22
#define D7 23
#define D8 24

// OLED data port
#define LCD_DATA ((1<<D0)|(1<<D1)|(1<<D2)|(1<<D3)|(1<<D4)|(1<<D5)|(1<<D6)|(1<<D7)|(1<<D8))

// other OLED pins
#define LCD_RD    (1<<25)			// P1.25
#define LCD_RW    (1<<26)			// P1.26
#define LCD_CS    (1<<27)			// P1.27
#define LCD_DC    (1<<28)			// P1.28
#define LCD_RSTB  (1<<29)			// P1.29

#define BS1			(1<<30)			// P1.30
#define BS2			(1<<31)			// P1.31

//==========================================
//===========If Using new OLED==============
//==========================================
#else
// data bus for LCD, pins on port 0
#define D0 16
#define D1 17
#define D2 18
#define D3 19
#define D4 20
#define D5 21
#define D6 22
#define D7 23

// OLED data port
#define LCD_DATA ((1<<D0)|(1<<D1)|(1<<D2)|(1<<D3)|(1<<D4)|(1<<D5)|(1<<D6)|(1<<D7))

// other OLED pins
#define LCD_RD    (1<<25)			// P1.25
#define LCD_RW    (1<<26)			// P1.26
#define LCD_CS    (1<<27)			// P1.27
#define LCD_DC    (1<<28)			// P1.28
#define LCD_RSTB  (1<<29)			// P1.29

#define BS0			(1<<30)			// P1.30
#define BS1			(1<<31)			// P1.31
#endif

#define OLED_END 	(unsigned int)127

#define MAX_VARIABLES		11
#define NUM_MEASUREMENTS	4

// convenience macros for OLED
#define RGB16(r, b, g) ((((r)<<11L)&0x1fL) | (((g)<<5L)&0x3fL) | (((b)<<0L)&0x1fL))

// inialize OLED
void oled_init(void);
void oled_deinit(void);
void oled_TurnOn(void);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t *data);

void oled_blank(void);
void oled_unblank(void);

#endif
