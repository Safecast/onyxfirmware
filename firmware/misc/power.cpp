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
#include "captouch.h"
#include "accel.h"

#include <stdio.h>


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

/**
 *  return true if the current boot was a wakeup from the WKUP pin.
 *  WKUP can wake the MCU from sleep on rising edge.
 *
 *  The PA0-WKUP Pin behaves as follows:
 * - If the "Wake up" switch is pulled to GND: WKUP moves with Geiger Pulse
 *    - If Geiger_Pulse is low, then WKUP is low
 *    - If Geiger_Pulse is high, then WKUP is high
 * - If the "Wake up switch" is connected to VMCU
 *     - WKUP is always high
 *
 *     Return:
 *     0 : did not wake up from Standby
 *     1 : woke up from standby and WKUP is low
 *     2:  woke up from standby and WKUP is high
 */
int power_get_wakeup_source() {

	gpio_set_mode (PIN_MAP[WAKEUP_GPIO].gpio_device,PIN_MAP[WAKEUP_GPIO].gpio_bit,GPIO_INPUT_PD);
	int wkup = gpio_read_bit(PIN_MAP[WAKEUP_GPIO].gpio_device, PIN_MAP[WAKEUP_GPIO].gpio_bit);

	// Check WUF bit to know if the device just came back from Standby mode
	// through WKUP pin or RTC alarm
	bool wokeup = false;
	if (PWR_BASE->CSR & BIT(PWR_CSR_WUF)) wokeup = true;

	if(!wokeup           ) return WAKEUP_NONE;
	if( wokeup && (!wkup)) return WAKEUP_RTC;
	if( wokeup &&   wkup ) return WAKEUP_WKUP;

	return -1;
}


/**
 * Power up just the minimum we need for the device in flash logging mode
 */
int power_initialise_minimum(void) {

	  gpio_set_mode (PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit,GPIO_OUTPUT_PP);
	  gpio_write_bit(PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit,0);

	  gpio_set_mode (PIN_MAP[CHG_STAT2_GPIO]    .gpio_device,PIN_MAP[CHG_STAT2_GPIO]    .gpio_bit,GPIO_INPUT_FLOATING);
	  gpio_set_mode (PIN_MAP[CHG_STAT1_GPIO]    .gpio_device,PIN_MAP[CHG_STAT1_GPIO]    .gpio_bit,GPIO_INPUT_FLOATING);
	  gpio_set_mode (PIN_MAP[WAKEUP_GPIO]       .gpio_device,PIN_MAP[WAKEUP_GPIO]       .gpio_bit,GPIO_INPUT_PD);
	  gpio_set_mode (PIN_MAP[BATT_MEASURE_ADC]  .gpio_device,PIN_MAP[BATT_MEASURE_ADC]  .gpio_bit,GPIO_INPUT_ANALOG);

	  // Configure ADC1 (doing this does not use more power)
	  rcc_set_prescaler(RCC_PRESCALER_ADC, RCC_ADCPRE_PCLK_DIV_6);
	  adc_init(ADC1);
	  adc_set_extsel(ADC1, ADC_SWSTART);
	  adc_set_exttrig(ADC1, true);

	  gpio_set_mode (PIN_MAP[MAGSENSE_GPIO]     .gpio_device,PIN_MAP[MAGSENSE_GPIO]     .gpio_bit,GPIO_INPUT_PD);

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

	  // initially OLED power supply is off
	  gpio_set_mode (PIN_MAP[LED_PWR_ENA].gpio_device,PIN_MAP[LED_PWR_ENA].gpio_bit,GPIO_OUTPUT_PP);
	  gpio_write_bit(PIN_MAP[LED_PWR_ENA].gpio_device,PIN_MAP[LED_PWR_ENA].gpio_bit,0);

	  return 0;

}

/**
 * Power up the rest of the peripherals for the device to work in user mode
 * (screen on, etc).
 *
 * Note: power_initialise_minimum has to be called first
 */
int power_initialise_full(void) {



  return 0;
}

/**
 * Enable/Disable ADC1 for battery level measurement (we keep it off
 * to save battery power)
 */
void power_adc_enable(bool en) {
	if (en) {
		  adc_enable(ADC1);
		  adc_calibrate(ADC1);
		  adc_set_sample_rate(ADC1, ADC_SMPR_55_5);
	} else {
		adc_disable(ADC1);
	}

}

/**
 *  returns a calibrated ADC code for the current battery voltage
 *  from 0 to 100, in percentage
 */
uint16 power_battery_level(void) {
  uint32 battVal;
  uint32 vrefVal;

  // enable reference voltage only during measurement
  uint32 cr2 = ADC1->regs->CR2;
  cr2 |= ADC_CR2_TSEREFE;
  ADC1->regs->CR2 = cr2;

  // Enable ADC1 (both Vref and Batt measurement are connected to ADC1 in our config)
  power_adc_enable(true);

  // Enable battery measurement circuit:
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,1);
  // Take battery measurement
  battVal = (uint32) adc_read(PIN_MAP[BATT_MEASURE_ADC].adc_device,PIN_MAP[BATT_MEASURE_ADC].adc_channel);
  // Disable battery measurement circuit:
  gpio_write_bit(PIN_MAP[MEASURE_FET_GPIO].gpio_device,PIN_MAP[MEASURE_FET_GPIO].gpio_bit,0);

  vrefVal = (uint32) adc_read(ADC1, 17);

  power_adc_enable(false);

  cr2 &= ~ADC_CR2_TSEREFE; // power down reference to save battery power
  ADC1->regs->CR2 = cr2;

  float batratio = (float)battVal/(float)vrefVal;

  float bat_min = 1.36; //3.3;
  float bat_max = 1.72; //4.2;

  int16 bat_percent = ((batratio-bat_min)/(bat_max-bat_min))*100;

  // in case our min and max are set incorrectly.
  if(bat_percent < 0  ) bat_percent = 0;
  if(bat_percent > 100) bat_percent = 100;
  return bat_percent;

  // The above takes into account the fact that VDDA == VMCU which is proportional to battery voltage
  // VREF is independent of battery voltage, and is 1.2V +/- 3.4%
  // we want to indicate system should shut down at 3.1V; 4.2V is full
  // this is a ratio from 1750 (= 4.2V) to 1292 (=3.1V)
}


/**
 *
 * power_is_battery_low should measure ADC and determine if the battery voltage is
 *  too low to continue operation. When that happens, we should immediately
 *  power down to prevent over-discharge of the battery.
 *
 *  returns 1 if battery is low, 0 otherwise.
 */
int power_is_battery_low(void) {

  // only once every LOG_BATT_FREQ events do we actually measure the battery
  // this is to reduce power consumption


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

/**
 * Goes into standby mode. Will wake up on RTC interrupt or Wakeup switch only.
 * On wakeup, we get the equivalent of a reset.
 *
 * TODO: somehow this routine does not use the structures of libmaple ?
 *       code style is inconsistent with other sections...
 */
void power_standby(void) {

	// Shutdown the display. It needs to be powered down properly,
	// otherwise naively switching off its power will lead to stray
	// current on the IO lines and 100/200 uA current waste.
	display_powerdown();

	// Put the captouch sensor in low power mode
	cap_deinit();

	// Put the accelerometer to sleep
	// Note: the accel should be sleeping all the time except when measuring,
	//       this is a safeguard.
	accel_deinit();

	// Disable all ADCs and timers:
	adc_foreach(adc_disable);
	timer_foreach(timer_disable);

	// No need to deconfigure the GPIOs because all pins are in
	// high impedance mode in Standby mode. this switches the iRover off automatically
	// as well as all other peripherals...

	/**
	 * Not quite sure what this is attempting to do ??? Since we are
	 * in standby mode, this looks totally irrelevant to me
	 */
	/*
	uint32_t val = RCC_APB1Periph_PWR | RCC_APB1Periph_BKP;
	volatile uint32_t *rcc_ahbenr = (uint32_t *) 0x40021000;
	*rcc_ahbenr |= val;

	volatile uint32_t *cr_dbp_bb = (uint32_t *) ((0x42000000 + (0x7000) * 32) + (0x08 * 4));
	*cr_dbp_bb = !0;

	delay_us(100);

	_set_CONTROL();
    */


	// Configure CPU to enter standby mode upon WFI:
	// As per manual:
	//  Enter standby with WFI (Wait for Interrupt) or WFE (Wait for Event) while:
	// – Set SLEEPDEEP in Cortex™-M3 System Control register
	// – Set PDDS bit in Power Control register (PWR_CR)
	// – Clear WUF bit in Power Control/Status register (PWR_CSR)

	volatile uint32_t *pwr_cr  = (uint32_t *) 0x40007000;
	volatile uint32_t *pwr_csr = (uint32_t *) 0x40007004;
	volatile uint32_t *scb_scr = (uint32_t *) 0xE000ED10;

	*scb_scr |= (uint16_t) 1 << 2;  // SCB_SCR_SLEEPDEEP;   // set deepsleep
	*pwr_cr  |= (uint16_t) 1 << 1;  // PWR_CR_PDDS          // set PDDS
  	  	  	  	  	  	  	      	// PDDS at 1: Enter Standby mode when the CPU enters Deepsleep
	*pwr_cr  |= (uint16_t) 1 << 2;  //PWR_CSR_WUF;         // clear WUF
	*pwr_csr |= (uint16_t) 1 << 8;  // EWUP                // enable wakeup pin

	delay_us(100);

	asm volatile (
			"WFI\n\t"
	);

	// We will never end up there after the WFI call above.

}

/**
 *  Enters sleep mode by calling "Wait for Interrupt". then returns
 */
void power_sleep(void) {

  // request wait for interrupt (in-line assembly)
  asm volatile (
    "WFI\n\t" // note for WFE, just replace this with WFE
    "BX r14" // The BX r14 instruction acts as a function return
		     // and pops the return address and ARM or Thumb state from the return stack.
  );
}

int power_deinit(void) {
  // disable wake on interrupt
  PWR_BASE->CSR &= ~BIT(PWR_CSR_EWUP);
  // make sure hall effect sensor is off
  gpio_write_bit(PIN_MAP[MAGPOWER_GPIO].gpio_device,PIN_MAP[MAGPOWER_GPIO].gpio_bit,0);
  // DAC Hack
  gpio_write_bit(PIN_MAP[LIMIT_VREF_DAC].gpio_device,PIN_MAP[LIMIT_VREF_DAC].gpio_bit,0);

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

int power_charging() {
  int stat2 = gpio_read_bit(PIN_MAP[CHG_STAT2_GPIO].gpio_device,PIN_MAP[CHG_STAT2_GPIO].gpio_bit);
  int stat1 = gpio_read_bit(PIN_MAP[CHG_STAT1_GPIO].gpio_device,PIN_MAP[CHG_STAT1_GPIO].gpio_bit);

  if((stat1 == 0) && (stat2 != 0)) return true;
                              else return false;
}
