#include "libmaple.h"
#include "gpio.h"
#include "safecast_config.h"
#include "usart.h"
#include "Geiger.h"
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "flashstorage.h"
#include "oled.h"
#include "display.h"
#include "buzzer.h"
#include <limits.h>
#include "dac.h"
#include "log_read.h"
#include "rtc.h"
#include "realtime.h"
#include <stdint.h>
#include <inttypes.h>
#include "captouch.h"
#include "power.h"
#include "libjson.h"

extern "C" {
  void signing_test();
  int signing_isKeyValid();
  void signing_printPubKey();
  void signing_printGUID();
  void signing_hashLog();
}

/**
 * This is defined at linking time
 */
extern uint8_t _binary___binary_data_private_key_data_start;
extern uint8_t _binary___binary_data_private_key_data_size;


extern Geiger *system_geiger;

#define TX1 BOARD_USART1_TX_PIN
#define RX1 BOARD_USART1_RX_PIN

typedef void (*command_process_t)(char *);

command_process_t command_stack[10];
int  command_stack_size = 0;


#define MAX_COMMAND_LEN 16
/* Make sure to increment when adding new commands */
#define MAX_COMMANDS 40

int command_list_size;
char command_list [MAX_COMMANDS][MAX_COMMAND_LEN];
command_process_t command_funcs[MAX_COMMANDS];

/**
 * Called during serial port initialization, registers a table
 * of pointers to the various serial commands, that is used by
 * serial_process_command afterwards.
 */
void register_cmd(const char *cmd,command_process_t func) {

  if(command_list_size > MAX_COMMANDS) return;

  strcpy(command_list[command_list_size],cmd);
  command_funcs[command_list_size] = func;

  command_list_size++;
}

void serial_write_string(const char *str) {
  for(uint32_t n=0;str[n]!=0;n++) {
    usart_putc(USART1, str[n]);
  }
}

void command_stack_pop() {
  command_stack_size--;
  serial_write_string("\r\n>");
}


/**
 * Very simplistic output of a json structure as:
 *   { "key": "val" }
 */
void json_keyval(const char *cmd, const char *val) {
  serial_write_string("{ \"");
  serial_write_string(cmd);
  serial_write_string("\": \"");
  serial_write_string(val);
  serial_write_string("\"}");
}

/**
 * hello: a simple "ping" command to check that the
 *        device is responsive
 */
void cmd_hello(char *line) {
  JSONNODE *n = json_new(JSON_NODE);
  json_push_back(n, json_new_a("hello", "Greetings professor Falken"));
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);
}

/**
 * Easter egg...
 */
void cmd_games(char *line) {
  serial_write_string("I'M KIND OF BORED OF GAMES, TURNS OUT THE ONLY WAY TO WIN IS NOT TO PLAY...\r\n");
}

/**
 * Dumps the data log as a json structure. Pauses
 * logging at the beginning and restarts afterwards.
 *
 * Note: this does not use libjson.
 */
void cmd_logxfer(char *line) {

  flashstorage_log_pause();
  log_read_start();

  char buffer[1024];
  int size = log_read_block(buffer);

  for(;size!=0;) {
    if(size != 0) serial_write_string(buffer);
    size = log_read_block(buffer);
  }

  flashstorage_log_resume();
}

/**
 * Dumps the data log as a comma-separated stream. Pauses
 * logging at the beginning and restarts afterwards.
 */
void cmd_logcsv(char *line) {

  flashstorage_log_pause();
  log_read_start();

  char buffer[1024];
  int size = log_read_csv(buffer);

  for(;size!=0;) {
    if(size != 0) serial_write_string(buffer);
    size = log_read_csv(buffer);
  }

  flashstorage_log_resume();
}

/**
 * Displays status of log area: records used, total records
 * and logging interval (in seconds)
 */
void cmd_logstatus(char *line) {
  JSONNODE *n = json_new(JSON_NODE);
  JSONNODE *n2 = json_new(JSON_NODE);
  json_set_name(n2, "logstatus");
  json_push_back(n2, json_new_i("used", flashstorage_log_currentrecords()));
  json_push_back(n2, json_new_i("total", flashstorage_log_maxrecords()));
  const char *sloginter = flashstorage_keyval_get("LOGINTERVAL");
  uint32_t c = 0;
  if(sloginter != 0) {
    sscanf(sloginter, "%"PRIu32"", &c);
  } else {
    c = 30 * 60;
  }
  json_push_back(n2, json_new_i("interval", c));
  json_push_back(n, n2);
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);
}


/**
 * Debugging command: changes the OLED display drive parameters.
 *  Use with caution, you can probably damage the screen if you use wrong
 *  parameters.
 *
 *  This is part 2 of the command that is run when the result of cmd_displayparams is
 *  received
 */
void serial_displayparams_run(char *line) {

  uint32_t clock, multiplex, functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh;

  sscanf(line,"%"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32"",&clock,&multiplex,&functionselect,&vsl,&phaselen,&prechargevolt,&prechargeperiod,&vcomh);

  char outline[1024];
  sprintf(outline,"Received values: %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32", calling reinit\r\n",clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh); //"
  serial_write_string(outline);

  oled_reinit(clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh);
  command_stack_pop();
}

/**
 * Debugging command: changes the OLED display drive parameters.
 *  Use with caution, you can probably damage the screen if you use wrong
 *  parameters.
 *
 *  This is part 1 of the command gets user input
 */
void cmd_displayparams(char *line) {

  serial_write_string("DISPLAY REINIT MODE\r\n");
  serial_write_string("COMMAND IS: <CLOCK> <MULTIPLEX> <FUNCTIONSELECT> <VSL> <PHASELEN> <PRECHARGEVOLT> <PRECHARGEPERIOD> <VCOMH>\r\n");
  serial_write_string("e.g: 241 127 1 1 50 23 8 5\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_displayparams_run;
  command_stack_size++;
}

/**
 * Outputs a list of supported commands
 */
void cmd_help(char *line) {

  serial_write_string("Available commands:\r\n\t");
  for(int n=0;n<command_list_size;n++) {
    serial_write_string(command_list[n]);
    serial_write_string(", ");
    if ((n%10 == 0) && (n > 0))
      serial_write_string("\r\n\t");	// put 10 commands per line
  }
  serial_write_string("\n");

}

/**
 * Launches a test of the OLED screen
 */
void cmd_displaytest(char *line) {
  display_test();
}

/**
 * Outputs firmware version
 */
void cmd_version(char *line) {

  JSONNODE *n = json_new(JSON_NODE);
  json_push_back(n, json_new_a("version", OS100VERSION));
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);
}

/**
 * Outputs device tag
 */
void cmd_getdevicetag(char *line) {
  const char *devicetag = flashstorage_keyval_get("DEVICETAG");
  JSONNODE *n = json_new(JSON_NODE);
  if(devicetag != 0) {
    json_push_back(n, json_new_a("devicetag", devicetag));
  } else {
    json_push_back(n, json_new_a("devicetag", "No device tag set"));
  }
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);
}

/**
 * Sets the device tag (part 2 of command)
 */
void serial_setdevicetag_run(char *line) {

  char devicetag[100];

  sscanf(line,"%s\r\n",devicetag);

  flashstorage_keyval_set("DEVICETAG",devicetag);
  command_stack_pop();
}

/**
 * Sets the device tag (part 1 of command)
 */
void cmd_setdevicetag(char *line) {

  serial_write_string("SETDEVICETAG:\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_setdevicetag_run;
  command_stack_size++;
}

void cmd_magread(char *line) {
  gpio_set_mode (PIN_MAP[41].gpio_device,PIN_MAP[41].gpio_bit, GPIO_OUTPUT_PP); // MAGPOWER
  gpio_set_mode (PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit, GPIO_INPUT_PU);  // MAGSENSE

  // Power up magsense
  gpio_write_bit(PIN_MAP[41].gpio_device,PIN_MAP[41].gpio_bit,1);

  // wait...
  delay_us(1000);

  // Read magsense
  int magsense = gpio_read_bit(PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit);

  char magsenses[50];
  sprintf(magsenses,"%u\r\n",magsense);
  serial_write_string(magsenses);
}

/**
 * Gets the current CPM reading as
 * { "cpm": {
 *     "value": val,      // Actual value
 *     "valid": boolean,  // Valid flag
 *     "raw": val,        // Uncompensated value
 *     "cpm30": val       // 30 second window
 *      }
 *  }
 *
 * TODO: also output uSv/h value that reflects the settings
 *       of the device.
 *
 *  See Geiger.cpp for a more in-depth explanation of
 *  raw and compensated CPM values.
 */
void cmd_cpm(char *line) {

  JSONNODE *n = json_new(JSON_NODE);
  JSONNODE *reading = json_new(JSON_NODE);
  json_set_name(reading, "cpm");
  json_push_back(reading, json_new_f("value", system_geiger->get_cpm_deadtime_compensated()));
  json_push_back(reading, json_new_b("valid", system_geiger->is_cpm_valid()));
  json_push_back(reading, json_new_f("raw", system_geiger->get_cpm()));
  json_push_back(reading, json_new_f("cpm30", system_geiger->get_cpm30()));
  json_push_back(n, reading);
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);

}

void cmd_writedac(char *line) {
  dac_init(DAC,DAC_CH2);

  int8_t idelta=1;
  uint8_t i=0;
  for(int n=0;n<1000000;n++) {

    if(i == 254) idelta = -1;
    if(i == 0  ) idelta =  1;

    i += idelta;

    dac_write_channel(DAC,2,i);
  }
  serial_write_string("WRITEDACFIN");
}

void cmd_testhp(char *line) {
  gpio_set_mode (PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit, GPIO_OUTPUT_PP);  // HP_COMBINED
  for(int n=0;n<100000;n++) {
    gpio_write_bit(PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit,1);
    delay_us(100);
    gpio_write_bit(PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit,0);
    delay_us(100);
  }
}

void cmd_readadc(char *line) {
  adc_init(PIN_MAP[12].adc_device); // all on ADC1

  adc_set_extsel(PIN_MAP[12].adc_device, ADC_SWSTART);
  adc_set_exttrig(PIN_MAP[12].adc_device, true);

  adc_enable(PIN_MAP[12].adc_device);
  adc_calibrate(PIN_MAP[12].adc_device);
  adc_set_sample_rate(PIN_MAP[12].adc_device, ADC_SMPR_55_5);

  gpio_set_mode (PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit, GPIO_INPUT_ANALOG);
  gpio_set_mode (PIN_MAP[19].gpio_device,PIN_MAP[19].gpio_bit, GPIO_INPUT_ANALOG);
  gpio_set_mode (PIN_MAP[20].gpio_device,PIN_MAP[20].gpio_bit, GPIO_INPUT_ANALOG);

  uint16 value1 = adc_read(PIN_MAP[12].adc_device,PIN_MAP[12].adc_channel);
  uint16 value2 = adc_read(PIN_MAP[19].adc_device,PIN_MAP[19].adc_channel);
  uint16 value3 = adc_read(PIN_MAP[20].adc_device,PIN_MAP[20].adc_channel);
  char values[50];
  sprintf(values,"PA6 ADC Read: %u\r\n",value1);
  serial_write_string(values);
  sprintf(values,"PC4 ADC Read: %u\r\n",value2);
  serial_write_string(values);
  sprintf(values,"PC5 ADC Read: %u\r\n",value3);
  serial_write_string(values);
}

void cmd_setmicreverse(char *line) {
  gpio_set_mode (PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit, GPIO_OUTPUT_PP);  // MICREVERSE
  gpio_set_mode (PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit, GPIO_OUTPUT_PP);  // MICIPHONE
  gpio_write_bit(PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,1); // MICREVERSE
  gpio_write_bit(PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,0); // MICIPHONE
  serial_write_string("Set MICREVERSE to 1, MICIPHONE to 0\r\n");
}

void cmd_setmiciphone(char *line) {
  gpio_set_mode (PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit, GPIO_OUTPUT_PP);  // MICREVERSE
  gpio_set_mode (PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit, GPIO_OUTPUT_PP);  // MICIPHONE
  gpio_write_bit(PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,0); // MICREVERSE
  gpio_write_bit(PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,1); // MICIPHONE
  serial_write_string("Set MICREVERSE to 0, MICIPHONE to 1\r\n");
}

void cmd_testsign(char *line) {
  signing_test();
}

void cmd_pubkey(char *line) {
  signing_printPubKey();
  serial_write_string("\r\n");
}

/**
 * Prints the Unique ID of the device. Does not use libjson
 * for output formatting for the sake of convenience.
 */
void cmd_guid(char *line) {
  serial_write_string("{ \"guid\": \"");
  signing_printGUID();
  serial_write_string("\"}\r\n");
}

void cmd_keyvalid(char *line) {
  if( signing_isKeyValid() == 1 )
    serial_write_string("uu_valid VALID KEY\r\n");
  else
    serial_write_string("uu_valid IMPROPER OR UNINITIALIZED KEY\r\n");
}

void cmd_logsig(char *line) {
  signing_hashLog();
  serial_write_string("\r\n");
}


/**
 * Stops logging in flash
 */
void cmd_logpause(char *line) {
  flashstorage_log_pause();
}

/**
 * Resumes logging in flash
 */
void cmd_logresume(char *line) {
  flashstorage_log_resume();
}

/**
 * Clears the flash log.
 */
void cmd_logclear(char *line) {
  serial_write_string("Clearing flash log\r\n");
  flashstorage_log_clear();
  serial_write_string("Cleared\r\n");
}

void cmd_dumpflash(char *line) {
	flashstorage_dump_hex();
}

void cmd_logstress(char *line) {
  serial_write_string("Log stress testing - 5000 writes\r\n");

  log_data_t data;

  data.time  = rtc_get_time(RTC);
  data.cpm   = 1;
  data.accel_x_start = 2;
  data.accel_y_start = 3;
  data.accel_z_start = 4;
  data.log_type      = UINT_MAX;

  char err[50];
  for(int n=0;n<5000;n++) {
    data.cpm = n;
    int r = flashstorage_log_pushback((uint8_t *) &data,sizeof(log_data_t));
    sprintf(err,"Entry: %i return code: %d\r\n",n, r);
    serial_write_string(err);
  }
  serial_write_string("Complete\r\n");
}


/**
 * Debug command: dumps all settings (key/value).
 * Output in this format:
 * { "keys" : {
 *    "key1": "value1",
 *    "key2": "value2",
 *    ...
 *    }
 *  }
 */
void cmd_keyvaldump(char *line) {
  char key[50];
  char val[50];

  serial_write_string("{ \"keys\": {\n");
  char str[110];
  for(int n=0;;n++) {
    flashstorage_keyval_by_idx(n,key,val);
    if(key[0] == 0) break;
    if (n>0) serial_write_string(",\n");
    sprintf(str,"  \"%s\": \"%s\"",key,val);
    serial_write_string(str);
  }
  serial_write_string("  }\n}\n");
}

/**
 * Debug: sets a setting (key/value) / part 2
 */
void serial_setkeyval_run(char *line) {

  char key[200];
  char val[200];

  int eqpos=-1;
  int len=strlen(line);
  for(int n=0;n<len;n++) {
    if(line[n] == '=') {
      eqpos = n;
    }
  }

  if(eqpos==-1) return;
  if(eqpos==0) {		// = on a line ends the command
    flashstorage_keyval_update();
    command_stack_pop();
    return;
  }
  for(int n=0;n<eqpos;n++) {
    key[n]=line[n];
    key[n+1]=0;
  }

  eqpos+=1;
  for(int n=eqpos;n<=len ;n++) {
    val[n-eqpos]=line[n];
    val[n-eqpos+1]=0;
  }

  flashstorage_keyval_set(key,val);
}

/**
 * Debug: sets a setting (key/value) / part 1
 */
void cmd_keyvalset(char *line) {
  serial_write_string("|| Enter KEY=VAL, end with a single = on a new line.\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_setkeyval_run;
  command_stack_size++;
}


/**
 * Sets the real time clock (unix time). Part 2
 */
void serial_setrtc_run(char *line) {

  uint32_t unixtime = 0;
  sscanf(line,"%"PRIu32"\r\n",&unixtime);

  realtime_set_unixtime(unixtime);
  command_stack_pop();
}

/**
 * Gets current time on the device
 */
void cmd_getrtc() {
  JSONNODE *n = json_new(JSON_NODE);
  json_push_back(n, json_new_i("rtc", realtime_get_unixtime()));
  json_char *jc = json_write_formatted(n);
  serial_write_string(jc);
  json_free(jc);
  json_delete(n);
}

/**
 * Sets the real time clock (unix time). Part 1
 */
void cmd_setrtc(char *line) {
  char info[100];
  sprintf(info,"Current unixtime is %"PRIu32"\r\n",realtime_get_unixtime());
  serial_write_string(info);
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_setrtc_run;
  command_stack_size++;
}

void cmd_setalarm(char *line) {
  serial_write_string("Alarm triggered for 10s\r\n");
  rtc_set_alarm(RTC,rtc_get_time(RTC)+10);
}


void cmd_captouchparams_run(char *line) {

  uint32_t mhd_r, nhd_r, ncl_r, fdl_r;
  uint32_t mhd_f, nhd_f, ncl_f, fdl_f;
  uint32_t dbr,touchthres,relthres;

  sscanf(line,"%"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32""
             ,&mhd_r,&nhd_r,&ncl_r,&fdl_r
             ,&mhd_f,&nhd_f,&ncl_f,&fdl_f
             ,&dbr,&touchthres,&relthres);

  char outline[1024];
  sprintf(outline,"Read values: %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32"\n"
             ,mhd_r,nhd_r,ncl_r,fdl_r
             ,mhd_f,nhd_f,ncl_f,fdl_f
             ,dbr,touchthres,relthres);
  serial_write_string(outline);

  cap_set_mhd_r(mhd_r);
  cap_set_nhd_r(nhd_r);
  cap_set_ncl_r(ncl_r);
  cap_set_fdl_r(fdl_r);

  cap_set_mhd_f(mhd_f);
  cap_set_nhd_f(nhd_f);
  cap_set_ncl_f(ncl_f);
  cap_set_fdl_f(fdl_f);

  cap_set_dbr  (dbr);
  cap_set_touch_threshold  (touchthres);
  cap_set_release_threshold(relthres);

  cap_deinit();
  cap_init();
  command_stack_pop();
}


void cmd_captouchparams(char *line) {
  serial_write_string("CAPTOUCH PARAMS TEST\r\n");
  serial_write_string("COMMAND IS: <MHD_R> <NHD_R> <NCL_R> <FDL_R> <MHD_F> <NHD_F> <NCL_F> <FDL_F> <DBR> <TOUCHTHRES> <RELTHRES>\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = cmd_captouchparams_run;
  command_stack_size++;
}

void cmd_captouchdump(char *line) {

  serial_write_string("CAPTOUCH ELECTRODE STATE\r\n");
  serial_write_string("ELECV ELECBASELINE KEYSTATE THISKEYSTATE\r\n");
  for(int n=0;n<11;n++) {
    char *s = cap_diagdata(n);
    serial_write_string(s);
    serial_write_string("\r\n");
  }
}

/**
 * Displays battery level on OLED for 100 seconds,
 * updates once per second
 */
void cmd_batinfodisp(char *line) {
  for(int n=0;n<100;n++) {
    int bat = power_battery_level();
    char s[20];
    sprintf(s,"%d   ",bat);

    display_draw_text(0,0,s,0);
    delay_us(1000000);
  }
}

/**
 * Initializes the list of firmware commands in a pointer table
 */
void register_cmds() {
  // help
  register_cmd("HELP"          ,cmd_help);
  // log
  register_cmd("LOGXFER"       ,cmd_logxfer);
  register_cmd("READ CSV LOG"  ,cmd_logcsv);
  register_cmd("LOGSIG"        ,cmd_logsig);
  register_cmd("LOGPAUSE"      ,cmd_logpause);
  register_cmd("LOGRESUME"     ,cmd_logresume);
  register_cmd("LOGCLEAR"      ,cmd_logclear);
  // get
  register_cmd("VERSION"       ,cmd_version);		// GETVERSION
  register_cmd("GUID"          ,cmd_guid);		// GETGUID
  register_cmd("GETDEVICETAG"  ,cmd_getdevicetag);
  register_cmd("MAGREAD"       ,cmd_magread);		// GETMAG ? hall sensor?
  // cpm
  register_cmd("GETCPM"        ,cmd_cpm);		// CPM
  // ??
  register_cmd("WRITEDAC"      ,cmd_writedac);
  register_cmd("READADC"       ,cmd_readadc);
  // set
  register_cmd("SETDEVICETAG"  ,cmd_setdevicetag);
  register_cmd("SETMICREVERSE" ,cmd_setmicreverse);
  register_cmd("SETMICIPHONE"  ,cmd_setmiciphone);
  register_cmd("DISPLAYPARAMS" ,cmd_displayparams);	// SETDIAPLAYPARAMS
  register_cmd("SETRTC"        ,cmd_setrtc);
  register_cmd("SETALARM"      ,cmd_setalarm);
  // test/debug
  register_cmd("DISPLAYTEST"   ,cmd_displaytest);	// TESTDISPLAY
  register_cmd("BATINFODISP"   ,cmd_batinfodisp);	// TESTBATDISPLAY
  register_cmd("TESTHP"        ,cmd_testhp);		// ??
  register_cmd("LOGSTRESS"     ,cmd_logstress);		// TESTLOG
  register_cmd("TESTSIGN"      ,cmd_testsign);
  register_cmd("PUBKEY"        ,cmd_pubkey);
  register_cmd("KEYVALID"      ,cmd_keyvalid);
  register_cmd("KEYVALDUMP"    ,cmd_keyvaldump);
  register_cmd("KEYVALSET"     ,cmd_keyvalset);
  register_cmd("CAPTOUCHTEST"  ,cmd_captouchparams);
  register_cmd("CAPTOUCHDUMP"  ,cmd_captouchdump);
  register_cmd("DUMPFLASH"	   , cmd_dumpflash);
  // misc
  register_cmd("HELLO"         ,cmd_hello);
  register_cmd("LIST GAMES"    ,cmd_games);
}

/**
 * This function is called by default with the latest received
 * command line as argument.
 */
void cmd_main_menu(char *line) {

  for(int n=0;n<command_list_size;n++) {
    if(strcmp(line,command_list[n]) == 0) {
      (*command_funcs[n])(line);
    }
  }

  if (command_stack_size == 1)
    serial_write_string("\r\n>");
}

/**
 * Initialize the serial port interface on the Onyx.
 */
void serial_initialise() {
  const stm32_pin_info *txi = &PIN_MAP[TX1];
  const stm32_pin_info *rxi = &PIN_MAP[RX1];

  gpio_set_mode(txi->gpio_device, txi->gpio_bit, GPIO_AF_OUTPUT_OD);
  gpio_set_mode(rxi->gpio_device, rxi->gpio_bit, GPIO_INPUT_FLOATING);

  if (txi->timer_device != NULL) {
      /* Turn off any PWM if there's a conflict on this GPIO bit. */
      timer_set_mode(txi->timer_device, txi->timer_channel, TIMER_DISABLED);
  }

  register_cmds();
  command_stack[0] = cmd_main_menu;
  command_stack_size = 1;

  usart_init(USART1);
  usart_set_baud_rate(USART1, STM32_PCLK2, ERROR_USART_BAUD);
  usart_enable(USART1);
}


/**
 * This command is not compiled in, so that the private key
 * cannot be retrieved on a standard running device
 */

/*
void serial_readprivatekey() {

  char stemp[50];
  uint8_t *source_data = ((uint8_t *) &_binary___binary_data_private_key_data_start);

  sprintf(stemp,"Private key area baseaddr: %x\r\n",source_data);
  serial_write_string(stemp);

  uint32_t pageoffset = ((uint32_t) source_data)%2048;
  sprintf(stemp,"Page offset: %x\r\n",pageoffset);

  serial_write_string("Private key region data: \r\n");
  for(uint32_t n=0;n<((6*1024))-pageoffset;n++) {
    if((n%32) == 0) {
      serial_write_string("\r\n");
      sprintf(stemp,"%x : ",source_data+n);
      serial_write_string(stemp);
    }

    sprintf(stemp,"%02x ",source_data[n]);
    serial_write_string(stemp);
  }
  serial_write_string("\r\n");

  serial_write_string("Private key programmable data only: \r\n");
  uint8_t *source_data_programmable = (uint8_t *) 0x8000800;

  sprintf(stemp,"Private key programmable data only baseaddr: %x\r\n",source_data_programmable);
  serial_write_string(stemp);

  for(uint32_t n=0;n<(6*1024);n++) {

    if((n%32) == 0) {
      serial_write_string("\r\n");
      sprintf(stemp,"%x : ",source_data_programmable+n);
      serial_write_string(stemp);
    }

    if(n == (4*1024)) {
      serial_write_string("\r\nShowing following 2k region, to confirm code not overwritten:\r\n");
      sprintf(stemp,"%x : ",source_data_programmable+n);
      serial_write_string(stemp);
    }

    sprintf(stemp,"%02x ",source_data_programmable[n]);
    serial_write_string(stemp);

  }
  serial_write_string("\r\n");

}

void serial_writeprivatekey() {

  // TODO: Will add code to read from serial and write data to flash region here.
  serial_write_string("Writing incrementing private key data\r\n");

  uint8_t pagedata[2048];
  for(int n=0;n<2048;n++) pagedata[n] = n;

  uint8_t *source_data_programmable = (uint8_t *) 0x8000800;
  flashstorage_unlock();
  flashstorage_erasepage(source_data_programmable);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage(pagedata,source_data_programmable);
  flashstorage_lock();

  source_data_programmable += 2048;
  flashstorage_unlock();
  flashstorage_erasepage(source_data_programmable);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage(pagedata,source_data_programmable);
  flashstorage_lock();
}
*/


/**
 * Called by serial_eventloop, this calls the relevant commands.
 *
 * This calls the next command in the stack. By default, the command
 * is cmd_main_menu.
 *
 * New in 12.17 onwards: we accept json-formatted commands as well. Those
 * commands are parsed & dispatched below.
 *
 * Important remark: the CLI does not support multiple commands in one
 * line. For instance { "get": "guid", "get": "rtc" } will not work.
 *
 * JSON commands are as follow:
 *
 * Settings not linked to setting keys
 * "set" {
 *      "rtc": integer (Unix timestamp)
 *      "devicetag": string (device tag)
 *      }
 *
 *  Get/set settings keys (Work in progress not implemented yet):
 * "setkey" { "name": string, "value": value }
 * "getkey": "name"
 *
 *  Getting values not linked to setting keys
 * "get":
 *    - "guid"
 *    - "rtc"
 *    - "devicetag"
 *    - "settings"
 *    - "cpm"
 *
 */
void serial_process_command(char *line) {

  JSONNODE *n = json_parse(line);
  if (n == NULL) {
    // Old style commands
    (*command_stack[command_stack_size-1])(line);
  } else {
    // Dispatch:
    int err = true;
    /////
    // get
    /////
    JSONNODE *cmd = json_get_nocase(n,"get");
    if (cmd != 0 && json_type(cmd) == JSON_STRING) {
      json_char *val = json_as_string(cmd);
      if (strcmp(val, "cpm") == 0) {
        err = false;
        cmd_cpm(0);
      } else
      if (strcmp(val, "guid") == 0) {
        err = false;
        cmd_guid(0);
      } else
      if (strcmp(val,"rtc") == 0) {
        err = false;
        cmd_getrtc();
      } else
      if (strcmp(val,"devicetag") == 0) {
        err = false;
        cmd_getdevicetag(0);
      } else
      if (strcmp(val, "settings") == 0) {
        err = false;
        cmd_keyvaldump(0);
      } else
      if (strcmp(val, "version") == 0) {
        err = false;
        cmd_version(0);
      } else
      if (strcmp(val,"logstatus") == 0) {
         err = false;
         cmd_logstatus(0);
      }
      json_free(val);
    }
    /////
    // set
    /////
    cmd = json_get_nocase(n,"set");
    if (cmd !=0 && json_type(cmd) == JSON_NODE) {
      // Find what set operation we wanted:
      JSONNODE *op = json_get_nocase(cmd, "devicetag");
      if (op != 0 && json_type(op) == JSON_STRING) {
        err = false;
        json_char *tag = json_as_string(op);
        flashstorage_keyval_set("DEVICETAG",tag);
        json_keyval("ok", "devicetag");
      }
      op = json_get_nocase(cmd, "rtc");
      if (op != 0 && json_type(op) == JSON_NUMBER) {
        err = false;
        uint32_t  time = json_as_int(op);
        if (time != 0) {
          realtime_set_unixtime(time);
          json_keyval("ok", "rtc");
        }
      }
    }
    if (err) {
      json_keyval("error", "unknown command");
    }
  }
  json_delete(n);
}



char currentline[1024];
uint32_t currentline_position=0;

/**
 * Receive and process what we received on the serial port. This
 * is called from the main event loop in main.cpp
 *
 */
void serial_eventloop() {
  char buf[64];

  // The ring buffer for the UART is 64 bytes, so we will never
  // get more than 63 bytes...
  uint32_t read_size = usart_rx(USART1,(uint8 *) buf,63);
  if (read_size == 0) return;

  for(uint32_t n=0;n<read_size;n++) {
    // We echo characters
    usart_putc(USART1, buf[n]);
    if((buf[n] == 13) || (buf[n] == 10)) {
      serial_write_string("\r\n");
      currentline[currentline_position]=0;
      serial_process_command(currentline);
      currentline_position=0;
      usart_reset_rx(USART1);
    } else {
      currentline[currentline_position] = buf[n];
      currentline_position++;
      if (currentline_position > 1024) {
    	  serial_write_string("Input buffer overflow");
    	  currentline_position = 0;
      }
    }
  }
}
