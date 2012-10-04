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
#include "buzzer.h"

#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5
#define MAX_RELOAD ((1 << 16) - 1)

Geiger *system_geiger;

using namespace std;

uint32_t current_count;
bool enable_beep=false;

uint32_t total_count;


void static geiger_min_log(void) {
  system_geiger->update_last_windows();
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
  total_count++;

  gpio_write_bit(PIN_MAP[25].gpio_device,PIN_MAP[25].gpio_bit,1);
  dac_write_channel(DAC,2,255);

  timer_generate_update(TIMER3); // refresh timer count, prescale, overflow
  timer_resume(TIMER3);
  if(enable_beep) buzzer_nonblocking_buzz(0.1);
}

Geiger::Geiger() {
}

void Geiger::initialise() {

  calibration_scaling=1;
  m_cpm_valid = false;

  // load from flash
  const char *sfloat = flashstorage_keyval_get("CALIBRATIONSCALING");
  if(sfloat != 0) {
    float c;
    sscanf(sfloat, "%f", &c);
    calibration_scaling = c+10;
  }

  system_geiger = this;
  for(uint32_t n=0;n<WINDOWS_STORED;n++) {
    last_windows[n] = 0;
  }
  last_windows_position=0;
  
  for(uint32_t n=0;n<WINDOWS_STORED;n++) {
    cpm_last_windows[n] = 0;
  }

  max_averaging_period = 240;
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


void Geiger::update_last_windows() {
  last_windows[last_windows_position] = current_count;
  current_count=0;
  last_windows_position++;
  if(last_windows_position >= WINDOWS_STORED) last_windows_position=0;
  m_samples_collected++;
}

uint32_t min(uint32_t a,uint32_t b) {
  if(a > b) return b; else return a;
}

uint32_t max(uint32_t a,uint32_t b) {
  if(a > b) return a; else return b;
}

float Geiger::get_cpm() {

  // Check we didn't just remove outselves from a source, which
  // makes the windows look crazy.

  // cpm for last 5 seconds
  if(m_cpm_valid) {
		int32_t last5sum=0;
    int32_t c_position = last_windows_position-1;
		for(uint32_t n=0;n<10;n++) {
			last5sum += last_windows[c_position];
	 
			c_position--;
			if(c_position < 0) c_position = WINDOWS_STORED-1;
		}
		
		// cpm for 5 seconds prior to above
		int32_t old5sum=0;
    c_position = last_windows_position-1;
		for(uint32_t n=10;n<20;n++) {
			old5sum += last_windows[c_position];
	 
			c_position--;
			if(c_position < 0) c_position = WINDOWS_STORED-1;
		}

		uint32_t delta = old5sum-last5sum;
		if(delta < 0) delta = 0-delta;
		uint32_t mincpm = min(old5sum,last5sum);
		uint32_t maxcpm = max(old5sum,last5sum);
		if((mincpm*100) < maxcpm) {
			m_cpm_valid = false;
			m_samples_collected=5;
		}
  }


  float sum = 0;

  int32_t c_position = last_windows_position-1;

  int32_t samples_used=0;
  for(uint32_t n=0;n<max_averaging_period;n++) {
   
    sum += last_windows[c_position];
 
    c_position--;
    if(c_position < 0) c_position = WINDOWS_STORED-1;
    samples_used++;
    if(sum > 1000) break; // 1000 datapoints is enough for an estimation
  }

  if(m_samples_collected > samples_used) {
    m_cpm_valid = true;
    return (sum/((float)samples_used))*((float)WINDOWS_PER_MIN);
  }
  m_cpm_valid = false;
 
  // returns an estimation before enough data has been collected. 
  return (sum/((float)m_samples_collected))*((float)WINDOWS_PER_MIN);
}



float Geiger::get_cpm30() {

  float sum = 0;

  int32_t c_position = last_windows_position-1;
  for(uint32_t n=0;n<max_averaging_period;n++) {
   
    sum += last_windows[c_position];
 
    c_position--;
    if(c_position < 0) c_position = WINDOWS_PER_MIN-1;
  }
  if(m_samples_collected > max_averaging_period) return (sum/((float)max_averaging_period))*((float)WINDOWS_PER_MIN);
 
  // returns an estimation before enough data has been collected. 
  return (sum/((float)m_samples_collected))*((float)WINDOWS_PER_MIN);
}

float Geiger::get_cpm_deadtime_compensated() {
  float cpm = get_cpm();
 
  // CPM correction from Medcom
  return cpm/(1-((cpm*1.8833e-6)));

//  float deadtime_us = cpm*40;
//  return (cpm/((60*1000000)-deadtime_us))*(60*1000000);
}

float Geiger::get_microrems() {
 return get_microsieverts()/100;
}

float Geiger::get_microsieverts() {
  float conversionCoefficient = 0.0029;
  float microsieverts =  (get_cpm_deadtime_compensated() * conversionCoefficient) * calibration_scaling;
  char t[50];
  float_to_char(microsieverts,t,6);
  return microsieverts;
}

float *Geiger::get_cpm_last_windows() {

  float cpm_last_windows_temp[WINDOWS_STORED];
  int32_t c_position = last_windows_position+1; // next value, i.e. oldest

  for(uint32_t n=0;n<WINDOWS_STORED;n++) {
    cpm_last_windows_temp[n] = last_windows[c_position];
    c_position++;
    if(c_position >= WINDOWS_STORED) c_position=0;
  }

  int32_t sum=0;
  int averaging_period=30;
  for(uint32_t n=0;n<averaging_period;n++) {
    sum += cpm_last_windows_temp[n];
    cpm_last_windows[n]=0;
  }

  for(uint32_t n=averaging_period;n<WINDOWS_STORED;n++) {
    sum -= cpm_last_windows_temp[n-averaging_period];
    sum += cpm_last_windows_temp[n];
    cpm_last_windows[n] = ((float)sum/(float)averaging_period)*WINDOWS_PER_MIN;
  }

  return cpm_last_windows;
}

bool Geiger::is_cpm_valid() {
  return m_cpm_valid;
}

bool Geiger::is_cpm30_valid() {

  if(m_samples_collected > max_averaging_period) return true;

  return false;
}

void Geiger::set_calibration(float c) {
  // save to flash
  char sfloat[50];
  sprintf(sfloat,"%f",c);
  flashstorage_keyval_set("CALIBRATIONSCALING",sfloat);
  
  calibration_scaling = c;
}


void Geiger::toggle_beep() {
  enable_beep = !enable_beep;
}

bool Geiger::is_beeping() {
  return enable_beep;
}

void Geiger::set_beep(bool b) {
  enable_beep = b;
}

void Geiger::reset_total_count() {
  total_count = 0;
}

uint32_t Geiger::get_total_count() {
  return total_count;
}

void Geiger::powerup  () {}
void Geiger::powerdown() {}
