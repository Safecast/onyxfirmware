#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"

class Controller {

public:

  Controller(Geiger &g) : m_geiger(g) {
    m_sleeping=false;
    m_powerup=false;
  }

  void set_gui(GUI &g) {
    m_gui = &g;
  }

  void receive_gui_event(char *event,char *value) {

    if(strcmp(event,"Sleep")) {
      if(m_sleeping == false) {
        display_powerdown();
        m_sleeping=true;
        m_gui->set_key_trigger();
        m_gui->set_sleeping(true);
      }
    } else
    if(strcmp(event,"KEYPRESS")) {
      m_powerup=true;
    } else
    if(strcmp(event,"Save")) {
      int c1 = m_gui->get_item_state_uint8("CAL1");
      int c2 = m_gui->get_item_state_uint8("CAL2");
      int c3 = m_gui->get_item_state_uint8("CAL3");
      int c4 = m_gui->get_item_state_uint8("CAL4");
      float calibration_offset = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);
      float base_sieverts = m_geiger.get_microsieverts();    

      char text_sieverts[50];
      float_to_char(base_sieverts+calibration_offset,text_sieverts,5);
      text_sieverts[5] = ' ';
      text_sieverts[6] = 'u';
      text_sieverts[7] = 'S';
      text_sieverts[8] = 'v';
      text_sieverts[9] = 0;

      m_gui->receive_update("Sieverts",text_sieverts);
      m_geiger.set_calibration(calibration_offset);
      m_gui->jump_to_screen(0);
    } else
    if(strcmp(event,"CALIBRATE")) {
      m_calibration_base = m_geiger.get_microsieverts();
      char text_sieverts[50];
      float_to_char(m_calibration_base,text_sieverts,5);
      text_sieverts[5] = ' ';
      text_sieverts[6] = 'u';
      text_sieverts[7] = 'S';
      text_sieverts[8] = 'v';
      text_sieverts[9] = 0;
      m_gui->receive_update("FIXEDSV",text_sieverts);
    } else
    if(strcmp(event,"varnumchange")) {
      int c1 = m_gui->get_item_state_uint8("CAL1");
      int c2 = m_gui->get_item_state_uint8("CAL2");
      int c3 = m_gui->get_item_state_uint8("CAL3");
      int c4 = m_gui->get_item_state_uint8("CAL4");
      float calibration_offset = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);

      char text_sieverts[50];
      float_to_char(m_calibration_base+calibration_offset,text_sieverts,5);
      text_sieverts[5] = ' ';
      text_sieverts[6] = 'u';
      text_sieverts[7] = 'S';
      text_sieverts[8] = 'v';
      text_sieverts[9] = 0;
      m_gui->receive_update("FIXEDSV",text_sieverts);
    }


  }

  
  void update() {

    //TODO: I should change this so it only sends the messages the GUI currently needs.

    if(m_powerup == true) {
      display_powerup();
      m_gui->set_sleeping(false);
      m_gui->redraw();
      m_sleeping=false;
      m_powerup =false;
    }


    if(m_sleeping) return;

    char text_cpm[50];
    char text_cpmd[50];
    char text_sieverts[50];

    text_cpm[0]     =0;
    text_sieverts[0]=0;
    int_to_char(m_geiger.get_cpm(),text_cpm,4);
    float_to_char(m_geiger.get_microsieverts(),text_sieverts,5);
    float_to_char(m_geiger.get_cpm_deadtime_compensated(),text_cpmd,5);

    text_sieverts[5] = ' ';
    text_sieverts[6] = 'u';
    text_sieverts[7] = 'S';
    text_sieverts[8] = 'v';
    text_sieverts[9] = 0;

    float *graph_data;
    graph_data = m_geiger.get_cpm_last_min();

    m_gui->receive_update("CPM",text_cpm);
    m_gui->receive_update("CPMDEAD",text_cpmd);
    m_gui->receive_update("SIEVERTS",text_sieverts);
    m_gui->receive_update("RECENTDATA",graph_data);
    m_gui->receive_update("DELAYA",NULL);
    m_gui->receive_update("DELAYB",NULL);
  }

  GUI    *m_gui;
  Geiger &m_geiger;
  bool    m_sleeping;
  bool    m_powerup;
  float   m_calibration_base;
};

#endif
