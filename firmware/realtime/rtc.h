/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
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

#ifndef _RTC_H_
#define _RTC_H_

#include "libmaple_types.h"
#include "rcc.h"
#include "nvic.h"
#include "gpio.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int rtc_alarm_on;

/*
 * Register maps
 */

/** RTC register map type. */
typedef struct rtc_reg_map {
    __io uint32 CRH;            /**< Control register high */
    __io uint32 CRL;            /**< Control register low */
    __io uint32 PRLH;           /**< Prescaler load register high */
    __io uint32 PRLL;           /**< Prescaler load register low */
    __io uint32 DIVH;           /**< Prescaler divider register high */
    __io uint32 DIVL;           /**< Prescaler divider register low */
    __io uint32 CNTH;           /**< Counter register high */
    __io uint32 CNTL;           /**< Counter register low */
    __io uint32 ALRH;           /**< Alarm register high */
    __io uint32 ALRL;           /**< Alarm register low */
} rtc_reg_map;

/** RTC register map base pointer */
#define RTC_BASE                       ((struct rtc_reg_map*)0x40002800)

/*
 * Register bit definitions
 */

/* Control register high */

#define RTC_CRH_OWIE_BIT                2
#define RTC_CRH_ALRIE_BIT               1
#define RTC_CRH_SECIE_BIT               0

#define RTC_CRH_OWIE                    BIT(RTC_CRH_OWIE_BIT)
#define RTC_CRH_ALRIE                   BIT(RTC_CRH_ALRIE_BIT)
#define RTC_CRH_SECIE                   BIT(RTC_CRH_SECIE_BIT)

/* Control register low */

#define RTC_CRL_RTOFF_BIT               5
#define RTC_CRL_CNF_BIT                 4
#define RTC_CRL_RSF_BIT                 3
#define RTC_CRL_OWF_BIT                 2
#define RTC_CRL_ALRF_BIT                1
#define RTC_CRL_SECF_BIT                0

#define RTC_CRL_RTOFF                   BIT(RTC_CRL_RTOFF_BIT)
#define RTC_CRL_CNF                     BIT(RTC_CRL_CNF_BIT)
#define RTC_CRL_RSF                     BIT(RTC_CRL_RSF_BIT)
#define RTC_CRL_OWF                     BIT(RTC_CRL_OWF_BIT)
#define RTC_CRL_ALRF                    BIT(RTC_CRL_ALRF_BIT)
#define RTC_CRL_SECF                    BIT(RTC_CRL_SECF_BIT)


/*
 * Devices
 */

/** RTC device type */
typedef struct rtc_dev {
    rtc_reg_map *regs;          /**< Register map */
    rcc_clk_id clk_id;          /**< RCC clock information */
    nvic_irq_num irq_num;       /**< NVIC interrupt number */
} rtc_dev;

extern rtc_dev *RTC;

/*
 * RTC Convenience functions
 */

void rtc_init(rtc_dev *dev);


void rtc_set_time(rtc_dev *dev, uint32 time);
uint32 rtc_get_time(rtc_dev *dev);
int rtc_set_alarm(rtc_dev *dev,uint32 time);
int rtc_enable_alarm(rtc_dev *dev);

void __irq_rtcalarm(void);
void __irq_rtc(void);

int rtc_alarmed();
void rtc_clear_alarmed();
void rtc_set_alarmed();


#ifdef __cplusplus
}
#endif

#endif
