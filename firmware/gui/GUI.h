#ifndef GUI_H
#define GUI_H

#include "screen_layout.h"
#include <stdint.h>
#include "display.h"
#include "UserInput.h"

#define NEW_KEYS_MAX_SIZE 40
#define FOREGROUND_COLOR 65535
#define BACKGROUND_COLOR 0
#define COLOR_BLUE  0xF800
#define COLOR_RED   0x7E0
#define COLOR_GREEN 0x1F

#define HEADER_COLOR_NORMAL 0xF800
#define HEADER_COLOR_CPMINVALID 0x03EF

#define LANGUAGE_ENGLISH  0
#define LANGUAGE_JAPANESE 1
#define TEMP_STR_LEN 50

#define KEY_BACK   6
#define KEY_HOME   8
#define KEY_DOWN   4
#define KEY_UP     3
#define KEY_SELECT 2
#define KEY_HELP   0
#define KEY_PRESSED  0
#define KEY_RELEASED 1

class Controller;

class GUI {

public:

  GUI(Controller &r);

  void render();
  void set_cpm_alarm(bool alarm, bool silent, float cpm);
  void receive_update(const char *tag,const void *value);
  void receive_key(int key,int type);
  void set_key_trigger();
  void redraw();
  void set_sleeping(bool sleeping);
  void jump_to_screen(uint8_t screen);
  void toggle_screen_lock();
  void set_language(uint8_t lang);
  void show_dialog      (const char *text1 ,const char *text2 ,const char *text3,const char *text4,bool buzz,int img1=255,int img2=255,int img3=255,int img4=255);
  void render_dialog    (const char *text1 ,const char *text2 ,const char *text3,const char *text4,int img1,int img2,int img3,int img4);
  Controller &controller;
  uint8_t get_item_state_uint8(const char *tag);

private:
  uint8_t current_screen;
  uint8_t selected_item;
  uint8_t last_selected_item;

  bool cpm_alarm; // indicator that we have a High CPM alarm (change display background in red)

  void process_key_up();
  void process_key_down();

  bool softkey_active(uint8_t keynum);
  uint8_t softkey_screen(uint8_t idx);
  uint8_t softkey_index(uint8_t idx);
   char* softkey_action(uint8_t idx);

  void process_key(int key_id,int type);
  void process_keys();

  void clear_screen();
  void leave_screen_actions(int screen);
  void clear_pending_keys();

  bool    m_trigger_any_key;
  bool    m_redraw;
  bool    m_screen_lock;



  int new_keys_start;
  int new_keys_end;
  int new_keys_key [NEW_KEYS_MAX_SIZE];
  int new_keys_type[NEW_KEYS_MAX_SIZE];
  bool m_displaying_dialog;
  char m_dialog_text1[20];
  char m_dialog_text2[20];
  char m_dialog_text3[20];
  char m_dialog_text4[20];
  bool m_displaying_dialog_complete;
  bool m_pause_display_updates;
  bool m_dialog_buzz;
  bool m_repeating;
  int  m_repeat_key;
  int m_repeat_time;
  int m_repeat_delay;
  bool m_repeated;
  bool m_discard_next_keypress;
};

void tick_item(const char *name,bool tick_val);
bool is_ticked(const char *name);


#endif
