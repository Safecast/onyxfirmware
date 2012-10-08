#include "oled.h"
#include "nfont.h"
#include "display.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define to565(r,g,b)                                            \
    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

extern uint8_t _binary___binary_data_splashscreen_start;
extern uint8_t _binary___binary_data_splashscreen_size;

extern uint8_t _binary___binary_data_fixed_images_start;
extern uint8_t _binary___binary_data_fixed_images_size;

extern uint8_t _binary___binary_data_help_screens_start;
extern uint8_t _binary___binary_data_help_screens_size;

uint8_t brightness;

void display_initialise() {
  oled_platform_init();
  oled_init();
}

void display_clear(int16 color) {
  CLS(color);
}

void display_draw_line(int start_x,int start_y,int end_x,int end_y,uint16_t color) {

  // Bresenham's
  int cx = start_x;
  int cy = start_y;

  int dx = end_x - cx;
  int dy = end_y - cy;
  if(dx<0) dx = 0-dx;
  if(dy<0) dy = 0-dy;

  int sx=0; int sy=0;
  if(cx < end_x) sx = 1; else sx = -1;
  if(cy < end_y) sy = 1; else sy = -1;
  int err = dx-dy;

  for(int n=0;n<1000;n++) {
    display_draw_point(cx,cy,color);
    if((cx==end_x) && (cy==end_y)) return;
    int e2 = 2*err;
    if(e2 > (0-dy)) { err = err - dy; cx = cx + sx; }
    if(e2 < dx    ) { err = err + dx; cy = cy + sy; }
  }
}
  
void display_draw_point(int x,int y,uint16_t color) {
  Set_Column_Address(x, x+1);
  Set_Row_Address(y, y+1);
  write_c(0x5c);
  write_d(color);
  write_d(color);
}

void display_draw_rectangle(int start_x,int start_y,int end_x,int end_y,uint16_t color) {

  if(start_x < 0  ) start_x = 0;
  if(start_y < 0  ) start_y = 0;
  if(end_x   > 127) end_x   = 127;
  if(end_y   > 127) end_y   = 127;
  if(end_x   < start_x) return;
  if(end_y   < start_y) return;

  Set_Column_Address(start_x, end_x);
  Set_Row_Address   (start_y, end_y);

  uint32_t size = (end_x-start_x+1)*(end_y-start_y+1);
   
  write_c(0x5C);    // Enable MCU to Read from RAM
  for (uint32_t i=0; i<size;i++) {
    write_d(color);
    write_d(color >> 8);
  } 
}

void display_draw_text(int x,int y,const char *text,int16_t background) {
  ::draw_text(x,y,text,background);
}

void display_draw_text_center(int y,const char *text,int16_t background) {
  int len=strlen(text);
  int w = 128;
  int x = (w-(len*8))/2;
  draw_text(x,y,text,background);
}

void display_draw_tinytext_center(int y,const char *text,int16_t background) {
  int len=strlen(text);
  int w = 128;
  int x = (w-(len*5))/2;
  draw_tinytext(x,y,text,background);
}

void display_draw_number(int x,int y,uint32_t number,int width,int16_t background) {
  char text[50];
  int_to_char(number,text,width);
  draw_text(x,y,text,background);
}

void display_draw_number_center(int x,int y,uint32_t number,int width,int16_t background) {
  char text[50];
  sprintf(text,"%u",number);
  int len=strlen(text);
  int w = width*8;
  x+= (w-(len*8))/2;
  draw_text(x,y,text,background);
}

void display_draw_tinytext(int x,int y,const char *text,int16_t background) {
  ::draw_tinytext(x,y,text,background);
}

void display_draw_bigtext(int x,int y,const char *text,int16_t background) {
  ::draw_bigtext(x,y,text,background);
}


void display_draw_tinynumber(int x,int y,uint32_t number,int width,int16_t background) {
  char text[50];
  int_to_char(number,text,width);
  draw_tinytext(x,y,text,background);
}

void display_draw_image(int x,int y,int width,int height, uint16 *image_data) {
  oled_draw_rect(x,y,width,height,(uint8_t *) image_data);
}

void display_splashscreen(const char *line1,const char *line2) {
  oled_draw_rect(0,0,128,127,((uint8_t *) &_binary___binary_data_splashscreen_start)+1);

  if(line1 != 0) display_draw_tinytext_center(128-8 ,line1,0);
  if(line2 != 0) display_draw_tinytext_center(128-14,line2,0);
}

void display_set_brightness(uint8 b) {
  brightness = b;
  oled_brightness(b);
}

uint8_t display_get_brightness() {
  return brightness;
}

void display_powerup() {
  oled_init();
}

void display_powerdown() {
  oled_deinit();
}

void display_draw_fixedimage_xlimit(uint8_t x,uint8_t y,uint8_t image_number,uint16_t background,uint8 xlimit) {

  // fixed image data is 2bit grayscale, convert to 16bit, it's also fixed size (128x16)

  uint16_t image_data[2048];
  uint8_t *source_data = ((uint8_t *) &_binary___binary_data_fixed_images_start);
  source_data += (image_number*512);

  uint32_t m=0;
  for(uint32_t n=0;n<2048;n++) {
    uint32_t byte = n/4;
    uint32_t bit  = n%4;
    bit = (bit * 2)+1;
    bit = 7-bit;

    uint16_t bit_1 = *(source_data+byte) & (1 << bit);
    uint16_t bit_2 = *(source_data+byte) & (1 << (bit+1));
    if(bit_1 > 0) bit_1 = 1;
    if(bit_2 > 0) bit_2 = 1;
    uint16_t value = bit_1 + (bit_2 << 1);

    if(value == 0) {value = 0;   }
    if(value == 1) {value = 85;  }
    if(value == 2) {value = 170; }
    if(value == 3) {value = 255; }

    value = to565(value,value,value);

    // Back and white background, simple inversion.
    if((background == 0) || (background == 65535)) value = background ^ value;

    int xpos = n%128;
    if(xpos < xlimit) {
      image_data[m] = value;
      m++;
    }
  }

  oled_draw_rect(x,y,xlimit,16,(uint8_t *) image_data);
}

void display_draw_fixedimage(uint8_t x,uint8_t y,uint8_t image_number,uint16_t background) {

  // fixed image data is 2bit grayscale, convert to 16bit, it's also fixed size (128x16)

  uint16_t image_data[2048];
  uint8_t *source_data = ((uint8_t *) &_binary___binary_data_fixed_images_start);
  source_data += (image_number*512);

  for(uint32_t n=0;n<2048;n++) {
    uint32_t byte = n/4;
    uint32_t bit  = n%4;
    bit = (bit * 2)+1;
    bit = 7-bit;

    uint16_t bit_1 = *(source_data+byte) & (1 << bit);
    uint16_t bit_2 = *(source_data+byte) & (1 << (bit+1));
    if(bit_1 > 0) bit_1 = 1;
    if(bit_2 > 0) bit_2 = 1;
    uint16_t value = bit_1 + (bit_2 << 1);

    if(value == 0) {value = 0;   }
    if(value == 1) {value = 85;  }
    if(value == 2) {value = 170; }
    if(value == 3) {value = 255; }

    value = to565(value,value,value);

    // Back and white background, simple inversion.
    if((background == 0) || (background == 65535)) value = background ^ value;
    image_data[n] = value;
  }

  oled_draw_rect(x,y,128,16,(uint8_t *) image_data);
}

void display_test() {
  display_clear(0);

  for(int x=0;x<128;x++) {
    display_draw_rectangle(x,0,x,127,0xFFFF);
    delay_us(400000);
    display_draw_rectangle(x,0,x,127,0x0000);
  }
  for(int y=0;y<128;y++) {
    display_draw_rectangle(0,y,127,y,0xFFFF);
    delay_us(400000);
    display_draw_rectangle(0,y,127,y,0x0000);
  }
}

//TODO: should be refactored into the same code as fixedimages
void display_draw_helpscreen(uint8_t n) {

  // fixed image data is 2bit grayscale, convert to 16bit, it's also fixed size (128x16)

  uint16_t image_data[2048];
  uint8_t *source_data = ((uint8_t *) &_binary___binary_data_help_screens_start);
  source_data += (n*((128*128*2)/8));

  for(uint32_t y_block=0;y_block<8;y_block++) {
		for(uint32_t n=0;n<2048;n++) {
			uint32_t byte = n/4;
			uint32_t bit  = n%4;
			bit = (bit * 2)+1;
			bit = 7-bit;

			uint16_t bit_1 = *(source_data+byte) & (1 << bit);
			uint16_t bit_2 = *(source_data+byte) & (1 << (bit+1));
			if(bit_1 > 0) bit_1 = 1;
			if(bit_2 > 0) bit_2 = 1;
			uint16_t value = bit_1 + (bit_2 << 1);

			if(value == 0) {value = 0;   }
			if(value == 1) {value = 85;  }
			if(value == 2) {value = 170; }
			if(value == 3) {value = 255; }

			value = to565(value,value,value);

			image_data[n] = value;
		}
    source_data += (128*16*2)/8;
    oled_draw_rect(0,y_block*(128/8),128,16,(uint8_t *) image_data);
  }

}
