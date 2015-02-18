/**************************************************
 *                                                 *
 *            Safecast Geiger Counter              *
 *                                                 *
 **************************************************/

#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"

#include "UserInput.h"
#include "Geiger.h"
#include "GUI.h"
#include "Controller.h"
#include <stdint.h>
#include "flashstorage.h"
#include "rtc.h"
#include "accel.h"
#include "realtime.h"
#include "serialinterface.h"
#include "switch.h"
#include "buzzer.h"
#include <stdio.h>
#include <string.h>
#include "nvic.h"
#include "rtc.h"

/**
 * This is defined at linking time
 */
extern uint8_t _binary___binary_data_private_key_data_start;
extern uint8_t _binary___binary_data_private_key_data_size;

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
	init();
	delay_us(100000);
}

/**
 * This is where the application starts after a reset.
 */
int main(void) {

	Geiger g;
	power_initialise_minimum();

	// Power management strategy: if battery level is lower than 20%, then we
	// refuse to boot, but we still check battery level regularly and proceed
	// only once battery level is high enough.
	if (power_battery_level() < 20) {

		// TODO: this is a hack, we should not have to initialize the Geiger counter
		// when we are in low battery mode. A simple timer trigger should be enough
		g.initialise(); // this will set up our timer so we wake up on sleep
		// turn off cap touch
		cap_deinit();
		slowClocks();
		do {
			// Go to sleep, and wake up periodically and check our battery level.
			// request wait for interrupt (in-line assembly)
			volatile uint32_t *scb_scr = (uint32_t *) 0xE000ED10; //ok

			// Set System control register to go into "Deep Sleep" upon WFE/WFI
			// and wake up when event or interrupt enters pending state
			*scb_scr |= (uint16_t) 0x14; // set  SLEEPDEEP
										 // sets SAVEONPEND
			delay_us(100);
			asm volatile (
					"SEV\r\n"
					"WFE\n\t"   // Call Wait For Event
					"SEV\r\n"
			);
		} while (power_battery_level() < 50);
		// Force system reset
		nvic_sys_reset();
	}

	flashstorage_initialise();
	serial_initialise();
	buzzer_initialise();
	realtime_initialise();
	g.initialise();
	switch_initialise();
	accel_init();

	Controller c;
	GUI m_gui(c);
	c.set_gui(m_gui);
	// We need the UserInput object in any case, because
	// if the user switches the counter on while it is logging
	// (on with screen off) it will switch on the screen.
	UserInput u(m_gui);
	u.initialise();

	uint8_t *private_key =
			((uint8_t *) &_binary___binary_data_private_key_data_start);
	if (private_key[0] != 0)
		delay_us(1000);

	delay_us(10000);  // can be removed?

	// Check if we woke up from the RTC. If that's the case, it means we have to
	// just wake up for 30 seconds for logging to flash. So we avoid turning on
	// most peripherals to save as much power as possible.
	if (power_get_wakeup_source() == WAKEUP_RTC) {
		rtc_set_alarmed(); // if we woke up from the RTC, force the the alarm trigger.
		                   // because it was cleared when we initialized the controller.
		buzzer_morse_debug("W"); // 'W'akeup
		c.m_sleeping = true;
	} else {
		// Switch on our display, and
		// display our welcome screen
		c.m_sleeping = false;

		display_powerup();
		const char *devicetag = flashstorage_keyval_get("DEVICETAG");
		char revtext[10];
		sprintf(revtext, "VERSION: %s ", OS100VERSION);
		display_splashscreen(devicetag, revtext);
		buzzer_morse("IMI"); // Hello :)
		delay_us(2000000);
		display_clear(0);
		bool full = flashstorage_log_isfull();
		if (full == true) {
			m_gui.show_dialog("Flash Log", "is full", 0, 0, 0);
		}
	}

	// TODO: move into flash storage keyval loading
	int utcoffsetmins_n = 0;
	const char *utcoffsetmins = flashstorage_keyval_get("UTCOFFSETMINS");
	if (utcoffsetmins != 0) {
		int c;
		sscanf(utcoffsetmins, "%d", &c);
		utcoffsetmins_n = c;

		realtime_setutcoffset_mins(utcoffsetmins_n);
	}

	// TODO Need to refactor out stored settings
	flashstorage_keyval_update();

	const char *language = flashstorage_keyval_get("LANGUAGE");
	if (language != 0) {
		if (strcmp(language, "English") == 0) {
			m_gui.set_language(LANGUAGE_ENGLISH);
			tick_item("English", true);
		} else if (strcmp(language, "Japanese") == 0) {
			m_gui.set_language(LANGUAGE_JAPANESE);
			tick_item("Japanese", true);
		}
	} else {
		m_gui.set_language(LANGUAGE_ENGLISH);
		tick_item("English", true);
	}
	m_gui.jump_to_screen(1);
	m_gui.push_stack(0, 1);

	/**
	 * Start of main event loop here, we will not leave this
	 * loop until battery runs our or user changes the standby switch
	 * position.
	 *
	 * c is our controller
	 * g is the Geiger object
	 * m_gui is the GUI
	 *
	 */
	buzzer_morse_debug("M");
	for (;;) {

		// If our battery is too low, then we force
		// standby and power off as many peripherals as we can
		// to limit further discharge.
		if (power_battery_level() < 10) {
			buzzer_morse_debug("B");
			rtc_clear_alarmed();
			rtc_disable_alarm(RTC);
			// turn iRover off
			g.powerdown();
			// turn off all interrupts and go into power_standby
			power_deinit();
			power_standby();
		}

		c.update();

		if (!c.m_sleeping) {
			m_gui.render();
			serial_eventloop();

			// Screen lock code
			uint32_t release1_time = cap_last_press(KEY_BACK);
			uint32_t press1_time = cap_last_release(KEY_BACK);
			uint32_t release2_time = cap_last_press(KEY_SELECT);
			uint32_t press2_time = cap_last_release(KEY_SELECT);
			uint32_t current_time = realtime_get_unixtime();

			int cap1 = cap_ispressed(KEY_BACK);
			int cap2 = cap_ispressed(KEY_SELECT);
			if ((release1_time != 0) && (release2_time != 0)
					&& ((current_time - press1_time) > 3)
					&& ((current_time - press2_time) > 3) && cap1 && cap2) {
				system_gui->toggle_screen_lock();
				cap_clear_press();
			}
		}
		// Now sleep until the next interrupt (user input, Geiger Pulse, etc).
		power_sleep();
	}

	/**
	 *   End of main event loop
	 */

	// should never get here
	for (int n = 0; n < 60; n++) {
		delay_us(100000);
		buzzer_blocking_buzz(1000);
	}
	return 0;
}
