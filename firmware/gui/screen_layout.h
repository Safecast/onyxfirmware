#ifndef OS100_LAYOUT_H
#define OS100_LAYOUT_H

#include <stdint.h>

#define TEXT_LENGTH 17
#define SCREEN_COUNT 6

#define ITEM_TYPE_MENU         0
#define ITEM_TYPE_LABEL        1
#define ITEM_TYPE_SELECTNUM    2
#define ITEM_TYPE_SELECTION    3
#define ITEM_TYPE_VARLABEL     4
#define ITEM_TYPE_GRAPH        5
#define ITEM_TYPE_HEAD         6
#define ITEM_TYPE_MENU_ACTION  7
#define ITEM_TYPE_VARNUM       8

#define INVALID_SCREEN 255

struct screen_item {
  uint8_t type;
  uint8_t val1;
  uint8_t val2;
  char          text[TEXT_LENGTH];
};

struct screen {
  uint8_t       item_count;
  screen_item   items[8];
};

extern screen screens_layout[SCREEN_COUNT];

#endif
