#include "libmaple.h"
#include "gpio.h"
#include "flashstorage.h"
#include "safecast_config.h"
#include "usart.h"
#include "Geiger.h"
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "oled.h"
#include "display.h"
#include <limits.h>
#include "dac.h"
#include "log_read.h"
#include "rtc.h"
#include "realtime.h"
#include <stdint.h>
#include <inttypes.h>
#include "captouch.h"
#include "power.h"

extern "C" {
  void signing_test();
  int signing_isKeyValid();
  void signing_printPubKey();
  void signing_printGUID();
  void signing_hashLog();
}

extern uint8_t _binary___binary_data_private_key_data_start;
extern uint8_t _binary___binary_data_private_key_data_size;
extern Geiger *system_geiger;

#define TX1 BOARD_USART1_TX_PIN
#define RX1 BOARD_USART1_RX_PIN

typedef void (*command_process_t)(char *);

command_process_t command_stack[10];
int  command_stack_size = 0;

void serial_process_command(char *line) {
  (*command_stack[command_stack_size-1])(line);
}

#define MAX_COMMAND_LEN 16
/* Make sure to increment when adding new commands */
#define MAX_COMMANDS 32

int command_list_size;
char command_list [MAX_COMMANDS][MAX_COMMAND_LEN];
command_process_t command_funcs[MAX_COMMANDS];

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

void cmd_hello(char *line) {
 serial_write_string("GREETINGS PROFESSOR FALKEN.\r\n");
}

void cmd_games(char *line) {
  serial_write_string("I'M KIND OF BORED OF GAMES, TURNS OUT THE ONLY WAY TO WIN IS NOT TO PLAY...\r\n");
}

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

void serial_displayparams_run(char *line) {

  uint32_t clock, multiplex, functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh;

  sscanf(line,"%"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32"",&clock,&multiplex,&functionselect,&vsl,&phaselen,&prechargevolt,&prechargeperiod,&vcomh);

  char outline[1024];
  sprintf(outline,"Received values: %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32" %"PRIu32", calling reinit\r\n",clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh); //"
  serial_write_string(outline); 

  oled_reinit(clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh);
  command_stack_pop();
}

void cmd_displayparams(char *line) {

  serial_write_string("DISPLAY REINIT MODE\r\n");
  serial_write_string("COMMAND IS: <CLOCK> <MULTIPLEX> <FUNCTIONSELECT> <VSL> <PHASELEN> <PRECHARGEVOLT> <PRECHARGEPERIOD> <VCOMH>\r\n");
  serial_write_string("e.g: 241 127 1 1 50 23 8 5\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_displayparams_run;
  command_stack_size++;
}

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

void cmd_displaytest(char *line) {
  display_test();
}

void cmd_version(char *line) {
  serial_write_string("Version: ");
  serial_write_string(OS100VERSION);
  serial_write_string("\r\n");
}

void cmd_getdevicetag(char *line) {
  const char *devicetag = flashstorage_keyval_get("DEVICETAG");
  if(devicetag != 0) {
    char stemp[100];
    sprintf(stemp,"Devicetag: %s\r\n",devicetag);
    serial_write_string(stemp);
  } else {
    serial_write_string("No device tag set");
  }
}

void serial_setdevicetag_run(char *line) {

  char devicetag[100];

  sscanf(line,"%s\r\n",devicetag);

  flashstorage_keyval_set("DEVICETAG",devicetag);
  command_stack_pop();
}

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

void cmd_cpm(char *line) {
  char str[16];
  sprintf(str,"%.3f",system_geiger->get_cpm());
  serial_write_string(str);
}

void cmd_cpm30(char *line) {
  char str[16];
  sprintf(str,"%.3f",system_geiger->get_cpm30());
  serial_write_string(str);
}
void cmd_cpmdeadtime(char *line) {
  char str[16];
  sprintf(str,"%.3f",system_geiger->get_cpm_deadtime_compensated());
  serial_write_string(str);
}

void cmd_cpmvalid(char *line) {

  if (system_geiger->is_cpm_valid()) {
    serial_write_string("1");
  } else {
    serial_write_string("0");
  }
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

void cmd_guid(char *line) {
  signing_printGUID();
  serial_write_string("\r\n");
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

void cmd_logpause(char *line) {
  flashstorage_log_pause();
}

void cmd_logresume(char *line) {
  flashstorage_log_resume();
}

void cmd_logclear(char *line) {
  serial_write_string("Clearing flash log\r\n");
  flashstorage_log_clear();
  serial_write_string("Cleared\r\n");
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
    sprintf(err,"return code: %d\r\n",r);
    serial_write_string(err);
  }
  serial_write_string("Complete\r\n");
}

void cmd_keyvaldump(char *line) {

  char key[100];
  char val[100];

  char str[200];
  sprintf(str,"key=val\r\n");
  serial_write_string(str);
  for(int n=0;;n++) {
    flashstorage_keyval_by_idx(n,key,val);
    if(key[0] == 0) return;
    sprintf(str,"%s=%s\r\n",key,val);
    serial_write_string(str);
  }

}

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

void cmd_keyvalset(char *line) {
  serial_write_string("KEY=VAL\r\n");
  serial_write_string("#>");

  command_stack[command_stack_size] = serial_setkeyval_run;
  command_stack_size++;
}


void serial_setrtc_run(char *line) {

  uint32_t unixtime = 0;
  sscanf(line,"%"PRIu32"\r\n",&unixtime);
 
  realtime_set_unixtime(unixtime);
  command_stack_pop();
}

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

void cmd_batinfodisp(char *line) {
  for(int n=0;n<100;n++) {
    int bat = power_battery_level();
    char s[20];
    sprintf(s,"%d   ",bat);

    display_draw_text(0,0,s,0);
    delay_us(1000000);
  }
}

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
  register_cmd("GETCPM30"      ,cmd_cpm30);		// CPM
  register_cmd("GETCPMDEADTIME",cmd_cpmdeadtime);	// CPM
  register_cmd("CPMVALID"      ,cmd_cpmvalid);		// CPM
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
  // misc
  register_cmd("HELLO"         ,cmd_hello);
  register_cmd("LIST GAMES"    ,cmd_games);
}

void cmd_main_menu(char *line) {

  for(int n=0;n<command_list_size;n++) {
    if(strcmp(line,command_list[n]) == 0) {
      (*command_funcs[n])(line);
    }
  }
 
  if (command_stack_size == 1)
    serial_write_string("\r\n>");
}

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


char currentline[1024];
uint32_t currentline_position=0;

void serial_eventloop() {
  char buf[1024];

  uint32_t read_size = usart_rx(USART1,(uint8 *) buf,1024);
  if(read_size == 0) return;

  if(read_size > 1024) return; // something went wrong

  for(uint32_t n=0;n<read_size;n++) {
    
    // echo
    usart_putc(USART1, buf[n]);

    if((buf[n] == 13) || (buf[n] == 10)) {
      serial_write_string("\r\n");
      currentline[currentline_position]=0;
      serial_process_command(currentline);
      currentline_position=0;
    } else {
      currentline[currentline_position] = buf[n];
      currentline_position++;
    }
  }
}
