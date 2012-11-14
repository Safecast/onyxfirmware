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

bool first_render=true;

uint16 header_color=HEADER_COLOR_CPMINVALID;

uint8_t m_language;

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

char     ticked_items[10][TEXT_LENGTH];
uint32_t ticked_items_size=0;

void tick_item(char *name,bool tick_val) {

  if((ticked_items_size >= 10) && (tick_val == true)) return;

  for(uint32_t n=0;n<ticked_items_size;n++) {
    if(strcmp(name,ticked_items[n]) == 0) {
      if(tick_val == true) return;
      if(tick_val == false) {
        for(uint32_t i=n;i<(ticked_items_size-1);i++) {
          strcpy(ticked_items[i],ticked_items[i+1]);
        }
        ticked_items_size--;
        return;
      }
    }
  }

  if(tick_val == true) {
    strcpy(ticked_items[ticked_items_size],name);
    ticked_items_size++;
  }
}

bool is_ticked(char *name) {
  for(uint32_t n=0;n<ticked_items_size;n++) {
    if(strcmp(name,ticked_items[n]) == 0) return true;
  }
  return false;
}


void render_item_menu(screen_item &item, bool selected) {
  
  uint16_t highlight = FOREGROUND_COLOR;
  if(selected) highlight = BACKGROUND_COLOR;
  
  // render tick
  bool ticked = is_ticked(item.text);
  if(ticked) {
    display_draw_text(128-8,item.val2*16,"\x7F",highlight);
  }

  if((m_language == LANGUAGE_ENGLISH) || (item.kanji_image == 255)) {



    int len = strlen(item.text);
    char text[50];
    strcpy(text,item.text);

    // Search for : replace with NULL in rendering
    for(int n=0;n<len;n++) {
      if(text[n] == ':') text[n] = 0;
    }
    len = strlen(text);


    // Pad to 16 characters
    for(int n=len;n<16;n++) {
      text[n  ]=' ';
      text[n+1]=0;
    }
    if(ticked) text[15]=0;

    display_draw_text(0,item.val2*16,text,highlight);
  } else
  if(m_language == LANGUAGE_JAPANESE) {
    if(!ticked) {
      display_draw_fixedimage(0,item.val2*16,item.kanji_image,highlight);
    } else {
      display_draw_fixedimage_xlimit(0,item.val2*16,item.kanji_image,highlight,128-8);
    }
  }

}

#define VARNUM_MAXSIZE 10

char    varnum_names[VARNUM_MAXSIZE][10];
uint8_t varnum_values[VARNUM_MAXSIZE];
uint8_t varnum_size = 0;

uint8_t get_item_state_varnum(const char *name) {

  for(uint32_t n=0;n<varnum_size;n++) {
    if(strcmp(varnum_names[n],name) == 0) {
      return varnum_values[n];
    }
  }
  return 0;
}

void set_item_state_varnum(char *name,uint8_t value) {

  int itemcount=0;
  int len = strlen(name);
  for(int n=0;n<len;n++) {
    if(name[n]==',') itemcount++;
  }

  if(itemcount != 0) {
    if(value > itemcount) return;
  }
  
  for(uint32_t n=0;n<varnum_size;n++) {
    if(strcmp(varnum_names[n],name) == 0) {
      varnum_values[n] = value;
      return;
    }
  }
  
  if(varnum_size >= VARNUM_MAXSIZE) return;
  strcpy(varnum_names[varnum_size],name);
  varnum_values[varnum_size] = value;
  varnum_size++;
}

void render_item_varnum(screen_item &item, bool selected) {

  uint8_t x = item.val1;
  uint8_t y = item.val2;

  int len = strlen(item.text);
  int colon_pos=-1;
  for(int n=0;n<len;n++) {
    if(item.text[n] == ':') colon_pos=n;
  }
  
  bool nonnumeric = false;
  char selitem[10][10];
  if(colon_pos != -1) {
    nonnumeric=true;

    char current[10];
    int  current_pos=0;
    int  cselitem=0;
    for(int n=colon_pos+1;n<len;n++) {
 
      if(item.text[n] != ',') {
        current[current_pos  ] = item.text[n];
        current[current_pos+1] = 0;
        current_pos++;
      } else {
        strcpy(selitem[cselitem],current);
        current_pos=0;
        current[0]=0;
        cselitem++;
      }
    }
  }

  uint8_t val = get_item_state_varnum(item.text);

  uint16_t color;
  if(selected) color = 0xcccc; else color = FOREGROUND_COLOR;
  display_draw_equtriangle(x,y,9,color);
  display_draw_equtriangle_inv(x,y+33,9,color);
  if(nonnumeric == false) {
    display_draw_number(x-4,y+9,val,1,FOREGROUND_COLOR);
  } else {
    display_draw_text(x-4,y+9,selitem[val],FOREGROUND_COLOR);
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
  int start_x = x-9;
  int end_x   = x+9;

  int start_y = y-2;
  int end_y   = y+40;

  if(start_x <   0) start_x = 0;
  if(  end_x > 127)   end_x = 127;

  if(start_y <   0) start_y = 0;
  if(  end_y > 127)   end_y = 127;
  
  display_draw_rectangle(start_x,start_y,end_x,end_y,BACKGROUND_COLOR);
}


void render_item_label(screen_item &item, bool selected) {

  if(m_language == LANGUAGE_ENGLISH) {
    if(item.val1 == 255) {display_draw_text_center          (item.val2,item.text,FOREGROUND_COLOR);}
                    else {display_draw_text       (item.val1,item.val2,item.text,FOREGROUND_COLOR);}
  }

  if(m_language == LANGUAGE_JAPANESE) {
    if(item.kanji_image != 255) {
			if((item.val1 != 255) && (item.val1 != 0)) {
				display_draw_fixedimage_xlimit(item.val1,item.val2,item.kanji_image,FOREGROUND_COLOR,128-item.val1);
			} else {
				// we can't center fixed images as we don't know their width, just draw at 0, full width.
				display_draw_fixedimage(0,item.val2,item.kanji_image,FOREGROUND_COLOR);
			}
    } else {
      if(item.val1 == 255) {display_draw_text_center          (item.val2,item.text,FOREGROUND_COLOR);}
                      else {display_draw_text       (item.val1,item.val2,item.text,FOREGROUND_COLOR);}
    }
  }
}

void render_item_smalllabel(screen_item &item, bool selected) {
  if(item.val1 == 255) {display_draw_tinytext_center          (item.val2,item.text,FOREGROUND_COLOR);}
                  else {display_draw_tinytext       (item.val1,item.val2,item.text,FOREGROUND_COLOR);}
}

void render_item_head(screen_item &item, bool selected) {
  draw_text(0,0,"                ",header_color);
}

float m_old_graph_data[120];
float m_graph_data[120];
float *source_graph_data;

void render_item_graph(screen_item &item, bool selected) {

  int32_t data_size=240;
  int32_t data_offset=600-240;
  int32_t data_increment=2;
  int32_t x_spacing=1;
  float   max_height = 80;

  int32_t m_x = item.val1;
  int32_t m_y = item.val2;
  
//  display_draw_rectangle(0,16,128,128,BACKGROUND_COLOR);

  // find min and max in data
  int32_t nmax = source_graph_data[data_offset];
  int32_t nmin = source_graph_data[data_offset];
  for(uint32_t n=data_offset;n<(data_offset+data_size);n++) {
    if(source_graph_data[n] > nmax) nmax = source_graph_data[n];
    if(source_graph_data[n] < nmin) nmin = source_graph_data[n];
  }
  
  // axis
  display_draw_line(m_x,m_y           ,m_x+(data_size/data_increment),m_y           ,FOREGROUND_COLOR);
  display_draw_line(m_x,m_y-max_height,m_x+(data_size/data_increment),m_y-max_height,FOREGROUND_COLOR);

  // if there's no data just draw labels and return.
  if((nmin == 0) && (nmax == 0)) {
    display_draw_tinynumber(m_x+5,m_y-max_height-10,nmax,4,FOREGROUND_COLOR);
    display_draw_tinynumber(m_x+5,m_y-10           ,nmin,4,FOREGROUND_COLOR);
    return;
  }


  // rescale data
  float source_y_range = nmax-nmin;
  float   dest_y_range = max_height;
  float source_x_range = data_size;
  float   dest_x_range = 120;

  float m_graph_count[120];
  for(int n=0;n<120;n++) m_graph_count[n]=0;
  for(int n=0;n<120;n++) m_graph_data [n]=0;

  for(uint32_t n=data_offset;n<(data_offset+data_size);n++) {
    // put data point in the range 0->1
    float data_point = (source_graph_data[n]-nmin)/source_y_range;

    // put data point in the range m_y -> m_y+dest_y_range
    data_point = (data_point*dest_y_range);

    uint32_t xpos = ((float)(n-data_offset)/source_x_range)*dest_x_range;
    m_graph_data [xpos] += data_point;
    m_graph_count[xpos]++;
  }

  // apply averaging, and offset to data.
  for(int n=0;n<120;n++) {
    if(m_graph_count[n] != 0) m_graph_data[n] = m_y - m_graph_data[n]/m_graph_count[n];
                         else m_graph_data[n] = 0;
  }

  // start clearing data, clear first 2.
  if(!first_render) display_draw_line(0,m_old_graph_data[0],1,m_old_graph_data[1],BACKGROUND_COLOR);
  if(!first_render) display_draw_line(1,m_old_graph_data[1],2,m_old_graph_data[2],BACKGROUND_COLOR);

  // render the data
  for(uint32_t n=1;n<120;n++) {
    uint32_t cx = n;

    if(!first_render && (n<118)) {
      display_draw_line(cx+1,m_old_graph_data[n+1],cx+2,m_old_graph_data[n+2],BACKGROUND_COLOR);
    }

    display_draw_line(cx-1,m_graph_data[n-1],cx,m_graph_data[n],FOREGROUND_COLOR);
  }

  for(uint32_t n=0;n<120;n++) {
    m_old_graph_data[n] = m_graph_data[n];
  }
  
  display_draw_tinynumber(m_x+5,m_y-max_height-10,nmax,4,FOREGROUND_COLOR);
  display_draw_tinynumber(m_x+5,m_y-10           ,nmin,4,FOREGROUND_COLOR);
  
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
   
  int x_max = 0;
  if(item.val1 == 255) { x_max = 127;} else
                       { x_max = item.val1+(text_len*8)-1;}
  int y_max = item.val2+16;
  if(x_max>=128) x_max=127;
  if(y_max>=128) y_max=127;
  int x_min = 0;
  if(item.val1 != 255) {x_min = item.val1; x_max=127;}
  display_draw_rectangle(x_min,item.val2,x_max,y_max,BACKGROUND_COLOR);
}

void clear_item_smalllabel(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;
   
  int x_max = 0;
  if(item.val1 == 255) { x_max = 127;} else
                       { x_max = item.val1+(text_len*8)-1;}
  int y_max = item.val2+16;
  if(x_max>=128) x_max=127;
  if(y_max>=128) y_max=127;
  int x_min = 0;
  if(item.val1 != 255) {x_min = item.val1; x_max=127;}
  display_draw_rectangle(x_min,item.val2,x_max,y_max,BACKGROUND_COLOR);
}

void clear_item_head(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;

  int x_max=127;
  int y_max=16;
  display_draw_rectangle(item.val1,item.val2,x_max,y_max,BACKGROUND_COLOR);
}
    
void clear_item_bigvarlabel(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;
  display_draw_rectangle(item.val1,item.val2,127,(item.val2+32),BACKGROUND_COLOR);
}


void clear_item_varlabel(screen_item &item, bool selected) {
  int32_t text_len = strlen(item.text);

  if(text_len == 0) return;
  int x=0;
  if(item.val1 == 255) x = 0; else x = item.val1;
  display_draw_rectangle(x,item.val2,127,(item.val2+16),BACKGROUND_COLOR);
}

void clear_item_graph(screen_item &item, bool selected) {

  display_draw_rectangle(0,16,128,128,BACKGROUND_COLOR);
}

void clear_item_delay(screen_item &item, bool selected) {
  display_draw_rectangle(item.val1,item.val2,item.val1+24,item.val2+16,BACKGROUND_COLOR);
}

void render_item_delay(screen_item &item,bool selected) {
  display_draw_number(item.val1,item.val2,delay_time,3,FOREGROUND_COLOR);
}


void render_item(screen_item &item,bool selected) {
  if(item.type == ITEM_TYPE_MENU) {
    render_item_menu(item,selected);
  } else
  if(item.type == ITEM_TYPE_LABEL) {
    render_item_label(item,selected);
  } else
  if(item.type == ITEM_TYPE_SMALLLABEL) {
    render_item_smalllabel(item,selected);
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
  } else 
  if(item.type == ITEM_TYPE_DELAY) {
    render_item_delay(item,selected);
  }
}

void clear_item(screen_item &item,bool selected) {
  if(item.type == ITEM_TYPE_MENU) {
    clear_item_menu(item,selected);
  } else
  if(item.type == ITEM_TYPE_LABEL) {
    clear_item_label(item,selected);
  } else
  if(item.type == ITEM_TYPE_SMALLLABEL) {
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
  } else
  if(item.type == ITEM_TYPE_DELAY) {
    clear_item_delay(item,selected);
  } else
  if(item.type == ITEM_TYPE_BIGVARLABEL) {
    clear_item_bigvarlabel(item,selected);
  }
}

void update_item_graph(screen_item &item,const void *value) {
 source_graph_data = (float *)value;
}


uint8 lock_mask [11][8] = {
  
  {0,1,1,1,1,1,1,0},
  {1,1,0,0,0,0,1,1},
  {1,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,0,0,1,1,1},
  {1,1,0,0,0,0,1,1},
  {1,1,0,0,0,0,1,1},
  {1,1,1,0,0,1,1,1},
  {1,1,1,0,0,1,1,1},
  {1,1,1,0,0,1,1,1},
  {1,1,1,1,1,1,1,1}

};


bool lock_state=false;
void render_lock(bool on) {
  
  if((on == lock_state) && (on != true)) return;
  lock_state = on;

  uint16 image_data[88]; // 8*11
  for(int x=0;x<8;x++) {
    for(int y=0;y<11;y++) {
      int16 render_value = lock_mask[y][x];

      if(render_value > 0)  render_value = 0xFFFF;
                       else render_value = 0;
 
      if(on == false) render_value = 0;

      image_data[(y*8)+x] = render_value;
    }
  }

  display_draw_image(128-8,128-11,8,11,image_data);
}

uint8 battery_mask [16][24] = {

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0,0,0,0},
  {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0},
  {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0},
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

};

uint16 last_batlevel=0;

void render_battery(int x,int y,int level) {

  uint16 image_data[384]; // 24*16

  // only move bat level, if there's a big difference to prevent flickering.
  level = (((float) level)/100)*23;
  uint16 level_delta = level-last_batlevel;
  if(level_delta < 0) level_delta = 0-level_delta;
  if(level_delta <= 1) level = last_batlevel;

  last_batlevel = level;

  for(int x=0;x<24;x++) {
    for(int y=0;y<16;y++) {
      int16 render_value = battery_mask[y][x];

      if(x <= level) {
        if(render_value > 0)  render_value = 0xF1FF - (2081*(render_value-1));
                         else render_value = header_color;// HEADER_COLOR; // header background
      }

      if(x > level) {
        if(render_value > 0)  render_value = BACKGROUND_COLOR;  //6243 - (2081*(render_value-1));
                         else render_value = header_color;//HEADER_COLOR; // header background
      }
      image_data[(y*24)+x] = render_value;
    }
  }

  display_draw_image(104,0,24,16,image_data);
}

void update_item_head(screen_item &item,const void *value) {

  uint16_t new_header_color;
  if(system_geiger->is_cpm_valid()) new_header_color = HEADER_COLOR_NORMAL;
                               else new_header_color = HEADER_COLOR_CPMINVALID;

  if(new_header_color != header_color) {
    draw_text(0,0,"                ",new_header_color);
  }
  header_color = new_header_color;

  int len = strlen((char *) value);
  char v[TEMP_STR_LEN];
  strcpy(v,(char *) value);
  for(int n=len;(n<6) && (n<(TEMP_STR_LEN-1));n++) {
    v[n  ] = ' ';
    v[n+1]=0;
  }
  if(len > 6) v[6]=0;

  draw_text(0,0,v,header_color);//HEADER_COLOR);

  // a hack!
  render_battery(0,128-24,power_battery_level());

  uint8_t hours,min,sec,day,month;
  uint16_t year;
  realtime_getdate_local(hours,min,sec,day,month,year);
  month+=1;
  year+=1900;
  if(year >= 2000) {year-=2000;} else
  if(year <  2000) {year-=1900;}

  char time[TEMP_STR_LEN];
  char date[TEMP_STR_LEN];
  sprintf(time,"%02u:%02u:%02u",hours,min,sec);
  sprintf(date,"%02u/%02u/%02u",month,day,year);
  if(year == 0) { sprintf(date,"%02u/%02u/00",month,day); }

  // pad out time and date
  int tlen = strlen(time);
  for(int n=tlen;(n<8) && (n<(TEMP_STR_LEN-1));n++) {
    time[n  ] = ' ';
    time[n+1]=0;
  }

  int dlen = strlen(date);
  for(int n=dlen;(n<8) && (n<(TEMP_STR_LEN-1));n++) {
    date[n  ] = ' ';
    date[n+1]=0;
  }

  display_draw_tinytext(128-75,2,time,header_color);//HEADER_COLOR);
  display_draw_tinytext(128-75,9,date,header_color);//HEADER_COLOR);
//  display_draw_tinytext(0,128-5,OS100VERSION,FOREGROUND_COLOR);
}

void update_item_varnum(screen_item &item,const void *value) {
  set_item_state_varnum(item.text,((uint8_t *) value)[0]);
}

void update_item_delay(screen_item &item,const void *value) {

  if(first_render == true) {
    // parse out delay time
    delay_time = str_to_uint(item.text+8);

  }

  if(delay_time >= 1) delay_time--;
  delay_us(1000000);
  display_draw_number(item.val1,item.val2,delay_time,3,FOREGROUND_COLOR);
}

int get_item_state_delay_destination(screen_item &item) {
  // parse out destination screen

  for(int n=9;n<50;n++) {
    if(!((item.text[n] >= '0') && (item.text[n] <= '9'))) {
      uint32_t destination_screen_start = n+1;
      int dest = str_to_uint(item.text+destination_screen_start);
      return dest;
    }
  }
  return 0;
}

void update_item(screen_item &item,const void *value) {
  if(item.type == ITEM_TYPE_VARLABEL) {
    if(item.val1 == 255) {
      // display_draw_rectangle(0,item.val2,128,item.val2+16,BACKGROUND_COLOR); unfortunately renders badly.
      display_draw_text_center(item.val2,(char *)value,FOREGROUND_COLOR);
    } else {
      display_draw_text(item.val1,item.val2,(char *)value,FOREGROUND_COLOR);
    }

  } else 
  if(item.type == ITEM_TYPE_GRAPH) {
    update_item_graph(item,value);
  } else 
  if(item.type == ITEM_TYPE_HEAD) {
    update_item_head(item,value);
  } else 
  if(item.type == ITEM_TYPE_VARNUM) {
    update_item_varnum(item,value);
  } else 
  if(item.type == ITEM_TYPE_DELAY) {
    update_item_delay(item,value);
  } else
  if(item.type == ITEM_TYPE_BIGVARLABEL) {
    display_draw_bigtext(item.val1,item.val2,(char *)value,FOREGROUND_COLOR);
  }
}
  
void GUI::clear_stack() {
  selected_stack_size=0;
}
  
void GUI::pop_stack(int &current_screen,int &selected_item) {

  if(selected_stack_size == 0) return;

  current_screen = selected_screen_stack[selected_stack_size-1];
  last_selected_item = selected_item;
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

  m_repeated =false;
  m_displaying_help = false;
  m_repeat_time = 0;
  m_repeat_delay= 8;
  m_repeating = false;
  new_keys_start = 0;
  new_keys_end   = 0;
  clear_next_render=false;
  current_screen = 0;
  selected_item=1;
  last_selected_item=1;
  clear_stack();
  m_trigger_any_key=false;
  m_sleeping=false;
  m_redraw=false;
  m_screen_lock=false;
  m_language = LANGUAGE_ENGLISH;
  m_displaying_dialog =false;
  m_displaying_dialog_complete=false;
  //m_dialog_text1[0]=0;
  //m_dialog_text2[0]=0;
  //m_dialog_text3[0]=0;
  //m_dialog_text4[0]=0;
  m_dialog_buzz=false;
  m_pause_display_updates=false;
}

void GUI::show_help_screen(uint8_t helpscreen) {
  if(m_language == LANGUAGE_ENGLISH ) display_draw_helpscreen_en(helpscreen);
  if(m_language == LANGUAGE_JAPANESE) display_draw_helpscreen_jp(helpscreen);
  m_displaying_dialog=true;
  m_displaying_dialog_complete=false;
  m_pause_display_updates = true;
  m_dialog_buzz = false;
  m_displaying_help = true;
}

void GUI::show_dialog(char *dialog_text1,char *dialog_text2,char *dialog_text3,char *dialog_text4,bool buzz,int img1,int img2,int img3,int img4) {
  display_draw_rectangle(0,0,128,128,BACKGROUND_COLOR);
  //strcpy(m_dialog_text1,dialog_text1);
  //strcpy(m_dialog_text2,dialog_text2);
  //strcpy(m_dialog_text3,dialog_text3);
  //strcpy(m_dialog_text4,dialog_text4);
  m_dialog_buzz = buzz;
  m_displaying_dialog=true;
  m_displaying_dialog_complete=false;
  m_pause_display_updates = true;
  render_dialog(dialog_text1,dialog_text2,dialog_text3,dialog_text4,img1,img2,img3,img4);
}

void GUI::render() {

  if(m_sleeping) {
//     process_keys();
    return;  
  }

  if(m_displaying_dialog) {
    if(m_dialog_buzz) buzzer_nonblocking_buzz(1);
    return;
  }

  if(m_displaying_dialog_complete) {
    m_displaying_dialog_complete=false;
    m_pause_display_updates = false;
    display_clear(0);
    clear_pending_keys();
    redraw();
  }
  
  if(m_repeating) {
    // This would be better incremented in a timer, but I don't want to use another timer.
    if(m_repeat_time == m_repeat_delay) {
      // verify button still pressed
      if(cap_ispressed(m_repeat_key) == false) {
        m_repeating=false;
        m_repeat_time=0;
      } else {
        if(m_repeat_key == KEY_DOWN) { receive_key(KEY_DOWN,KEY_PRESSED); }
        if(m_repeat_key == KEY_UP  ) { receive_key(KEY_UP  ,KEY_PRESSED); }
        m_repeat_time = 0;
        m_repeated = true;
      }
    }
    m_repeat_time++;
  }


  // following two items really need to be atomic...
  int32_t cscreen = current_screen;

  if(clear_next_render) {
    clear_next_render=false;
    clear_screen(clear_screen_screen,clear_screen_selected);
    first_render=true;
  }

  bool do_redraw = false;
  if(m_redraw) do_redraw = true;
  m_redraw = false;

  render_lock(m_screen_lock);
  for(uint32_t n=0;n<screens_layout[cscreen].item_count;n++) {

    if(first_render) {
      if(screens_layout[current_screen].items[n].type == ITEM_TYPE_ACTION) {
        receive_gui_events.receive_gui_event(screens_layout[cscreen].items[n].text,
                                             screens_layout[cscreen].items[n].text);
      }
    }

    //bool selected = false;
    bool select_render = false;
    if(n == selected_item     ) select_render = true;
    if(n == last_selected_item) select_render = true;

    if(first_render || select_render || do_redraw) {
      //bool do_render = true;
 
      // don't render labels, just because they are near other things...
      //if(!first_render && select_render && (screens_layout[cscreen].items[n].type == ITEM_TYPE_LABEL)) {
      //  do_render = false;
      //} 

      //if(do_render)
      bool selected = false;
      if(selected_item == n) selected=true;
      render_item(screens_layout[cscreen].items[n],selected);
    }
  }
  //last_selected_item = selected_item;

  first_render=false;
  process_keys();
}

void GUI::clear_screen(int32_t c_screen,int32_t c_selected) {
  for(uint32_t n=0;n<screens_layout[c_screen].item_count;n++) {

    bool selected = false;
    if(n == c_selected) selected = true;

    clear_item(screens_layout[c_screen].items[n],selected);
  }
  varnum_size=0;
}

void GUI::set_key_trigger() {

  m_trigger_any_key=true;

}

void GUI::redraw() {
  m_redraw = true;
}

void GUI::receive_key(int key_id,int type) {
  //char s[50];
  //if(key_id == KEY_HELP  ) sprintf(s,"rkey: HELP   %d\r\n",type);
  //if(key_id == KEY_UP    ) sprintf(s,"rkey: UP     %d\r\n",type);
  //if(key_id == KEY_DOWN  ) sprintf(s,"rkey: DOWN   %d\r\n",type);
  //if(key_id == KEY_SELECT) sprintf(s,"rkey: SELECT %d\r\n",type);
  //if(key_id == KEY_HOME  ) sprintf(s,"rkey: HOME   %d\r\n",type);
  //serial_write_string(s);
    
  // don't activate HELP key when in a help screen
  if((m_displaying_help == true) && (key_id == KEY_HELP)) return;

  if(m_displaying_dialog==true) {
    m_displaying_dialog=false;
    m_displaying_help = false;
    m_displaying_dialog_complete=true;
    return;
  }
  
//  char s[25];
//  sprintf(s,"keys cache: %d %d\r\n",new_keys_start,new_keys_end);
//  serial_write_string(s);

  new_keys_key [new_keys_end] = key_id;
  new_keys_type[new_keys_end] = type;
  new_keys_end++;

  if(new_keys_end >= NEW_KEYS_MAX_SIZE) new_keys_end=0;
}
    

void GUI::clear_pending_keys() {
  new_keys_end   = 0;
  new_keys_start = 0;
}

void GUI::process_keys() {

  if(new_keys_start != new_keys_end) {
    process_key(new_keys_key[new_keys_start],new_keys_type[new_keys_start]);

    new_keys_start++;
    if(new_keys_start >= NEW_KEYS_MAX_SIZE) new_keys_start=0;
  }

}

void GUI::leave_screen_actions(int screen) {
  for(int n=0;n<screens_layout[screen].item_count;n++) {
    if(screens_layout[screen].items[n].type == ITEM_TYPE_LEAVE_ACTION) {
      receive_gui_events.receive_gui_event(screens_layout[screen].items[n].text,"Left");
    }
  }
}

void GUI::process_key_down() {
	if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
		uint8_t current = get_item_state_varnum(screens_layout[current_screen].items[selected_item]);

		int8_t val[1];
		val[0] = current-1;
		if(val[0] < 0) val[0] = 0;
		update_item(screens_layout[current_screen].items[selected_item],val);

		receive_gui_events.receive_gui_event("varnumchange",screens_layout[current_screen].items[selected_item].text);
		return;
	}

	if((selected_item+1) < screens_layout[current_screen].item_count) {
    last_selected_item = selected_item;
	  selected_item++;
	}
}

void GUI::process_key_up() {
	if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
		uint8_t current = get_item_state_varnum(screens_layout[current_screen].items[selected_item]);

		int8_t val[1];
		val[0] = current+1;
		if(val[0] > 9) val[0] = 9;
		update_item(screens_layout[current_screen].items[selected_item],val);
		receive_gui_events.receive_gui_event("varnumchange",screens_layout[current_screen].items[selected_item].text);
		return;
	}


	if(selected_item == 1) return;
  last_selected_item = selected_item;
	selected_item--;
}

void GUI::process_key(int key_id,int type) {

  //char s[50];
  //if(key_id == KEY_HELP  ) sprintf(s,"key: HELP   %d\r\n",type);
  //if(key_id == KEY_UP    ) sprintf(s,"key: UP     %d\r\n",type);
  //if(key_id == KEY_DOWN  ) sprintf(s,"key: DOWN   %d\r\n",type);
  //if(key_id == KEY_SELECT) sprintf(s,"key: SELECT %d\r\n",type);
  //if(key_id == KEY_HOME  ) sprintf(s,"key: HOME   %d\r\n",type);
  //serial_write_string(s);

  if(m_screen_lock) return;

  if(m_trigger_any_key) {
    receive_gui_events.receive_gui_event("KEYPRESS","any");
    m_trigger_any_key=false;
  }

  if(m_sleeping) return;

  if((key_id == KEY_HELP) && (type == KEY_RELEASED) && (!m_displaying_help)) {
    if(screens_layout[current_screen].help_screen != 255) show_help_screen(screens_layout[current_screen].help_screen);
  }


  if((key_id == KEY_UP) && (type == KEY_PRESSED)) {
    process_key_up();
    m_repeating=true;
    m_repeat_key = KEY_UP;
  }

  if((key_id == KEY_DOWN) && (type == KEY_PRESSED)) {
    process_key_down();
    m_repeating=true;
    m_repeat_key = KEY_DOWN;
  }

  if((key_id == KEY_DOWN) && (type == KEY_RELEASED)) {
//    if(!m_repeated) process_key_down();
    m_repeating=false;
    m_repeated =false;
  }

  if((key_id == KEY_UP) && (type == KEY_RELEASED)) {
//    if(!m_repeated) process_key_up();
    m_repeating=false;
    m_repeated =false;
  }

  if((key_id == KEY_SELECT) && (type == KEY_RELEASED)) {

    // if a VARNUM is selected...
    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
      if(selected_item != 0) {
        if((selected_item+1) < screens_layout[current_screen].item_count) {
          last_selected_item = selected_item;
          selected_item++;
          return;
        }
      }
    }

    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_MENU) {
      if(screens_layout[current_screen].items[selected_item].val1 != INVALID_SCREEN) {
        
        if(clear_next_render == false) {
          clear_next_render = true;
          first_render=true;
          clear_screen_screen   = current_screen;
          clear_screen_selected = selected_item;
        }

        push_stack(current_screen,selected_item);
        current_screen = screens_layout[current_screen].items[selected_item].val1;
        last_selected_item = 1;
        selected_item = 1;
      }
    } else 
    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_MENU_ACTION) {
      receive_gui_events.receive_gui_event(screens_layout[current_screen].items[selected_item].text,"select");
    }
    
  }

  if((key_id == KEY_BACK) && (type == KEY_RELEASED)) {

    // if a varnum is selected
    if(screens_layout[current_screen].items[selected_item].type == ITEM_TYPE_VARNUM) {
      if(selected_item != 0) {
        if((selected_item-1) >= 0) {
          if(screens_layout[current_screen].items[selected_item-1].type == ITEM_TYPE_VARNUM) {
            last_selected_item = selected_item;
		        selected_item--;
            receive_gui_events.receive_gui_event("varnumchange",screens_layout[current_screen].items[selected_item].text);
            return;
					}
				}
			}
		}

    if((current_screen != 0) && (!clear_next_render)) {
			if(selected_stack_size !=0) {
				clear_next_render = true; 
				first_render=true;
				clear_screen_screen   = current_screen;
				clear_screen_selected = selected_item;


        leave_screen_actions(current_screen);

				pop_stack(current_screen,selected_item);
			}
    }
  }

  if((key_id == KEY_HOME) && (type == KEY_RELEASED) && (!clear_next_render)) {
    if(current_screen != 0) {
      clear_next_render = true;
      first_render=true;
      clear_screen_screen   = current_screen;
      clear_screen_selected = selected_item;
      leave_screen_actions(current_screen);
      clear_stack();
      current_screen = 0;
      last_selected_item = 1;
      selected_item  = 1;
    }
  }
  
}

void GUI::jump_to_screen(const char screen) {
  clear_next_render = true;
  first_render = true;
  clear_screen_screen   = current_screen;
  clear_screen_selected = selected_item;
  leave_screen_actions(current_screen);

  clear_stack();
  current_screen = screen; 
  last_selected_item = 1;
  selected_item  = 1;
}

void GUI::receive_update(const char *tag,const void *value) {
  
  if(m_pause_display_updates) return;

  for(uint32_t n=0;n<screens_layout[current_screen].item_count;n++) {
    if(strcmp(tag,screens_layout[current_screen].items[n].text) == 0) {
      update_item(screens_layout[current_screen].items[n],value);

      // has to be in the GUI object, because we don't have access to current_screen outside it.
      if(screens_layout[current_screen].items[n].type == ITEM_TYPE_DELAY) {
        if(get_item_state_delay(screens_layout[current_screen].items[n]) == 0) {
          jump_to_screen(get_item_state_delay_destination(screens_layout[current_screen].items[n]));
        }
      }
    }
  }
}

void GUI::set_sleeping(bool v) {
  m_sleeping = v;
}

void GUI::toggle_screen_lock() {

  if(m_screen_lock == true) m_screen_lock = false; else m_screen_lock = true;
}

uint8_t GUI::get_item_state_uint8(const char *tag) {
  // should check item type and repsond appropriately, however only varnum currently returns uint8_t
  return get_item_state_varnum(tag);
}

void GUI::set_language(uint8_t lang) {
  m_language = lang;
}

void GUI::render_dialog(char *text1,char *text2,char *text3,char *text4,int img1,int img2,int img3,int img4) {

  if(m_language == LANGUAGE_JAPANESE) {
    if(img1 == 255) { display_draw_text_center(20,text1,FOREGROUND_COLOR); } else
    if(img1 == 254) { } else
                    { display_draw_fixedimage(0,20,img1,FOREGROUND_COLOR); }

    if(img2 == 255) { display_draw_text_center(36,text2,FOREGROUND_COLOR); } else
    if(img2 == 254) { } else
                    { display_draw_fixedimage(0,36,img2,FOREGROUND_COLOR); }

    if(img3 == 255) { display_draw_text_center(52,text3,FOREGROUND_COLOR); } else
    if(img3 == 254) { } else
                    { display_draw_fixedimage(0,52,img3,FOREGROUND_COLOR); }


    if(img4 == 255) { display_draw_text_center(68,text4,FOREGROUND_COLOR); } else
    if(img4 == 254) { } else
                    { display_draw_fixedimage(0,68,img4,FOREGROUND_COLOR); }

    display_draw_fixedimage(0,94,49,FOREGROUND_COLOR); // press any key kanji image
  }

  if(m_language == LANGUAGE_ENGLISH) {
    display_draw_text_center(20,text1,FOREGROUND_COLOR);
    display_draw_text_center(36,text2,FOREGROUND_COLOR);
    display_draw_text_center(52,text3,FOREGROUND_COLOR);
    display_draw_text_center(68,text4,FOREGROUND_COLOR);
    display_draw_text_center(94,"PRESS ANY KEY",FOREGROUND_COLOR);
  }
}
