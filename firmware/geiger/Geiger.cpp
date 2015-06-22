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
#include <inttypes.h>
#include "buzzer.h"
#include "serialinterface.h"

#define MAX_RELOAD ((1 << 16) - 1)
#define LED_PULSE 2000 // Microseconds

extern bool headphones_out; // from buzzer.cpp

Geiger *system_geiger;

using namespace std;

uint32_t current_count;
bool enable_beep = false;

uint32_t total_count;
bool mic_output = false;
bool headphones_output = false;
bool led_on = false;
uint16_t pulse_width = 0;

/**
 * Interrupt handler for TIMER4.
 *
 * At every interrupt by TIMER4 (2Hz currently), update
 * the number of counts during that period.
 *
 */
void static geiger_min_log(void) {
	system_geiger->update_last_windows();
}

/**
 * Interrupt handler for end of pulse. Triggered when Timer3
 * overflows
 *
 * - Switch off LED
 * - Switch off the headphones pulse output
 * - Pause Timer3
 *
 */
void pulse_output_end(void) {

	timer_pause(TIMER3);
	// Now if we just finished a pulse, we should reload the timer
	// and finish counting to 2000 microseconds before switching the LED
	// off.
	if (pulse_width > 0 && led_on) {
		gpio_write_bit(PIN_MAP[HP_COMBINED].gpio_device,
				PIN_MAP[HP_COMBINED].gpio_bit, 0);
		timer_set_compare(TIMER3, TIMER_CH3, LED_PULSE - pulse_width - 1);
		timer_generate_update(TIMER3);
		timer_resume(TIMER3);
	} else {
		gpio_write_bit(PIN_MAP[BOARD_LED_PIN].gpio_device,
				PIN_MAP[BOARD_LED_PIN].gpio_bit, 0);
	}
	led_on = false; // Need to put it here because we use this as a flag
	                // to discard switch reading during pulses.
}

/**
 * Interrupt handler for rising pulse coming from the Geiger sensor
 * controller board.
 *
 * - Increment pulse count
 * - Set pulse output to '1' (if enabled)
 * - Set LED on
 * - Restart TIMER3 which is the pulse output/LED timer
 */
void static geiger_rising(void) {

	current_count++;
	total_count++;
	led_on = true;

	gpio_write_bit(PIN_MAP[BOARD_LED_PIN].gpio_device,
			PIN_MAP[BOARD_LED_PIN].gpio_bit, 1);

	if (pulse_width > 0) {
		gpio_write_bit(PIN_MAP[HP_COMBINED].gpio_device,
				PIN_MAP[HP_COMBINED].gpio_bit, 1);
		timer_set_compare(TIMER3, TIMER_CH3, pulse_width - 1);
	}

	timer_generate_update(TIMER3);
	timer_resume(TIMER3);
	buzzer_nonblocking_buzz(0.1, enable_beep, headphones_output);
}

/**
 * The Geiger counter object.
 * The tube is connected (through the IMI 'iRover' controller board) to
 * the GPIOs defined on top of Geiger.cpp:
 *   - GEIGER_PULSE_GPIO
 *   - GEIGER_ON_GPIO
 */
Geiger::Geiger() {
}

/**
 * Initializes the geiger counter object, including
 * the timer and interrupt.
 */
void Geiger::initialise() {

	calibration_scaling = 1;
	current_cpm_deadtime_compensated = 0;
	m_cpm_valid = false;
	total_count = 0;

	// Load settings from flash. Those values
	// are used by the methods that return calibrated values.
	const char *sfloat = flashstorage_keyval_get("CALIBRATIONSCALING");
	if (sfloat != 0) {
		float c;
		sscanf(sfloat, "%f", &c);
		calibration_scaling = c;
	} else {
		calibration_scaling = 1;
	}

	const char *bfloat = flashstorage_keyval_get("BECQEFF");
	if (bfloat != 0) {
		float c;
		sscanf(bfloat, "%f", &c);
		m_becquerel_eff = c;
	} else {
		m_becquerel_eff = -1;
	}

	system_geiger = this;
	for (uint32_t n = 0; n < WINDOWS_STORED; n++) {
		last_windows[n] = 0;
	}
	last_windows_position = 0;

	for (uint32_t n = 0; n < CPM_HISTORY_SIZE; n++) {
		cpm_history[n] = 0;
		cpm_history_p[n] = 0;
	}
	cpm_history_position = 0;

	max_averaging_period = 240;
	current_count = 0;
	//  geiger_count = 0;
	AFIO_BASE->MAPR |= 0x02000000; // turn off JTAG pin sharing

	// Turn on power on the radiation sensor:
	gpio_set_mode(PIN_MAP[GEIGER_ON_GPIO].gpio_device,
			PIN_MAP[GEIGER_ON_GPIO].gpio_bit, GPIO_OUTPUT_PP);
	gpio_write_bit(PIN_MAP[GEIGER_ON_GPIO].gpio_device,
			PIN_MAP[GEIGER_ON_GPIO].gpio_bit, 1);
	delay_us(1000); // 1 ms for the geiger to settle

	// Set up interrupts on the Geiger counter pulse input:
	gpio_set_mode(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,
			PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit, GPIO_INPUT_PD);

	int bit = gpio_read_bit(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,
			PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit);
	if (bit)
		geiger_rising(); // in case there was a pulse right at this moment...

	// we attach the interrupt to "geiger_rising":
	exti_attach_interrupt((afio_exti_num) PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,
			gpio_exti_port(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device),
			geiger_rising, EXTI_RISING);

	// flashing/buzz timer.
	gpio_set_mode(PIN_MAP[BOARD_LED_PIN].gpio_device,
			PIN_MAP[BOARD_LED_PIN].gpio_bit, GPIO_OUTPUT_PP);

	//gpio_set_mode(PIN_MAP[LIMIT_VREF_DAC].gpio_device,PIN_MAP[LIMIT_VREF_DAC].gpio_bit,GPIO_OUTPUT_PP);

	// If you want to use it as a digital output
	//gpio_set_mode (PIN_MAP[MODEM_OUT].gpio_device,PIN_MAP[MODEM_OUT].gpio_bit,GPIO_OUTPUT_PP);
	//gpio_write_bit(PIN_MAP[MODEM_OUT].gpio_device,PIN_MAP[MODEM_OUT].gpio_bit,0);

	gpio_set_mode(PIN_MAP[MIC_IPHONE].gpio_device, PIN_MAP[MIC_IPHONE].gpio_bit,
			GPIO_OUTPUT_PP);
	gpio_write_bit(PIN_MAP[MIC_IPHONE].gpio_device,
			PIN_MAP[MIC_IPHONE].gpio_bit, 1);

	gpio_set_mode(PIN_MAP[MIC_REVERSE].gpio_device,
			PIN_MAP[MIC_REVERSE].gpio_bit, GPIO_OUTPUT_PP);
	gpio_write_bit(PIN_MAP[MIC_REVERSE].gpio_device,
			PIN_MAP[MIC_REVERSE].gpio_bit, 0);

	// Headphone output
	gpio_set_mode(PIN_MAP[HP_COMBINED].gpio_device,
			PIN_MAP[HP_COMBINED].gpio_bit, GPIO_OUTPUT_PP);
	gpio_write_bit(PIN_MAP[HP_COMBINED].gpio_device,
			PIN_MAP[HP_COMBINED].gpio_bit, 0);

	// Restore the pulse width settings:
	uint16_t pwidth = 0;
	const char *spulsewidth = flashstorage_keyval_get("PULSE");
	if (spulsewidth != 0) {
		sscanf(spulsewidth, "%"SCNu16"", &pwidth);
	}
	set_pulsewidth(pwidth);

	// Initialize timer at 2Hz (500k microseconds = 0.5 s)
	timer_pause(TIMER4);
	//TODO: fix this so it uses both prescaler and reload...
	timer_set_prescaler(TIMER4,
			((500000 * CYCLES_PER_MICROSECOND) / MAX_RELOAD));
	timer_set_reload(TIMER4, MAX_RELOAD);

	// setup interrupt on channel 4
	timer_set_mode(TIMER4, TIMER_CH4, TIMER_OUTPUT_COMPARE);
	timer_set_compare(TIMER4, TIMER_CH4, MAX_RELOAD - 1);
	timer_attach_interrupt(TIMER4, TIMER_CH4, geiger_min_log);

	timer_generate_update(TIMER4); // refresh timer count, prescale, overflow

	timer_resume(TIMER4);

	m_samples_collected = 0;
}

/**
 * Initialize the LED timer (Timer3). Timer3 is started at each
 * incoming pulse on the Geiger counter, and is used to get a fixed width
 * output pulse on the LED.
 *
 *  We setup the timer to count at 1MHz (sys clock divided by 36 since the
 *  processor is running at 36MHz). And we trigger an interrupt after
 *  'pulse_width' microseconds.
 *
 */
void Geiger::pulse_timer_init() {
	timer_pause(TIMER3);
	uint16 width = (pulse_width > 0) ? pulse_width : LED_PULSE;
	timer_set_prescaler(TIMER3, CYCLES_PER_MICROSECOND);
	timer_set_reload(TIMER3, MAX_RELOAD);

	// setup interrupt on channel 3
	timer_set_mode(TIMER3, TIMER_CH3, TIMER_OUTPUT_COMPARE);
	timer_set_compare(TIMER3, TIMER_CH3, width - 1);
	timer_attach_interrupt(TIMER3, TIMER_CH3, pulse_output_end);
}

/**
 * The current production Onyx units have a wrong chip (U103 is a 98, not 97),
 * which leads to the headphones comparator (U102) being not powered when the
 * onyx is running. Which in turn leads to random voltages on MANUAL_WAKEUP
 * when +IN is modulated through HP_COMBINED, then leading to false detection
 * of switch changes. This took me a couple of hours to understand...
 * 2015.06.17, E. Lafargue
 *
 * This is a workaround: the check_sleep_switch method in the controller calls this
 * to ignore those random switch changes during pulses.
 *
 */
bool Geiger::pulse_triggered() {
	return led_on || headphones_out;
}

/**
 * Update the number of count received during the last window
 * (0.5 seconds currently) and stores this in the circular
 * buffer (last_windows).
 */
void Geiger::update_last_windows() {
	last_windows[last_windows_position] = current_count;
	current_count = 0;
	last_windows_position = (last_windows_position + 1) % WINDOWS_STORED;
	if (m_samples_collected < WINDOWS_STORED)
		m_samples_collected++;
	// Update the CPM
	calc_cpm_deadtime_compensated();
}

/**
 * Utility functions: min and max
 */
uint32_t min(uint32_t a, uint32_t b) {
	if (a > b)
		return b;
	else
		return a;
}

uint32_t max(uint32_t a, uint32_t b) {
	if (a > b)
		return a;
	else
		return b;
}

/**
 * Get the current CPM reading. CPM is kept over a 300 seconds
 * window, and this method checks that there was no strong variation
 * on radiations, which could indicate that we got close to a source, or
 * get away from a source. If that's the case, then this method invalidates
 * the measurement window.
 *
 *  IMPORTANT NOTE: get_cpm should not be used to display/output actual
 *  count per minute values, since it does not account for the tube's
 *  resolving time (aka dead time). If you want to get the real CPM value,
 *  get this from get_cpm_deadtime_compensated below.
 *
 *
 */
float Geiger::calc_cpm() {

	// If there are no samples, return 0
	if (m_samples_collected == 0) {
		m_cpm_valid = false;
		return 0;
	}

	// Check we didn't just remove ourselves from a source, which
	// makes the windows look crazy.

	// cpm for last 5 seconds
	int32_t last5sum = 0;
	int32_t c_position = last_windows_position - 1- 10;
	if (c_position < 0)
		c_position += WINDOWS_STORED;
	for (uint8_t n = 0; n < 10; n++) {
		last5sum += last_windows[c_position+n];
	}

	// cpm for 5 seconds prior to above
	int32_t old5sum = 0;
	c_position = last_windows_position - 1 - 20;
	if (c_position < 0)
		c_position += WINDOWS_STORED;
	for (uint8_t n = 0; n < 10; n++) {
		old5sum += last_windows[c_position+n];
	}

	// Invalidate if the last two 5 second windows differ by more than 100 times.
	uint32_t delta = old5sum - last5sum;
	if (delta < 0)
		delta = 0 - delta;
	uint32_t mincpm = min(old5sum, last5sum);
	uint32_t maxcpm = max(old5sum, last5sum);
	if (((mincpm * 100) < maxcpm) && (mincpm != 0)) {
		m_cpm_valid = false;
		m_samples_collected = 5;
	}

	// Invalidate if the cpm30 differs from the cpm5 reading by more than 100 times the cpm5 reading.
	if (is_cpm30_valid()) {
		float cpm30 = get_cpm30();
		float cpm5 = last5sum * 12;
		float delta30 = cpm5 - cpm30;
		if (delta30 < 0)
			delta30 = 0 - delta30;
		if ((delta30 > (cpm5 * 100)) && (cpm5 > 500)) {
			m_cpm_valid = false;
			m_samples_collected = 5;
		}
	}

	float sum = 0;

	c_position = last_windows_position - 1;
	if (c_position < 0)
		c_position = WINDOWS_STORED - 1;

	int16_t samples_used = 0;
	for (uint16_t n = 0;
			(n < max_averaging_period) && (n < m_samples_collected); n++) {

		sum += last_windows[c_position];

		c_position--;
		if (c_position < 0)
			c_position = WINDOWS_STORED - 1;
		samples_used++;
		if (sum > 1000)
			break; // 1000 datapoints is enough for an estimation
	}

	if (m_samples_collected > samples_used) {
		m_cpm_valid = true;
		float cpm = (sum / ((float) samples_used)) * ((float) WINDOWS_PER_MIN);
		if (cpm > MAX_CPM)
			m_cpm_valid = false; // Let's hope you never end up here
								 // while holding the device in your hand...
		return cpm;
	}
	m_cpm_valid = false;

	// returns an estimation before enough data has been collected.
	return (sum / ((float) m_samples_collected)) * ((float) WINDOWS_PER_MIN);
}

/**
 * Returns CPM over the last 30 seconds. Do not use directly,
 * because it does not account for the tube dead time. This is used for
 * internal computations only.
 */
float Geiger::get_cpm30() {

	float sum = 0;

	uint32_t windows_in_30s = WINDOWS_PER_MIN / 2;
	int32_t c_position = last_windows_position - 1;
	if (c_position < 0)
		c_position = WINDOWS_PER_MIN - 1;
	for (uint32_t n = 0; n < windows_in_30s; n++) {

		sum += last_windows[c_position];

		c_position--;
		if (c_position < 0)
			c_position = WINDOWS_PER_MIN - 1;
	}
	if (m_samples_collected > windows_in_30s)
		return (sum / ((float) windows_in_30s)) * ((float) WINDOWS_PER_MIN);

	// returns an estimation before enough data has been collected.
	return (sum / ((float) m_samples_collected)) * ((float) WINDOWS_PER_MIN);
}

/**
 * Returns the CPM count, compensated for the tube's 'Dead time'.
 *
 * Explanation: After each detection, a Geiger tube needs a bit of time to
 * recover/recharge before being able to detect activity again. If a way,
 * after each count it is 'blind' for a short amount of time.
 *
 * In order to get the real CPM value, we therefore need to account for this
 * dead time, to measure only the actual 'live time' during which the tube is
 * able to detect radiation.
 *
 * This dead time is specific to the tube and provided by the manufacturer.
 *
 * IMPORTANT NOTE: if you need to output/display the CPM count, this is the method
 * to use, not the get_cpm above.
 */
float Geiger::calc_cpm_deadtime_compensated() {
	float cpm = calc_cpm();

	// CPM correction from Medcom
	current_cpm_deadtime_compensated = cpm / (1 - ((cpm * 1.8833e-6)));

	// And we also save the CPM value into a circular buffer
	// for graphing
	cpm_history_p[cpm_history_position] = current_cpm_deadtime_compensated;
	cpm_history_position = (cpm_history_position+1) % CPM_HISTORY_SIZE;

}

/**
 * Returns the current CPM
 */
float Geiger::get_cpm_deadtime_compensated() {
	return current_cpm_deadtime_compensated;
}


/**
 * Returns CPM count over the last 30 seconds, 'dead time compensated'
 * (see full explanation on get_cpm_deadtime_compensated for details)
 */
float Geiger::get_cpm30_deadtime_compensated() {
	float cpm = get_cpm30();

	// CPM correction from Medcom
	return cpm / (1 - ((cpm * 1.8833e-6)));

}


/**
 * Gets measurement converted using stored calibration values
 */
float Geiger::get_microrems() {
	return get_microsieverts() * 100;
}

/**
 * Gets measurement converted using stored calibration values
 */
float Geiger::get_microsieverts() {
	float conversionCoefficient = 0.00294;
	float microsieverts = (get_cpm_deadtime_compensated()
			* conversionCoefficient) * calibration_scaling;
	char t[16];
	sprintf(t, "%5.3f \x80Sv", microsieverts);

	return microsieverts;
}

/**
 * Gets measurement without the calibration value (raw measurement)
 */
float Geiger::get_microsieverts_nocal() {
	float conversionCoefficient = 0.00294;
	float microsieverts = (get_cpm_deadtime_compensated()
			* conversionCoefficient);
	return microsieverts;
}


/**
 * Returns a float array of the CPM values for the last X seconds for graphing it
 */
float* Geiger::get_cpm_history() {

	int16_t c_position = cpm_history_position + 1;

	// This copies the cpm history to a new array, realigned
	for (uint8_t n = 0; n < CPM_HISTORY_SIZE; n++) {
		cpm_history[n] = cpm_history_p[ (c_position + n ) % CPM_HISTORY_SIZE];
	}

	return cpm_history;
}

bool Geiger::is_cpm_valid() {
	return m_cpm_valid;
}

/**
 * Will turn true once enough samples collected to make
 * the 30 seconds window significant
 */
bool Geiger::is_cpm30_valid() {

	if (m_samples_collected > ((WINDOWS_PER_MIN / 2) + 1))
		return true;

	return false;
}

float Geiger::get_calibration() {
	return calibration_scaling;
}

void Geiger::set_calibration(float c) {
	// save to flash
	char sfloat[50];
	sprintf(sfloat, "%f", c);
	flashstorage_keyval_set("CALIBRATIONSCALING", sfloat);

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

float Geiger::get_becquerel() {
	if (m_becquerel_eff < 0)
		return -1;

	return get_cpm_deadtime_compensated() * m_becquerel_eff;
}

void Geiger::set_becquerel_eff(float c) {
	// save to flash
	char sfloat[50];
	sprintf(sfloat, "%f", c);
	flashstorage_keyval_set("BECQEFF", sfloat);

	m_becquerel_eff = c;
}

/**
 *
 */
void Geiger::powerup() {
}

/**
 * Powers down the iRover board, Microphone and other peripherals
 */
void Geiger::powerdown() {

	dac_disable_channel(DAC, DAC_CH1);
	dac_disable_channel(DAC, DAC_CH2);

	// reduce current by shutting down outputs
	// remove dc from DAC out
	gpio_set_mode(PIN_MAP[MODEM_OUT].gpio_device, PIN_MAP[MODEM_OUT].gpio_bit,
			GPIO_OUTPUT_PP);
	gpio_write_bit(PIN_MAP[MODEM_OUT].gpio_device, PIN_MAP[MODEM_OUT].gpio_bit,
			0);
	gpio_write_bit(PIN_MAP[MIC_IPHONE].gpio_device,
			PIN_MAP[MIC_IPHONE].gpio_bit, 0);
	gpio_write_bit(PIN_MAP[GEIGER_ON_GPIO].gpio_device,
			PIN_MAP[GEIGER_ON_GPIO].gpio_bit, 0);
	gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,
			PIN_MAP[BUZZER_PWM].gpio_bit, 0);

}

void Geiger::enable_headphones(bool en) {
	headphones_output = en;
	pulse_width = 0;
	pulse_timer_init();
}

void Geiger::enable_micout() {
	mic_output = true;
}

void Geiger::disable_micout() {
	mic_output = false;
}

/**
 * Set the headphone audio out pulse width in microseconds.
 * Note: we use the special value "65535" to indicate we want
 *       an audio beep, not a click.
 */
void Geiger::set_pulsewidth(uint32_t p) {
	if (p == 65535) {
		pulse_width = 0;
		headphones_output = true;
	} else {
		headphones_output = false;
		pulse_width = p;
	}
	pulse_timer_init();
}

uint32_t Geiger::get_pulsewidth() {
	return pulse_width;
}
