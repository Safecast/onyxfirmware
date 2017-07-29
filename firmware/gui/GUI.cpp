#include "screen_layout.h"
#include "Controller.h"
#include <stdint.h>
#include "display.h"
#include "UserInput.h"
#include "GUI.h"
#include <stdint.h>
#include "utils.h"
#include "power.h"
#include "realtime.h"
#include <stdio.h>
#include "Geiger.h"
#include <string.h>
#include "buzzer.h"
#include "captouch.h"
#include "serialinterface.h"

bool first_render = true;
bool clear_next_render = true;

uint16 header_color = HEADER_COLOR_CPMINVALID;

uint8_t m_language;
char softkeys[3][6] = { "  X  ", "  X  ", "  X  " };
uint16_t  background_color = BACKGROUND_COLOR;

/****************
 *   Utility functions for drawing on the screen
 */

/**
 * Used to draw arrow above digits for settings menus
 */
void display_draw_equtriangle(uint8_t x, uint8_t y, uint8_t s, uint16_t color) {

	uint8_t start = x;
	uint8_t end = x;
	for (uint8_t n = 0; n < s; n++) {

		for (uint8_t i = start; i <= end; i++) {
			display_draw_point(i, y, color);
		}
		start--;
		end++;
		y++;
	}
}

/**
 * Used to draw arrow below digits for settings menus
 */
void display_draw_equtriangle_inv(uint8_t x, uint8_t y, uint8_t s,
		uint16_t color) {

	uint8_t start = x;
	uint8_t end = x;
	for (uint8_t n = 0; n < s; n++) {

		for (uint8_t i = start; i <= end; i++) {
			display_draw_point(i, y, color);
		}
		start--;
		end++;
		y--;
	}
}

#define MAX_TICKED_ITEMS 22
char ticked_items[MAX_TICKED_ITEMS][TEXT_LENGTH];
uint32_t ticked_items_size = 0;

/**
 * Toggles a menu option. This is only called from controller.cpp and
 * is really ugly - this should be a method in the GUI class, but this is
 * how the firmware was initially written :(
 */
void tick_item(const char *name, bool tick_val) {

	if ((ticked_items_size >= MAX_TICKED_ITEMS) && (tick_val == true))
		return;

	for (uint32_t n = 0; n < ticked_items_size; n++) {
		if (strcmp(name, ticked_items[n]) == 0) {
			if (tick_val == true)
				return;
			if (tick_val == false) {
				for (uint32_t i = n; i < (ticked_items_size - 1); i++) {
					strcpy(ticked_items[i], ticked_items[i + 1]);
				}
				ticked_items_size--;
				return;
			}
		}
	}

	if (tick_val == true) {
		strcpy(ticked_items[ticked_items_size], name);
		ticked_items_size++;
	}
}

bool is_ticked(const char *name) {
	for (uint32_t n = 0; n < ticked_items_size; n++) {
		if (strcmp(name, ticked_items[n]) == 0)
			return true;
	}
	return false;
}

/**
 * Renders a menu item, with an optional "Tick" mark if the
 * menu can be toggled.
 *   item.val2 is used as the menu number to decide where to draw
 *   it on screen.
 */
void render_item_menu(screen_item &item, bool selected) {

	uint16_t bg = background_color;
	uint16_t fg = FOREGROUND_COLOR;
	if (selected) {
		bg = FOREGROUND_COLOR;
		fg = background_color;
	}

	// render tick
	bool ticked = is_ticked(item.text);
	if (ticked) {
		display_draw_text(128 - 8, item.val2 * 16, "\x7F", fg, bg);
	}

	if ((m_language == LANGUAGE_ENGLISH) || (item.kanji_image == 255)) {

		int len = strlen(item.text);
		char text[50];
		strcpy(text, item.text);

		// Search for : replace with NULL in rendering
		for (int n = 0; n < len; n++) {
			if (text[n] == ':')
				text[n] = 0;
		}
		len = strlen(text);

		// Pad to 16 characters
		for (int n = len; n < 16; n++) {
			text[n] = ' ';
			text[n + 1] = 0;
		}
		if (ticked)
			text[15] = 0;

		display_draw_text(0, item.val2 * 16, text, fg, bg);
	} else if (m_language == LANGUAGE_JAPANESE) {
		if (!ticked) {
			display_draw_fixedimage(0, item.val2 * 16, item.kanji_image,
					bg);
		} else {
			display_draw_fixedimage_xlimit(0, item.val2 * 16, item.kanji_image,
					bg, 128 - 8);
		}
	}

}

/**
 * We maintain a global array of variable items (VARNUM) that are
 * displayed in settings menus mainly. a VARNUM is a single digit
 * or single character, hence the uint8_t
 */

#define VARNUM_MAXSIZE 50  // Number of variables we can track
char varnum_names[VARNUM_MAXSIZE][11];  // Name of the variables
uint8_t varnum_values[VARNUM_MAXSIZE]; // Value of the variables
uint8_t varnum_size = 0;

/**
 * Get the current value of a variable (a single digit or character)
 */
uint8_t get_item_state_varnum(const char *name) {

	for (uint32_t n = 0; n < varnum_size; n++) {
		if (strcmp(varnum_names[n], name) == 0) {
			return varnum_values[n];
		}
	}
	return 0;
}

/**
 * Update state of a variable number in the varnum_values table
 *
 * @param name of the varnum
 * @param value
 */
void set_item_state_varnum(char *name, uint8_t value) {

	int itemcount = 0;
	int len = strlen(name);
	for (int n = 0; n < len; n++) {
		if (name[n] == ',')
			itemcount++;
	}

	if (itemcount != 0) {
		if (value > itemcount)
			return;
	}

	for (uint32_t n = 0; n < varnum_size; n++) {
		if (strcmp(varnum_names[n], name) == 0) {
			varnum_values[n] = value;
			return;
		}
	}

	if (varnum_size >= VARNUM_MAXSIZE)
		return;
	strcpy(varnum_names[varnum_size], name);
	varnum_values[varnum_size] = value;
	varnum_size++;
}

/**
 * Rendering of a variable number/char (single digit with
 * up/down arrows above/below it).
 *
 * @param item should be an ITEM_TYPE_VARNUM
 * @param selected if cursor should be on that particular digit
 */
void render_item_varnum(screen_item &item, bool selected) {

	// Position on screen where to draw the digit
	uint8_t x = item.val1;
	uint8_t y = item.val2;

	int len = strlen(item.text);
	int colon_pos = -1;
	// Text can be with this format:
	// "NAME:char1,char2,char..." to let user select
	// non-numeric values, we detect this below:
	for (int n = 0; n < len; n++) {
		if (item.text[n] == ':')
			colon_pos = n;
	}

	bool nonnumeric = false;
	char selitem[10][10];
	// We have a non-numeric variable, initialize the
	// possible values (comma-separated string following ":"
	// in the item.text. For instance: "SIGN:-,+,"
	if (colon_pos != -1) {
		nonnumeric = true;

		char current[10];
		int current_pos = 0;
		int cselitem = 0;
		for (int n = colon_pos + 1; n < len; n++) {

			if (item.text[n] != ',') {
				current[current_pos] = item.text[n];
				current[current_pos + 1] = 0;
				current_pos++;
			} else {
				strcpy(selitem[cselitem], current);
				current_pos = 0;
				current[0] = 0;
				cselitem++;
			}
		}
	}

	// Retrieve the current value of this VARNUM
	uint8_t val = get_item_state_varnum(item.text);

	uint16_t color;
	if (selected)
		color = 0xcccc;
	else
		color = FOREGROUND_COLOR;
	display_draw_equtriangle(x, y, 9, color);
	display_draw_equtriangle_inv(x, y + 33, 9, color);
	if (nonnumeric == false) {
		display_draw_number(x - 4, y + 9, val, 1, FOREGROUND_COLOR, background_color);
	} else {
		display_draw_text(x - 4, y + 9, selitem[val], FOREGROUND_COLOR, background_color);
	}
}

uint8_t get_item_state_varnum(screen_item &item) {
	return get_item_state_varnum(item.text);
}

uint32_t delay_time;
uint32_t get_item_state_delay(screen_item &item) {
	return delay_time;
}

void clear_item_varnum(screen_item &item, bool selected) {
	int x = item.val1;
	int y = item.val2;
	int start_x = x - 9;
	int end_x = x + 9;

	int start_y = y - 2;
	int end_y = y + 40;

	if (start_x < 0)
		start_x = 0;
	if (end_x > 127)
		end_x = 127;

	if (start_y < 0)
		start_y = 0;
	if (end_y > 127)
		end_y = 127;

	display_draw_rectangle(start_x, start_y, end_x, end_y, background_color);
}

/**
 * Render a static text label.
 *
 * @param item should be an ITEM_TYPE_LABEL
 */
void render_item_label(screen_item &item, bool selected) {

	if (m_language == LANGUAGE_ENGLISH) {
		if (item.val1 == 255) {
			display_draw_text_center(item.val2, item.text, FOREGROUND_COLOR, background_color);
		} else {
			display_draw_text(item.val1, item.val2, item.text, FOREGROUND_COLOR, background_color);
		}
	}

	if (m_language == LANGUAGE_JAPANESE) {
		if (item.kanji_image != 255) {
			if ((item.val1 != 255) && (item.val1 != 0)) {
				display_draw_fixedimage_xlimit(item.val1, item.val2,
						item.kanji_image, background_color, 128 - item.val1);
			} else {
				// we can't center fixed images as we don't know their width, just draw at 0, full width.
				display_draw_fixedimage(0, item.val2, item.kanji_image,
				background_color);
			}
		} else {
			if (item.val1 == 255) {
				display_draw_text_center(item.val2, item.text, FOREGROUND_COLOR,
				background_color);
			} else {
				display_draw_text(item.val1, item.val2, item.text, FOREGROUND_COLOR,
				background_color);
			}
		}
	}
}

/**
 * Render a small label. Use item.val1=255 to center the label
 * item.val1: x
 * item.val2: y
 * item.text: label
 */
void render_item_smalllabel(screen_item &item, bool selected) {
	if (item.val1 == 255) {
		display_draw_tinytext_center(item.val2, item.text, background_color);
	} else {
		display_draw_tinytext(item.val1, item.val2, item.text,
		background_color);
	}
}

/**
 * Render a soft key.
 * item.val1: 0 to 2 (soft key number)
 * item.val2: unused
 * item.text: label (5 char max)
 */
void render_item_softkey(screen_item &item) {
	char text[6];

	if (item.val1 > 2)
		return;

	// Truncate the label to 5 characters
	if (strlen(item.text) == 0)
		return;
	// If it's a variable, then don't render now, wait
	// for the update to come from the controller
	if (item.text[0] == '$') {
		// softkeys[item.val1][0] = 1; // Invalidate softkey menu to force re-render next update.
		return;
	}

	sprintf(text, "%1.5s", item.text);
	// Search for : and truncate the string there if necessary:
	char* c = strchr(text,':');
	if (c != NULL) {
		text[c-text] = 0;
	}

	// render tick
	if (is_ticked(item.text)) {
		// Reformat to 5 characters wide, left-justified
		sprintf(text, "%-5.5s", item.text);
		text[4] = 0x7F;
	}

	// We don't want to re-render an identical softkey, because
	// it causes flickering (we don't have double buffering on the screen)
	if (strcmp(softkeys[item.val1], text) == 0)
		return;
	strcpy(softkeys[item.val1], text);

	uint8_t x1 = 0;
	uint8_t x2 = 0;
	if (item.val1 == 0) {
		x2 = 41;
	} else if (item.val1 == 1) {
		x1 = 43;
		x2 = 84;
	} else {
		x1 = 86;
		x2 = 127;
	}
	display_draw_rectangle(x1, 110, x2, 127, header_color);
	// We want to center the label
	draw_text(x1 + 1 + 4 * (5 - strlen(text)), 111, text, COLOR_BLACK, header_color);
}

/**
 * Draw the header background
 */
void render_item_head(screen_item &item, bool selected) {
	draw_text(0, 0, "                ", COLOR_BLACK, header_color);
}

float m_old_graph_data[CPM_HISTORY_SIZE];
float m_graph_data[CPM_HISTORY_SIZE];
float *source_graph_data;

/**
 * Draw the CPM graph for the last 128 CPM values.
 *
 */
void render_item_graph(screen_item &item, bool selected) {

	float max_height = 80;
	uint8_t m_x = item.val1;
	uint8_t m_y = item.val2;

	// find min and max in data
	float nmax = source_graph_data[0];
	float nmin = source_graph_data[0];
	for (uint8_t n = 1; n < CPM_HISTORY_SIZE; n++) {
		if (source_graph_data[n] > nmax)
			nmax = source_graph_data[n];
		if (source_graph_data[n] < nmin)
			nmin = source_graph_data[n];
	}

	// Draw axis
	if (first_render) {
		display_draw_line(m_x, m_y, m_x + CPM_HISTORY_SIZE, m_y, FOREGROUND_COLOR);
		display_draw_line(m_x, m_y - max_height, m_x + CPM_HISTORY_SIZE,
				m_y - max_height, FOREGROUND_COLOR);
	}

	if ((nmax-nmin) == 0)
		return;

	// Adjust so that the graph does not overwrite the axes
	m_y--;
	max_height = max_height-2;

	// Rescale data vertically
	float scale = max_height / (nmax-nmin);
	float data_point = m_y - (source_graph_data[0] - nmin) * scale;

	// Render the graph
	for (uint32_t n = 0; n < CPM_HISTORY_SIZE-2; n++) {
		float next_data_point = m_y - (source_graph_data[n+1] - nmin) * scale;
		display_draw_line(n, m_old_graph_data[n], n+1, m_old_graph_data[n+1], background_color);
		display_draw_line(n, data_point    , n+1, next_data_point    , FOREGROUND_COLOR);
		m_old_graph_data[n] = data_point;
		data_point = next_data_point;
	}
	m_old_graph_data[CPM_HISTORY_SIZE-2] = data_point;
	display_draw_point(CPM_HISTORY_SIZE-2, data_point, background_color);

	display_draw_tinynumber(m_x + 5, m_y - max_height - 10, nmax, 4, FOREGROUND_COLOR);
	display_draw_tinynumber(m_x + 5, m_y - 10, nmin, 4, FOREGROUND_COLOR);

}

void clear_item_menu(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;

	uint8_t x_max = 127;
	uint8_t y_max = (item.val2 + 1) * 16;
	if (y_max > 127)
		y_max = 127;
	display_draw_rectangle(0, item.val2 * 16, x_max, y_max, background_color);
}

void clear_item_label(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;

	int x_max = 0;
	if (item.val1 == 255) {
		x_max = 127;
	} else {
		x_max = item.val1 + (text_len * 8) - 1;
	}
	int y_max = item.val2 + 16;
	if (x_max >= 128)
		x_max = 127;
	if (y_max >= 128)
		y_max = 127;
	int x_min = 0;
	if (item.val1 != 255) {
		x_min = item.val1;
		x_max = 127;
	}
	display_draw_rectangle(x_min, item.val2, x_max, y_max, background_color);
}

void clear_item_smalllabel(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;

	int x_max = 0;
	if (item.val1 == 255) {
		x_max = 127;
	} else {
		x_max = item.val1 + (text_len * 8) - 1;
	}
	int y_max = item.val2 + 16;
	if (x_max >= 128)
		x_max = 127;
	if (y_max >= 128)
		y_max = 127;
	int x_min = 0;
	if (item.val1 != 255) {
		x_min = item.val1;
		x_max = 127;
	}
	display_draw_rectangle(x_min, item.val2, x_max, y_max, background_color);
}

void clear_item_head(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;

	int x_max = 127;
	int y_max = 16;
	display_draw_rectangle(item.val1, item.val2, x_max, y_max,
	background_color);
}

/**
 * Clear a soft key.
 * item.val1: 0 to 2 (soft key number)
 * item.val2: unused
 * item.text: label (5 char max)
 */
void clear_item_softkey(screen_item &item, bool selected) {

	// Truncate the label to 5 characters
	if (item.val1 > 2)
		return;
	uint8_t x1 = 0;
	uint8_t x2 = 0;
	if (item.val1 == 0) {
		x2 = 41;
	} else if (item.val1 == 1) {
		x1 = 43;
		x2 = 84;
	} else {
		x1 = 86;
		x2 = 127;
	}
	display_draw_rectangle(x1, 109, x2, 127, background_color);

}

void clear_item_bigvarlabel(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;
	display_draw_rectangle(item.val1, item.val2, 127, (item.val2 + 43),
	background_color);
}

void clear_item_varlabel(screen_item &item, bool selected) {
	int32_t text_len = strlen(item.text);

	if (text_len == 0)
		return;
	int x = 0;
	if (item.val1 == 255)
		x = 0;
	else
		x = item.val1;
	display_draw_rectangle(x, item.val2, 127, (item.val2 + 16),
	background_color);
}

void clear_item_graph(screen_item &item, bool selected) {

	display_draw_rectangle(0, 16, 128, 128, background_color);
}

void clear_item_delay(screen_item &item, bool selected) {
	display_draw_rectangle(item.val1, item.val2, item.val1 + 24, item.val2 + 16,
	background_color);
}

void render_item_delay(screen_item &item, bool selected) {
	display_draw_number(item.val1, item.val2, delay_time, 3, FOREGROUND_COLOR, background_color);
}

/**
 * Renders a GUI item. render_item is used for the initial rendering of the
 * item. Further updates are done through update_item.
 */
void render_item(screen_item &item, bool selected) {
	if (item.type == ITEM_TYPE_MENU) {
		render_item_menu(item, selected);
	} else if (item.type == ITEM_TYPE_LABEL) {
		render_item_label(item, selected);
	} else if (item.type == ITEM_TYPE_SOFTKEY
			|| item.type == ITEM_TYPE_SOFTKEY_ACTION) {
		render_item_softkey(item);
	} else if (item.type == ITEM_TYPE_SMALLLABEL) {
		render_item_smalllabel(item, selected);
	} else if (item.type == ITEM_TYPE_GRAPH) {
		render_item_graph(item, selected);
	} else if (item.type == ITEM_TYPE_HEAD) {
		render_item_head(item, selected);
	} else if (item.type == ITEM_TYPE_MENU_ACTION) {
		render_item_menu(item, selected);
	} else if (item.type == ITEM_TYPE_VARNUM) {
		render_item_varnum(item, selected);
	} else if (item.type == ITEM_TYPE_DELAY) {
		render_item_delay(item, selected);
	}
}

void clear_item(screen_item &item, bool selected) {
	if (item.type == ITEM_TYPE_MENU) {
		clear_item_menu(item, selected);
	} else if ((item.type == ITEM_TYPE_LABEL) ||
			    (item.type == ITEM_TYPE_RED_VARLABEL)) {
		clear_item_label(item, selected);
	} else if (item.type == ITEM_TYPE_SOFTKEY
			|| item.type == ITEM_TYPE_SOFTKEY_ACTION) {
		clear_item_softkey(item, selected);
	} else if (item.type == ITEM_TYPE_SMALLLABEL) {
		clear_item_label(item, selected);
	} else if (item.type == ITEM_TYPE_VARLABEL) {
		clear_item_varlabel(item, selected);
	} else if (item.type == ITEM_TYPE_GRAPH) {
		clear_item_graph(item, selected);
	} else if (item.type == ITEM_TYPE_HEAD) {
		clear_item_head(item, selected);
	} else if (item.type == ITEM_TYPE_MENU_ACTION) {
		clear_item_menu(item, selected);
	} else if (item.type == ITEM_TYPE_VARNUM) {
		clear_item_varnum(item, selected);
	} else if (item.type == ITEM_TYPE_DELAY) {
		clear_item_delay(item, selected);
	} else if (item.type == ITEM_TYPE_BIGVARLABEL) {
		clear_item_bigvarlabel(item, selected);
	}
}

void update_item_graph(screen_item &item, const void *value) {
	source_graph_data = (float *) value;
}

uint8 lock_mask[11][8] = { { 0, 1, 1, 1, 1, 1, 1, 0 },
		{ 1, 1, 0, 0, 0, 0, 1, 1 }, { 1, 0, 0, 0, 0, 0, 0, 1 }, { 1, 1, 1, 1, 1,
				1, 1, 1 }, { 1, 1, 1, 0, 0, 1, 1, 1 },
		{ 1, 1, 0, 0, 0, 0, 1, 1 }, { 1, 1, 0, 0, 0, 0, 1, 1 }, { 1, 1, 1, 0, 0,
				1, 1, 1 }, { 1, 1, 1, 0, 0, 1, 1, 1 },
		{ 1, 1, 1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1 }

};

bool lock_state = false;
void render_lock(bool on) {

	if ((on == lock_state) && (on != true))
		return;
	lock_state = on;

	uint16 image_data[88]; // 8*11
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 11; y++) {
			int16 render_value = lock_mask[y][x];

			if (render_value > 0)
				render_value = 0xFFFF;
			else
				render_value = 0;

			if (on == false)
				render_value = 0;

			image_data[(y * 8) + x] = render_value;
		}
	}

	display_draw_image(128 - 8, 128 - 11, 8, 11, image_data);
}

/**
 * A battery icon for the status bar
 */
uint8 battery_mask[16][24] =
		{

				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 4, 4, 4, 4, 4, 4, 4,
						4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0 }, {
						3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
						3, 0, 0, 0, 0 }, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0 },
				{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
						1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, { 1, 1, 1, 1, 1, 1, 1,
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, {
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
						1, 1, 1, 1, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
				{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 0 }, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 2, 2, 2, 2, 2, 2, 2, 0 }, { 3, 3, 3, 3, 3, 3, 3,
						3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 }, {
						4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
						4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0 }

		};

/**
 * A battery icon with a "Charging" sign for the status bar
 */
uint8 battery_mask_chg[16][24] =
		{

				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 4, 4, 4, 4, 4, 4, 4,
						4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0 }, {
						3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 9, 9, 3, 3, 3, 3, 3,
						3, 0, 0, 0, 0 }, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 9,
						9, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0 },
				{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 9, 9, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 1, 1, 1,
						1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, { 1, 1, 1, 1, 1, 1, 1,
						1, 9, 9, 9, 9, 9, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, {
						1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 1, 1, 1, 1, 1,
						1, 1, 1, 1, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9,
						9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
				{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 9, 9, 2, 2, 2, 2, 2, 2, 2, 2, 2,
						2, 2, 0 }, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 9, 9, 2, 2, 2,
						2, 2, 2, 2, 2, 2, 2, 2, 2, 0 }, { 3, 3, 3, 3, 3, 3, 3,
						3, 9, 9, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 }, {
						4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
						4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0 }

		};

uint16 last_batlevel = 0;

/**
 * Draw a battery sign on the screen
 *
 * @param x        X location on screen
 * @param y        Y location on screen
 * @param level    level of charge in percent
 * @param charging display charging symbol or not
 */
void render_battery(int x, int y, int level, int charging) {

	uint16 image_data[384]; // 24*16

	// only move bat level, if there's a big difference to prevent flickering.
	level = (((float) level) / 100) * 23;
	uint16 level_delta = level - last_batlevel;
	if (level_delta < 0)
		level_delta = 0 - level_delta;
	if (level_delta <= 1)
		level = last_batlevel;

	last_batlevel = level;

	for (int x = 0; x < 24; x++) {
		for (int y = 0; y < 16; y++) {
			int16 render_value;
			if (!charging)
				render_value = battery_mask[y][x];
			else
				render_value = battery_mask_chg[y][x];

			if (render_value != 9) {
				if (x <= level) {
					if (render_value > 0)
						render_value = 0xF1FF - (2081 * (render_value - 1));
					else
						render_value = header_color; // HEADER_COLOR; // header background
				}

				if (x > level) {
					if (render_value > 0)
						render_value = background_color; //6243 - (2081*(render_value-1));
					else
						render_value = header_color; //HEADER_COLOR; // header background
				}
			}
			if (render_value == 9)
				render_value = 0xF1F0 - (2001 * (render_value - 1));
			image_data[(y * 24) + x] = render_value;
		}
	}

	display_draw_image(104, 0, 24, 16, image_data);
}

/**
 * Update the text of a softkey
 */
void update_item_softkey(screen_item &item, char * value) {
	char text[6];
	// Truncate the label to 5 characters
	if (strlen(value) == 0)
		return;
	if (item.val1 > 2)
		return;

	sprintf(text, "%1.5s", value);
	// render tick
	if (is_ticked(item.text)) {
		// Reformat to 5 characters wide, left-justified
		sprintf(text, "%-5.5s", value);
		text[4] = 0x7F;
	}

	// We don't want to re-render an identical softkey, because
	// it causes flickering (we don't have double buffering on the screen)
	if (strcmp(softkeys[item.val1], text) == 0)
		return;

	strcpy(softkeys[item.val1], text);

	uint8_t x1 = 0;
	uint8_t x2 = 0;
	if (item.val1 == 0) {
		x2 = 41;
	} else if (item.val1 == 1) {
		x1 = 43;
		x2 = 84;
	} else {
		x1 = 86;
		x2 = 127;
	}
	display_draw_rectangle(x1, 110, x2, 127, header_color);
	// Center the text when we draw it
	draw_text(x1 + 1 + 4 * (5 - strlen(text)), 111, text, COLOR_BLACK, header_color);
}

/**
 * Draw the GUI screen header
 *
 * @param item  unused
 * @param value text to draw at start of header (CPM usually)
 *
 */
void update_item_head(screen_item &item, const void *value) {

	uint16_t new_header_color;
	if (system_geiger->is_cpm_valid())
		new_header_color = HEADER_COLOR_NORMAL;
	else
		new_header_color = HEADER_COLOR_CPMINVALID;

	if (new_header_color != header_color) {
		draw_text(0, 0, "                ", COLOR_BLACK, new_header_color);
		// Also force update of softkeys
		first_render = true;
	}
	header_color = new_header_color;

	int len = strlen((char *) value);
	char v[TEMP_STR_LEN];
	strcpy(v, (char *) value);
	for (int n = len; (n < 6) && (n < (TEMP_STR_LEN - 1)); n++) {
		v[n] = ' ';
		v[n + 1] = 0;
	}
	if (len > 6)
		v[6] = 0;

	draw_text(0, 0, v, COLOR_BLACK, header_color);

	render_battery(0, 128 - 24, power_battery_level(), power_charging());

	uint8_t hours, min, sec, day, month;
	uint16_t year;
	realtime_getdate_local(hours, min, sec, day, month, year);
	month += 1;
	year += 1900;
	if (year >= 2000) {
		year -= 2000;
	} else if (year < 2000) {
		year -= 1900;
	}

	char time[TEMP_STR_LEN];
	char date[TEMP_STR_LEN];
	sprintf(time, "%02u:%02u:%02u", hours, min, sec);
	sprintf(date, "%02u/%02u/%02u", month, day, year);
	if (year == 0) {
		sprintf(date, "%02u/%02u/00", month, day);
	}

	// pad out time and date
	int tlen = strlen(time);
	for (int n = tlen; (n < 8) && (n < (TEMP_STR_LEN - 1)); n++) {
		time[n] = ' ';
		time[n + 1] = 0;
	}

	int dlen = strlen(date);
	for (int n = dlen; (n < 8) && (n < (TEMP_STR_LEN - 1)); n++) {
		date[n] = ' ';
		date[n + 1] = 0;
	}

	display_draw_tinytext(128 - 75, 2, time, header_color);
	display_draw_tinytext(128 - 75, 9, date, header_color);
}

/**
 * Update the value of a VARNUM in the varnum_values table
 */
void update_item_varnum(screen_item &item, const void *value) {
	set_item_state_varnum(item.text, ((uint8_t *) value)[0]);
}

/**
 * Update our delay countdown, which is used for calibrations.
 */
void update_item_delay(screen_item &item, const void *value) {

	// Because of the way the main loop works, this function will be called before
	// the first screen render, so this is where we initially setup the delay_time.
	if (clear_next_render == true) {
		// parse out delay time
		delay_time = str_to_uint(item.text + 9);
	} else {

		if (delay_time >= 1)
			delay_time--;

		// 1 second delay
		delay_us(1000000);
		display_draw_number(item.val1, item.val2, delay_time, 3, FOREGROUND_COLOR, background_color);

		if (delay_time > 5) {
			buzzer_morse("E");  // .
		} else if (delay_time > 1 ){
			buzzer_morse("I");  // ..
		} else {
			buzzer_morse("H"); // ....
		}
	}
}

/**
 * Find the menu number for the destination after a delay (second argument
 * in the menu text)
 */
int get_item_state_delay_destination(screen_item &item) {
	// parse out destination screen

	for (int n = 10; n < 16; n++) {
		if (!((item.text[n] >= '0') && (item.text[n] <= '9'))) {
			uint32_t destination_screen_start = n + 1;
			int dest = str_to_uint(item.text + destination_screen_start);
			return dest;
		}
	}
	return 0;
}

/**
 * Update a part of the screen. Called when we receive an update from the
 * controller where the "text" of the item is the same as the text coming
 * from the controller
 *
 * @param item  type of item to draw
 * @param value value of the item to draw
 */
void update_item(screen_item &item, const void *value) {
	if (item.type == ITEM_TYPE_VARLABEL) {
		if (item.val1 == 255) {
			display_draw_text_center(item.val2, (char *) value, FOREGROUND_COLOR,
			background_color);
		} else {
			display_draw_text(item.val1, item.val2, (char *) value, FOREGROUND_COLOR,
			background_color);
		}
	} else if (item.type == ITEM_TYPE_RED_VARLABEL) {
		// Convention: if first character is " " (space) then clear the label
		if (((char*)value)[0] == ' ') {
			display_draw_text(item.val1, item.val2, (char *) value, COLOR_WHITE, background_color);
		} else {
			display_draw_text(item.val1, item.val2, (char *) value, COLOR_WHITE, COLOR_RED);
		}
	} else if ((item.type == ITEM_TYPE_SOFTKEY)
			|| (item.type == ITEM_TYPE_SOFTKEY_ACTION)) {
		update_item_softkey(item, (char*) value);
	} else if (item.type == ITEM_TYPE_GRAPH) {
		update_item_graph(item, value);
	} else if (item.type == ITEM_TYPE_HEAD) {
		update_item_head(item, value);
	} else if (item.type == ITEM_TYPE_VARNUM) {
		update_item_varnum(item, value);
	} else if (item.type == ITEM_TYPE_DELAY) {
		update_item_delay(item, value);
	} else if (item.type == ITEM_TYPE_BIGVARLABEL) {
		display_draw_bigtext(item.val1, item.val2, (char *) value, FOREGROUND_COLOR,
		background_color);
	}
}

/**
 * The GUI object. Interacts with the Controller object, manipulated from
 * the main firmware event loop in main.cpp
 */
GUI::GUI(Controller &r) :
		controller(r) {

	m_repeated = false;
	m_repeat_time = 0;
	m_repeat_delay = 8;
	m_repeating = false;
	new_keys_start = 0;
	new_keys_end = 0;
	clear_next_render = false;
	current_screen = 0;
	selected_item = 1;
	last_selected_item = 1;
	m_trigger_any_key = false;
	m_redraw = false;
	m_screen_lock = false;
	m_language = LANGUAGE_ENGLISH;
	m_displaying_dialog = false;
	m_displaying_dialog_complete = false;
	m_dialog_buzz = false;
	m_pause_display_updates = false;
}


void GUI::set_cpm_alarm(bool alrm, bool silent, float cpm) {
	if (alrm) {
		background_color = COLOR_RED;
		if (!silent)
			m_dialog_buzz = true;
	} else {
		background_color = COLOR_BLACK;
		m_dialog_buzz = false;
	}
	display_clear(background_color);
	first_render = true;
//	redraw();
}

void GUI::show_dialog(const char *dialog_text1, const char *dialog_text2,
		const char *dialog_text3, const char *dialog_text4, bool buzz, int img1,
		int img2, int img3, int img4) {
	display_draw_rectangle(0, 0, 128, 128, background_color);
	m_dialog_buzz = buzz;
	m_displaying_dialog = true;
	m_displaying_dialog_complete = false;
	m_pause_display_updates = true;
	render_dialog(dialog_text1, dialog_text2, dialog_text3, dialog_text4, img1,
			img2, img3, img4);
}

/**
 * The main render loop of the GUI. Called about twice per second.
 *
 * TODO: do not update parts of the screen which have not changed!
 */
void GUI::render() {

	if (m_dialog_buzz)
		buzzer_nonblocking_buzz(0.2, true, false);

	// We check for the buzzer before checking for m_sleeping, because
	// the alarm system can be triggered during a logging period. This will
	// turn the screen red (no display)
	if (controller.m_sleeping) {
		return;
	}

	if (m_displaying_dialog_complete) {
		m_displaying_dialog_complete = false;
		m_pause_display_updates = false;
		display_clear(background_color);
		clear_pending_keys();
		jump_to_screen(0);
		return;
	}

	if (m_repeating) {
		// This would be better incremented in a timer, but I don't want to use another timer.
		if (m_repeat_time == m_repeat_delay) {
			// verify button still pressed
			if (cap_ispressed(m_repeat_key) == false) {
				m_repeating = false;
				m_repeat_time = 0;
			} else {
				if (m_repeat_key == KEY_DOWN) {
					receive_key(KEY_DOWN, KEY_PRESSED);
				}
				if (m_repeat_key == KEY_UP) {
					receive_key(KEY_UP, KEY_PRESSED);
				}
				m_repeat_time = 0;
				m_repeated = true;
			}
		}
		m_repeat_time++;
	}

	if (!m_pause_display_updates) {

		if (clear_next_render) {
			clear_screen();
			clear_next_render = false;
			first_render = true;
		}

		// Now render all items on the screen:
		for (int32_t n = 0; n < screens_layout[current_screen].item_count; n++) {
			if (first_render) {
				if (screens_layout[current_screen].items[n].type == ITEM_TYPE_ACTION) {
					controller.receive_gui_event(
							screens_layout[current_screen].items[n].text,
							screens_layout[current_screen].items[n].text);
				}
				// Invalidate softkeys to force re-render next update.
				softkeys[0][0] = 1;
				softkeys[1][0] = 1;
				softkeys[2][0] = 1;
			}

			bool select_render = ((n == selected_item) || (n == last_selected_item));
			if (first_render || select_render || m_redraw) {
				render_item(screens_layout[current_screen].items[n], (selected_item == n));
			}
		}

		render_lock(m_screen_lock);
		// We need to check for the first_render flag, because in case there is
		// an ITEM_TYPE_ACTION in the loop above that resets the m_redraw flag to 'true',
		// we need that flag to remain true until the next rendering loop.
		if (!first_render)
			m_redraw = false;
		first_render = false;
	}
	process_keys();
}

/**
 * Clears the screen...
 */
void GUI::clear_screen() {
	varnum_size = 0;
	CLS(background_color);
}

void GUI::set_key_trigger() {
	m_trigger_any_key = true;
}

void GUI::redraw() {
	m_redraw = true;
}

/**
 * Called by the Captouch driver whenever a keypress is detected.
 * Simply adds the keypresses to a key buffer that is processed in
 * process_keys when that method is called.
 */
void GUI::receive_key(int key_id, int type) {

	if (m_displaying_dialog == true) {
		m_displaying_dialog = false;
		m_displaying_dialog_complete = true;
		return;
	}

	// We discard keys until the screen is undimmed
	// Whenever we press a key, we actually get two events:
	// - Keypress
	// - Key release
	// --> We need to use the discard_next_keypress to discard
	//     both events
	if (controller.m_screen_dimmed) {
		m_discard_next_keypress = true;
		return;
	}

	if (m_discard_next_keypress) {
		m_discard_next_keypress = false;
		return;
	}

	new_keys_key[new_keys_end] = key_id;
	new_keys_type[new_keys_end] = type;

	new_keys_end = ++new_keys_end % NEW_KEYS_MAX_SIZE;

}

void GUI::clear_pending_keys() {
	new_keys_end = 0;
	new_keys_start = 0;
}

/**
 * Process pending key presses which are in the new_keys_key/type buffers
 */
void GUI::process_keys() {

	if (new_keys_start != new_keys_end) {
		process_key(new_keys_key[new_keys_start],
				new_keys_type[new_keys_start]);

		new_keys_start++;
		if (new_keys_start >= NEW_KEYS_MAX_SIZE)
			new_keys_start = 0;
	}

}

void GUI::leave_screen_actions(int screen) {
	for (int n = 0; n < screens_layout[screen].item_count; n++) {
		if (screens_layout[screen].items[n].type == ITEM_TYPE_LEAVE_ACTION) {
			controller.receive_gui_event(
					screens_layout[screen].items[n].text, "Left");
		}
	}
}

void GUI::process_key_down() {
	if (screens_layout[current_screen].items[selected_item].type
			== ITEM_TYPE_VARNUM) {
		uint8_t current = get_item_state_varnum(
				screens_layout[current_screen].items[selected_item]);

		int8_t val[1];
		val[0] = current - 1;
		if (val[0] < 0)
			val[0] = 0;
		update_item(screens_layout[current_screen].items[selected_item], val);

		controller.receive_gui_event("varnumchange",
				screens_layout[current_screen].items[selected_item].text);
		return;
	}

	if (((selected_item + 1) < screens_layout[current_screen].item_count)
			&& (screens_layout[current_screen].items[selected_item + 1].type
					!= ITEM_TYPE_SOFTKEY)) {
		last_selected_item = selected_item;
		selected_item++;
	}
}

void GUI::process_key_up() {
	if (screens_layout[current_screen].items[selected_item].type
			== ITEM_TYPE_VARNUM) {
		uint8_t current = get_item_state_varnum(
				screens_layout[current_screen].items[selected_item]);

		int8_t val[1];
		val[0] = current + 1;
		if (val[0] > 9)
			val[0] = 9;
		update_item(screens_layout[current_screen].items[selected_item], val);
		controller.receive_gui_event("varnumchange",
				screens_layout[current_screen].items[selected_item].text);
		return;
	}

	if (selected_item == 1)
		return;
	last_selected_item = selected_item;
	selected_item--;
}

/**
 * Check if there is a softkey definition in the current screen.
 * Starts at the end since this is usually where softkey definitions are.
 */
bool GUI::softkey_active(uint8_t keynum) {
	if (keynum > 2)
		return false;
	for (int i = screens_layout[current_screen].item_count - 1; i >= 0; i--) {
		if ( (screens_layout[current_screen].items[i].val1 == keynum) &&
				((screens_layout[current_screen].items[i].type == ITEM_TYPE_SOFTKEY)
				|| (screens_layout[current_screen].items[i].type
						== ITEM_TYPE_SOFTKEY_ACTION))
				)
			return true;
	}
	return false;
}

/**
 * Returns the index of the screen for that soft key.
 * Note that we do not assume a particular location for the softkey definition
 * Returns 255 in case soft keys are not active
 */
uint8_t GUI::softkey_screen(uint8_t idx) {
	if (!softkey_active(idx))
		return INVALID_SCREEN;

	// Scan the whole screen definition until we find the correct softkey
	for (int i = screens_layout[current_screen].item_count - 1; i >= 0; i--) {
		if ((screens_layout[current_screen].items[i].type == ITEM_TYPE_SOFTKEY)
				&& (screens_layout[current_screen].items[i].val1 == idx))
			return screens_layout[current_screen].items[i].val2;
	}
	return INVALID_SCREEN;
}

/**
 * Returns the text of an action softkey, or NULL (0) if we did not
 * find the softkey or it is not an action key.
 */
char* GUI::softkey_action(uint8_t idx) {
	if (!softkey_active(idx))
		return NULL;

	// Scan the whole screen definition until we find the correct softkey
	for (int i = screens_layout[current_screen].item_count - 1; i >= 0; i--) {
		if ((screens_layout[current_screen].items[i].type
				== ITEM_TYPE_SOFTKEY_ACTION)
				&& (screens_layout[current_screen].items[i].val1 == idx))
			return screens_layout[current_screen].items[i].text;
	}
	return NULL;
}

/**
 * Returns the index of a softkey in the current menu, or 255 if we did not
 * find the softkey.
 */
uint8_t GUI::softkey_index(uint8_t idx) {
	if (!softkey_active(idx))
		return NULL;

	// Scan the whole screen definition until we find the correct softkey
	for (uint8_t i = screens_layout[current_screen].item_count - 1; i >= 0;
			i--) {
		if (((screens_layout[current_screen].items[i].type
				== ITEM_TYPE_SOFTKEY_ACTION)
				|| (screens_layout[current_screen].items[i].type
						== ITEM_TYPE_SOFTKEY))
				&& (screens_layout[current_screen].items[i].val1 == idx))
			return i;
	}
	return 255;
}

/**
 * Process a key press.
 * key_id is the name of the key
 * type is the type of even (pressed or released)
 */
void GUI::process_key(int key_id, int type) {

	// Don't react to locked keyboards!
	if (m_screen_lock)
		return;

	// Typically used when displaying a dialog (or similar)
	if (m_trigger_any_key) {
		controller.receive_gui_event("KEYPRESS", "any");
		m_trigger_any_key = false;
	}

	if (controller.m_sleeping)
		return;

	// This used to be for the help system, now just softkey 2
	if ((key_id == KEY_HELP) && (type == KEY_RELEASED)) {
			// We want softkey 2 here
			if (softkey_action(2) != NULL) {
				// We have an action key, process it:
				controller.receive_gui_event(softkey_action(2),
						"select");
				// Flag the softkey for re-rendering
				last_selected_item = softkey_index(2);
				return;
			} else if (softkey_screen(2) != INVALID_SCREEN) {
				jump_to_screen(softkey_screen(2));
				return;
			}
	}

	if ((key_id == KEY_UP) && (type == KEY_PRESSED) && !softkey_active(1)) {
		process_key_up();
		m_repeating = true;
		m_repeat_key = KEY_UP;
	}

	if ((key_id == KEY_DOWN) && (type == KEY_PRESSED)) {
		process_key_down();
		m_repeating = true;
		m_repeat_key = KEY_DOWN;
	}

	if ((key_id == KEY_DOWN) && (type == KEY_RELEASED)) {
//    if(!m_repeated) process_key_down();
		m_repeating = false;
		m_repeated = false;
	}

	if ((key_id == KEY_UP) && (type == KEY_RELEASED)) {
		// If softkeys are active, we act on key release only.
			// We want softkey 1 here
			if (softkey_action(1) != NULL) {
				// We have an action key:
				controller.receive_gui_event(softkey_action(1),
						"select");
				last_selected_item = softkey_index(1);
				return;
			} else if (softkey_screen(1) != INVALID_SCREEN) {
				jump_to_screen( softkey_screen(1));
				return;
			}
		m_repeating = false;
		m_repeated = false;
	}

	if ((key_id == KEY_SELECT) && (type == KEY_RELEASED)) {

		// if a VARNUM is selected...
		if (screens_layout[current_screen].items[selected_item].type
				== ITEM_TYPE_VARNUM) {
			if (selected_item != 0) {
				if (((selected_item + 1) < screens_layout[current_screen].item_count) &&
					(screens_layout[current_screen].items[selected_item+1].type
										== ITEM_TYPE_VARNUM)
						) {
					last_selected_item = selected_item;
					selected_item++;
					return;
				}
			}
		}

		if (screens_layout[current_screen].items[selected_item].type
				== ITEM_TYPE_MENU) {
			if (screens_layout[current_screen].items[selected_item].val1
					!= INVALID_SCREEN) {
				jump_to_screen(screens_layout[current_screen].items[selected_item].val1);
				return;
			}
		} else if (screens_layout[current_screen].items[selected_item].type
				== ITEM_TYPE_MENU_ACTION) {
			controller.receive_gui_event(
					screens_layout[current_screen].items[selected_item].text,
					"select");
			return;
		}

	}

	if ((key_id == KEY_BACK) && (type == KEY_RELEASED)) {

		// if a varnum is selected
		if (screens_layout[current_screen].items[selected_item].type
				== ITEM_TYPE_VARNUM) {
			if (selected_item != 0) {
				if ((selected_item - 1) >= 0) {
					if (screens_layout[current_screen].items[selected_item - 1].type
							== ITEM_TYPE_VARNUM) {
						last_selected_item = selected_item;
						selected_item--;
						controller.receive_gui_event("varnumchange",
								screens_layout[current_screen].items[selected_item].text);
						return;
					}
				}
			}
		}

		if ((current_screen != 0) && (!clear_next_render)) {
				clear_next_render = true;
				first_render = true;
				leave_screen_actions(current_screen);
		}
	}

	if ((key_id == KEY_HOME) && (type == KEY_RELEASED)) {
				// We want softkey 0 here
				if (softkey_action(0) != NULL) {
					// We have an action key:
					controller.receive_gui_event(softkey_action(0),
							"select");
					// Flag the softkey for re-rendering
					last_selected_item = softkey_index(0);
					return;
				} else if (softkey_screen(0) != INVALID_SCREEN) {
					jump_to_screen(softkey_screen(0));
					return;
				} else if (current_screen != 0) {
					// Go to "Home" screen
					jump_to_screen(0);
				}
	}
}

/**
 * Direct jump to another screen. Autoselects the first item on
 * that screen.
 */
void GUI::jump_to_screen(uint8_t screen) {
	leave_screen_actions(current_screen);
	clear_next_render = true;
	current_screen = screen;
	last_selected_item = 1;
	selected_item = 1;

	// We need this to make sure the labels for the
	// "next mode" label are correct:
	controller.event_next_opmode(false);
}

/**
 * Received GUI update events sent by the Controller. Check if the
 * currently displayed screen template contains the tag, and update
 * if on screen if it does. If the current screen does not display the
 * tag, then do nothing
 *
 * @param tag   name of the variable to update
 * @param value value of the variable to update
 */
void GUI::receive_update(const char *tag, const void *value) {

	if (m_pause_display_updates)
		return;

	for (uint32_t n = 0; n < screens_layout[current_screen].item_count; n++) {
		if (strcmp(tag, screens_layout[current_screen].items[n].text) == 0) {
			update_item(screens_layout[current_screen].items[n], value);

			// has to be in the GUI object, because we don't have access to current_screen outside it.
			if (screens_layout[current_screen].items[n].type == ITEM_TYPE_DELAY) {
				if (delay_time == 0) {
					jump_to_screen(
							get_item_state_delay_destination(
									screens_layout[current_screen].items[n]));
				}
			}
		}
	}
}

void GUI::toggle_screen_lock() {

	if (m_screen_lock == true)
		m_screen_lock = false;
	else
		m_screen_lock = true;
}

uint8_t GUI::get_item_state_uint8(const char *tag) {
	// should check item type and repsond appropriately, however only varnum currently returns uint8_t
	return get_item_state_varnum(tag);
}

void GUI::set_language(uint8_t lang) {
	m_language = lang;
}

void GUI::render_dialog(const char *text1, const char *text2, const char *text3,
		const char *text4, int img1, int img2, int img3, int img4) {

	if (m_language == LANGUAGE_JAPANESE) {
		if (img1 == 255) {
			display_draw_text_center(20, text1, FOREGROUND_COLOR, background_color);
		} else if (img1 == 254) {
		} else {
			display_draw_fixedimage(0, 20, img1, background_color);
		}

		if (img2 == 255) {
			display_draw_text_center(36, text2, FOREGROUND_COLOR, background_color);
		} else if (img2 == 254) {
		} else {
			display_draw_fixedimage(0, 36, img2, background_color);
		}

		if (img3 == 255) {
			display_draw_text_center(52, text3, FOREGROUND_COLOR, background_color);
		} else if (img3 == 254) {
		} else {
			display_draw_fixedimage(0, 52, img3, background_color);
		}

		if (img4 == 255) {
			display_draw_text_center(68, text4, FOREGROUND_COLOR, background_color);
		} else if (img4 == 254) {
		} else {
			display_draw_fixedimage(0, 68, img4, background_color);
		}

		display_draw_fixedimage(0, 94, 49, background_color); // press any key kanji image
	}

	if (m_language == LANGUAGE_ENGLISH) {
		display_draw_text_center(20, text1, FOREGROUND_COLOR, background_color);
		display_draw_text_center(36, text2, FOREGROUND_COLOR, background_color);
		display_draw_text_center(52, text3, FOREGROUND_COLOR, background_color);

		display_draw_text_center(68, text4, FOREGROUND_COLOR, background_color);
		display_draw_text_center(94, "PRESS ANY KEY", FOREGROUND_COLOR, background_color);
	}
}
