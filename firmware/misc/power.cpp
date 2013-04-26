//Power control functions
#include "power.h"
#include "safecast_config.h"

// for power control support
#include "pwr.h"
#include "scb.h"
#include "exti.h"
#include "adc.h"
#include "bkp.h"
#include "display.h"

#include <stdio.h>

#define MANUAL_WAKEUP_GPIO 18 // PC3
#define CHG_STAT2_GPIO     44 // PC11
#define CHG_STAT1_GPIO     26 // PC10
#define MAGPOWER_GPIO      41 // PA15
#define MEASURE_FET_GPIO   45 // PC12
#define BATT_MEASURE_ADC   28 // PB1
#define MAGSENSE_GPIO      29 // PB10
#define LIMIT_VREF_DAC     10 // PA4 -- should be DAC eventually, but GPIO initially to tied own
#define CHG_TIMEREN_N_GPIO 37 // PC8
#define LED_PWR_ENA_GPIO   16 // PC1 // handled in OLED platform_init
#define WAKEUP_GPIO        2

#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state

// maximum range for battery, where the value is "full" and 
// 0 means the system should shut down
#define BATT_RANGE 16

#define RCC_APB1Periph_PWR               ((uint32_t)0x10000000)
#define RCC_APB1Periph_BKP               ((uint32_t)0x08000000)

static uint8 powerState = PWRSTATE_BOOT;
static uint8 lastPowerState = PWRSTATE_OFF;

// return true if the current boot was a wakeup from the WKUP pin.
int power_get_wakeup_source() {

  gpio_set_mode (GPIOA,0, GPIO_INPUT_PD);
  int wkup = gpio_read_bit(GPIOA,0);

  volatile uint32_t *pwr_csr = (uint32_t *) 0x40007004;
  bool wokeup = false;
  if(*pwr_csr & 1) wokeup = true;

  if(!wokeup           ) return 0;
  if( wokeup && (!wkup)) return 1;
  if( wokeup &&   wkup ) return 2;

  return -1;
}

int power_initialise(void) {
  gpio_set_mode (PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit,0);

  gpio_set_mode (PIN_MAP[CHG_STAT2_GPIO]    .gpio_device,PIN_MAP[CHG_STAT2_GPIO]    .gpio_bit,GPIO_INPUT_PD);
  gpio_set_mode (PIN_MAP[CHG_STAT1_GPIO]    .gpio_device,PIN_MAP[CHG_STAT1_GPIO]    .gpio_bit,GPIO_INPUT_PD);
  gpio_set_mode (PIN_MAP[WAKEUP_GPIO]       .gpio_device,PIN_MAP[WAKEUP_GPIO]       .gpio_bit,GPIO_INPUT_PD);
  gpio_set_mode (PIN_MAP[BATT_MEASURE_ADC]  .gpio_device,PIN_MAP[BATT_MEASURE_ADC]  .gpio_bit,GPIO_INPUT_ANALOG);
  gpio_set_mode (PIN_MAP[MAGSENSE_GPIO]     .gpio_device,PIN_MAP[MAGSENSE_GPIO]     .gpio_bit,GPIO_INPUT_PD);

  // setup and initialize the outputs
  // initially, don't measure battery voltage
  gpio_set_mode (PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,0);
    

  // initially, turn off the hall effect sensor
  gpio_set_mode (PIN_MAP[MAGPOWER_GPIO].gpio_device,PIN_MAP[MAGPOWER_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[MAGPOWER_GPIO].gpio_device,PIN_MAP[MAGPOWER_GPIO].gpio_bit,0);

  // as a hack, tie this low to reduce current consumption
  // until we hook it up to a proper DAC output
  gpio_set_mode (PIN_MAP[LIMIT_VREF_DAC].gpio_device,PIN_MAP[LIMIT_VREF_DAC].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[LIMIT_VREF_DAC].gpio_device,PIN_MAP[LIMIT_VREF_DAC].gpio_bit,0);
    
  // initially, charge timer is enabled (active low)
  gpio_set_mode (PIN_MAP[CHG_TIMEREN_N_GPIO].gpio_device,PIN_MAP[CHG_TIMEREN_N_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[CHG_TIMEREN_N_GPIO].gpio_device,PIN_MAP[CHG_TIMEREN_N_GPIO].gpio_bit,0);

  // initially OLED is off
  gpio_set_mode (PIN_MAP[LED_PWR_ENA_GPIO].gpio_device,PIN_MAP[LED_PWR_ENA_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[LED_PWR_ENA_GPIO].gpio_device,PIN_MAP[LED_PWR_ENA_GPIO].gpio_bit,0);

  return 0;
}

// returns a calibrated ADC code for the current battery voltage
uint16 power_battery_level(void) {
  uint32 battVal;
  uint32 vrefVal;

  uint32 cr2 = ADC1->regs->CR2;
  cr2 |= ADC_CR2_TSEREFE; // enable reference voltage only for this measurement
  ADC1->regs->CR2 = cr2;

  gpio_set_mode(PIN_MAP[BATT_MEASURE_ADC].gpio_device,PIN_MAP[BATT_MEASURE_ADC].gpio_bit,GPIO_INPUT_ANALOG);
  gpio_set_mode(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,1);

  battVal = (uint32) adc_read(PIN_MAP[BATT_MEASURE_ADC].adc_device,PIN_MAP[BATT_MEASURE_ADC].adc_channel);
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,0);
  vrefVal = (uint32) adc_read(ADC1, 17);

  cr2 &= ~ADC_CR2_TSEREFE; // power down reference to save battery power
  ADC1->regs->CR2 = cr2; 

  float batratio = (float)battVal/(float)vrefVal;

  float bat_min = 1.36; //3.3;
  float bat_max = 1.72; //4.2;

  int16 bat_percent = ((batratio-bat_min)/(bat_max-bat_min))*100;   

  // incase our min and max are set incorrectly.
  if(bat_percent < 0  ) bat_percent = 0;
  if(bat_percent > 100) bat_percent = 100;
  return bat_percent;

  // The above takes into account the fact that VDDA == VMCU which is proportional to battery voltage
  // VREF is independent of battery voltage, and is 1.2V +/- 3.4%
  // we want to indicate system should shut down at 3.1V; 4.2V is full
  // this is a ratio from 1750 (= 4.2V) to 1292 (=3.1V)
}


// power_is_battery_low should measure ADC and determine if the battery voltage is
// too low to continue operation. When that happens, we should immediately
// power down to prevent over-discharge of the battery.
int power_is_battery_low(void) {
  // only once every LOG_BATT_FREQ events do we actually measure the battery
  // this is to reduce power consumption
  gpio_init_all();
  afio_init();
            
  // init ADC
  rcc_set_prescaler(RCC_PRESCALER_ADC, RCC_ADCPRE_PCLK_DIV_6);
  adc_init(ADC1);

  // this is from "adcDefaultConfig" inside boards.cpp
  // lifted and modified here so *only* ADC1 is initialized
  // the default routine "does them all"
  adc_set_extsel(ADC1, ADC_SWSTART);
  adc_set_exttrig(ADC1, true);

  adc_enable(ADC1);
  adc_calibrate(ADC1);
  adc_set_sample_rate(ADC1, ADC_SMPR_55_5);

  // again, a minimal set of operations done to save power; these are lifted from
  // setup_gpio()
  gpio_set_mode(PIN_MAP[BATT_MEASURE_ADC].gpio_device,PIN_MAP[BATT_MEASURE_ADC].gpio_bit,GPIO_INPUT_ANALOG);
  gpio_set_mode(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,0);
 
  // normally 0, 5 for testing
  if( power_battery_level() <= 1 ) return 1;
                              else return 0;
}

void _set_CONTROL()
{
    asm volatile (
        "mov r0,#0\r\n"
        "msr control,r0\r\n"
        "bx r14"
        );
}

uint32_t  _get_CONTROL()
{
  asm volatile (
    "mrs r0, control\r\n"
    "bx lr\r\n"
  );
  return 0;
}

void power_standby(void) {

  // ensure display is shutdown

  adc_foreach(adc_disable);
  timer_foreach(timer_disable);

  //RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    // RCC->AHBENR |= RCC_AHBPeriph;
  
  uint32_t val = RCC_APB1Periph_PWR | RCC_APB1Periph_BKP;
  volatile uint32_t *rcc_ahbenr = (uint32_t *) 0x40021000;
  *rcc_ahbenr |= val;


  //PWR_BackupAccessCmd(ENABLE);
  //*(__IO uint32_t *) CR_DBP_BB = (uint32_t)NewState;
  volatile uint32_t *cr_dbp_bb = (uint32_t *) ((0x42000000 + (0x7000) * 32) + (0x08 * 4));
  *cr_dbp_bb = !0;

  delay_us(100);

  _set_CONTROL();

  volatile uint32_t *pwr_cr = (uint32_t *) 0x40007000;
  volatile uint32_t *pwr_csr = (uint32_t *) 0x40007004;
  volatile uint32_t *scb_scr = (uint32_t *) 0xE000ED10;

  *pwr_csr |= (uint16_t) 256; // EWUP (enable wakeup)
  *pwr_cr |= (uint8_t) 0x06; //PWR_CR_PDDS); // set PDDS

  uint32_t temp = *pwr_cr;
  temp |= 6;
  *pwr_cr = temp;
  *pwr_cr |= (uint16_t) 0x04; //PWR_CSR_WUF; // clear WUF


  *scb_scr |= (uint16_t) 0x04; // SCB_SCR_SLEEPDEEP; // set deepsleep

  delay_us(100);

  asm volatile (
    "WFI\n\t" // note for WFE, just replace this with WFE
  );

  // should never get here - manual beeps for debugging if we do.
  for(int i=0;i<11;i++) {
    for(int n=0;n<100;n++) {
      gpio_toggle_bit(GPIOB,9);
      delay_us(1000);
    }
    delay_us(100000);
  }

}

void power_wfi(void) {
  // request wait for interrupt (in-line assembly)
  asm volatile (
    "WFI\n\t" // note for WFE, just replace this with WFE
    "BX r14"
  );
}

int power_deinit(void) {
  // disable wake on interrupt
  PWR_BASE->CSR &= ~PWR_CSR_EWUP;
  power_standby();
  return 0;
}


int power_get_state(void) {
  return powerState;
}

int power_set_state(int state) {
    lastPowerState = powerState;
    powerState = state;
    return lastPowerState;
}
