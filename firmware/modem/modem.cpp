#include "modem.h"
#include "Geiger.h"
#include "flashstorage.h"
#include "log_read.h"
#include "dma.h"
#include "dac.h"
#include "display.h"
#include "timer.h"
#include "buzzer.h"

#define SAMPLES_PER_BIT  32
#define SAMPLES_PER_BYTE 256
//#define SAMPLES_PER_BIT  256
//#define SAMPLES_PER_BYTE 2048

using namespace std;
uint8_t *transmit_data;
int     transmit_data_position = 0;
int     transmit_data_size     = 0;

volatile bool transmission_complete = false;

void modem_send_byte(int b);

void byte_transmission_complete() {
  buzzer_blocking_buzz(1);
  display_draw_text(0,0,"transint",0);
  transmission_complete=true;
  return;

  transmit_data_position++;
  if(transmit_data_position > transmit_data_size) {
    transmission_complete=true;
    display_draw_text(0,0,"transcomp",0);
    return;
  }

 // modem_send_byte(transmit_data[transmit_data_position]);
  display_draw_number(0,128-16-16,transmit_data_position,15,0);
}

void modem_init() {

  // initialise timer
  //timer_pause(TIMER7);
  //TODO: fix this so it uses both prescaler and reload...
//  timer_set_prescaler(TIMER7,100);
//  timer_set_reload(TIMER7,100);


  // manual timer7 config
  TIMER7->regs.bas->PSC = 0xF;
  TIMER7->regs.bas->EGR = 1; // set UG BIT!
  TIMER7->regs.bas->ARR = 0xFF;
  TIMER7->regs.bas->CR2 &= (uint16_t)~((uint16_t)0x0070);
  TIMER7->regs.bas->CR2 |= 0x0020; // was 50
  TIMER7->regs.bas->DIER = TIMER7->regs.bas->DIER | (1 & (1 << 8)); // enable DMA update?


  // setup interrupt on channel 4
 // timer_set_mode(TIMER7,TIMER_CH4,TIMER_OUTPUT_COMPARE);
 // timer_set_compare(TIMER7,TIMER_CH4,10000);

 // timer_generate_update(TIMER7); // refresh timer count, prescale, overflow


  DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);

  dac_init(DAC,DAC_CH2);

  // attach dac to timer7
  DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
  //DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
}

void modem_deinit() {
  dma_disable(DMA2, DMA_CH4);
}

void modem_send_byte(int b) {
  uint8_t data_buffer_one [SAMPLES_PER_BIT];
  uint8_t data_buffer_zero[SAMPLES_PER_BIT];

  int v=0;
  for(int n=0;n<SAMPLES_PER_BIT;n++) {data_buffer_one [n] = v; if(v==0) v=255; else v=0;}
  for(int n=0;n<SAMPLES_PER_BIT;n++) {data_buffer_zero[n] = v; if(v==0) v=128; else v=0;}

  uint8_t data_buffer[SAMPLES_PER_BYTE];

  int pos=0;

  // fill data buffer
  for(int bit=0;bit<8;bit++) {

    for(int i=0;i<SAMPLES_PER_BIT;i++) {
      int v=0;
      if((b & (1 << bit)) > 0) v =  data_buffer_one[i];
                          else v = data_buffer_zero[i];
      data_buffer[pos] = v;
      pos++;
    }
  }


  // need to use DAC DMA2 channel 4

  //dma_setup_transfer();
  display_draw_text(0,0,"preinit",0);
  dma_init(DMA2);
  display_draw_text(0,0,"presetup",0);

  dma_setup_transfer(DMA2, DMA_CH4, &DAC->regs->DHR8R2, DMA_SIZE_8BITS,
                     data_buffer, DMA_SIZE_8BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT | DMA_CIRC_MODE | DMA_HALF_TRNS | DMA_FROM_MEM));

  display_draw_text(0,0,"preatt  ",0);
  dma_attach_interrupt(DMA2, DMA_CH4, byte_transmission_complete);
  //uint32_t *dma2ch4int = (uint32_t *) 0x0000012C;
  //*dma2ch4int = (uint32_t) &byte_transmission_complete;


  display_draw_text(0,0,"prepri",0);
  //8. Setup the priority for the DMA transfer.
  //dma_set_priority(DMA2, DMA_CH4, DMA_PRIORITY_VERY_HIGH);

  //display_draw_text(0,0,"prenum",0);
  // 9. Setup the number of bytes that we are going to transfer.
  dma_set_num_transfers(DMA2, DMA_CH4, SAMPLES_PER_BYTE);

  // 10. Enable DMA to start transmitting. When the transmission
  // finishes the event will be triggered and we will jump to
  // function DMAEvent.
  display_draw_text(0,0,"preen",0);
  dma_enable(DMA2, DMA_CH4);
  //DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
  DAC->regs->CR |= (1 << 16); // Enable
  DAC->regs->CR |= (1 << 28); // DMA Enable
  //timer_resume(TIMER7);
  TIMER7->regs.bas->CR1 |= TIMER_CR1_CEN_BIT;
  display_draw_text(0,0,"posten  ",0);
}


void modem_send(uint8_t *data,int size) {

  transmit_data = data;
  transmit_data_position = 0;
  transmission_complete  = false;
  transmit_data_size     = size;

  display_draw_number(0,16,data[0],5,0);
  modem_send_byte(data[0]); 
  
  for(int a=0;;a++) {
    if(transmission_complete) {
      display_draw_text(0,0,"exit 1",0);
      return;
    }

    display_draw_number(0,16,a,5,0);
    //display_draw_number(0,32,TIMER7->regs.bas->CNT,10,0);
  }
  display_draw_text(0,0,"exit 2",0);
}

void modem_logxfer() {
  system_geiger->disable_micout();

  modem_init();

  flashstorage_log_pause();

  int id_pos =0;
  char inputdata[1024];
  inputdata[0]=0;
  
  log_read_start();
  for(int z=0;;z++) {

    // fill data
    int size = log_read_block(inputdata);
    if(size == 0) break;

    // send inputdata...
    modem_send((uint8_t *) inputdata,size);

    display_draw_text(0,0,"xfloop",0);
    display_draw_number(0,128-16-16-16,z,15,0);
    break;
  }

  modem_deinit();
  flashstorage_log_resume();
  system_geiger->enable_micout();
}
