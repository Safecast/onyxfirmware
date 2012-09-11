/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung, Sean Cross, Nava Whiteford
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
 * @file rtc.c
 * @author Sean Cross <xobs@kosagi.com>
 * @brief Real Time Clock (RTC) support.
 */

#include "rtc.h"
#include "rcc.h"
#include "bkp.h"

int rtc_alarm_on;

/*
 * RTC device
 */

static rtc_dev rtc = {
    RTC_BASE,
    (rcc_clk_id) 0,
    NVIC_RTC
};
rtc_dev *RTC = &rtc;

void rtoff_wait(rtc_dev *dev) {
  dev->regs->CRL &= (uint16)~RTC_CRL_RSF;
  while(!(dev->regs->CRL & RTC_CRL_RTOFF));
  //for(n=0;(n<=100000) && ((dev->regs->CRL & RTC_CRL_RTOFF) == 0);n++); if(n>=100000) return 0;
}

void rtc_sync(rtc_dev *dev) {

  rtoff_wait(dev);

  /* Clear RSF flag */
  dev->regs->CRL &= (uint16)~RTC_CRL_RSF ;

  /* Loop until RSF flag is set */
  while((dev->regs->CRL & RTC_CRL_RSF) == 0) {
    int n=0;
    for(n=0;n<100000;n++);
  }

}

int rtc_alarmed() {

  return RTC->regs->CRL & RTC_CRL_ALRF;
}

/*
 * RTC convenience routines
 */

/**
 * @brief Initialize the RTC
 * @param dev Device to initialize and reset.
 */
void rtc_init(rtc_dev *dev) {
    //RCC_BASE->BDCR &= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE;
    RCC_BASE->APB1ENR |= RCC_APB1ENR_PWREN;

    /* Disable Backup Write Protection so we can modify RTC bits */
    bkp_enable_writes();

    RCC_BASE->BDCR |= RCC_BDCR_LSEON;
    while (!(RCC_BASE->BDCR & RCC_BDCR_LSERDY));

    /* Enable the RTC */
    RCC_BASE->BDCR |= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE;

    /* Wait for it to stabilize and setup RTC for config */
    rtoff_wait(dev);
    dev->regs->CRL |= RTC_CRL_CNF;

    /* Set the 32 kHz oscillator to have a divider of 32768 */
    dev->regs->PRLH = 0x0;
    dev->regs->PRLL = 0x7fff;

    /* Flush the config to the analog domain and wait for sync */
    dev->regs->CRL &= ~RTC_CRL_CNF;
    rtoff_wait(dev);

    /* Disable writes to the backup region */
    bkp_disable_writes();
}


// this int is not called.
void __irq_rtcalarm(void) {
  rtc_alarm_on=1;
}

// This int is called
void __irq_rtc(void) {
  nvic_clear_pending_msk(NVIC_RTC);
  nvic_irq_disable(NVIC_RTC);
  rtc_alarm_on=1;
 // gpio_write_bit(GPIOD,2,1);
 // gpio_write_bit(GPIOD,2,0);
}


int rtc_set_alarm(rtc_dev *dev, uint32 time) {

    bkp_enable_writes();

    /* Wait for RTC writes to stabilize */
    rtoff_wait(dev);
    dev->regs->CRL |= RTC_CRL_CNF;
    rtoff_wait(dev);

  //  dev->regs->CNTH = (time >> 16);
  //  dev->regs->CNTL = (time & 0xffff);
  dev->regs->ALRH = (uint32) 0;
  dev->regs->ALRL = (uint32) 5;//(uint16) (time & 0x00FF);

    /* Flush the config to the analog domain and wait for sync */
    dev->regs->CRL &= ~RTC_CRL_CNF;
    rtoff_wait(dev);

    /* Disable writes to the backup region */
    bkp_disable_writes();


/*



  bkp_enable_writes();

  rtc_sync(dev);
  int n=0;
  // set the CNF bit in the RTC_CRL register.
  rtoff_wait(dev);
  for(n=0;(n<=100000) && ((dev->regs->CRL & RTC_CRL_CNF) == 0);n++) dev->regs->CRL |= RTC_CRL_CNF;
  if(n>=100000) return 0;

  //dev->regs->ALRH = (uint16) (time >> 16);
  //dev->regs->ALRL = (uint16) (time & 0x00FF);
  rtoff_wait(dev);
  dev->regs->ALRL = (uint32) 5;//(uint16) (time & 0x00FF);
  rtoff_wait(dev);
  dev->regs->ALRH = (uint32) 0;
  rtoff_wait(dev);

  rtoff_wait(dev);
  dev->regs->CRL &= ~RTC_CRL_CNF;
  rtoff_wait(dev);

  bkp_disable_writes();
*/
  return 1;
}

int rtc_enable_alarm(rtc_dev *dev) {

  rtc_sync(dev);
  bkp_enable_writes();
  // enable in nvic
////  nvic_irq_enable(NVIC_RTCALARM); // 41
  nvic_irq_enable(NVIC_RTC); // 3
//    NVIC->ISER[0] |= (1 << (RTC_IRQChannel & 0x1F));            // enable interrupt


    rtoff_wait(dev);
    dev->regs->CRL |= RTC_CRL_CNF;
    rtoff_wait(dev);

  // set can fail, only retry 100000 times.

  rtoff_wait(dev);
//  for(n=0;((dev->regs->CRH & 2) == 0) && (n <= 100000);n++) dev->regs->CRH = dev->regs->CRH | 2; 
  dev->regs->CRH = 0x2;
 // if(n>=100000) return 0;
  rtoff_wait(dev);
    dev->regs->CRL &= ~RTC_CRL_CNF;
    rtoff_wait(dev);

  bkp_disable_writes();
  return 1;
}


void rtc_set_time(rtc_dev *dev, uint32 time) {
    bkp_enable_writes();

    /* Wait for RTC writes to stabilize */
    rtoff_wait(dev);
    dev->regs->CRL |= RTC_CRL_CNF;

    dev->regs->CNTH = (time >> 16);
    dev->regs->CNTL = (time & 0xffff);

    /* Flush the config to the analog domain and wait for sync */
    dev->regs->CRL &= ~RTC_CRL_CNF;
    rtoff_wait(dev);

    /* Disable writes to the backup region */
    bkp_disable_writes();
}

uint32 rtc_get_time(rtc_dev *dev) {
    return ((dev->regs->CNTH << 16) & 0xffff0000) | ((dev->regs->CNTL & 0xffff));
}

