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

//#include "io.h"
#include "spi.h"
//#include "boards.h"
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
//#define GRAPH_START	(unsigned int)45
//#define GRAPH_END	(unsigned int)115
//#define GRAPH_RIGHT	(unsigned int)127
//#define GRAPH_LEFT	(unsigned int)0
//#define TIME_HEIGHT	(unsigned int)120
//#define TEXT_OFFSET	(unsigned int)5
//#define TEXT_SPACING	(unsigned int)4
//#define VAL_OFFSET	(unsigned int)90
//#define TEXT_HEIGHT	(unsigned int)45


#define MAX_VARIABLES		11
#define NUM_MEASUREMENTS	4

//#define RST_INDEX			4
//#define PONG_INDEX			5
//#define CUBE_INDEX			6
//#define GRAPH_INDEX			7
//#define LOG_INDEX			8
//#define CLOCK_INDEX			9
//#define PWR_INDEX			10


//#define DAY_SIZE		(unsigned char)24
//#define WEEK_SIZE		(unsigned char)42
//#define MONTH_SIZE		(unsigned char)32
//#define YEAR_SIZE		(unsigned char)52


//#define DOWN		0
//#define UP			1

//#define PADDLE_THICKNESS	(int)0x02
//#define PADDLE_HEIGHT		(int)0x18
//#define PADDLE_X_OFFSET		(int)0x02
//#define PADDLE_START		((int)OLED_END-PADDLE_THICKNESS)/2
//#define BALL_X_START		(int)60
//#define BALL_Y_START		(int)60
//#define BALL_THICKNESS		(int)5

//#define PONG_TOP			(int)15
//#define PONG_SCORE_TOP		(int)(PONG_TOP - 12)

//#define COMPUTER	0
//#define PLAYER		1

//#define PLAYER_MAX_SPD		(int)0x07
//#define COMPUTER_MAX_SPD	(int)0x05
//#define BALL_MAX_SPD		(int)0x0B

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
