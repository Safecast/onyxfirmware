#ifndef NFONT
#define NFONT

#include <stdio.h>
#include <string.h>
#include "nfont.h"
#include <stdint.h>
#include "oled.h"
#include "display.h"

extern uint8_t _binary___binary_data_font_data_start;
extern uint8_t _binary___binary_data_font_data_size;

extern uint8_t _binary___binary_data_tinyfont_data_start;
extern uint8_t _binary___binary_data_tinyfont_data_size;

extern uint8_t _binary___binary_data_bignumbers_data_start;
extern uint8_t _binary___binary_data_bignumbers_data_size;

#define to565(r,g,b)                                            \
    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

#define from565_r(x) ((((x) >> 11) & 0x1f) * 255 / 31)
#define from565_g(x) ((((x) >> 5) & 0x3f) * 255 / 63)
#define from565_b(x) (((x) & 0x1f) * 255 / 31)

uint16_t get_bigpixel(char c,int c_x,int c_y) {
  
  int ypos = (c/(128/16)) * 32;
  int xpos = (c%(128/16)) * 16;
//  ypos+=1;

  int bitposition = ((ypos*128)+(c_y*128) + (xpos)+c_x) *2;

  int byte_position = bitposition/8;
  int bit_in_byte   = 7-(bitposition%8);

  uint8_t byte = ((uint8_t *) &_binary___binary_data_bignumbers_data_start)[byte_position];
  
  uint8_t value=0;
  if((byte & (1 << bit_in_byte)) > 0) value += 2;
  
  bit_in_byte--;
  if((byte & (1 << bit_in_byte)) > 0) value += 1;

  if(value == 0) {value = 0;   }
  if(value == 1) {value = 85;  }
  if(value == 2) {value = 170; }
  if(value == 3) {value = 255; }

  return to565(value,value,value);
}

uint16_t get_pixel(char c,int c_x,int c_y) {
  
  int ypos = (c/(128/8)) * 16;
  int xpos = (c%(128/8)) * 8;
  ypos+=1;

  int bitposition = ((ypos*128)+(c_y*128) + (xpos)+c_x) *2;

  int byte_position = bitposition/8;
  int bit_in_byte   = 7-(bitposition%8);

  uint8_t byte = ((uint8_t *) &_binary___binary_data_font_data_start)[byte_position];
  
  uint8_t value=0;
  if((byte & (1 << bit_in_byte)) > 0) value += 2;
  
  bit_in_byte--;
  if((byte & (1 << bit_in_byte)) > 0) value += 1;

  if(value == 0) {value = 0;   }
  if(value == 1) {value = 85;  }
  if(value == 2) {value = 170; }
  if(value == 3) {value = 255; }

  return to565(value,value,value);
}

uint16_t get_tinypixel(char c,int c_x,int c_y) {
  
  int ypos = (c/(120/5)) * 5;
  int xpos = (c%(120/5)) * 5;

  int bitposition = ((ypos*120)+(c_y*120) + (xpos)+c_x);

  int byte_position = bitposition/8;
  int bit_in_byte   = 7-(bitposition%8);//was8-
  uint8_t byte = ((uint8_t *) &_binary___binary_data_tinyfont_data_start)[byte_position];
  
  uint8_t value=0;
  if((byte & (1 << bit_in_byte)) > 0) value = 1;
  
  if(value == 0) {value = 0;   }
  if(value == 1) {value = 255; }

  return to565(value,value,value);
}

void draw_character(uint32_t x,uint32_t y,char c,uint16_t background) {

  uint16_t character_data[8*16];
//  for(int n=0;n<(8*16);n++) character_data[n] = 0xF000;//background;

  for(size_t c_y=0;c_y<16;c_y++) {
    for(size_t c_x=0;c_x<8;c_x++) {
      int32_t px = get_pixel(c-32,c_x,c_y);
      int32_t value;
 
      if((background != 0) || (background != 65535)) {
				if(px == 65535) {
					value = background;
				} else
        if(px == 0) {
          value = 0;
        } else {
					uint16_t r = from565_r(px);
					uint16_t g = from565_g(px);
					uint16_t b = from565_b(px);
					value = to565(r/background,g/background,b/background);
				}
        if(value < 0) value=0;
      }
      
      if(background == 65535) value = background ^ get_pixel(c-32,c_x,c_y); 
      if(background ==     0) value = get_pixel(c-32,c_x,c_y); 

      character_data[(c_y*8)+c_x] = value;
    }
  }

  oled_draw_rect(x,y,8,16,(uint8_t *) character_data);
}

void draw_bigcharacter(int x,int y,char c,uint16_t background) {

  uint16_t character_data[16*32];
  for(int n=0;n<(16*32);n++) character_data[n]=background^65535;

  if(((c >= 'a')&&(c <= 'z')) ||
     ((c >= 'A')&&(c <= 'Z'))) {
    oled_draw_rect(x,y,16,32,(uint8_t *) character_data);
    draw_character(x,y,c,background);
    return;
  }

  if((c >= '0')&&(c <= '9')) c = c - 48; else
  if(c == '.') c = 10;

  if(c != ' ')
  for(size_t c_y=0;c_y<32;c_y++) {
    for(size_t c_x=0;c_x<16;c_x++) {
      int32_t px = get_bigpixel(c,c_x,c_y);
      int32_t value;
      if(px == 65535) {
        value = background;
      } else {
        int r = from565_r(px);
        int g = from565_g(px);
        int b = from565_b(px);
        value = to565(r/background,g/background,b/background);
      }
      
      if(value < 0) value=0;

      if(background == 65535) value = background ^ get_bigpixel(c,c_x,c_y); 
      if(background ==     0) value = get_bigpixel(c,c_x,c_y); 

      character_data[(c_y*16)+c_x] = value;
    }
  }

  oled_draw_rect(x,y,16,32,(uint8_t *) character_data);
}


void draw_tinycharacter(int x,int y,char c,uint16_t background) {

  uint16_t character_data[5*5];
  for(int n=0;n<(5*5);n++) character_data[n]=n;

  for(size_t c_y=0;c_y<5;c_y++) {
    for(size_t c_x=0;c_x<5;c_x++) {
      int32_t px = get_tinypixel(c-32,c_x,c_y);
      int32_t value=9;

      if((background != 0) && (background != 65535)) {
        if(px == 65535) {
          value = background;
        } 
      } else {
        int r = from565_r(px);
        int g = from565_g(px);
        int b = from565_b(px);
        value = to565(r/background,g/background,b/background);
      }
      
      if(value < 0) value=0;

      if(background == 65535) value = background ^ get_tinypixel(c-32,c_x,c_y); 
      if(background ==     0) value = get_tinypixel(c-32,c_x,c_y); 

      character_data[(c_y*5)+c_x] = value;
    }
  }

  oled_draw_rect(x,y,5,5,(uint8_t *) character_data);
}

void draw_text(int x,int y,const char *text,uint16_t background) {

  size_t length = strlen(text);
  if(length < 0    ) return;
  if(length > 10000) return;

  uint32_t c_x = x;
  uint32_t c_y = y;
  for(size_t n=0;n<length;n++) {
    draw_character(c_x,c_y,text[n],background);
    c_x+=8;
  }
}

void draw_bigtext(int x,int y,const char *text,uint16_t background) {

  uint32_t length = strlen(text);
  if(length < 0    ) return;
  if(length > 10000) return;

  int c_x = x;
  int c_y = y;
  for(size_t n=0;n<length;n++) {
    draw_bigcharacter(c_x,c_y,text[n],background);
    c_x+=16;
  }
}

void draw_tinytext(int x,int y,const char *text,uint16_t background) {

  uint32_t length = strlen(text);
  if(length < 0    ) return;
  if(length > 10000) return;

  int c_x = x;
  int c_y = y;
  for(size_t n=0;n<length;n++) {
    draw_tinycharacter(c_x,c_y,text[n],background);
    c_x+=6;
  }
}

#endif
