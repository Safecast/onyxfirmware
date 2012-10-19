#include "qr_xfer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "qr_encodeem.h"
#include "flashstorage.h"
#include <iostream>
#include "display.h"
#include "captouch.h"
#include "GUI.h"
#include "log_read.h"

using namespace std;

extern GUI *system_gui;

void qr_draw(char *inputdata) {
  int first_keys = cap_lastkey();

  display_clear(0xFFFF);
  for(;;) {
    int keys = cap_lastkey();
    if(keys != first_keys) {
      display_clear(0);
      system_gui->redraw();
      return;
    }
		
    int outputdata_len=16;
		uint8_t image[2048]; // can be smaller

		int width=2;
		int ok = qr_encode_data(0,0,0,1,(uint8_t *) inputdata,strlen(inputdata),image,&outputdata_len,&width);

	  if(ok != 0) display_draw_text(0,64-8,"QR Error",0);
	  if(ok != 0) display_draw_number(0,80,ok,5,0);

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
  }
}

void qr_logxfer() {
  flashstorage_log_pause();

  int id_pos =0;
  char inputdata[1024];
  inputdata[0]=0;
  
  int data_per_qr = 50;
  int first_keys = cap_lastkey();

  log_read_start();
  for(int z=0;;z++) {

    int keys = cap_lastkey();
    if(keys != first_keys) {
      display_clear(0);
      system_gui->redraw();
      flashstorage_log_resume();
      return;
    }

    // fill data
    if(id_pos >= strlen(inputdata)) {
      int size = log_read_block(inputdata);
      if(size == 0) {
        z=0;
        log_read_start();
        log_read_block(inputdata);
      }
      id_pos = 0;
    }

		int outputdata_len=16;
		uint8_t image[2048]; // can be smaller

		int width=2;
    int len=data_per_qr;


    if((strlen(inputdata+id_pos)+4) < data_per_qr) len = strlen(inputdata+id_pos)+4;

    // add QR number tag to inputdata
    char renderdata[1028];
    for(int n=0;n<1028;n++) renderdata[n]='0';
    strcpy(renderdata+4,inputdata+id_pos);
    
    char zstr[10];
    sprintf(zstr,"%04u",z);
    renderdata[0] = zstr[0];
    renderdata[1] = zstr[1];
    renderdata[2] = zstr[2];
    renderdata[3] = zstr[3];

		int ok = qr_encode_data(0,0,0,1,(uint8_t *) renderdata,len,image,&outputdata_len,&width);
    id_pos+=(data_per_qr-4);

		display_clear(0xFFFF);
	  if(ok != 0) display_draw_text(0,64-8,"QR Error",0);
	  if(ok != 0) display_draw_number(0,80,ok,5,0);

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
    delay_us(200000);
  }

  flashstorage_log_resume();
}
