#include "gpio.h"
#include "exti.h"
#include "Buzzer.h"
#include "power.h"
#include "RealTime.h"

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5

using namespace std;

uint32_t geiger_count;
uint32_t pulses[1000];
size_t   pulses_size;

void static geiger_rising(void) {

  // for now, set to defaults but may want to lower clock rate so we're not burning battery
  // to run a CPU just while the buzzer does its thing
//  rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9);
//  rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
//  rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
//  rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);

  // buzz manually
  //for(int n=0;n<10;n++) {
  //  gpio_toggle_bit(GPIOB,9);
  //  delay_us(1000);
  //}
  //gpio_write_bit(GPIOB,9,0);
  RealTime r;
  pulses[pulses_size] = r.get_time();
  pulses_size++;
  geiger_count++;
}

class Geiger {

public:

  Geiger() {
  }

  void initialise() {

    pulses_size=-0;

    geiger_count = 0;
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

  uint32_t get_count() {
    return geiger_count;
  }

  float get_cpm() {
    int pulse_count=0;
    bool first=true;
    uint32_t one_min_ago = 0;
    if(time_get() > 60) one_min_ago=time_get()-60;

    for(size_t n=0;n<pulses_size;n++) {
      if(pulses[n] >= one_min_ago) {
        pulse_count++;
        first=false;
      }
    }

    // erase pulses more than 30mins old
    uint32_t thirty_mins_ago = 0;
    size_t first_pulse=0;
    if(time_get() > (60*30)) thirty_mins_ago=time_get()-(60*30);
    for(size_t n=0;n<pulses_size;n++) {
      if(pulses[n] >= thirty_mins_ago) {
        if(first) first_pulse=n;
        first=false;
      }
    }
    if(first_pulse > 500) {
      size_t i=0;
      for(size_t n=first_pulse;n<pulses_size;n++) {
        pulses[i] = pulses[n];
        i++;
      }
      pulses_size = i+1;
    }

    return pulse_count;
  }

  float cpm_last_30min[30];
  float *get_cpm_last_30mins() {

    for(size_t n=0;n<30;n++) {
      cpm_last_30min[n] = 0;
    }

    uint32_t thirty_mins_ago = 0;
    if(time_get() > 60) thirty_mins_ago=time_get()-(60*30);

    for(size_t n=0;n<pulses_size;n++) {
      if(pulses[n] > thirty_mins_ago) {
        cpm_last_30min[((pulses[n]-thirty_mins_ago)/60)]++;
      }
    }
    return cpm_last_30min;
  }

  void powerup  () {}
  void powerdown() {}
};
