#include "libmaple.h"
#include "gpio.h"
#include "flashstorage.h"
#include "safecast_config.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "oled.h"

#define TX1 BOARD_USART1_TX_PIN
#define RX1 BOARD_USART1_RX_PIN

void serial_initialise() {
  const stm32_pin_info *txi = &PIN_MAP[TX1];
  const stm32_pin_info *rxi = &PIN_MAP[RX1];

  gpio_set_mode(txi->gpio_device, txi->gpio_bit, GPIO_AF_OUTPUT_OD); 
  gpio_set_mode(rxi->gpio_device, rxi->gpio_bit, GPIO_INPUT_FLOATING);

  if (txi->timer_device != NULL) {
      /* Turn off any PWM if there's a conflict on this GPIO bit. */
      timer_set_mode(txi->timer_device, txi->timer_channel, TIMER_DISABLED);
  }

  usart_init(USART1);
  usart_set_baud_rate(USART1, STM32_PCLK2, 115200); 
  usart_enable(USART1);
}

void serial_write_string(const char *str) {
  for(uint32_t n=0;str[n]!=0;n++) {
    usart_putc(USART1, str[n]);
  }
}

void serial_sendlog() {

  log_data_t *flash_log = (log_data_t *) flashstorage_log_get();

  uint32_t logsize = flashstorage_log_size()/sizeof(log_data_t);

  char lsize[20];
  sprintf(lsize,"Log size: %u\n",logsize);
  serial_write_string(lsize);
  //char s[1000];
  //sprintf(s,"%u",flashstorage_log_size());
  //serial_write_string(s);
  serial_write_string("{\n");
  for(uint32_t n=0;n<logsize;n++) {
      
    char strdata1[500];
    char strdata2[500];
    char strdata3[500];
    sprintf(strdata1,"{\"unixtime\":%u,\"cpm\":%u,",flash_log[n].time,flash_log[n].cpm);
    sprintf(strdata2,"\"accel_x_start\":%d,\"accel_y_start\":%d,\"accel_z_start\":%d,",flash_log[n].accel_x_start,flash_log[n].accel_y_start,flash_log[n].accel_z_start);
    sprintf(strdata3,"\"accel_x_end\":%d,\"accel_y_end\":%d,\"accel_z_end\":%d}\n",flash_log[n].accel_x_end,flash_log[n].accel_y_end,flash_log[n].accel_z_end);
    strdata1[499]=0;
    strdata2[499]=0;
    strdata3[499]=0;
    serial_write_string(strdata1);
    serial_write_string(strdata2);
    serial_write_string(strdata3);
  }
  serial_write_string("}");
}

void serial_writeprivatekey() {

  // TODO: Will add code to read from serial and write data to flash region here.

}

bool in_displaytest = false;

void serial_displaytest() {

  serial_write_string("DISPLAY REINIT MODE\r\n");
  serial_write_string("COMMAND IS: <CLOCK> <MULTIPLEX> <FUNCTIONSELECT> <VSL> <PHASELEN> <PRECHARGEVOLT> <PRECHARGEPERIOD> <VCOMH>\r\n");
  serial_write_string("e.g: 241 127 1 1 50 23 8 5\r\n");
  serial_write_string("#>");
  in_displaytest=true;
}

void serial_displaytest_run(char *line) {

  uint32_t clock, multiplex, functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh;

  sscanf(line,"%u %u %u %u %u %u %u %u",&clock,&multiplex,&functionselect,&vsl,&phaselen,&prechargevolt,&prechargeperiod,&vcomh);

  char outline[1024];
  sprintf(outline,"Received values: %u %u %u %u %u %u %u %u, calling reinit\r\n",clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh);
  serial_write_string(outline);

  oled_reinit(clock,multiplex,functionselect,vsl,phaselen,prechargevolt,prechargeperiod,vcomh);
}

char currentline[1024];
uint32_t currentline_position=0;

void serial_process_command(char *line) {

  if(in_displaytest) {
    serial_displaytest_run(line);
    in_displaytest = false;
  }

  serial_write_string("\r\n");
  if(strcmp(line,"HELLO") == 0) {
   serial_write_string("GREETINGS PROFESSOR FALKEN.\r\n");
  } else 
  if(strcmp(line,"LIST GAMES") == 0) {
    serial_write_string("I'M KIND OF BORED OF GAMES, TURNS OUT THE ONLY WAY TO WIN IS NOT TO PLAY...\r\n");
  } else 
  if(strcmp(line,"LOGXFER") == 0) {
    serial_sendlog();
  } else 
  if(strcmp(line,"WRITEKEY") == 0) {
    serial_writeprivatekey();
  } else
  if(strcmp(line,"DISPLAYTEST") == 0) {
    serial_displaytest();
  } else
  if(strcmp(line,"HELP") == 0) {
    serial_write_string("Available commands: HELP, LOGXFER, WRITEKEY, DISPLAYTEST, HELLO");
  }

  serial_write_string("\r\n>");
}

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
