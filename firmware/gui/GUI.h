#ifndef GUI_H
#define GUI_H

#include "screen_layout.h"
#include <stdint.h>
#include "display.h"
#include "UserInput.h"

#define MAX_SCREEN_STACK 10
#define NEW_KEYS_MAX_SIZE 10
#define FOREGROUND_COLOR 65535
#define BACKGROUND_COLOR 0
#define HEADER_COLOR_NORMAL 0xF800
#define HEADER_COLOR_CPMINVALID 0x00E0

class Controller;

class GUI {

private:
  int current_screen;
  int selected_item;

public:

  GUI(Controller &r);

  void render();
  void receive_touch(int key_id,int type);
  void receive_update(const char *tag,void *value);
  void receive_key(int key,int type);
  void set_key_trigger();
  void redraw();
  void set_sleeping(bool sleeping);
  void jump_to_screen(const char screen);
  void push_stack(int current_screen,int selected_item);
  void toggle_screen_lock();
  Controller &receive_gui_events;

  uint8_t get_item_state_uint8(const char *tag);
private:
  int32_t selected_screen_stack[MAX_SCREEN_STACK];
  int32_t selected_item_stack  [MAX_SCREEN_STACK];
  int32_t selected_stack_size;

  void pop_stack(int &current_screen,int &selected_item);
  void clear_stack();

  void process_key(int key_id,int type);
  void process_keys();

  void clear_screen(int32_t c_screen,int32_t c_selected);
  int32_t clear_screen_selected;
  int32_t clear_screen_screen;
  bool    clear_next_render;
  bool    m_trigger_any_key;
  bool    m_sleeping;
  bool    m_redraw;
  bool    m_screen_lock;

  int new_keys_size;
  int new_keys_key [NEW_KEYS_MAX_SIZE];
  int new_keys_type[NEW_KEYS_MAX_SIZE];
};


#endif
