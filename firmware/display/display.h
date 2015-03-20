#ifndef DISPLAY_H
#define DISPLAY_H

#include "oled.h"
#include "nfont.h"

extern uint8_t _binary_image_1_data_start;
extern uint8_t _binary_image_1_data_size;


void display_clear(int16 color);
void display_draw_image(int x,int y,int width,int height, uint16 *image_data);
void display_draw_line(int start_x,int start_y,int end_x,int end_y,uint16_t color=65535);
void display_draw_point(int x,int y,uint16_t color=65535);
void display_draw_rectangle(int start_x,int start_y,int end_x,int end_y,uint16_t color);
void display_draw_text( int x,int y,const char *text, int16_t foreground, int16_t background);
void display_draw_text_center(int y,const char *text, int16_t foreground, int16_t background);
void display_draw_number(int x,int y,uint32_t number,int width,int16_t foreground, int16_t background);
void display_draw_number_center(int x,int y,uint32_t number,int width,int16_t foreground, int16_t background);
void display_draw_tinytext(int x,int y,const char *text,int16_t background);
void display_draw_tinytext_center(int y,const char *text,int16_t background);
void display_draw_tinynumber(int x,int y,uint32_t number,int width,int16_t background);
void display_draw_bigtext(int x,int y,const char *text,int16_t foreground, int16_t background);
void display_draw_fixedimage(uint8_t x,uint8_t y,uint8_t image_number,uint16_t background);
void display_draw_fixedimage_xlimit(uint8_t x,uint8_t y,uint8_t image_number,uint16_t background,uint8 xlimit);
void display_set_brightness(uint8 b);
uint8_t display_get_brightness();
void display_splashscreen(const char *line1,const char *line2);
void display_powerup();
void display_powerdown();
void display_test();
void display_draw_helpscreen_en(uint8_t n);
void display_draw_helpscreen_jp(uint8_t n);

#endif
