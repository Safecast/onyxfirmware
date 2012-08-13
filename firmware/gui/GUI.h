#ifndef GUI_H
#define GUI_H

#include "screen_layout.h"
#include <stdint.h>
#include "display.h"
#include "UserInput.h"

#define MAX_SCREEN_STACK 10

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
  Controller &receive_gui_events;

private:
  int32_t selected_screen_stack[MAX_SCREEN_STACK];
  int32_t selected_item_stack  [MAX_SCREEN_STACK];
  int32_t selected_stack_size;

  void pop_stack(int &current_screen,int &selected_item);
  void push_stack(int current_screen,int selected_item);
  void clear_stack();

  void clear_screen(int32_t c_screen,int32_t c_selected);
  int32_t clear_screen_selected;
  int32_t clear_screen_screen;
  bool    clear_next_render;
  bool    m_trigger_any_key;
  bool    m_sleeping;
};

#endif
