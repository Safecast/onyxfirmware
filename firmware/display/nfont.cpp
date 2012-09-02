#ifndef NFONT
#define NFONT

#include <stdio.h>
#include <string.h>
#include "nfont.h"
#include <stdint.h>
#include "oled.h"

extern uint8_t _binary_font_data_start;
extern uint8_t _binary_font_data_size;

#define to565(r,g,b)                                            \
    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

uint16_t get_pixel(char c,int c_x,int c_y) {
  
  int ypos = (c/(128/8)) * 16;
  int xpos = (c%(128/8)) * 8;
  ypos+=1;

  int bitposition = ((ypos*128)+(c_y*128) + (xpos)+c_x) *2;

  int byte_position = bitposition/8;
  int bit_in_byte   = 7-(bitposition%8);

  uint8_t byte = ((uint8_t *) &_binary_font_data_start)[byte_position];
  
  uint8_t value=0;
  if((byte & (1 << bit_in_byte)) > 0) value += 2;
  
  bit_in_byte--;
/*  if(bit_in_byte < 0) {
    byte_position++;
    bit_in_byte=7;
    byte = ((uint8_t *) &_binary_font_data_start)[byte_position];
  }
*/
  if((byte & (1 << bit_in_byte)) > 0) value += 1;

  if(value == 0) {value = 0;   }
  if(value == 1) {value = 85;  }
  if(value == 2) {value = 170; }
  if(value == 3) {value = 255; }

  return to565(value,value,value);
 // return 0;
}

void draw_character(int x,int y,char c,uint16_t background) {

  uint16_t character_data[8*16];
  for(int n=0;n<(8*16);n++) character_data[n]=n;

  for(size_t c_y=0;c_y<16;c_y++) {
    for(size_t c_x=0;c_x<8;c_x++) {
      int32_t value = get_pixel(c-32,c_x,c_y) - background;
      if(value < 0) value=0;

      if(background == 65535) value = background ^ get_pixel(c-32,c_x,c_y); 

      character_data[(c_y*8)+c_x] = value;
    }
  }

  oled_draw_rect(x,y,8,16,(uint8_t *) character_data);
}

void draw_text(int x,int y,const char *text,int16_t background) {

  int length = strlen(text);
  if(length < 0    ) return;
  if(length > 10000) return;

  int c_x = x;
  int c_y = y;
  for(size_t n=0;n<length;n++) {
    draw_character(c_x,c_y,text[n],background);
    c_x+=8;
  }
}


#endif
