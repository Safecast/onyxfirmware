#include "gpio.h"
#include "exti.h"
#include "Buzzer.h"
#include "Led.h"
#include "power.h"

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5

void static geiger_rising(void) {

  // for now, set to defaults but may want to lower clock rate so we're not burning battery
  // to run a CPU just while the buzzer does its thing
  rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9);
  rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
  rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
  rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);

  //delay_us(100);
  //Serial1.println("beep.");
  // buzz manually
  for(int i=0;i<1;i++) {
    for(int n=0;n<1000;n++) {
      gpio_toggle_bit(GPIOB,9);
      delay_us(1000);
    }
    delay_us(100000);
  }
}

class Geiger {

public:

  Geiger() {
  }

  void initialise() {

    AFIO_BASE->MAPR |= 0x02000000; // turn off JTAG pin sharing

    gpio_set_mode(PIN_MAP[GEIGER_ON_GPIO].gpio_device,PIN_MAP[GEIGER_ON_GPIO].gpio_bit,GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[GEIGER_ON_GPIO].gpio_device,PIN_MAP[GEIGER_ON_GPIO].gpio_bit,1);
    delay_us(1000); // 1 ms for the geiger to settle

    gpio_set_mode(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,GPIO_INPUT_PD);



    int bit = gpio_read_bit(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit);
    if(bit) geiger_rising();
    exti_attach_interrupt((afio_exti_num)PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,
                          gpio_exti_port(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device),geiger_rising,EXTI_RISING);
  }

  void powerup  () {}
  void powerdown() {}
};
