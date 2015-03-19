#ifndef NFONT_H
#define NFONT_H

#include <stdint.h>

#define COLOR_WHITE 65535
#define COLOR_BLACK 0
#define COLOR_BLUE  0xF800
#define COLOR_RED   0x7E0
#define COLOR_GREEN 0x1F


void draw_text    (int x,int y,const char *text,uint16_t foreground, uint16_t background);
void draw_tinytext(int x,int y,const char *text,uint16_t background);
void draw_bigtext (int x,int y,const char *text,uint16_t background);

#endif
