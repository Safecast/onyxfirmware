#include "oled.h"


class Display {

public:

  Display() {
  }

  void initialise() {
    oled_init();
  }

  void test() {

    uint8 data[100];
    for(int n=0;n<100;n++) data[n]=n;

    oled_draw_rect(10,10,20,20,data);
  }

};
