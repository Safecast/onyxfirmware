#include "libmaple.h"
#include "gpio.h"
#include "flashstorage.h"
#include "safecast_config.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "oled.h"
#include "display.h"
#include <limits.h>
#include "dac.h"

extern "C" {
  static void signing_test();
  static int signing_isKeyValid();
  static void signing_printPubKey();
  static void signing_printGUID();
  static void signing_hashLog();
}

extern uint8_t _binary___binary_data_private_key_data_start;
extern uint8_t _binary___binary_data_private_key_data_size;

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
    sprintf(strdata3,"\"accel_x_end\":%d,\"accel_y_end\":%d,\"accel_z_end\":%d}\r\n",flash_log[n].accel_x_end,flash_log[n].accel_y_end,flash_log[n].accel_z_end);
    strdata1[499]=0;
    strdata2[499]=0;
    strdata3[499]=0;
    serial_write_string(strdata1);
    serial_write_string(strdata2);
    serial_write_string(strdata3);
  }
  serial_write_string("}");
}

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
      sprintf(stemp,"%x : ",source_data+n);
      serial_write_string(stemp);
    }

    if(n == (4*1024)) {
      serial_write_string("\r\nShowing following 2k region, to confirm code not overwritten:\r\n");
      sprintf(stemp,"%x : ",source_data+n);
      serial_write_string(stemp);
    }

    sprintf(stemp,"%02x ",source_data_programmable[n]);
    serial_write_string(stemp);

  }
  serial_write_string("\r\n");

}

void serial_signing_test() {
  signing_test();
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

bool in_displayparams = false;
bool in_setdevicetag = false;

void serial_displayparams() {

  serial_write_string("DISPLAY REINIT MODE\r\n");
  serial_write_string("COMMAND IS: <CLOCK> <MULTIPLEX> <FUNCTIONSELECT> <VSL> <PHASELEN> <PRECHARGEVOLT> <PRECHARGEPERIOD> <VCOMH>\r\n");
  serial_write_string("e.g: 241 127 1 1 50 23 8 5\r\n");
  serial_write_string("#>");
  in_displayparams=true;
}

void serial_setdevicetag() {

  serial_write_string("SETDEVICETAG:\r\n");
  serial_write_string("#>");
  in_setdevicetag=true;
}

void serial_setdevicetag_run(char *line) {

  char devicetag[100];

  sscanf(line,"%s\r\n",&devicetag);

  flashstorage_keyval_set("DEVICETAG",devicetag);
}

void serial_displayparams_run(char *line) {

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

  if(in_displayparams) {
    serial_displayparams_run(line);
    in_displayparams = false;
  }

  if(in_setdevicetag) {
    serial_setdevicetag_run(line);
    in_setdevicetag = false;
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
  if(strcmp(line,"DISPLAYPARAMS") == 0) {
    serial_displayparams();
  } else
  if(strcmp(line,"HELP") == 0) {
    serial_write_string("Available commands: HELP, LOGXFER, WRITEKEY, DISPLAYTEST, HELLO");
  } else 
  if(strcmp(line,"DISPLAYTEST") == 0) {
    display_test();
  } else
  if(strcmp(line,"LOGTEST") == 0) {
    char stemp[100];


    sprintf(stemp,"Raw log data\r\n");
    serial_write_string(stemp);

    uint8_t *flash_log = flashstorage_log_get();
    for(int n=0;n<1024;n++) {
      sprintf(stemp,"%u ",flash_log[n]);
      serial_write_string(stemp);
      if(n%64 == 0) serial_write_string("\r\n");
    }
    serial_write_string("\r\n");

    log_data_t data;
    data.time = 0;
    data.cpm  = 1;
    data.accel_x_start = 2;
    data.accel_y_start = 3;
    data.accel_z_start = 4;
    data.accel_x_end   = 5;
    data.accel_y_end   = 6;
    data.accel_z_end   = 7;
    data.log_type      = UINT_MAX;
    sprintf(stemp,"Log size: %u\r\n",flashstorage_log_size());
    serial_write_string(stemp);

    sprintf(stemp,"Writing test log entry of size: %u\r\n",sizeof(log_data_t));
    serial_write_string(stemp);

    flashstorage_log_pushback((uint8 *) &data,sizeof(log_data_t));

    sprintf(stemp,"Log size: %u\r\n",flashstorage_log_size());
    serial_write_string(stemp);
  } else 
  if(strcmp(line,"VERSION") == 0) {
    char stemp[50];
    sprintf(stemp,"Version: %s\r\n",OS100VERSION);
    serial_write_string(stemp);
  } else 
  if(strcmp(line,"GETDEVICETAG") == 0) {
   const char *devicetag = flashstorage_keyval_get("DEVICETAG");
   if(devicetag != 0) {
     char stemp[100];
     sprintf(stemp,"Devicetag: %s\r\n",devicetag);
     serial_write_string(stemp);
   } else {
     serial_write_string("No device tag set");
   }
  } else
  if(strcmp(line,"SETDEVICETAG") == 0) {
    serial_setdevicetag();
  } else 
  if(strcmp(line,"READPRIVATEKEY") == 0) {
    serial_readprivatekey();
  } else
  if(strcmp(line,"WRITEPRIVATEKEY") == 0) {
    serial_writeprivatekey();
  } else 
  if(strcmp(line,"MAGREAD") == 0) {
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
  } else
  if(strcmp(line,"WRITEDAC") == 0) {
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
  } else
  if(strcmp(line,"TESTHP") == 0) {
    gpio_set_mode (PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit, GPIO_OUTPUT_PP);  // HP_COMBINED
    for(int n=0;n<100000;n++) {
      gpio_write_bit(PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit,1);
      delay_us(100);
      gpio_write_bit(PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit,0);
      delay_us(100);
    }
  } else 
  if(strcmp(line,"READADC") == 0) {

    adc_init(PIN_MAP[12].adc_device); // all on ADC1

    adc_set_extsel(PIN_MAP[12].adc_device, ADC_SWSTART);
    adc_set_exttrig(PIN_MAP[12].adc_device, true);

    adc_enable(PIN_MAP[12].adc_device);
    adc_calibrate(PIN_MAP[12].adc_device);
    adc_set_sample_rate(PIN_MAP[12].adc_device, ADC_SMPR_55_5);

    gpio_set_mode (PIN_MAP[12].gpio_device,PIN_MAP[12].gpio_bit, GPIO_INPUT_ANALOG);
    gpio_set_mode (PIN_MAP[19].gpio_device,PIN_MAP[19].gpio_bit, GPIO_INPUT_ANALOG);
    gpio_set_mode (PIN_MAP[20].gpio_device,PIN_MAP[20].gpio_bit, GPIO_INPUT_ANALOG);

    int n=0;
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
  } else
  if(strcmp(line,"SETMICREVERSE") == 0) {
    gpio_set_mode (PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit, GPIO_OUTPUT_PP);  // MICREVERSE
    gpio_set_mode (PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit, GPIO_OUTPUT_PP);  // MICIPHONE
    gpio_write_bit(PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,1); // MICREVERSE
    gpio_write_bit(PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,0); // MICIPHONE
    serial_write_string("Set MICREVERSE to 1, MICIPHONE to 0\r\n");
  } else 
  if(strcmp(line,"SETMICIPHONE") == 0) {
    gpio_set_mode (PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit, GPIO_OUTPUT_PP);  // MICREVERSE
    gpio_set_mode (PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit, GPIO_OUTPUT_PP);  // MICIPHONE
    gpio_write_bit(PIN_MAP[36].gpio_device,PIN_MAP[36].gpio_bit,0); // MICREVERSE
    gpio_write_bit(PIN_MAP[35].gpio_device,PIN_MAP[35].gpio_bit,1); // MICIPHONE
    serial_write_string("Set MICREVERSE to 0, MICIPHONE to 1\r\n");
  } else
  if(strcmp(line,"TESTSIGN") == 0) {
    serial_signing_test();
  } else 
  if(strcmp(line,"PUBKEY") == 0) {
    signing_printPubKey();
    serial_write_string("\n\r");
  } else 
  if(strcmp(line,"GUID") == 0) {
    signing_printGUID();
    serial_write_string("\n\r");
  } else 
  if(strcmp(line,"KEYVALID") == 0) {
    if( signing_isKeyValid() == 1 )
      serial_write_string("VALID KEY\r\n");
    else
      serial_write_string("IMPROPER OR UNINITIALIZED KEY\r\n");
  } else 
  if(strcmp(line,"LOGSIG") == 0) {
    signing_hashLog();
    serial_write_string("\n\r");
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
