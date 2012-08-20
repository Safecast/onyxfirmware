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
#define KEY_HELP   0
#define KEY_PRESSED  0
#define KEY_RELEASED 1
#define BACKGROUND_COLOR 65535

bool first_render=true;

void display_draw_equtriangle(uint8_t x,uint8_t y,uint8_t s,uint16_t color) {

  uint8_t start = x;
  uint8_t end   = x;
  for(uint8_t n=0;n<s;n++) {
 
    for(uint8_t i=start;i<=end;i++) {
      display_draw_point(i,y,color);
    }
    start--;
    end++;
    y++;
  }
}

void display_draw_equtriangle_inv(uint8_t x,uint8_t y,uint8_t s,uint16_t color) {

  uint8_t start = x;
  uint8_t end   = x;
  for(uint8_t n=0;n<s;n++) {
 
    for(uint8_t i=start;i<=end;i++) {
      display_draw_point(i,y,color);
    }
    start--;
    end++;
    y--;
  }
}

void render_item_menu(screen_item &item, bool selected) {

  uint16_t highlight = 0;
  if(selected) highlight = 65535;

  display_draw_text(0,item.val2*16,item.text,highlight);
}

uint8_t varval=0;
void render_item_varnum(screen_item &item, bool selected) {

  uint8_t x = item.val1;
  uint8_t y = item.val2;

  display_draw_equtriangle(x,y,9,0);
  display_draw_equtriangle_inv(x,y+33,9,0);
  display_draw_number(x-4,y+9,varval,1,0);
}
      
uint8_t get_item_state_varnum(screen_item &item) {

  return varval;

}

void clear_item_varnum(screen_item &item, bool selected) {
  uint8_t x = item.val1;
  uint8_t y = item.val2;
  display_draw_rectangle(x-8,y-2,x+8,y+40,BACKGROUND_COLOR);
}


void render_item_label(screen_item &item, bool selected) {
  display_draw_text(item.val1,item.val2,item.text,0);
}

void render_item_head(screen_item &item, bool selected) {
  display_draw_text(0,0,"os100   CPM ",0x001F);
}

float m_graph_data[120];
float *source_graph_data;
bool  graph_first;

void render_item_graph(screen_item &item, bool selected) {

  int32_t m_x = item.val1;
  int32_t m_y = item.val2;
  
  graph_first = first_render;
  int32_t size=60;

  int offset=60;
  int32_t lastx=m_x;
  int32_t lastoy=m_y-m_graph_data[offset];
  int32_t lastny=m_y-source_graph_data[offset];
  for(uint32_t n=0;n<size;n++) {
    int cx = m_x+n;
    int oy = m_y-m_graph_data[n+offset];
    int ny = m_y-source_graph_data[n+offset];
    if(!((lastoy == lastny) && (oy == ny) && !graph_first)) {
      if(!first_render) display_draw_line(lastx,lastoy,cx,oy,BACKGROUND_COLOR);
      display_draw_line(lastx,lastny,cx,ny,0x0000);
   //   display_draw_point(cx,ny,0x000);
    }
    lastx=cx;
    lastoy=oy;
    lastny=ny;
  }

  for(int32_t n=0;n<120;n++) {
    m_graph_data[n] = source_graph_data[n];
  }
}

void clear_item_menu(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;

  
  uint8_t x_max=127;
  uint8_t y_max=(item.val2+1)*16;
  if(y_max > 127) y_max=127;
  display_draw_rectangle(0,item.val2*16,x_max,y_max,BACKGROUND_COLOR);
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
  
  graph_first = true;
  int32_t size=60;

  int offset=60;
  int32_t lastx=m_x;
  int32_t lastoy=m_y-m_graph_data[offset];
  for(uint32_t n=0;n<size;n++) {
    int cx = m_x+n;
    int oy = m_y-m_graph_data[n+offset];
    display_draw_line(lastx,lastoy,cx,oy,BACKGROUND_COLOR);
    lastx=cx;
    lastoy=oy;
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
  } else 
  if(item.type == ITEM_TYPE_VARNUM) {
    render_item_varnum(item,selected);
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
  } else
  if(item.type == ITEM_TYPE_VARNUM) {
    clear_item_varnum(item,selected);
  }
}

void update_item_graph(screen_item &item,void *value) {
 source_graph_data = (float *)value;
}

void update_item_head(screen_item &item,void *value) {
  draw_text(128-16-16,0,(char *)value,0x001F);
}

void update_item_varnum(screen_item &item,void *value) {
  varval = ((uint8_t *) value)[0];
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
  } else 
  if(item.type == ITEM_TYPE_VARNUM) {
    update_item_varnum(item,value);
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

  new_keys_size = 0;
  clear_next_render=false;
  current_screen = 0;
  selected_item=1;
  clear_stack();
  m_trigger_any_key=false;
  m_sleeping=false;
}


void GUI::render() {

  if(m_sleeping) {
     process_keys();
     return;  
  }

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
  process_keys();
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

  if(new_keys_size > NEW_KEYS_MAX_SIZE) return;

  new_keys_key [new_keys_size] = key_id;
  new_keys_type[new_keys_size] = type;
  new_keys_size++;
}

void GUI::process_keys() {

  //TODO: if a key press happens while we are in this loop, it will be lost
  for(uint32_t n=0;n<new_keys_size;n++) {
    process_key(new_keys_key[n],new_keys_type[n]);
  }
  new_keys_size=0;
}

void GUI::process_key(int key_id,int type) {

  if(m_trigger_any_key) {
    receive_gui_events.receive_gui_event("KEYPRESS","any");
    m_trigger_any_key=false;
  }

  if(m_sleeping) return;

  if((key_id == KEY_DOWN) && (type == KEY_RELEASED)) {

    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
      uint8_t current = get_item_state_varnum(screens_layout[current_screen].items[selected_item]);

      int8_t val[1];
      val[0] = current-1;
      if(val[0] < 0) val[0] = 0;
      update_item(screens_layout[current_screen].items[selected_item],val);
      return;
    }

    selected_item++;
    if(selected_item >= screens_layout[current_screen].item_count) {
      selected_item = screens_layout[current_screen].item_count-1;
    }
  }

  if((key_id == KEY_UP) && (type == KEY_RELEASED)) {

    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
      uint8_t current = get_item_state_varnum(screens_layout[current_screen].items[selected_item]);

      int8_t val[1];
      val[0] = current+1;
      if(val[0] > 9) val[0] = 9;
      update_item(screens_layout[current_screen].items[selected_item],val);
      return;
    }


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
      clear_stack();
      current_screen = 0;
      selected_item  = 1;
    }

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
