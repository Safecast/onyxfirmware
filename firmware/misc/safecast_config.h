/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file   safecast.h
 * @author Marti Bolivar <mbolivar@leaflabs.com>
 * @brief  Private include file for Maple in boards.h
 */

#ifndef _BOARD_SAFECAST_H_
#define _BOARD_SAFECAST_H_

#include "safecast_wirish_types.h"

extern const stm32_pin_info PIN_MAP[];
extern const uint8 boardUsedPins[];


#define CYCLES_PER_MICROSECOND  36
#define SYSTICK_RELOAD_VAL      35999 /* takes a cycle to reload */

/**
 *  Use of timers in our firmware:
 *
 *  TIMER1: Unused ?
 *  TIMER2: Buzzer
 *  TIMER3: Geiger output pulse (mic port)
 *  TIMER4: Geiger wait for pulse timeout
 *  TIMER5: Unused ?
 *  TIMER6: Unused ?
 *  TIMER7: Modem
 *  TIMER8: Unused ?
 */




////////////////////
// How our peripherals are connected to the MCU: this is the index
// of the pin definition in  PIN_MAP (defined in safecast_config.cpp)
//
// We need to use those macros rather than direct indexes everywhere in
// the code.

////////////////////

#define BOARD_LED_PIN           25  // PD2
#define MIC_IPHONE              35  // PC6
#define MIC_REVERSE             36  // PC7
#define HP_COMBINED             12  // PA6
#define GEIGER_PULSE_GPIO       42 // PB3
#define GEIGER_ON_GPIO           4  // PB5

#define BUZZER_PWM              24 // PB9

#define MANUAL_WAKEUP_GPIO      18 // PC3
#define CHG_STAT2_GPIO          44 // PC11
#define CHG_STAT1_GPIO          26 // PC10
#define MAGPOWER_GPIO           41 // PA15
#define MEASURE_FET_GPIO        45 // PC12
#define BATT_MEASURE_ADC        28 // PB1
#define MAGSENSE_GPIO           29 // PB10
#define LIMIT_VREF_DAC          10 // PA4 -- should be DAC eventually, but GPIO initially to tied own
#define CHG_TIMEREN_N_GPIO      37 // PC8
#define LED_PWR_ENA_GPIO        16 // PC1 // handled in OLED platform_init
#define WAKEUP_GPIO              2 // PA0

#define LCD_DC_GPIO  31
#define LCD_CS_GPIO  33
#define LCD_PWR_GPIO 16
#define LCD_RES_GPIO 17


/* Number of USARTs/UARTs whose pins are broken out to headers */
#define BOARD_NR_USARTS         2

/* Default USART pin numbers (not considering AFIO remap) */
#define BOARD_USART1_TX_PIN     7
#define BOARD_USART1_RX_PIN     8
#define BOARD_USART2_TX_PIN     1
#define BOARD_USART2_RX_PIN     0

/* Number of SPI ports */
#define BOARD_NR_SPI            0

#define LCD_SPI      SPI2

/* Default SPI pin numbers (not considering AFIO remap) */
#define BOARD_SPI1_NSS_PIN      10
#define BOARD_SPI1_MOSI_PIN     11
#define BOARD_SPI1_MISO_PIN     12
#define BOARD_SPI1_SCK_PIN      13
#define BOARD_SPI2_NSS_PIN      31
#define BOARD_SPI2_MOSI_PIN     34
#define BOARD_SPI2_MISO_PIN     33
#define BOARD_SPI2_SCK_PIN      32

/* Total number of GPIO pins that are broken out to headers and
 * intended for general use. */
#define BOARD_NR_GPIO_PINS      48

/* Number of pins capable of PWM output */
#define BOARD_NR_PWM_PINS       0

/* Number of pins capable of ADC conversion */
#define BOARD_NR_ADC_PINS       15

/* Number of pins already connected to external hardware.  For Maple,
 * these are just BOARD_LED_PIN and BOARD_BUTTON_PIN. */
#define BOARD_NR_USED_PINS       1

/* Debug port pins */
#define BOARD_JTMS_SWDIO_PIN    39
#define BOARD_JTCK_SWCLK_PIN    40
#define BOARD_JTDI_PIN          41
#define BOARD_JTDO_PIN          42
#define BOARD_NJTRST_PIN        43

/* USB configuration.  BOARD_USB_DISC_DEV is the GPIO port containing
 * the USB_DISC pin, and BOARD_USB_DISC_BIT is that pin's bit. */
#define BOARD_USB_DISC_DEV      GPIOC
#define BOARD_USB_DISC_BIT      12

#define ERROR_USART            USART1
#define ERROR_USART_CLK_SPEED  STM32_PCLK2
#define ERROR_USART_BAUD       115200
#define ERROR_TX_PORT          GPIOA
#define ERROR_TX_PIN           7

#endif
