#include "gpio.h"
#include "exti.h"
#include "Geiger.h"
#include "power.h"
#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"
#include "display.h"
#include "utils.h"
#include "dac.h"
#include <stdlib.h>
#include "flashstorage.h"
#include <stdio.h>

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5
#define MAX_RELOAD ((1 << 16) - 1)

using namespace std;

uint32_t current_count;


Geiger *system_geiger;


void static geiger_min_log(void) {
  system_geiger->update_last_min();
}

void pulse_output_end(void) {
  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,0);
  dac_write_channel(DAC,2,0);
  timer_pause(TIMER3);
}

void static geiger_rising(void) {

  // for now, set to defaults but may want to lower clock rate so we're not burning battery
  // to run a CPU just while the buzzer does its thing
  //  rcc_clk_init(RCC_CLKSRC_PLL, RCC_PLLSRC_HSI_DIV_2, RCC_PLLMUL_9);
  //  rcc_set_prescaler(RCC_PRESCALER_AHB, RCC_AHB_SYSCLK_DIV_1);
  //  rcc_set_prescaler(RCC_PRESCALER_APB1, RCC_APB2_HCLK_DIV_1);
  //  rcc_set_prescaler(RCC_PRESCALER_APB2, RCC_APB2_HCLK_DIV_1);

  current_count++;

  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,1);
  dac_write_channel(DAC,2,255);

  timer_generate_update(TIMER3); // refresh timer count, prescale, overflow
  timer_resume(TIMER3);
}

Geiger::Geiger() {
}

void Geiger::initialise() {

  calibration_scaling=1;

  // load from flash
  const char *sfloat = flashstorage_keyval_get("CALIBRATIONSCALING");
  if(sfloat != 0) {
    float c;
    sscanf(sfloat, "%f", &c);
    calibration_scaling = c+10;
  }

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
  
  // flashing/buzz timer.
  gpio_set_mode(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,GPIO_OUTPUT_PP);

//  gpio_set_mode(PIN_MAP[10].gpio_device,PIN_MAP[10].gpio_bit,GPIO_OUTPUT_PP);
//  gpio_set_mode (PIN_MAP[13].gpio_device,PIN_MAP[13].gpio_bit,GPIO_OUTPUT_PP);
  dac_init(DAC,DAC_CH2);

  gpio_set_mode (PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,1);

  gpio_set_mode (PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,0);
//  gpio_set_mode(PIN_MAP[6].gpio_device,PIN_MAP[6].gpio_bit,GPIO_INPUT_PD);

  timer_pause(TIMER3);
  timer_set_prescaler(TIMER3,((10000*CYCLES_PER_MICROSECOND)/MAX_RELOAD));
  timer_set_reload(TIMER3,MAX_RELOAD);

  // setup interrupt on channel 3
  timer_set_mode(TIMER3,TIMER_CH3,TIMER_OUTPUT_COMPARE);
  timer_set_compare(TIMER3,TIMER_CH3,MAX_RELOAD-1);
  timer_attach_interrupt(TIMER3,TIMER_CH3,pulse_output_end);

  // initialise timer
  timer_pause(TIMER4);
  //TODO: fix this so it uses both prescaler and reload...
  timer_set_prescaler(TIMER4,((500000*CYCLES_PER_MICROSECOND)/MAX_RELOAD));
  timer_set_reload(TIMER4,MAX_RELOAD);

  // setup interrupt on channel 4
  timer_set_mode(TIMER4,TIMER_CH4,TIMER_OUTPUT_COMPARE);
  timer_set_compare(TIMER4,TIMER_CH4,MAX_RELOAD-1);
  timer_attach_interrupt(TIMER4,TIMER_CH4,geiger_min_log);

  timer_generate_update(TIMER4); // refresh timer count, prescale, overflow

  timer_resume(TIMER4);
  m_samples_collected=0;
}


void Geiger::update_last_min() {
  last_min[last_min_position] = current_count;
  current_count=0;
  last_min_position++;
  if(last_min_position >= COUNTS_PER_MIN) last_min_position=0;
  m_samples_collected++;
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

float Geiger::get_microsieverts() {
  float conversionCoefficient = 0.0029;
  float microsieverts =  (get_cpm_deadtime_compensated() * conversionCoefficient) * calibration_scaling;
  char t[50];
  float_to_char(microsieverts,t,6);
  return microsieverts;
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

bool Geiger::is_cpm_valid() {

  if(m_samples_collected > averaging_period) return true;

  return false;
}

void Geiger::set_calibration(float c) {
  // save to flash
  char sfloat[50];
  sprintf(sfloat,"%f",c);
  flashstorage_keyval_set("CALIBRATIONSCALING",sfloat);
  
  calibration_scaling = c;
}

void Geiger::powerup  () {}
void Geiger::powerdown() {}
