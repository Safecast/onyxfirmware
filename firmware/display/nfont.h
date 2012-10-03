#ifndef NFONT_H
#define NFONT_H

#include <stdint.h>

void draw_text    (int x,int y,const char *text,uint16_t background);
void draw_tinytext(int x,int y,const char *text,uint16_t background);
void draw_bigtext (int x,int y,const char *text,uint16_t background);

#endif
