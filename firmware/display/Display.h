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
