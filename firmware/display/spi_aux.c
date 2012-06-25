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
 * @author Marti Bolivar <mbolivar@leaflabs.com>
 * @brief Wirish SPI implementation.
 */

#include "safecast_config.h"
#include "spi_aux.h"
#include "timer.h"
#include "util.h"
#include "rcc.h"
#include "safecast_wirish_types.h"
//#include "wirish.h"
//#include "boards.h"

static void configure_gpios(spi_dev *dev, bool as_master);
static spi_baud_rate determine_baud_rate(spi_dev *dev, SPIFrequency freq);

void spi_aux_write(spi_dev *spi_device, const uint8 *data, uint32 length) {
    uint32 txed = 0;
    while (txed < length) {
        txed += spi_tx(spi_device, data + txed, length - txed);
    }
}

/* Enables the device in master or slave full duplex mode.  If you
 * change this code, you must ensure that appropriate changes are made
 * to HardwareSPI::end(). */
void spi_aux_enable_device(spi_dev *dev,
                          bool as_master,
                          SPIFrequency freq,
                          spi_cfg_flag endianness,
                          spi_mode mode) {
    spi_baud_rate baud = determine_baud_rate(dev, freq);
    uint32 cfg_flags = (endianness | SPI_DFF_8_BIT | SPI_SW_SLAVE |
                        (as_master ? SPI_SOFT_SS : 0));

    spi_init(dev);
    configure_gpios(dev, as_master);
    if (as_master) {
        spi_master_enable(dev, baud, mode, cfg_flags);
    } else {
        spi_slave_enable(dev, mode, cfg_flags);
    }
}


//TODO: if merged back to libmaple, this will need fixing for different board types.
static const struct spi_pins board_spi_pins[2] __attr_flash = {
    {BOARD_SPI1_NSS_PIN,
     BOARD_SPI1_SCK_PIN,
     BOARD_SPI1_MISO_PIN,
     BOARD_SPI1_MOSI_PIN},
    {BOARD_SPI2_NSS_PIN,
     BOARD_SPI2_SCK_PIN,
     BOARD_SPI2_MISO_PIN,
     BOARD_SPI2_MOSI_PIN},
//    {BOARD_SPI3_NSS_PIN,
//     BOARD_SPI3_SCK_PIN,
//     BOARD_SPI3_MISO_PIN,
//     BOARD_SPI3_MOSI_PIN},
};



/*
 * Auxiliary functions
 */


static const struct spi_pins* dev_to_spi_pins(spi_dev *dev) {
    switch (dev->clk_id) {
    case RCC_SPI1: return board_spi_pins;
    case RCC_SPI2: return board_spi_pins + 1;
#ifdef STM32_HIGH_DENSITY
    case RCC_SPI3: return board_spi_pins + 2;
#endif
    default:       return NULL;
    }
}


static void disable_pwm(const struct stm32_pin_info *i) {
    if (i->timer_device) {
        timer_set_mode(i->timer_device, i->timer_channel, TIMER_DISABLED);
    }
}

static void configure_gpios(spi_dev *dev, bool as_master) {
    const struct spi_pins *pins = dev_to_spi_pins(dev);

    if (!pins) {
        return;
    }

    const stm32_pin_info *nssi = &PIN_MAP[pins->nss];
    const stm32_pin_info *scki = &PIN_MAP[pins->sck];
    const stm32_pin_info *misoi = &PIN_MAP[pins->miso];
    const stm32_pin_info *mosii = &PIN_MAP[pins->mosi];

    disable_pwm(nssi);
    disable_pwm(scki);
    disable_pwm(misoi);
    disable_pwm(mosii);

    spi_gpio_cfg(as_master,
                 nssi->gpio_device,
                 nssi->gpio_bit,
                 scki->gpio_device,
                 scki->gpio_bit,
                 misoi->gpio_bit,
                 mosii->gpio_bit);
}

static const spi_baud_rate baud_rates[MAX_SPI_FREQS] __FLASH__ = {
    SPI_BAUD_PCLK_DIV_2,
    SPI_BAUD_PCLK_DIV_4,
    SPI_BAUD_PCLK_DIV_8,
    SPI_BAUD_PCLK_DIV_16,
    SPI_BAUD_PCLK_DIV_32,
    SPI_BAUD_PCLK_DIV_64,
    SPI_BAUD_PCLK_DIV_128,
    SPI_BAUD_PCLK_DIV_256,
};

/*
 * Note: This assumes you're on a LeafLabs-style board
 * (CYCLES_PER_MICROSECOND == 72, APB2 at 72MHz, APB1 at 36MHz).
 */
static spi_baud_rate determine_baud_rate(spi_dev *dev, SPIFrequency freq) {
    if (rcc_dev_clk(dev->clk_id) == RCC_APB2 && freq == SPI_140_625KHZ) {
        /* APB2 peripherals are too fast for 140.625 KHz */
        ASSERT(0);
        return (spi_baud_rate)~0;
    }
    return (rcc_dev_clk(dev->clk_id) == RCC_APB2 ?
            baud_rates[freq] :
            baud_rates[freq-1]);
}
