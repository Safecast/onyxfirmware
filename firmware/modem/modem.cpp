#include "modem.h"
#include "Geiger.h"
#include "flashstorage.h"
#include "log_read.h"
#include "dma.h"
#include "dac.h"
#include "display.h"
#include "timer.h"
#include "buzzer.h"

#define SAMPLES_PER_BIT  204
#define SAMPLES_PER_BYTE 8976
//#define SAMPLES_PER_BYTE 22440

//7350 - 4
uint8_t data_buffer_one [SAMPLES_PER_BIT] = { 128, 139, 151, 163, 174, 185, 195, 205, 214, 222, 230, 236, 242, 247, 251, 253, 255, 255, 255, 253, 251, 247, 242, 236, 230, 222, 214, 205, 195, 185, 174, 163, 151, 139, 128, 116, 104, 93, 81, 71, 60, 50, 41, 33, 25, 19, 13, 8, 4, 2, 0, 0, 0, 2, 4, 8, 13, 19, 25, 33, 41, 50, 60, 70, 81, 92, 104, 116, 127, 139, 151, 162, 174, 184, 195, 205, 214, 222, 230, 236, 242, 247, 251, 253, 255, 255, 255, 253, 251, 247, 242, 236, 230, 222, 214, 205, 195, 185, 174, 163, 151, 140, 128, 116, 104, 93, 81, 71, 60, 51, 41, 33, 26, 19, 13, 8, 4, 2, 0, 0, 0, 2, 4, 8, 13, 19, 25, 33, 41, 50, 60, 70, 81, 92, 104, 115, 127, 139, 151, 162, 173, 184, 195, 204, 213, 222, 229, 236, 242, 247, 251, 253, 255, 255, 255, 253, 251, 247, 242, 237, 230, 222, 214, 205, 195, 185, 174, 163, 151, 140, 128, 116, 104, 93, 82, 71, 60, 51, 42, 33, 26, 19, 13, 8, 4, 2, 0, 0, 0, 2, 4, 8, 13, 18, 25, 33, 41, 50, 60, 70, 81, 92, 104, 115 };

//4900 - 6
uint8_t data_buffer_zero[SAMPLES_PER_BIT] = { 128, 135, 143, 151, 159, 166, 174, 181, 188, 195, 201, 208, 214, 219, 225, 230, 234, 238, 242, 245, 248, 251, 253, 254, 255, 255, 255, 255, 254, 253, 251, 248, 245, 242, 238, 234, 230, 225, 219, 214, 208, 202, 195, 188, 181, 174, 166, 159, 151, 143, 135, 128, 120, 112, 104, 96, 89, 81, 74, 67, 60, 54, 47, 41, 36, 30, 25, 21, 17, 13, 10, 7, 4, 2, 1, 0, 0, 0, 0, 1, 2, 4, 7, 10, 13, 17, 21, 25, 30, 36, 41, 47, 53, 60, 67, 74, 81, 89, 96, 104, 112, 119, 127, 135, 143, 151, 159, 166, 174, 181, 188, 195, 201, 208, 214, 219, 225, 230, 234, 238, 242, 245, 248, 251, 252, 254, 255, 255, 255, 255, 254, 253, 251, 248, 245, 242, 238, 234, 230, 225, 220, 214, 208, 202, 195, 188, 181, 174, 167, 159, 151, 143, 136, 128, 120, 112, 104, 97, 89, 81, 74, 67, 60, 54, 47, 41, 36, 30, 26, 21, 17, 13, 10, 7, 4, 3, 1, 0, 0, 0, 0, 1, 2, 4, 7, 10, 13, 17, 21, 25, 30, 35, 41, 47, 53, 60, 67, 74, 81, 88, 96, 104, 111, 119 };


using namespace std;
uint8_t *transmit_data;
int     transmit_data_position = 0;
int     transmit_data_size     = 0;

volatile bool transmission_complete = false;

void modem_send_byte(int b);

uint8_t data_buffer[SAMPLES_PER_BYTE];

void write_to_buffer(int half,int b) {

  int offset = 0;
  if(half == 0) offset = 0;
  if(half == 1) offset = SAMPLES_PER_BYTE/2;

  int pos=0;

  b = b | 0x300;
  b = b << 1;
  // fill data buffer
  for(int bit=0;bit<11;bit++) {

    for(int j=0;j<2;j++)
    for(int i=0;i<SAMPLES_PER_BIT;i++) {
      int v=0;
      if((b & (1 << bit)) > 0) v = 60 + (data_buffer_one[i]/4);
                          else v = 60 + (data_buffer_zero[i]/4);
      data_buffer[offset+pos] = v;
      pos++;
    }
  }
}

void byte_transmission_complete() {
  buzzer_blocking_buzz(0.00001);

  dma_irq_cause event = dma_get_irq_cause(DMA2, DMA_CH4);

  if(event == DMA_TRANSFER_COMPLETE) {

    // write next byte in second half of data_buffer
    int b = transmit_data[transmit_data_position];
    transmit_data_position++;

    write_to_buffer(1,b);

  } else
  if(event == DMA_TRANSFER_HALF_COMPLETE) {
    // write next byte in first half of data_buffer
    int b = transmit_data[transmit_data_position];
    transmit_data_position++;

    write_to_buffer(0,b);
  }

  if(transmit_data_position > transmit_data_size) {
    transmission_complete=true;
    return;
  }
}

void modem_init() {

  // manual timer7 config
  TIMER7->regs.bas->PSC = 0x2;// was F
  TIMER7->regs.bas->EGR = 1; // set UG BIT!
  TIMER7->regs.bas->ARR = 24; // was 20
  TIMER7->regs.bas->CR2 &= (uint16_t)~((uint16_t)0x0070);
  TIMER7->regs.bas->CR2 |= 0x0020; // was 50
  TIMER7->regs.bas->DIER = TIMER7->regs.bas->DIER | (1 & (1 << 8)); // enable DMA update?


  DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);

  dac_init(DAC,DAC_CH2);

  // attach dac to timer7
  DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
  //DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
}

void modem_deinit() {
  dma_disable(DMA2, DMA_CH4);
}


void modem_send_init() {

  // prime buffer
  write_to_buffer(0,transmit_data[0]);
  write_to_buffer(1,transmit_data[1]);

  // need to use DAC DMA2 channel 4

  //dma_setup_transfer();
  dma_init(DMA2);

  dma_setup_transfer(DMA2, DMA_CH4, &DAC->regs->DHR8R2, DMA_SIZE_8BITS,
                     data_buffer, DMA_SIZE_8BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT | DMA_CIRC_MODE | DMA_HALF_TRNS | DMA_FROM_MEM));

  dma_attach_interrupt(DMA2, DMA_CH4, byte_transmission_complete);
  //uint32_t *dma2ch4int = (uint32_t *) 0x0000012C;
  //*dma2ch4int = (uint32_t) &byte_transmission_complete;


  //8. Setup the priority for the DMA transfer.
  //dma_set_priority(DMA2, DMA_CH4, DMA_PRIORITY_VERY_HIGH);

  // 9. Setup the number of bytes that we are going to transfer.
  dma_set_num_transfers(DMA2, DMA_CH4, SAMPLES_PER_BYTE);

  // 10. Enable DMA to start transmitting. When the transmission
  // finishes the event will be triggered and we will jump to
  // function DMAEvent.
  dma_enable(DMA2, DMA_CH4);
  //DAC->regs->CR = (DAC->regs->CR & 0xFFC3FFFF) | (1 << 20) | (1 << 18);
  DAC->regs->CR |= (1 << 16); // Enable
  DAC->regs->CR |= (1 << 28); // DMA Enable

  TIMER7->regs.bas->CR1 |= TIMER_CR1_CEN_BIT;
}


void modem_send(uint8_t *data,int size) {

  transmit_data = data;
  transmit_data_position = 0;
  transmission_complete  = false;
  transmit_data_size     = size;

  modem_send_init(); 
  
  for(int a=0;;a++) {
    if(transmission_complete) {
      return;
    }
  }
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

    //for(int n=0;n<1020;n++) {inputdata[n] = 'A'; inputdata[n+1]=0;}
    // send inputdata...
    modem_send((uint8_t *) inputdata,size);
  }

  modem_deinit();
  flashstorage_log_resume();
  system_geiger->enable_micout();
}
