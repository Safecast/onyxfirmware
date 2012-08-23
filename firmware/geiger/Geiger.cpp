#include "gpio.h"
#include "exti.h"
#include "Geiger.h"
#include "power.h"
#include "RealTime.h"
#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"
#include "display.h"

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5
#define MAX_RELOAD ((1 << 16) - 1)

using namespace std;

uint32_t current_count;


Geiger *system_geiger;


void static geiger_min_log(void) {
  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,1);
  delay_us(100);
  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,0);
  system_geiger->update_last_min();
}

void static geiger_rising(void) {

  // for now, set to defaults but may want to lower clock rate so we're not burning battery
  // to run a CPU just while the buzzer does its thing
  //  rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9);
  //  rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
  //  rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
  //  rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);

  current_count++;
}

Geiger::Geiger() {
}

void Geiger::initialise() {

  system_geiger = this;
  for(uint32_t n=0;n<(60*COUNTS_PER_SECOND);n++) {
    last_min[n] = 0;
  }
  last_min_position=0;
  
  for(uint32_t n=0;n<120;n++) {
    cpm_last_min[n] = 0;
  }

  averaging_period = 60;
  current_count =0;
//  geiger_count = 0;
  AFIO_BASE->MAPR |= 0x02000000; // turn off JTAG pin sharing

  gpio_set_mode(PIN_MAP[GEIGER_ON_GPIO].gpio_device,PIN_MAP[GEIGER_ON_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[GEIGER_ON_GPIO].gpio_device,PIN_MAP[GEIGER_ON_GPIO].gpio_bit,1);
  delay_us(1000); // 1 ms for the geiger to settle

  gpio_set_mode(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,GPIO_INPUT_PD);

  int bit = gpio_read_bit(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit);
  if(bit) geiger_rising();
  exti_attach_interrupt((afio_exti_num)PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,
			gpio_exti_port(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device),geiger_rising,EXTI_RISING);

  // initialise timer
  timer_pause(TIMER4);
  //TODO: fix this so it uses both prescaler and reload...
  timer_set_prescaler(TIMER4,((500000*CYCLES_PER_MICROSECOND)/MAX_RELOAD));
  timer_set_reload(TIMER4,MAX_RELOAD);

  // setup interrupt on channel 4
  timer_set_mode(TIMER4,TIMER_CH4,TIMER_OUTPUT_COMPARE);
  timer_set_compare(TIMER4,TIMER_CH4,1);
  timer_attach_interrupt(TIMER4,TIMER_CH4,geiger_min_log);

  timer_generate_update(TIMER4); // refresh timer count, prescale, overflow

  timer_resume(TIMER4);
}


void Geiger::update_last_min() {
  last_min[last_min_position] = current_count;
  current_count=0;
  last_min_position++;
  if(last_min_position >= COUNTS_PER_MIN) last_min_position=0;
}

float Geiger::get_cpm() {

  float sum = 0;

  int32_t c_position = last_min_position-1;
  for(uint32_t n=0;n<averaging_period;n++) {
   
    sum += last_min[c_position];
 
    c_position--;
    if(c_position < 0) c_position = COUNTS_PER_MIN-1;
  }
  return (sum/((float)averaging_period))*((float)COUNTS_PER_MIN);
}

float Geiger::get_cpm_deadtime_compensated() {
  float cpm = get_cpm();
  float deadtime_us = cpm*40;
  return (cpm/((60*1000000)-deadtime_us))*(60*1000000);
}

float Geiger::get_microseiverts() {
  float conversionCoefficient = 0.0029;
  return get_cpm_deadtime_compensated() * conversionCoefficient;
}

float *Geiger::get_cpm_last_min() {

  float cpm_last_min_temp[120];
  int32_t c_position = last_min_position; // next value, i.e. oldest
  if(c_position > 120) return cpm_last_min;

  for(uint32_t n=0;n<COUNTS_PER_MIN;n++) {
    cpm_last_min_temp[n] = last_min[c_position];
    c_position++;
    if(c_position >= COUNTS_PER_MIN) c_position=0;
  }

  int32_t sum=0;
  for(uint32_t n=0;n<averaging_period;n++) {
    sum += cpm_last_min_temp[n];
    cpm_last_min[n]=0;
  }

  for(uint32_t n=averaging_period;n<COUNTS_PER_MIN;n++) {
    sum -= cpm_last_min_temp[n-averaging_period];
    sum += cpm_last_min_temp[n];
    cpm_last_min[n] = ((float)sum/(float)averaging_period)*120;
   // if(cpm_last_min[n] > 80) cpm_last_min[n]=0;
  }

  return cpm_last_min;
}

void Geiger::powerup  () {}
void Geiger::powerdown() {}
