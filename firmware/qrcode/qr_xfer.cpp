#include "qr_xfer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "qr_encodeem.h"
#include <iostream>
#include "display.h"
#include "captouch.h"
#include "GUI.h"

using namespace std;

extern GUI *system_gui;

void qr_logxfer() {

  char inputdata[200];
//  char *inputdata = "TESTTESTTESTTEST";
//  char *inputdata = "abcdefghijklmnop";
  
  int first_keys = cap_lastkey();
  for(int z=0;;z++) {

    int keys = cap_lastkey();
    if(keys != first_keys) {
      display_clear(0);
      system_gui->redraw();
      return;
    }

    char i = 'A'+(z%10);
    for(int n=0;n<50;n++) {
      inputdata[n] = i+n;
      inputdata[n+1]=0;
    }
    i = 'A'+(z%10)-2;
    for(int n=0;n<50;n++) {
      inputdata[n+50] = i+n;
      inputdata[n+50+1]=0;
    }

		int outputdata_len=16;
		uint8_t image[2048]; // can be smaller

		int width=2;
		int ok = qr_encode_data(0,0,0,1,(uint8_t *) inputdata,50,image,&outputdata_len,&width);

		display_clear(0xFFFF);
	  if(ok != 0) display_draw_text(0,64-8,"QR Error",0);
	  if(ok != 0) display_draw_number(0,80,ok,5,0);
    //display_draw_text(0,50,"OK",0);
	  //display_draw_number(0,80,width,5,0);
  //display_draw_number(0,100,ok,5,0);
//    if(width == 0) return;

		int block_count = 4;
		int scale       = 3;
		int block_size  = (width*scale)/block_count;
		uint16_t block_data[2048];
		int block_y = 0;
		int pad = (128-(width*scale))/2;
		for(int block=0;block<=block_count;block++) {
			// fill block data
			for(int x=0;x<(width*scale);x++) {
				for(int y=block_y;y<(block_y+block_size);y++) {
					uint16_t v = qr_getmodule(image,width,x/scale,y/scale);
					if(v != 0) v = 0x0000; else v = 0xFFFF;

					block_data[((y-block_y)*(width*scale))+x] = v;
				}
			}

			display_draw_image(0+pad,block_y+pad,(width*scale),block_size,block_data);
			block_y += block_size;
		}
    delay_us(500000);

  }

}
