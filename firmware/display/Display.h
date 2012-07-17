#ifndef DISPLAY_H
#define DISPLAY_H

#include "oled.h"
#include "nfont.h"

extern uint8_t _binary_image_1_data_start;
extern uint8_t _binary_image_1_data_size;

class Display {

public:

  Display() {
  }

  void initialise() {
    oled_init();
  }

  void test() {

    dump_image();
 /*
    uint8 data[100];
    for(int n=0;n<100;n++) data[n]=n;

    oled_draw_rect(10,10,20,20,data);
*/
  }
  
  void clear() {
    CLS();
  }

  void draw_line(int start_x,int start_y,int end_x,int end_y,uint16_t color=65535) {
    Set_Column_Address(start_x, end_x+1);
    Set_Row_Address   (start_y, end_y+1);
    write_c(0x5c);
    for(size_t n=0;n<((end_x-start_x)*(end_y*start_y));n++) write_d(color);
  }
  
  void draw_point(int x,int y,uint16_t color=65535) {
    Set_Column_Address(x, x+1);
    Set_Row_Address(y, y+1);
    write_c(0x5c);
    write_d(color);
    write_d(color);
  }

  void draw_rectangle(int start_x,int start_y,int end_x,int end_y,uint16_t color) {
    Set_Column_Address(start_x, end_x);
    Set_Row_Address   (start_y, end_y);

    size_t size = (end_x-start_x)*(end_y-start_y);
   
    write_c(0x5C);    // Enable MCU to Read from RAM
    for (size_t i=1; i<=size;i++) {
      write_d(color);
      write_d(color >> 8);
    } 
  }

  void draw_text(int x,int y,char *text,int16_t background) {
    ::draw_text(x,y,text,background);
  }

  void dump_image() {
    oled_draw_rect(0,0,128,127,((uint8_t *) &_binary_image_1_data_start)+1);
  }

  void powerup  () {}
  void powerdown() {
    oled_deinit();
  }
};

#endif
