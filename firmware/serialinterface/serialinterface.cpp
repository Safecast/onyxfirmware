#include "libmaple.h"
#include "gpio.h"
#include "flashstorage.h"
#include "safecast_config.h"
#include "usart.h"
#include <stdio.h>
#include "log.h"

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

/*
uint8 HardwareSerial::read(void) {
    return usart_getc(USART1);
}

uint32 HardwareSerial::available(void) {
    return usart_data_available(USART1);
}

void HardwareSerial::write(unsigned char ch) {
    usart_putc(USART1, ch);
}

void HardwareSerial::flush(void) {
    usart_reset_rx(USART1);
}
*/
