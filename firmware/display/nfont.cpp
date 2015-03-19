#ifndef NFONT
#define NFONT

#include <stdio.h>
#include <string.h>
#include "nfont.h"
#include <stdint.h>
#include "oled.h"
#include "display.h"

extern uint8_t _binary___binary_data_font_data_start;
extern uint8_t _binary___binary_data_font_data_size;

extern uint8_t _binary___binary_data_tinyfont_data_start;
extern uint8_t _binary___binary_data_tinyfont_data_size;

extern uint8_t _binary___binary_data_bignumbers_data_start;
extern uint8_t _binary___binary_data_bignumbers_data_size;

#define to565(r,g,b)                                            \
    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

#define from565_r(x) ((((x) >> 11) & 0x1f) * 255 / 31)
#define from565_g(x) ((((x) >> 5) & 0x3f) * 255 / 63)
#define from565_b(x) (((x) & 0x1f) * 255 / 31)

/**
 * Get the bitmap for a Large number. value of c is 0 to 10
 * for digits 0 to 9 and 10 is '.'
 *
 * Note: our bitmap is in black characters over a white background,
 *       so in practice, we are returning a mask rather than a character
 */
uint16_t get_bigpixel(char c, int c_x, int c_y) {

	// Get Y and X offset for this character
	// Big numbers are 30 pixels wide, and 43 high
	int ypos = (c / (128 / 30)) * 43;
	int xpos = (c % (128 / 30)) * 30;

	int bitposition = ((ypos * 128) + (c_y * 128) + (xpos) + c_x) * 2;

	int byte_position = bitposition / 8;
	int bit_in_byte = 7 - (bitposition % 8);

	uint8_t byte =
			((uint8_t *) &_binary___binary_data_bignumbers_data_start)[byte_position];

	uint8_t value = 0;
	if ((byte & (1 << bit_in_byte)) > 0)
		value += 2;

	bit_in_byte--;
	if ((byte & (1 << bit_in_byte)) > 0)
		value += 1;

	if (value == 0) {
		return 0;
	}
	if (value == 1) {
		return to565(85, 85, 85);
	}
	if (value == 2) {
		return to565(170, 170, 170);
	}
	if (value == 3) {
		return to565(255, 255, 255);
	}

	return 0;
}

/**
 * Get a pixel value for the standard font. This is a simple greyscale bitmap
 * with 2 bits depth (see the Makefile to see how it is generated)
 *
 * Returns an alpha value from 0 to 255
 *
 * Note: characters are black over a white background, so we are actually
 *       returning a character mask, not a character bitmap.
 */
uint8_t get_pixel(char c, int c_x, int c_y) {

	int ypos = (c / (128 / 8)) * 16;
	int xpos = (c % (128 / 8)) * 8;
	ypos += 1;

	int bitposition = ((ypos * 128) + (c_y * 128) + (xpos) + c_x) * 2;

	int byte_position = bitposition / 8;
	int bit_in_byte = 7 - (bitposition % 8);

	uint8_t byte =
			((uint8_t *) &_binary___binary_data_font_data_start)[byte_position];

	uint8_t value = 0;
	if ((byte & (1 << bit_in_byte)) > 0)
		value += 2;

	bit_in_byte--;
	if ((byte & (1 << bit_in_byte)) > 0)
		value += 1;

	// Return the pixel value
	if (value == 0) {
		return 0;;
	}
	if (value == 1) {
		return 85;
	}
	if (value == 2) {
		return 170;
	}
	if (value == 3) {
		return 255;
	}

	return 0;
}

uint16_t get_tinypixel(char c, int c_x, int c_y) {

	int ypos = (c / (120 / 5)) * 5;
	int xpos = (c % (120 / 5)) * 5;

	int bitposition = ((ypos * 120) + (c_y * 120) + (xpos) + c_x);

	int byte_position = bitposition / 8;
	int bit_in_byte = 7 - (bitposition % 8);	//was8-
	uint8_t byte =
			((uint8_t *) &_binary___binary_data_tinyfont_data_start)[byte_position];

	uint8_t value = 0;
	if ((byte & (1 << bit_in_byte)) > 0)
		value = 1;

	if (value == 0) {
		value = 0;
	}
	if (value == 1) {
		value = 255;
	}

	return to565(value, value, value);
}

/**
 * Draws one character in the standard font (8x16)
 *  background is the screen background color to use (character is drawn
 *  in black over the background, unless background is black, and character
 *  is then drawn in white).
 *
 *  Note: if necessary, we could introduce special case optimizations in case
 *  we are drawing in black/white, but in practice, it's impossible to see
 *  the difference, because calculations are faster than the screen refresh rate,
 *  as far as I can tell.
 */
void draw_character(uint32_t x, uint32_t y, char c, uint16_t foreground, uint16_t background) {

	uint16_t character_data[8 * 16];

	for (size_t c_y = 0; c_y < 16; c_y++) {
		for (size_t c_x = 0; c_x < 8; c_x++) {
			uint8_t px = get_pixel(c - 32, c_x, c_y);
			int32_t value;
			// Write in black over the background:
			if (px == 255) {
				value = background;
			} else if (px == 0) {
				value = foreground;
			} else {
				uint16_t rb = from565_r(background);
				uint16_t gb = from565_g(background);
				uint16_t bb = from565_b(background);
				uint16_t rf = from565_r(foreground);
				uint16_t gf = from565_g(foreground);
				uint16_t bf = from565_b(foreground);
				value = to565((rb * px + rf * (255 - px)) / 255,
						      (gb * px + gf * (255 - px)) / 255,
						      (bb * px + bf * (255 - px)) / 255 );
			}
			character_data[(c_y * 8) + c_x] = value;
		}
	}
	oled_draw_rect(x, y, 8, 16, (uint8_t *) character_data);
}

/**
 * Draws a character in "big" font. Currently only supports numbers.
 * Big numbers are 30 pixels wide, and 43 high
 *
 */
void draw_bigcharacter(int x, int y, char c, uint16_t background) {

	uint16_t character_data[43 * 30];

	// Our big font only contains digits, so fallback to standard font in case
	// we're asked to draw something else:
	if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) {
		oled_draw_rect(x, y, 30, 43, (uint8_t *) character_data);
		draw_character(x, y, c, COLOR_WHITE, background);
		return;
	}

	if ((c >= '0') && (c <= '9'))
		c = c - 48;
	else if (c == '.')
		c = 10;

	if (c != ' ') {
		for (size_t c_y = 0; c_y < 43; c_y++) {
			for (size_t c_x = 0; c_x < 30; c_x++) {
				// get_pixel returns a mask - white background
				// and black/grey character.
				int32_t px = get_bigpixel(c, c_x, c_y);
				int32_t value;

				if (background == COLOR_WHITE) {
					value = px;
				} else if (background == COLOR_BLACK) {
					value = ~px;
				} else {
					if (px == 65535) {
						value = background;
					} else if (px == 0) {
						value = 0;
					} else {
						int r = from565_r(px);
						int g = from565_g(px);
						int b = from565_b(px);
						value = to565(r / background, g / background,
								b / background);
					}
				}
				character_data[(c_y * 30) + c_x] = value;
			}
		}
		oled_draw_rect(x, y, 30, 43, (uint8_t *) character_data);
	} else {
		// Space: just create an empty space
		display_draw_rectangle(x, y, 30, 43, background);
	}
}

/**
 * Special case: draw a big dot
 */
void draw_bigdot(int x, int y, uint16_t background) {

	uint16_t character_data[8 * 7];

	for (size_t c_y = 31; c_y < 39; c_y++) {
		for (size_t c_x = 2; c_x < 10; c_x++) {
			int32_t px = get_bigpixel(10, c_x, c_y);
			int32_t value;
			if (background == COLOR_WHITE) {
				value = px;
			} else if (background == COLOR_BLACK) {
				value = ~px;
			} else {
				if (px == 65535) {
					value = background;
				} else {
					int r = from565_r(px);
					int g = from565_g(px);
					int b = from565_b(px);
					value = to565(r / background, g / background,
							b / background);
				}
			}
			character_data[((c_y - 31) * 8) + c_x - 2] = value;
		}
	}
	oled_draw_rect(x, y + 31, 8, 7, (uint8_t *) character_data);
}

/**
 * Draw one character in tiny font.
 * background: if zero: background is white (?) and 65535 is black.
 */
void draw_tinycharacter(int x, int y, char c, uint16_t background) {

	uint16_t character_data[5 * 5];
	for (int n = 0; n < (5 * 5); n++)
		character_data[n] = n;

	for (size_t c_y = 0; c_y < 5; c_y++) {
		for (size_t c_x = 0; c_x < 5; c_x++) {
			int32_t px = get_tinypixel(c - 32, c_x, c_y);
			int32_t value = 9;

			if ((background != 0) && (background != 65535)) {
				if (px == 65535) {
					value = background;
				}
			} else {
				int r = from565_r(px);
				int g = from565_g(px);
				int b = from565_b(px);
				value = to565(r / background, g / background, b / background);
			}

			if (value < 0)
				value = 0;

			if (background == 65535)
				value = background ^ get_tinypixel(c - 32, c_x, c_y);
			if (background == 0)
				value = get_tinypixel(c - 32, c_x, c_y);

			character_data[(c_y * 5) + c_x] = value;
		}
	}

	oled_draw_rect(x, y, 5, 5, (uint8_t *) character_data);
}

void draw_text(int x, int y, const char *text, uint16_t foreground,
		uint16_t background) {

	size_t length = strlen(text);
	if (length < 0)
		return;
	if (length > 10000)
		return;

	uint32_t c_x = x;
	uint32_t c_y = y;
	for (size_t n = 0; n < length; n++) {
		draw_character(c_x, c_y, text[n], foreground, background);
		c_x += 8;
	}
}

/**
 * Draw a large text (43px high characters, the screen can take 4 chars wide)
 * One note: only digits are supported. We do fancy stuff with the dot so that
 * it does not take extra space.
 */
void draw_bigtext(int x, int y, const char *text, uint16_t background) {

	bool dot = false;
	int dot_x = 0;
	uint32_t length = strlen(text);
	if ((length < 0) || (length > 6))
		return;

	int c_x = x;
	int c_y = y;
	for (size_t n = 0; n < length; n++) {
		if (text[n] == '.') {
			dot = true;
			// Clear the area to avoid artifacts
			uint16_t character_data[8 * 31];
			for (int n = 0; n < (8 * 31); n++)
				character_data[n] = 0;
			oled_draw_rect(c_x, c_y, 8, 31, (uint8_t *) character_data);
			dot_x = c_x - ((c_x > 3) ? 3 : 0);
			c_x += 6;
		} else {
			draw_bigcharacter(c_x, c_y, text[n], background);
			c_x += 30;
		}
	}
	if (dot)
		draw_bigdot(dot_x, c_y, background);
}

/**
 * Draw a text in tiny font.
 */
void draw_tinytext(int x, int y, const char *text, uint16_t background) {

	uint32_t length = strlen(text);
	if (length < 0)
		return;
	if (length > 10000)
		return;

	int c_x = x;
	int c_y = y;
	for (size_t n = 0; n < length; n++) {
		draw_tinycharacter(c_x, c_y, text[n], background);
		c_x += 6;
	}
}

#endif
