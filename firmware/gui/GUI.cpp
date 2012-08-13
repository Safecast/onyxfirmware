#include "screen_layout.h"
#include "Controller.h"
#include <stdint.h>
#include "display.h"
#include "UserInput.h"
#include "GUI.h"
#include <stdint.h>
#include "utils.h"

#define KEY_BACK   6
#define KEY_HOME   8
#define KEY_DOWN   4
#define KEY_UP     3
#define KEY_SELECT 2
#define KEY_PRESSED  0
#define KEY_RELEASED 1
#define BACKGROUND_COLOR 65535


void render_item_menu(screen_item &item, bool selected) {

  uint16_t highlight = 0;
  if(selected) highlight = 65535;

  display_draw_text(0,item.val2*16,item.text,highlight);
}

void render_item_label(screen_item &item, bool selected) {
  display_draw_text(item.val1,item.val2,item.text,0);
}

void render_item_head(screen_item &item, bool selected) {
  display_draw_text(0,0,"os100   CPM ",0x001F);
}

float m_graph_data[30];
float *source_graph_data;
bool  graph_first;

void render_item_graph(screen_item &item, bool selected) {

  int32_t m_x = item.val1;
  int32_t m_y = item.val2;
  
  graph_first = true;
  int32_t size=30;
  int32_t m_size=30;
  for(uint32_t n=0;n<size;n++) {
    if(n >= m_size) {
      display_draw_point(m_x+(n*4),80-source_graph_data[n],0x0000);
    } else {
      if((m_graph_data[n] != source_graph_data[n]) || graph_first) {
	display_draw_point(m_x+(n*4),80-m_graph_data[n],0xFFFF);
	display_draw_point(m_x+(n*4),80-source_graph_data[n],0x0000);
      }
    }
  }

  for(int32_t n=0;n<size;n++) {
    m_graph_data[n] = source_graph_data[n];
  }
  m_size = size;
}

void clear_item_menu(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;
  display_draw_rectangle(0,item.val2*16,(text_len*8)-1,((item.val2+1)*16),BACKGROUND_COLOR);
}

void clear_item_label(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;

  int x_max = item.val1+(text_len*8)-1;
  int y_max = item.val2+16;
  if(x_max>=128) x_max=127;
  if(y_max>=128) y_max=127;
  display_draw_rectangle(item.val1,item.val2,x_max,y_max,BACKGROUND_COLOR);
}

void clear_item_head(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;

  int x_max=127;
  int y_max=16;
  display_draw_rectangle(item.val1,item.val2,x_max,y_max,BACKGROUND_COLOR);
}

void clear_item_varlabel(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;
  display_draw_rectangle(item.val1,item.val2,127,(item.val2+16),BACKGROUND_COLOR);
}

void clear_item_graph(screen_item &item, bool selected) {
  int32_t m_x = item.val1;
  int32_t m_y = item.val2;
  
  int32_t size=30;
  int32_t m_size=30;
  for(uint32_t n=0;n<size;n++) {
    if(n >= m_size) {
      display_draw_point(m_x+(n*4),80-source_graph_data[n],BACKGROUND_COLOR);
    } else {
      display_draw_point(m_x+(n*4),80-m_graph_data[n],BACKGROUND_COLOR);
      display_draw_point(m_x+(n*4),80-source_graph_data[n],BACKGROUND_COLOR);
    }
  }
}

void render_item(screen_item &item,bool selected) {
  if(item.type == ITEM_TYPE_MENU) {
    render_item_menu(item,selected);
  } else
  if(item.type == ITEM_TYPE_LABEL) {
    render_item_label(item,selected);
  } else
  if(item.type == ITEM_TYPE_GRAPH) {
    render_item_graph(item,selected);
  } else 
  if(item.type == ITEM_TYPE_HEAD) {
    render_item_head(item,selected);
  } else 
  if(item.type == ITEM_TYPE_MENU_ACTION) {
    render_item_menu(item,selected);
  }
  
}

void clear_item(screen_item &item,bool selected) {
  if(item.type == ITEM_TYPE_MENU) {
    clear_item_menu(item,selected);
  } else
  if(item.type == ITEM_TYPE_LABEL) {
    clear_item_label(item,selected);
  } else
  if(item.type == ITEM_TYPE_VARLABEL) {
    clear_item_varlabel(item,selected);
  } else
  if(item.type == ITEM_TYPE_GRAPH) {
    clear_item_graph(item,selected);
  } else 
  if(item.type == ITEM_TYPE_HEAD) {
    clear_item_head(item,selected);
  } else
  if(item.type == ITEM_TYPE_MENU_ACTION) {
    clear_item_menu(item,selected);
  }
}

void update_item_graph(screen_item &item,void *value) {
 source_graph_data = (float *)value;
}

void update_item_head(screen_item &item,void *value) {
  draw_text(128-16-16,0,(char *)value,0x001F);
}

void update_item(screen_item &item,void *value) {
  if(item.type == ITEM_TYPE_VARLABEL) {
    display_draw_text(item.val1,item.val2,(char *)value,0);
  } else 
  if(item.type == ITEM_TYPE_GRAPH) {
    update_item_graph(item,value);
  } else 
  if(item.type == ITEM_TYPE_HEAD) {
    update_item_head(item,value);
  }
}
  
void GUI::clear_stack() {
  selected_stack_size=0;
}
  
void GUI::pop_stack(int &current_screen,int &selected_item) {

  if(selected_stack_size == 0) return;

  current_screen = selected_screen_stack[selected_stack_size-1];
  selected_item  = selected_item_stack[selected_stack_size-1];
  selected_stack_size--;
}

void GUI::push_stack(int current_screen,int selected_item) {

  if(selected_stack_size >= MAX_SCREEN_STACK) return;  

  selected_screen_stack[selected_stack_size] = current_screen;
  selected_item_stack  [selected_stack_size] = selected_item;
  selected_stack_size++;
}

GUI::GUI(Controller &r) : receive_gui_events(r) {

  clear_next_render=false;
  current_screen = 0;
  selected_item=1;
  clear_stack();
  m_trigger_any_key=false;
}


bool first_render=true;
void GUI::render() {

  if(m_sleeping) return;  

  // following two items really need to be atomic...
  int32_t cscreen = current_screen;
  int32_t citem   = selected_item;

  if(clear_next_render) {
    clear_next_render=false;
    clear_screen(clear_screen_screen,clear_screen_selected);
    first_render=true;
  }


  for(uint32_t n=0;n<screens_layout[cscreen].item_count;n++) {

    bool selected = false;
    bool near_selected = false;
    if(n == citem) selected = true;

    if(n-1 == citem) near_selected = true;
    if(n+1 == citem) near_selected = true;

    if(n==0) near_selected = false;

    if(first_render || (selected) || (near_selected)) {
      render_item(screens_layout[cscreen].items[n],selected);
    }
  }
  first_render=false;
}

void GUI::clear_screen(int32_t c_screen,int32_t c_selected) {
  for(uint32_t n=0;n<screens_layout[c_screen].item_count;n++) {

    bool selected = false;
    if(n == c_selected) selected = true;

    clear_item(screens_layout[c_screen].items[n],selected);
  }
}

void GUI::set_key_trigger() {

  m_trigger_any_key=true;

}

void GUI::redraw() {
  clear_next_render = true;
}

void GUI::receive_key(int key_id,int type) {

  if(m_trigger_any_key) {
    receive_gui_events.receive_gui_event("KEYPRESS","any");
    m_trigger_any_key=false;
  }

  if(m_sleeping) return;

  if((key_id == KEY_DOWN) && (type == KEY_RELEASED)) {
    selected_item++;
    if(selected_item >= screens_layout[current_screen].item_count) {
      selected_item = screens_layout[current_screen].item_count-1;
    }
  }

  if((key_id == KEY_UP) && (type == KEY_RELEASED)) {
    if(selected_item == 1) return;
    selected_item--;
  }

  if((key_id == KEY_SELECT) && (type == KEY_RELEASED)) {
    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_MENU) {
      if(screens_layout[current_screen].items[selected_item].val1 != INVALID_SCREEN) {
        
        if(clear_next_render == false) {
          clear_next_render = true;
          clear_screen_screen   = current_screen;
          clear_screen_selected = selected_item;
        }

        push_stack(current_screen,selected_item);
	current_screen = screens_layout[current_screen].items[selected_item].val1;
	selected_item = 1;
      }
    } else 
    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_MENU_ACTION) {
      receive_gui_events.receive_gui_event(screens_layout[current_screen].items[selected_item].text,"select");
    }
    
  }

  if((key_id == KEY_BACK) && (type == KEY_RELEASED)) {

    if(selected_stack_size !=0) {
      clear_next_render = true; 
      clear_screen_screen   = current_screen;
      clear_screen_selected = selected_item;

      pop_stack(current_screen,selected_item);
    }
  }

  if((key_id == KEY_HOME) && (type == KEY_RELEASED)) {

    if(current_screen != 0) {
      clear_next_render = true;
      clear_screen_screen   = current_screen;
      clear_screen_selected = selected_item;
    }

    clear_stack();
    current_screen = 0;
    selected_item  = 1;
  }
  
}


void GUI::receive_update(const char *tag,void *value) {
  for(uint32_t n=0;n<screens_layout[current_screen].item_count;n++) {
    if(strcmp(tag,screens_layout[current_screen].items[n].text) == true) {
      update_item(screens_layout[current_screen].items[n],value);
    }
  }
}

void GUI::set_sleeping(bool v) {
  m_sleeping = v;
}
