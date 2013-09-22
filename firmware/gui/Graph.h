#ifndef GRAPH_H
#define GRAPH_H

#include "Display.h"
#include <stdlib.h>

using namespace std;

class Graph {

public:

  Graph(char *name,int x,int y) : m_x(x),m_y(y), m_size(0), m_enable_render(false), m_first_render(false) {
    strcpy(m_name,name);
  }

  void render(Display &d,uint16_t color=0x0000) {
//    d.draw_line(m_x,m_y,m_x,m_y+50);
//    d.draw_line(m_x,m_y+50,m_x+100,m_y+50);
  }

  void set_graph_data(float *data,size_t size,Display &d) {

    if(m_enable_render) {
      for(size_t n=0;n<size;n++) {
        if(n >= m_size) {
          d.draw_point(m_x+(n*4),100-data[n],0x0000);
        } else {
          if(m_graph_data[n] != data[n]) {
            d.draw_point(m_x+(n*4),100-m_graph_data[n],0xFFFF);
            d.draw_point(m_x+(n*4),100-data[n],0x0000);
          }
        }
      }
      if(m_first_render) d.draw_text(0,112,"CPM last 30mins",0);
      m_first_render=false;
    }

    for(size_t n=0;n<size;n++) {
      m_graph_data[n] = data[n];
    }
    m_size = size;

  }

  void set_enable_render(bool v) {
    if((m_enable_render == false) && (v == true)) {
      m_size = 0;
      m_first_render=true;
    }
    m_enable_render = v;
  }

  int m_x;
  int m_y;
  char m_name[50];
  float m_graph_data[128];
  size_t m_size;
  bool m_enable_render;
  bool m_first_render;
};

#endif
