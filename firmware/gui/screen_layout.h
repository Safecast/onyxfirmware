#ifndef OS100_LAYOUT_H
#define OS100_LAYOUT_H

#include "safecast_wirish_types.h"
#include <stdint.h>

#define TEXT_LENGTH 25
#define SCREEN_COUNT 27


// The constants below are the type of items that can be drawn
// on the screen.

#define ITEM_TYPE_MENU            0  // A normal menu item
#define ITEM_TYPE_LABEL           1  // A static label, can not be changed
#define ITEM_TYPE_SOFTKEY		  2  // A soft key label, a the bottom of the screen
#define ITEM_TYPE_SELECTION       3  // An item selection, currently not implemented
#define ITEM_TYPE_VARLABEL        4  // A variable label, can be set to a value in software
#define ITEM_TYPE_GRAPH           5  // System graph.
#define ITEM_TYPE_HEAD            6  // Display HUD. Shows CPM etc.
#define ITEM_TYPE_MENU_ACTION     7  // selection triggers a GUI event, which the Controller receives.
#define ITEM_TYPE_VARNUM          8  // Variable single number
#define ITEM_TYPE_DELAY           9  // A delay, basically a countdown timer.
#define ITEM_TYPE_ACTION         10  // Generate an action as soon as the screen is created.
									 // Usually leads to a screen redraw, so it has to be last on the menu definition.
#define ITEM_TYPE_BIGVARLABEL    11  // A bigfont variable label, currently only numeric.
#define ITEM_TYPE_SMALLLABEL     12  // A tinyfont static label
#define ITEM_TYPE_LEAVE_ACTION   13  // Action that occurs when you leave the screen
#define ITEM_TYPE_SOFTKEY_ACTION 14  // A soft key label that triggers a GUI event, which the Controller receives
#define ITEM_TYPE_RED_VARLABEL   15  // A variable label with a red background, can be set to a value in software
#define ITEM_TYPE_INVALID		255  // Invalid type

#define INVALID_SCREEN 255

/**
 * the GUI displays "Screens". These screens are drawn based on a screen
 * layout templated, defined in the "screen" struct below, that contains
 * screen items, defined in the screen_item struct below.
 */

/**
 * An item to be displayed on a screen template. An item contains a
 * type, as defined in ITEM_TYPE_* and several variables, which are used
 * or not depending on item type:
 *
 * ITEM_TYPE_HEAD : val1, val2, and kanji_image are unused.
 *                  text: name of the variable to display in header
 * ITEM_TYPE_MENU : val1: Screen index of the menu (starts at 1)
 *                  val2: Line on screen where menu should be drawn
 *                  text: English text of menu entry
 *                  kanji_image: index of Kanji of menu entry
 * ITEM_TYPE_LABEL : val1: x position. If 255, draw label centered on screen
 *                   val2: y position
 *                   text, kanji_image: label text
 *
 * ITEM_TYPE_VARNUM: val1, val2: x,y position on screen
 *                   text: name of the variable on the screen (one
 *                         name per digit) If the variable is not numeric,
 *                         you can use the following syntax:
 *                         "NAME:char1,char2,char3...". See UTC offset menu
 *                         for instance: "SIGN:-,+,"
 *                   kanji_image:not used ?
 *
 */
struct screen_item {
  uint8_t type;
  uint8_t val1;
  uint8_t val2;
  char    text[TEXT_LENGTH];
  uint8_t kanji_image;
};

/**
 * Screen layout template. All templates are defined in screen_layout.cpp and
 * stored as flash variables since they are constant, this way they do not take
 * space in our RAM.
 *
 * A screen contains up to 11 items and a reference to a help screen (optional)
 */
struct screen {
  uint8_t       item_count;
  screen_item   items[13];
  uint8_t help_screen;
};

extern __FLASH__ screen screens_layout[SCREEN_COUNT];

#endif
