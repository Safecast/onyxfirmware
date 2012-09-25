#ifndef DISPLAY_H
#define DISPLAY_H

#include "oled.h"
#include "nfont.h"

extern uint8_t _binary_image_1_data_start;
extern uint8_t _binary_image_1_data_size;


void display_initialise();
void display_clear(int16 color);
void display_draw_image(int x,int y,int width,int height, uint16 *image_data);
void display_draw_line(int start_x,int start_y,int end_x,int end_y,uint16_t color=65535);
void display_draw_point(int x,int y,uint16_t color=65535);
void display_draw_rectangle(int start_x,int start_y,int end_x,int end_y,uint16_t color);
void display_draw_text(int x,int y,const char *text,int16_t background);
void display_draw_text_center(int y,const char *text,int16_t background);
void display_draw_number(int x,int y,uint32_t number,int width,int16_t background);
void display_draw_number_center(int x,int y,uint32_t number,int width,int16_t background);
void display_draw_tinytext(int x,int y,const char *text,int16_t background);
void display_draw_tinynumber(int x,int y,uint32_t number,int width,int16_t background);
void display_draw_bigtext(int x,int y,const char *text,int16_t background);
void display_brightness(uint8 b);
void display_dump_image();
void display_powerup();
void display_powerdown();

#endif
