#include "Controller.h"
#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"
#include "realtime.h"
#include "flashstorage.h"
#include "usart.h"
#include "captouch.h"
#include "rtc.h"
#include "power.h"
#include <stdio.h>
#include "math.h"
#include "accel.h"
#include "log.h"
#include "switch.h"
#include "qr_xfer.h"
#include "buzzer.h"
#include <string.h>
#include <limits.h>
#include "modem.h"
#include <stdint.h>
#include <inttypes.h>
#include "gpio.h"
#include "safecast_config.h"

#define UNITS_CPS 1
#define UNITS_CPM 2

Controller *system_controller;

// Define bit masks for the operating modes:
#define OPMODE_CPM 1
#define OPMODE_USV 2
#define OPMODE_GRAPH 4
#define OPMODE_COUNT 8
#define OPMODE_BECQ 16
#define OPMODE_QRCODE 32


/**
 * Initialize the system controller. In particular:
 *   - Load settings
 *   - Setup the log interval alarm.
 */
Controller::Controller() {

	m_sleeping = false;
	m_powerup = false;
	m_alarm_log = false;

	system_controller = this;
	m_last_switch_state = true;
	m_screen_dimmed = true;

	bool sstate = switch_state();
	m_last_switch_state = sstate;

	m_warning_raised = false;
	m_dim_off = false;

	m_cpm_cps_switch = false;
	m_displaying_cps = false; // For hysteresis
	m_cpm_cps_threshold = 1010;
	m_cps_cpm_threshold = 990;
	next_mode_label = 0;

	// Get warning cpm from flash
	m_warncpm = 0;
	const char *swarncpm = flashstorage_keyval_get("WARNCPM");
	if (swarncpm != 0) {
		sscanf(swarncpm, "%"SCNd32"", &m_warncpm);
	}

	// Get enabled operating modes
	enabled_modes = 255; // By default, all modes enabled
	const char *opm = flashstorage_keyval_get("OPMODES");
	if (opm != 0) {
		sscanf(opm, "%"SCNu8"", &enabled_modes);
	}
	// Restore the ticked status of all menu items:
	if (enabled_modes & OPMODE_CPM) {
		tick_item("CPM:Mode", true);
	}
	if (enabled_modes & OPMODE_USV) {
		tick_item("\x80Sv / \x80R:Mode", true);
	}
	if (enabled_modes & OPMODE_GRAPH) {
		tick_item("Graph:Mode", true);
	}
	if (enabled_modes & OPMODE_COUNT) {
		tick_item("Timed Count:Mode", true);
	}
	if (enabled_modes & OPMODE_BECQ) {
		tick_item("Becquerel:Mode", true);
	}
	if (enabled_modes & OPMODE_QRCODE) {
		tick_item("QR Code:Mode", true);
	}

	// Get never dim from flash
	m_never_dim = false;
	const char *sneverdim = flashstorage_keyval_get("NEVERDIM");
	if (sneverdim != 0) {
		if (strcmp(sneverdim, "true") == 0)
			m_never_dim = true;
	}
	// Send this info to the GUI so that the tick correct on the display
	tick_item("Never Dim", m_never_dim);

	// Get "mute alarm" from flash
	m_mute_alarm = false;
	const char *mute = flashstorage_keyval_get("MUTEALARM");
	if (mute != 0) {
		if (strcmp(mute, "true") == 0)
			m_mute_alarm = true;
	}
	// Send this info to the GUI so that the tick correct on the display
	tick_item("Mute Alarm", m_mute_alarm);

	// Restore Beep settings from flash
	const char *beep = flashstorage_keyval_get("GEIGERBEEP");
	if (strcmp(beep, "true") == 0) {
		tick_item("Beep", true);
	} else {
		tick_item("Beep", false);
	}

	// And restore uSv/ uR/h from flash
	const char *sv = flashstorage_keyval_get("SVREM");
	if (strcmp(sv, "REM") == 0) {
		tick_item(" \x80Sv", false);
		tick_item(" \x80R", true);
	} else {
		tick_item(" \x80Sv", true);
		tick_item(" \x80R", false);
	}

	// Get logging interval from flash if it exists
	// We do not log by default
	m_log_interval_seconds = 0;

	const char *sloginter = flashstorage_keyval_get("LOGINTERVAL");
	if (sloginter) {
		int32_t c;
		sscanf(sloginter, "%"PRIu32"", &c);
		m_log_interval_seconds = c;
	}

	if (m_log_interval_seconds == 0) {
		// No need to enable the Alarm until we have
		// a logging interval above zero
		rtc_disable_alarm(RTC);
		rtc_clear_alarmed();
	}

	// Get the headphone pulse output from flash as well
	// We do not update the Geiger object because it initializes
	// by itself, here we just want to get the default tick values
	// for the menu
	const char *spw = flashstorage_keyval_get("PULSE");
	if (spw) {
		if (strcmp(spw, "0") == 0) {
			tick_item(" No pulse", true);
		} else if (strcmp(spw, "10") == 0) {
			tick_item(" 10 \x80s", true);
		} else if (strcmp(spw, "1000") == 0) {
			tick_item("  1 ms", true);
		} else if (strcmp(spw, "65535") == 0) {
			tick_item(" Audio tone", true);
		}
	}

	// And also restore the count timer:
	m_count_timer_max = 0;
	event_totaltimer();

}

void Controller::set_gui(GUI &g) {
	m_gui = &g;
}

/**
 * Called whenever the unser changes the calibration value
 * in the settings screen.
 */
void Controller::update_calibration() {
	int c1 = m_gui->get_item_state_uint8("$CAL1");
	int c2 = m_gui->get_item_state_uint8("$CAL2");
	int c3 = m_gui->get_item_state_uint8("$CAL3");
	int c4 = m_gui->get_item_state_uint8("$CAL4");
	float calibration_scaling = ((float) c1) + (((float) c2) / 10)
			+ (((float) c3) / 100) + (((float) c4) / 1000);

	char text_sieverts[16];
	sprintf(text_sieverts, "%5.3f \x80Sv",
			m_calibration_base * calibration_scaling);

	m_gui->receive_update("$FIXEDSV", text_sieverts);
}

/**
 * Saves the calibration value
 */
void Controller::event_save_calibration() {
	int c1 = m_gui->get_item_state_uint8("$CAL1");
	int c2 = m_gui->get_item_state_uint8("$CAL2");
	int c3 = m_gui->get_item_state_uint8("$CAL3");
	int c4 = m_gui->get_item_state_uint8("$CAL4");
	float calibration_scaling = ((float) c1) + (((float) c2) / 10)
			+ (((float) c3) / 100) + (((float) c4) / 1000);
	float base_sieverts = system_geiger->get_microsieverts_nocal();

	char text_sieverts[16];
	sprintf(text_sieverts, "%5.3f \x80Sv", base_sieverts * calibration_scaling);

	// FIXME: there is no entry in the screen definition called "Sieverts" !!
	m_gui->receive_update("Sieverts", text_sieverts);
	system_geiger->set_calibration(calibration_scaling);
	m_dim_off = false;
	m_gui->jump_to_screen(0);
}

/**
 * Initialize the final calibration screen
 */
void Controller::initialise_calibration() {
	m_dim_off = true;
	display_set_brightness(15);
	m_calibration_base = system_geiger->get_microsieverts_nocal();
	char text_sieverts[16];
	sprintf(text_sieverts, "%5.3f \x80Sv",
			m_calibration_base * system_geiger->get_calibration());

	m_gui->receive_update("$FIXEDSV", text_sieverts);

	uint8_t c1 = system_geiger->get_calibration();
	uint8_t c2 = ((uint32_t) (system_geiger->get_calibration() * 10)) % 10;
	uint8_t c3 = ((uint32_t) (system_geiger->get_calibration() * 100)) % 10;
	uint8_t c4 = ((uint32_t) (system_geiger->get_calibration() * 1000)) % 10;
	m_gui->receive_update("$CAL1", &c1);
	m_gui->receive_update("$CAL2", &c2);
	m_gui->receive_update("$CAL3", &c3);
	m_gui->receive_update("$CAL4", &c4);
}

void Controller::save_warncpm() {
	m_warning_raised = false;
	int w1 = m_gui->get_item_state_uint8("$WARNCPM1");
	int w2 = m_gui->get_item_state_uint8("$WARNCPM2");
	int w3 = m_gui->get_item_state_uint8("$WARNCPM3");
	int w4 = m_gui->get_item_state_uint8("$WARNCPM4");
	int w5 = m_gui->get_item_state_uint8("$WARNCPM5");

	int32_t warn_cpm = 0;
	warn_cpm += w1 * 10000;
	warn_cpm += w2 * 1000;
	warn_cpm += w3 * 100;
	warn_cpm += w4 * 10;
	warn_cpm += w5 * 1;

	// Reset warning state (will re-trigger at next refresh
	// if new setting is under current CPM count)
	reset_alarm(warn_cpm);

	char swarncpm[50];
	sprintf(swarncpm, "%"PRIi32"", warn_cpm);
	flashstorage_keyval_set("WARNCPM", swarncpm);

	m_gui->jump_to_screen(0);
}

/**
 * Saves the Logging interval ("Save" softkey in the GUI).
 * Enables and resets the RTC alarm if logging interval
 * is > 0
 */
void Controller::save_loginterval() {
	int l1 = m_gui->get_item_state_uint8("$LOGINTER1");
	int l2 = m_gui->get_item_state_uint8("$LOGINTER2");
	int l3 = m_gui->get_item_state_uint8("$LOGINTER3");
	int32_t log_interval_mins = (l1 * 100) + (l2 * 10) + l3;
	m_log_interval_seconds = log_interval_mins * 60;
	char sloginterval[16];
	sprintf(sloginterval, "%"PRIu32"", m_log_interval_seconds);
	flashstorage_keyval_set("LOGINTERVAL", sloginterval);
	if (m_log_interval_seconds > 0) {
		rtc_enable_alarm(RTC);
		uint32_t current_time = realtime_get_unixtime();
		rtc_set_alarm(RTC, current_time + m_log_interval_seconds);
	} else {
		rtc_clear_alarmed();
		rtc_disable_alarm(RTC);
	}
	m_gui->jump_to_screen(0);
}

void Controller::save_counterwindow() {
	int l1 = m_gui->get_item_state_uint8("$COUNTWIN1");
	int l2 = m_gui->get_item_state_uint8("$COUNTWIN2");
	int l3 = m_gui->get_item_state_uint8("$COUNTWIN3");
	int l4 = m_gui->get_item_state_uint8("$COUNTWIN4");
	int l5 = m_gui->get_item_state_uint8("$COUNTWIN5");
	int32_t counter_window = l1 * 10000 + l2 * 1000 + l3 * 100 + l4 * 10 + l5;
	char cwin[16];
	sprintf(cwin, "%"PRIu32"", counter_window);
	flashstorage_keyval_set("COUNTERWIN", cwin);
	m_gui->jump_to_screen(15); // Counter screen
}

void Controller::save_time() {
	int h1 = m_gui->get_item_state_uint8("$TIMEHOUR1");
	int h2 = m_gui->get_item_state_uint8("$TIMEHOUR2");
	int m1 = m_gui->get_item_state_uint8("$TIMEMIN1");
	int m2 = m_gui->get_item_state_uint8("$TIMEMIN2");
	int s1 = m_gui->get_item_state_uint8("$TIMESEC1");
	int s2 = m_gui->get_item_state_uint8("$TIMESEC2");

	int new_hours = h2 + (h1 * 10);
	int new_min = m2 + (m1 * 10);
	int new_sec = s2 + (s1 * 10);

	uint8_t hours, min, sec, day, month;
	uint16_t year;
	realtime_getdate(hours, min, sec, day, month, year);
	hours = new_hours;
	min = new_min;
	sec = new_sec;
	realtime_setdate(hours, min, sec, day, month, year);
	// If we are logging, we must reset the alarm!
	if (m_log_interval_seconds > 0) {
		rtc_set_alarm(RTC, rtc_get_time(RTC) + m_log_interval_seconds);
	}

	flashstorage_log_userchange();
	m_gui->jump_to_screen(0);
}

void Controller::save_date() {
	int d1 = m_gui->get_item_state_uint8("$DATEDAY1");
	int d2 = m_gui->get_item_state_uint8("$DATEDAY2");
	int m1 = m_gui->get_item_state_uint8("$DATEMON1");
	int m2 = m_gui->get_item_state_uint8("$DATEMON2");
	int y1 = m_gui->get_item_state_uint8("$DATEYEAR1");
	int y2 = m_gui->get_item_state_uint8("$DATEYEAR2");

	int new_day = d2 + (d1 * 10);
	int new_mon = m2 + (m1 * 10);
	int new_year = y2 + (y1 * 10);

	uint8_t hours, min, sec, day, month;
	uint16_t year;
	realtime_getdate(hours, min, sec, day, month, year);
	day = new_day;
	month = new_mon - 1;
	year = (2000 + new_year) - 1900;
	realtime_setdate(hours, min, sec, day, month, year);
	// If we are logging, we must reset the alarm!
	if (m_log_interval_seconds > 0) {
		rtc_set_alarm(RTC, rtc_get_time(RTC) + m_log_interval_seconds);
	}

	flashstorage_log_userchange();
	m_gui->jump_to_screen(0);
}

bool is_leap(int month, int day, int year) {
	if ((year % 400) == 0)
		return true;

	if ((year % 100) == 0)
		return false;

	if ((year % 4) == 0)
		return true;

	return false;
}

/**
 * Put the device in sleep mode (will wakeup with
 * standby switch toggle, or RTC alarm for logging)
 */
void Controller::event_sleep(const char *event, const char *value) {
	if (m_sleeping == false) {
		m_sleeping = true;
		m_gui->set_key_trigger();
		power_standby();
	}
}

/**
 * Initialize the total timer values when the GUI enters that screen.
 * and resets the total timer value
 */
void Controller::event_totaltimer() {

	// Reset the counting period and Geiger counts:
	m_total_timer_start = realtime_get_unixtime();
	system_geiger->reset_total_count();

	const char *win = flashstorage_keyval_get("COUNTERWIN");
	if (win) {
		uint32_t win_int;
		sscanf(win, "%"PRIu32"", &win_int);
		m_count_timer_max = win_int;
	}

	// Last, update the GUI with the value of the counter duration
	if (m_count_timer_max > 0) {
		m_gui->receive_update("$COUNTWIN", win);
	} else {
		m_gui->receive_update("$COUNTWIN", "inf.");
	}

}

void Controller::event_save_becq(const char *event, const char *value) {
	int b1 = m_gui->get_item_state_uint8("$BECQ1");
	int b2 = m_gui->get_item_state_uint8("$BECQ2");
	int b3 = m_gui->get_item_state_uint8("$BECQ3");
	int b4 = m_gui->get_item_state_uint8("$BECQ4");

	float beff = b1 * 1000 + b2 * 100 + b3 * 10 + b4;
	system_geiger->set_becquerel_eff(beff);
	m_gui->jump_to_screen(0);
}

void Controller::event_save_utcoff(const char *event, const char *value) {
	int h1 = m_gui->get_item_state_uint8("$OFFHOUR1");
	int h2 = m_gui->get_item_state_uint8("$OFFHOUR2");
	int m1 = m_gui->get_item_state_uint8("$OFFMIN1");
	int m2 = m_gui->get_item_state_uint8("$OFFMIN2");

	int utcoffset = (((h1 * 10) + h2) * 60) + (m1 * 10) + m2;
	if (m_gui->get_item_state_uint8("SIGN:-,+,") == 0) {
		utcoffset = 0 - utcoffset;
	}
	realtime_setutcoffset_mins(utcoffset);

	char sutcoffset[50];
	sprintf(sutcoffset, "%d", utcoffset);
	flashstorage_keyval_set("UTCOFFSETMINS", sutcoffset);

	m_gui->jump_to_screen(0);
}

/**
 * Update the value of the "Never Dim" setting
 */
void Controller::event_neverdim(const char *event, const char *value) {
	if (m_never_dim == false) {
		flashstorage_keyval_set("NEVERDIM", "true");
		m_never_dim = true;
	} else {
		flashstorage_keyval_set("NEVERDIM", "false");
		m_never_dim = false;
	}
	tick_item("Never Dim", m_never_dim);
}

/**
 * Update the value of the "Mute Alarm" setting
 */
void Controller::event_mute_alarm() {
	if (m_mute_alarm == false) {
		flashstorage_keyval_set("MUTEALARM", "true");
		tick_item("Mute Alarm", true);
		m_mute_alarm = true;
	} else {
		flashstorage_keyval_set("MUTEALARM", "false");
		tick_item("Mute Alarm", false);
		m_mute_alarm = false;
	}
	// Clear the alarm to force a re-trigger on the GUI with or witout
	// sound depending on mute_alarm value
	reset_alarm(-1);

}

/**
 * Resets the Warning alarm to a new value (or just clears the GUI
 * if warn_cpm = -1)
 */
void Controller::reset_alarm(int32_t warn_cpm) {
	if (warn_cpm > -1)
		m_warncpm = warn_cpm;
	m_warning_raised = false;
	m_gui->set_cpm_alarm(false, m_mute_alarm, 0);
}

void Controller::event_japanese(const char *event, const char *value) {
	m_gui->set_language(LANGUAGE_JAPANESE);
	flashstorage_keyval_set("LANGUAGE", "Japanese");
	tick_item("English", false);
	tick_item("Japanese", true);
}

void Controller::event_english(const char *event, const char *value) {
	m_gui->set_language(LANGUAGE_ENGLISH);
	flashstorage_keyval_set("LANGUAGE", "English");
	tick_item("English", true);
	tick_item("Japanese", false);
}

void Controller::event_cpm_cps_auto(const char *event, const char *value) {
	if (m_cpm_cps_switch == false) {
		m_cpm_cps_switch = true;
		flashstorage_keyval_set("CPMCPSAUTO", "true");
	} else {
		m_cpm_cps_switch = false;
		flashstorage_keyval_set("CPMCPSAUTO", "false");
	}
	tick_item("CPM/CPS Auto", m_cpm_cps_switch);
}

void Controller::event_geiger_beep(const char *event, const char *value) {
	system_geiger->toggle_beep();
	if (system_geiger->is_beeping()) {
		flashstorage_keyval_set("GEIGERBEEP", "true");
		tick_item("Beep", true);
	} else {
		flashstorage_keyval_set("GEIGERBEEP", "false");
		tick_item("Beep", false);
	}
}

/**
 * Toggle an operating mode
 */
void Controller::event_opmode(const char *event, uint8_t mode_val) {

	// Toggle the operating mode called "event"
	if (enabled_modes & mode_val) {
		tick_item(event, false);
		enabled_modes &= ~mode_val;
	} else {
		tick_item(event, true);
		enabled_modes |= mode_val;
	}

	// Safeguard: if user disabled all opmodes, then force
	// CPM mode back on.
	if ((enabled_modes & (OPMODE_CPM | OPMODE_USV | OPMODE_GRAPH | OPMODE_COUNT | OPMODE_BECQ | OPMODE_QRCODE))==0) {
		enabled_modes = 1;
		tick_item("CPM:Mode",true);
	}

	// Save the opmode to flash
	char opmode[10];
	sprintf(opmode, "%"PRIu8"", enabled_modes);
	flashstorage_keyval_set("OPMODES", opmode);
}

/**
 * Request a refresh of the next opmode menu entry label.
 * Needs to be called after the GUI object is initialized
 */
void Controller::refresh_next_opmode_name() {
	event_next_opmode(false);
}

void Controller::send_mode_label() {
	const char* mode_names[] = { "CPM", "\x80Sv/h", "Graph", "Count", "Becq", "QR" };
	m_gui->receive_update("$NEXTMODE", mode_names[next_mode_label]);
}

/**
 * Request to jump to the next active Operating mode screen, or
 * just find the name of the next active operating mode, depending
 * on 'jump' flag.
 */
void Controller::event_next_opmode(bool jump) {
	uint8_t opmodes[] = { 0,  // Main menu
						  1,  // CPM
						  2,  // MicroSievert
						  4,  // Graph
						  15, // Timed count
						  21, // Becquerel
						  28, // QR Code
	};

	uint8_t cs = m_gui->get_current_screen();
	uint8_t i = 0;
	// Find out where we are in the screen succession
	while ((opmodes[i] != cs) && (i < 8))
		i++;
	if (i == 7) {
		// If we were called while we're not in a screen where we have a
		// 'next opmode' button, we'll end up here:
		return;
	}

	// Select the next screen to start searching for the next
	// enabled mode:
	if (i == 6)
		i = 1;
	else
		i++;

	// Find the next enabled screen (We always have at least one menu enabled so
	// we will never be in an infinite loop)
	while ( (enabled_modes & ( 1 << (i-1) ) ) == 0) {
		if (i == 6)
			i = 1;
		else
			i++;
	}

	// Now, i is the next enabled menu entry,
	// and index of this mode name is (i-1)
	next_mode_label = i-1;
	if (jump) {
		m_gui->jump_to_screen(opmodes[i]);
	}

}

void Controller::event_usv(const char *event, const char *value) {
	flashstorage_keyval_set("SVREM", "SV");
	tick_item(" \x80Sv", true);
	tick_item(" \x80R", false);
}

void Controller::event_rem(const char *event, const char *value) {
	flashstorage_keyval_set("SVREM", "REM");
	tick_item(" \x80Sv", false);
	tick_item(" \x80R", true);
}

/**
 * Clears the event log. TODO: ask for confirmation
 */
void Controller::event_clear_log(const char *event, const char *value) {
	flashstorage_log_clear();
	m_gui->show_dialog("Log Cleared", 0, 0, 0, 0, 48, 254, 254, 254);
}

void Controller::event_save_brightness(const char *event, const char *value) {
	uint8 b = m_gui->get_item_state_uint8("$BRIGHTNESS");

	int br;
	if (b <= 5)
		br = (b * 2) + 1;
	if (b > 5)
		br = b + 6;
	display_set_brightness(br);

	char sbright[50];
	sprintf(sbright, "%u", br);
	flashstorage_keyval_set("BRIGHTNESS", sbright);

	m_dim_off = false;
	m_gui->jump_to_screen(0);
}

void Controller::event_utcscreen(const char *event, const char *value) {
	int offset = realtime_getutcoffset_mins();
	if (offset < 0)
		offset = 0 - offset;

	int hours = offset / 60;
	int min = offset - (hours * 60);

	uint8_t h1, h2, m1, m2;
	h1 = hours / 10;
	h2 = hours % 10;
	m1 = min / 10;
	m2 = min % 10;

	m_gui->receive_update("$OFFHOUR1", &h1);
	m_gui->receive_update("$OFFHOUR2", &h2);
	m_gui->receive_update("$OFFMIN1", &m1);
	m_gui->receive_update("$OFFMIN2", &m2);

	uint8 zero = 0;
	uint8 one = 1;

	offset = realtime_getutcoffset_mins();
	if (offset <= 0)
		m_gui->receive_update("SIGN:-,+,", &zero);
	else
		m_gui->receive_update("SIGN:-,+,", &one);
	m_gui->redraw();
}

void Controller::event_timescreen(const char *event, const char *value) {
	uint8_t hours;
	uint8_t min;
	uint8_t sec;
	uint8_t day;
	uint8_t month;
	uint16_t year;

	realtime_getdate(hours, min, sec, day, month, year);

	uint8 h1, h2, m1, m2, s1, s2;
	h1 = hours / 10;
	h2 = hours % 10;
	m1 = min / 10;
	m2 = min % 10;
	s1 = sec / 10;
	s2 = sec % 10;

	m_gui->receive_update("$TIMEHOUR1", &h1);
	m_gui->receive_update("$TIMEHOUR2", &h2);
	m_gui->receive_update("$TIMEMIN1", &m1);
	m_gui->receive_update("$TIMEMIN2", &m2);
	m_gui->receive_update("$TIMESEC1", &s1);
	m_gui->receive_update("$TIMESEC2", &s2);
	m_gui->redraw();
}

void Controller::event_becqscreen(const char *event, const char *value) {
	float becq_val = 0;
	const char *val = flashstorage_keyval_get("BECQEFF");
	if (val != NULL) {
		sscanf(val, "%f", &becq_val);
	}

	uint8_t b1 = ((int) becq_val % 10000) / 1000;
	uint8_t b2 = ((int) becq_val % 1000) / 100;
	uint8_t b3 = ((int) becq_val % 100) / 10;
	uint8_t b4 = ((int) becq_val % 10);

	m_gui->receive_update("$BECQ1", &b1);
	m_gui->receive_update("$BECQ2", &b2);
	m_gui->receive_update("$BECQ3", &b3);
	m_gui->receive_update("$BECQ4", &b4);
	m_gui->redraw();
}

/**
 * Send the current counter window to the GUI
 * (this is the duration for the total/timer mode
 */
void Controller::event_getcountwin() {
	int32_t win = 0; // by default, no limit

	const char *val = flashstorage_keyval_get("COUNTERWIN");
	if (val != NULL) {
		sscanf(val, "%"PRIi32"", &win);
	}

	uint8_t l1 = (win % 100000) / 10000;
	uint8_t l2 = (win % 10000) / 1000;
	uint8_t l3 = (win % 1000) / 100;
	uint8_t l4 = (win % 100) / 10;
	uint8_t l5 = (win % 10);

	m_gui->receive_update("$COUNTWIN1", &l1);
	m_gui->receive_update("$COUNTWIN2", &l2);
	m_gui->receive_update("$COUNTWIN3", &l3);
	m_gui->receive_update("$COUNTWIN4", &l4);
	m_gui->receive_update("$COUNTWIN5", &l5);
	m_gui->redraw();

}

/**
 * Send the current logging interval to the GUI
 */
void Controller::event_loginterval() {
	int32_t log_interval = 0; // We do not log by default

	const char *val = flashstorage_keyval_get("LOGINTERVAL");
	if (val != NULL) {
		sscanf(val, "%"PRIi32"", &log_interval);
	}

	log_interval = log_interval / 60; // Turn it into minutes

	uint8_t l1 = (log_interval % 1000) / 100;
	uint8_t l2 = (log_interval % 100) / 10;
	uint8_t l3 = (log_interval % 10) / 1;

	m_gui->receive_update("$LOGINTER1", &l1);
	m_gui->receive_update("$LOGINTER2", &l2);
	m_gui->receive_update("$LOGINTER3", &l3);
	m_gui->redraw();
}

void Controller::event_warnscreen(const char *event, const char *value) {
	int32_t warn_level = 0;
	const char *val = flashstorage_keyval_get("WARNCPM");
	if (val != NULL) {
		sscanf(val, "%"PRIi32"", &warn_level);
	}

	uint8_t w1 = (warn_level % 100000) / 10000;
	uint8_t w2 = (warn_level % 10000) / 1000;
	uint8_t w3 = (warn_level % 1000) / 100;
	uint8_t w4 = (warn_level % 100) / 10;
	uint8_t w5 = (warn_level % 10) / 1;

	m_gui->receive_update("$WARNCPM1", &w1);
	m_gui->receive_update("$WARNCPM2", &w2);
	m_gui->receive_update("$WARNCPM3", &w3);
	m_gui->receive_update("$WARNCPM4", &w4);
	m_gui->receive_update("$WARNCPM5", &w5);
	m_gui->redraw();
}

void Controller::event_datescreen(const char *event, const char *value) {
	uint8_t hours;
	uint8_t min;
	uint8_t sec;
	uint8_t day;
	uint8_t month;
	uint16_t year;

	realtime_getdate(hours, min, sec, day, month, year);

	month += 1;

	year = (year + 1900) - 2000;

	uint8 m1, m2, d1, d2, y1, y2;
	d1 = day / 10;
	d2 = day % 10;
	m1 = month / 10;
	m2 = month % 10;
	y1 = year / 10;
	y2 = year % 10;

	m_gui->receive_update("$DATEMON1", &m1);
	m_gui->receive_update("$DATEMON2", &m2);
	m_gui->receive_update("$DATEDAY1", &d1);
	m_gui->receive_update("$DATEDAY2", &d2);
	m_gui->receive_update("$DATEYEAR1", &y1);
	m_gui->receive_update("$DATEYEAR2", &y2);
	m_gui->redraw();
}

void Controller::event_brightnessscn(const char *event, const char *value) {
	const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
	unsigned int c = 15;
	if (sbright != 0) {
		sscanf(sbright, "%u", &c);
		display_set_brightness(c);
	}

	uint8 b;
	if (c <= 11)
		b = (c - 1) / 2;
	if (c > 11)
		b = c - 6;

	m_gui->receive_update("$BRIGHTNESS", &b);
	m_gui->redraw();
}

void Controller::event_leftbrightness(const char *event, const char *value) {
	const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
	if (sbright != 0) {
		unsigned int c;
		sscanf(sbright, "%u", &c);
		display_set_brightness(c);
	}
	m_dim_off = false;
}

void Controller::event_varnum_brightness(const char *event, const char *value) {
	int b = m_gui->get_item_state_uint8("$BRIGHTNESS");
	m_dim_off = true;

	int br;
	if (b <= 5)
		br = (b * 2) + 1;
	if (b > 5)
		br = b + 6;
	display_set_brightness(br);
}

void Controller::event_varnum_date(const char *event, const char *value) {
	int d1 = m_gui->get_item_state_uint8("$DATEDAY1");
	int d2 = m_gui->get_item_state_uint8("$DATEDAY2");
	int m1 = m_gui->get_item_state_uint8("$DATEMON1");
	int m2 = m_gui->get_item_state_uint8("$DATEMON2");
	int y1 = m_gui->get_item_state_uint8("$DATEYEAR1");
	int y2 = m_gui->get_item_state_uint8("$DATEYEAR2");

	if ((m1 == 0) && (m2 == 0))
		m2 = 1;
	if ((d1 == 0) && (d2 == 0))
		d2 = 1;

	if ((m1 >= 1) && (m2 > 2)) {
		m1 = 1;
		m2 = 2;
	}

	uint8 month = m1 * 10 + m2;
	uint8 day = d1 * 10 + d2;
	int year = 2000 + ((y1 * 10) + y2);
	if ((month == 1) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // Jan
	if ((month == 2) && (day > 29)) {
		d1 = 2;
		d2 = 9;
	} // Feb
	if ((month == 3) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // March
	if ((month == 4) && (day > 30)) {
		d1 = 3;
		d2 = 0;
	} // April
	if ((month == 5) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // May
	if ((month == 6) && (day > 30)) {
		d1 = 3;
		d2 = 0;
	} // June
	if ((month == 7) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // July
	if ((month == 8) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // Aug
	if ((month == 9) && (day > 30)) {
		d1 = 3;
		d2 = 0;
	} // Sept
	if ((month == 10) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // Oct
	if ((month == 11) && (day > 30)) {
		d1 = 3;
		d2 = 0;
	} // Nov
	if ((month == 12) && (day > 31)) {
		d1 = 3;
		d2 = 1;
	} // Dec

	if (is_leap(month, day, year) && (month == 2) && (day > 28)) {
		d1 = 2;
		d2 = 8;
	} // Feb

	m_gui->receive_update("$DATEMON1", &m1);
	m_gui->receive_update("$DATEMON2", &m2);
	m_gui->receive_update("$DATEDAY1", &d1);
	m_gui->receive_update("$DATEDAY2", &d2);
}

void Controller::event_varnum_time(const char *event, const char *value) {
	uint8 h1 = m_gui->get_item_state_uint8("$TIMEHOUR1");
	uint8 h2 = m_gui->get_item_state_uint8("$TIMEHOUR2");
	uint8 m1 = m_gui->get_item_state_uint8("$TIMEMIN1");
	uint8 m2 = m_gui->get_item_state_uint8("$TIMEMIN2");
	uint8 s1 = m_gui->get_item_state_uint8("$TIMESEC1");
	uint8 s2 = m_gui->get_item_state_uint8("$TIMESEC2");

	uint8 h = (h1 * 10) + h2;
	uint8 m = (m1 * 10) + m2;
	uint8 s = (s1 * 10) + s2;
	if (h > 23) {
		h1 = 2;
		h2 = 3;
	}
	if (m > 59) {
		m1 = 5;
		m2 = 9;
	}
	if (s > 59) {
		s1 = 5;
		s2 = 9;
	}

	m_gui->receive_update("$TIMEHOUR1", &h1);
	m_gui->receive_update("$TIMEHOUR2", &h2);
	m_gui->receive_update("$TIMEMIN1", &m1);
	m_gui->receive_update("$TIMEMIN2", &m2);
	m_gui->receive_update("$TIMESEC1", &s1);
	m_gui->receive_update("$TIMESEC2", &s2);
}

void Controller::event_audioxfer(const char *event, const char *value) {
	display_clear(0);
	modem_full_range = false;
	display_draw_text(0, 16, " Audio Transfer ", 0, 65535);
	display_draw_text(4, 32, "  in progress  ", 0, 65535);
	modem_logxfer();
	m_gui->jump_to_screen(0);
}

/**
 * Generate a QR Code to tweet current CPM
 */
void Controller::event_qrtweet(const char *event, const char *value) {
	char str[1024];

	if (system_geiger->is_cpm_valid()) {
		//12345678901234567890123456789012345    1   2 34567890
		sprintf(str, "http://twitter.com/home?status=CPM:%u%%23scast",
				(int) system_geiger->get_cpm_deadtime_compensated());
	} else {
		//12345678901234567890123456789012345    1   2 34567890
		sprintf(str, "http://twitter.com/home?status=CPM:%u%%23bad",
				(int) system_geiger->get_cpm_deadtime_compensated());
	}
	qr_draw(str);
}

/**
 * Setup how the headphone output pulse behaves.
 */
void Controller::event_pulse(uint16_t width) {
	// First, deactivate any current option, then
	// reactivate the right one, this is faster and
	// cleaner:
	tick_item(" No pulse", false);
	tick_item(" 10 \x80s", false);
	tick_item("  1 ms", false);
	tick_item(" Audio tone", false);
	system_geiger->enable_headphones(false);
	switch (width) {
	case PULSE_NONE:
		flashstorage_keyval_set("PULSE", "0");
		tick_item(" No pulse", true);
		system_geiger->set_pulsewidth(0);
		break;
	case PULSE_10us:
		flashstorage_keyval_set("PULSE", "10");
		tick_item(" 10 \x80s", true);
		system_geiger->set_pulsewidth(10);
		break;
	case PULSE_1ms:
		flashstorage_keyval_set("PULSE", "1000");
		tick_item("  1 ms", true);
		system_geiger->set_pulsewidth(1000);
		break;
	case PULSE_AUDIO:
		flashstorage_keyval_set("PULSE", "65535");
		tick_item(" Audio tone", true);
		system_geiger->enable_headphones(true);
		break;
	}
	m_gui->redraw(); // Force the redraw of all menu items
}

/**
 * Called by the GUI when a key event is received, triggers an action in the
 * controller.
 *
 */
void Controller::receive_gui_event(const char *event, const char *value) {
	if (m_sleeping)
		return;

	//TODO: Fix this total mess, refactor into switch, break conditions out into methods.
	if (strcmp(event, "Sleep") == 0)
		event_sleep(event, value);
	else if (strcmp(event, "KEYPRESS") == 0)
		m_powerup = true;
	else if (strcmp(event, "TOTALTIMER") == 0)
		event_totaltimer();
	else if (strcmp(event, "Save:Calib") == 0)
		event_save_calibration();
	else if (strcmp(event, "Save:Becq") == 0)
		event_save_becq(event, value);
	else if (strcmp(event, "Save:UTCOff") == 0)
		event_save_utcoff(event, value);
	else if (strcmp(event, "Save:Time") == 0)
		save_time();
	else if (strcmp(event, "Save:Date") == 0)
		save_date();
	else if (strcmp(event, "Save:WarnCPM") == 0)
		save_warncpm();
	else if (strcmp(event, "Japanese") == 0)
		event_japanese(event, value);
	else if (strcmp(event, "Never Dim") == 0)
		event_neverdim(event, value);
	else if (strcmp(event, "Mute Alarm") == 0)
		event_mute_alarm();
	else if (strcmp(event, "English") == 0)
		event_english(event, value);
	else if (strcmp(event, "CPM/CPS Auto") == 0)
		event_cpm_cps_auto(event, value);
	else if (strcmp(event, "Beep") == 0)
		event_geiger_beep(event, value);
	else if (strcmp(event, " \x80Sv") == 0)
		event_usv(event, value);
	else if (strcmp(event, " \x80R") == 0)
		event_rem(event, value);
	else if (strcmp(event, " No pulse") == 0)
		event_pulse(PULSE_NONE);
	else if (strcmp(event, " 10 \x80s") == 0)
		event_pulse(PULSE_10us);
	else if (strcmp(event, "  1 ms") == 0)
		event_pulse(PULSE_1ms);
	else if (strcmp(event, " Audio tone") == 0)
		event_pulse(PULSE_AUDIO);
	else if (strcmp(event, "CPM:Mode") == 0)
		event_opmode(event, OPMODE_CPM);
	else if (strcmp(event, "\x80Sv / \x80R:Mode") == 0)
		event_opmode(event, OPMODE_USV);
	else if (strcmp(event, "Graph:Mode") == 0)
		event_opmode(event, OPMODE_GRAPH);
	else if (strcmp(event, "Timed Count:Mode") == 0)
		event_opmode(event, OPMODE_COUNT);
	else if (strcmp(event, "Becquerel:Mode") == 0)
		event_opmode(event, OPMODE_BECQ);
	else if (strcmp(event, "QR Code:Mode") == 0)
		event_opmode(event, OPMODE_QRCODE);
	else if (strcmp(event, "Clear Log") == 0)
		event_clear_log(event, value);
	else if (strcmp(event,"$NEXTMODE") == 0)
		event_next_opmode(true); // true for "jump to next mode"
	else if (strcmp(event, "Save:Brightness") == 0)
		event_save_brightness(event, value);
	else if (strcmp(event, "Save:LogInter") == 0)
		save_loginterval();
	else if (strcmp(event, "Save:CountWin") == 0)
		save_counterwindow();
	else if (strcmp(event, "CALIBRATE") == 0)
		initialise_calibration();
	else if (strcmp(event, "UTCSCREEN") == 0)
		event_utcscreen(event, value);
	else if (strcmp(event, "TIMESCREEN") == 0)
		event_timescreen(event, value);
	else if (strcmp(event, "BECQSCREEN") == 0)
		event_becqscreen(event, value);
	else if (strcmp(event, "LOGINTERVAL") == 0)
		event_loginterval();
	else if (strcmp(event, "COUNTWINSCR") == 0)
		event_getcountwin();
	else if (strcmp(event, "WARNSCREEN") == 0)
		event_warnscreen(event, value);
	else if (strcmp(event, "DATESCREEN") == 0)
		event_datescreen(event, value);
	else if (strcmp(event, "BrightnessSCN") == 0)
		event_brightnessscn(event, value);
	else if (strcmp(event, "LeftBrightness") == 0)
		event_leftbrightness(event, value);
	else if (strcmp(event, "QR Transfer") == 0)
		qr_logxfer();
	else if (strcmp(event, "Audio Xfer") == 0)
		event_audioxfer(event, value);
	else if (strcmp(event, "QR Tweet") == 0)
		event_qrtweet(event, value);
	else if (strcmp(event, "varnumchange") == 0) {
		if (strcmp("$BRIGHTNESS", value) == 0)
			event_varnum_brightness(event, value);
		else if (strcmpl("$CAL", value, 4))
			update_calibration();
		else if (strcmpl("$DATE", value, 5))
			event_varnum_date(event, value);
		else if (strcmpl("$TIME", value, 5))
			event_varnum_time(event, value);
	}
}

void Controller::check_warning_level() {

	// Note: we used to have a "reading valid" check here, but this is actually
	// dangerous. it is better to raise a false alarm than expose the user to up to
	// 2 minutes of harmful levels because the Onyx considers the reading as not valid
	// yet !
	if ((m_warncpm > 0)) {
		float cpm = system_geiger->get_cpm_deadtime_compensated();
		if ((cpm >= m_warncpm) && (m_warning_raised == false)) {
			if (m_sleeping)
				display_powerup();

			m_warning_raised = true;
			m_dim_off = true;
			m_gui->set_cpm_alarm(true, m_mute_alarm,
					system_geiger->get_cpm_deadtime_compensated());
		} else if ((cpm < m_warncpm) && (m_warning_raised == true)) {
			// We are back to normal
			m_warning_raised = false;
			m_gui->set_cpm_alarm(false, m_mute_alarm, 0);
			m_dim_off = false;
		}
	}
}

/**
 * Append a new entry to the log if necessary. Checks the
 * state of m_alarm_log to determine if we should append a log
 * entry.
 */
void Controller::do_logging() {
	if (rtc_alarmed()) {
		buzzer_morse_debug("R");  // for 'R'TC alarm  .-.
		m_alarm_log = true;
		m_last_alarm_time = rtc_get_time(RTC);
#ifndef DISABLE_ACCEL
		accel_read_state(&m_accel_x_stored, &m_accel_y_stored,
				&m_accel_z_stored);
#endif
		//m_magsensor_stored = gpio_read_bit(PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit);
		// set new alarm for log_interval_seconds from now.
		rtc_clear_alarmed();
	}

	if (m_alarm_log == true) {
		if (system_geiger->is_cpm30_valid()) {
			buzzer_morse_debug("L");  // for 'L'og  .-..

			log_data_t data;
#ifndef DISABLE_ACCEL
			accel_read_state(&data.accel_x_end, &data.accel_y_end,
					&data.accel_z_end);
#endif
			//data.magsensor_end = gpio_read_bit(PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit);

			data.time = rtc_get_time(RTC);
			data.cpm = system_geiger->get_cpm30_deadtime_compensated();

			data.accel_x_start = m_accel_x_stored;
			data.accel_y_start = m_accel_y_stored;
			data.accel_z_start = m_accel_z_stored;
			//data.magsensor_start = m_magsensor_stored;
			data.log_type = UINT_MAX;

			flashstorage_log_pushback((uint8_t *) &data, sizeof(log_data_t));

			bool full = flashstorage_log_isfull();
			if ((full == true) && (!m_sleeping)) {
				m_gui->show_dialog("Flash Log", "is full", 0, 0, 0, 43, 44, 255,
						255);
			}

			m_alarm_log = false;
			// We can have one last alarm after m_log_interval_seconds is set to zero,
			// so make sure we don't re-enable the alarms then.
			if (m_log_interval_seconds > 0) {
				rtc_set_alarm(RTC, m_last_alarm_time + m_log_interval_seconds);
				rtc_enable_alarm(RTC);
			}
			if (m_sleeping) {
				buzzer_morse_debug("S");  // for 'S'tandby  ...
				power_standby();
			} else {
				buzzer_morse_debug("A"); // for 'A'wake   .-
			}
		}
	}
}

/**
 * Checks position of "Sleep" (standby) switch at the back of the device and
 * take action if switch position changed.
 */
void Controller::check_sleep_switch() {

	// Check explanation in Geiger.cpp
	if (system_geiger->pulse_triggered())
		return;

	bool sstate = switch_state();
	if (sstate != m_last_switch_state) {
		m_last_switch_state = sstate;

		// If switch went from power to sleep,
		// then go so sleep
		if (sstate == SWITCH_SLEEP) {
			if (m_alarm_log && (!m_sleeping)) {
				m_sleeping = true;
				display_powerdown();
			} else {
				m_sleeping = true;
				power_standby();
			}
		} else if (sstate == SWITCH_POWERUP) {
			m_powerup = true;
		}
	}

	// Power up the display here only if we were already on but
	// with display sleeping (in logging mode)
	if (m_powerup == true && m_sleeping) {
		display_powerup();

		const char *devicetag = flashstorage_keyval_get("DEVICETAG");
		char revtext[10];
		sprintf(revtext, "VERSION: %s ", OS100VERSION);
		display_splashscreen(devicetag, revtext);
		delay_us(3000000);
		display_clear(0);

		m_sleeping = false;
		m_powerup = false;

	}
}

/**
 * Dim the screen if necessary. Called by the Controller. Will dim in
 * several steps (one dim level at each call). Undims instantly for useability reasons
 * (don't want to wait for display to undim if the operator wants to check the reading
 * quickly)
 */
void Controller::do_dimming() {

	if (m_never_dim && !m_screen_dimmed)
		return;

	// only dim if not in brightness changing mode
	if (!m_dim_off) {
		// Check for no key presses then dim screen
		uint32_t release_time = cap_last_press_any();
		uint32_t press_time = cap_last_release_any();
		uint32_t current_time = realtime_get_unixtime();

		uint8_t current_brightness = display_get_brightness();
		if (((current_time - press_time) > 10)
				&& ((current_time - release_time) > 10)) {
			if (current_brightness > 1)
				display_set_brightness(current_brightness / 2);
			m_screen_dimmed = true;
		} else {
			const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
			unsigned int user_brightness = 15;
			if (sbright != 0) {
				sscanf(sbright, "%u", &user_brightness);
			}
			if (current_brightness < user_brightness) {
				display_set_brightness(user_brightness);
				m_screen_dimmed = false;
			}
		}
	} else if (m_screen_dimmed) {
		// We have dim_off and the screen is dimmed: undim it!
		const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
		unsigned int user_brightness = 15;
		if (sbright != 0) {
			sscanf(sbright, "%u", &user_brightness);
		}
		display_set_brightness(user_brightness);
		m_screen_dimmed = false;
	}
}

/**
 * Send current CPM value to the GUI for display.
 * Since the display font is large, we need limit this to max 4 digits, or 5 if there is a dot.
 */
void Controller::send_cpm_values() {

	char text_cpmdint[16]; // CPM integer
	char text_cpmd[6];     // CPM with decimals

	// Add 0.5 to have a nearest integer rounding
	uint32_t cpm = (uint32_t) floor(
			system_geiger->get_cpm_deadtime_compensated() + 0.5);

	sprintf(text_cpmdint, "%-*"PRIu32"", 7, cpm);

	if (!m_cpm_cps_switch) {       // no auto switch, just display CPM
		// If CPM is above 9999, then we divide it by 1000, print it in red
		// and turn on the "x1000" indicator.
		if (cpm > 9999) {
			float kcpm = (float) cpm / 1000;
			char tmp[16];
			sprintf(tmp, "%5.3f", kcpm);
			// Truncate to 5 character string (includes '.')
			sprintf(text_cpmd, "%5.5s", tmp);
			m_gui->receive_update("$X1000", "x1000");
		} else {
			m_gui->receive_update("$X1000", "     ");
			sprintf(text_cpmd, "%4"PRIu32"", cpm);
		}
		m_gui->receive_update("$CPMSLABEL", "CPM");
	} else {
		if ((cpm > m_cpm_cps_threshold)
				|| ((cpm > m_cps_cpm_threshold) && m_displaying_cps)) {
			float cps = (float) cpm / 60;
			char tmp[16];
			sprintf(tmp, "%5.3f", cps);
			sprintf(text_cpmd, "%5.5s", tmp);
			m_gui->receive_update("$CPMSLABEL", "CPS");
			m_displaying_cps = true;
		} else { // We are under cps to cpm threshold
			m_displaying_cps = false;
			sprintf(text_cpmd, "%4"PRIu32"", cpm);
			m_gui->receive_update("$CPMSLABEL", "CPM");
		}

	}

	if (cpm > MAX_CPM) {
		sprintf(text_cpmd, "TOO HIGH");
		sprintf(text_cpmdint, "TOO HIGH"); // kanji image is 45
	}

	m_gui->receive_update("$CPMDEADINT", text_cpmdint);
	m_gui->receive_update("$CPMDEAD", text_cpmd);
}

void Controller::send_graph_data() {
	float *graph_data;
	graph_data = system_geiger->get_cpm_history();
	m_gui->receive_update("$RECENTDATA", graph_data);
}

/**
 * Total counts
 */
void Controller::send_total_timer() {
	char text_totaltimer_avg[16];
	char text_totaltimer_time[16];
	char text_totaltimer_count[16];
	uint32_t totaltimer_time = realtime_get_unixtime() - m_total_timer_start;

	// Only update the timer if we are below the max count
	// window:
	if (totaltimer_time <= m_count_timer_max || m_count_timer_max == 0) {
		uint32_t cnt = system_geiger->get_total_count();
		sprintf(text_totaltimer_time, "%8"PRIu32" s", totaltimer_time);
		sprintf(text_totaltimer_count, "%9"PRIu32"", cnt);
		sprintf(text_totaltimer_avg, "%7.2f CPM",
				((float) cnt / ((float) totaltimer_time)) * 60);
		m_gui->receive_update("$TTCOUNT", text_totaltimer_count);
		m_gui->receive_update("$TTAVG", text_totaltimer_avg);
		m_gui->receive_update("$TTTIME", text_totaltimer_time);
	}

	m_gui->receive_update("$DELAYA", NULL);
	m_gui->receive_update("$DELAYB", NULL);
}

void Controller::send_svrem() {
	const char *svrem = flashstorage_keyval_get("SVREM");

	if ((svrem != 0) && (strcmp(svrem, "REM") == 0)) {
		char text_rem[6];
		char tmp[16];
		sprintf(tmp, "%5.3f", system_geiger->get_microrems());
		// Truncate to 5 character string (includes '.')
		sprintf(text_rem, "%5.5s", tmp);
		if ((system_geiger->get_cpm_deadtime_compensated() > MAX_CPM)
				|| (system_geiger->get_microrems() > 99999999)) {
			sprintf(text_rem, "TOO HIGH");
		}

		m_gui->receive_update("$SVREM", text_rem);
		m_gui->receive_update("$SVREMLABEL", "\x80R/h");
	} else {
		char text_sieverts[9];
		char tmp[16];
		sprintf(tmp, "%5.3f", system_geiger->get_microsieverts());
		// Truncate to 5 character string (includes '.')
		sprintf(text_sieverts, "%5.5s", tmp);
		if ((system_geiger->get_cpm_deadtime_compensated() > MAX_CPM)
				|| (system_geiger->get_microsieverts() > 99999999)) {
			sprintf(text_sieverts, "TOO HIGH");
		}

		m_gui->receive_update("$SVREM", text_sieverts);
		m_gui->receive_update("$SVREMLABEL", "\x80Sv/h");
	}
}

void Controller::send_becq() {
	char text_becq_tmp[16];
	char text_becq[6];
	float becq = system_geiger->get_becquerel();
	if (becq >= 0) {
		if (becq > 9999) {
			sprintf(text_becq, "TOO HIGH");
		} else {
			sprintf(text_becq_tmp, "%5.3f", becq);
			// Truncate to 5 character string (includes '.')
			sprintf(text_becq, "%5.5s", text_becq_tmp);
		}
		m_gui->receive_update("$BECQ", text_becq);
	} else {
		m_gui->receive_update("$BECQINFO", "Becquerel unset"); // kanji image is: 46
	}
}

/**
 * Send current log area status: time remaining at current log
 * interval, and percentage full.
 */
void Controller::send_logstatus() {
	uint32_t total = flashstorage_log_maxrecords();
	uint32_t current = flashstorage_log_currentrecords();

	uint32_t percent = current * 100 / total;
	if (percent > 100)
		percent = 100; // We should never have this, but in case
					   // the flashstorage functions return wrong values,
					   // we'll be safe.
	char text[20];
	char text2[20];
	sprintf(text, "%"PRIu32"%% full", percent);
	m_gui->receive_update("$LOGPERCENT", text);

	// Now compute how much time we have left in the log area at
	// current log interval:
	if (flashstorage_logpaused()) {
		sprintf(text, "Logging paused  ");
		sprintf(text2, "         ");
	} else if (m_log_interval_seconds) {
		uint32_t time_left_h = (total - current) * m_log_interval_seconds
				/ 3600;
		if (time_left_h > 99999)
			time_left_h = 99999; // Just to keep string under 16 characters
		// Make the user's life easier: above 48 hours, display in days/hours
		if (time_left_h < 49) {
			sprintf(text, "%"PRIu32" hours", time_left_h);
		} else {
			uint32_t days = time_left_h / 24;
			uint32_t hours = time_left_h % 24;
			sprintf(text, "%"PRIu32" days %"PRIu32" hrs", days, hours);
		}
		sprintf(text2, "remaining");
	} else {
		sprintf(text, "Logging disabled");
		sprintf(text2, "         ");
	}
	m_gui->receive_update("$LOGREMAIN", text);
	m_gui->receive_update("$LOGREMAIN2", text2);

	// Last, display at the bottom of the screen the log status in
	// terms of current/total
	sprintf(text, "%"PRIu32"/%"PRIu32" recs", current, total);
	m_gui->receive_update("$LOGREMAIN3", text);

}

/**
 * The main update method of the controller, called from
 * main.cpp. This sends all the current variables to the GUI, so that
 * they can be updated on the screen if necessary.
 */
void Controller::update() {

	check_warning_level();
	do_logging();
	check_sleep_switch();

	if (m_sleeping)
		return;

	do_dimming();
	send_cpm_values();
	send_graph_data();
	send_total_timer();
	send_svrem();
	send_becq();
	send_logstatus();
	send_mode_label();
}
