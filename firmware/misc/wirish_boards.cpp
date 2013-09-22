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

/**
 * @brief Generic board initialization routines.
 *
 * By default, we bring up all Maple boards to 72MHz, clocked off the
 * PLL, driven by the 8MHz external crystal. AHB and APB2 are clocked
 * at 72MHz.  APB1 is clocked at 36MHz.
 */

#include "wirish_boards.h"
#include "safecast_config.h"

#include "flash.h"
#include "rcc.h"
#include "nvic.h"
#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include "power.h"
//#include "usb_cdcacm.h"

static void setupFlash(void);
static void setupClocks(void);
static void setupNVIC(void);
static void setupADC(void);
static void setupTimers(void);

// short_init is used by safecast to do a quick power-on
// it's special cased to shorten the "ON" duty-cycle
void short_init(void) {
    setupFlash();
    setupClocks();
    setupNVIC();
    setupTimers();
}

void init(void) {
    setupFlash();
// ok
    setupClocks();
// ok
    setupNVIC();
// ok
    systick_init(SYSTICK_RELOAD_VAL);
// ok
    gpio_init_all();
// ok
    afio_init();
// ok
    setupADC();
// adcs increase mA!
    setupTimers();
}


static void setupFlash(void) {
    flash_enable_prefetch();
    flash_set_latency(FLASH_WAIT_STATE_2);
}

/*
 * Clock setup.  Note that some of this only takes effect if we're
 * running bare metal and the bootloader hasn't done it for us
 * already.
 *
 * If you change this function, you MUST change the file-level Doxygen
 * comment above.
 */
static void setupClocks() {
    rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9);
    rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB1_HCLK_DIV_1);
    rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);
}

static void setupNVIC() {
#ifdef VECT_TAB_FLASH
    nvic_init(USER_ADDR_ROM, 0);
#elif defined VECT_TAB_RAM
    nvic_init(USER_ADDR_RAM, 0);
#elif defined VECT_TAB_BASE
    nvic_init((uint32)0x08000000, 0);
#else
#error "You must select a base address for the vector table."
#endif
}

static void adcDefaultConfig(const adc_dev* dev);

static void setupADC() {
    rcc_set_prescaler(RCC_PRESCALER_ADC, RCC_ADCPRE_PCLK_DIV_6);
    // ok
    adc_foreach(adcDefaultConfig);
}

static void timerDefaultConfig(timer_dev*);

static void setupTimers() {
    timer_foreach(timerDefaultConfig);
}

static void adcDefaultConfig(const adc_dev *dev) {
    adc_init(dev);
    // ok

    adc_set_extsel(dev, ADC_SWSTART);
    // ok
    adc_set_exttrig(dev, true);
    // ok

    adc_enable(dev);
    // up to 1mA
    adc_calibrate(dev);
    // stick at 1mA
    adc_set_sample_rate(dev, ADC_SMPR_55_5);
    // stick at 1mA
}

static void timerDefaultConfig(timer_dev *dev) {
    timer_adv_reg_map *regs = (dev->regs).adv;
    const uint16 full_overflow = 0xFFFF;
    const uint16 half_duty = 0x8FFF;

    timer_init(dev);
    timer_pause(dev);

    regs->CR1 = TIMER_CR1_ARPE;
    regs->PSC = 1;
    regs->SR = 0;
    regs->DIER = 0;
    regs->EGR = TIMER_EGR_UG;

    switch (dev->type) {
    case TIMER_ADVANCED:
        regs->BDTR = TIMER_BDTR_MOE | TIMER_BDTR_LOCK_OFF;
        // fall-through
    case TIMER_GENERAL:
        timer_set_reload(dev, full_overflow);

        for (int channel = 1; channel <= 4; channel++) {
            timer_set_compare(dev, channel, half_duty);
            timer_oc_set_mode(dev, channel, TIMER_OC_MODE_PWM_1, TIMER_OC_PE);
        }
        // fall-through
    case TIMER_BASIC:
        break;
    }

    timer_resume(dev);
}
