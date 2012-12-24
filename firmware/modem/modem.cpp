#include "modem.h"
#include "Geiger.h"
#include "flashstorage.h"
#include "log_read.h"
#include "dma.h"
#include "dac.h"
#include "display.h"
#include "timer.h"
#include "buzzer.h"

#define SAMPLES_PER_BIT  128
#define SAMPLES_PER_BYTE 2048

uint8_t data_buffer_one [SAMPLES_PER_BIT] = {
128, 176, 218, 246, 255, 246, 218, 177, 128, 79, 37, 9, 0, 9, 37, 78, 127, 176, 218, 246, 255, 246, 218, 177, 128, 79, 37, 9, 0, 9, 37, 78, 127, 176, 218, 246, 255, 246, 218, 177, 128, 79, 37, 9, 0, 9, 37, 78, 127, 176, 218, 246, 255, 246, 218, 177, 128, 79, 37, 9, 0, 9, 37, 78, 127, 176, 218, 246, 255, 246, 218, 177, 128, 79, 37, 10, 0, 9, 36, 78, 127, 176, 217, 245, 255, 246, 219, 177, 128, 79, 38, 10, 0, 9, 36, 78, 127, 176, 217, 245, 255, 246, 219, 177, 128, 79, 38, 10, 0, 9, 36, 78, 126, 175, 217, 245, 255, 246, 219, 178, 129, 80, 38, 10, 0, 9, 36, 77 };

uint8_t data_buffer_zero[SAMPLES_PER_BIT] = {
128, 218, 255, 218, 128, 37, 0, 37, 127, 218, 255, 218, 128, 37, 0, 37, 127, 218, 255, 218, 128, 37, 0, 37, 127, 218, 255, 218, 128, 37, 0, 37, 127, 218, 255, 218, 128, 37, 0, 36, 127, 217, 255, 219, 128, 38, 0, 36, 127, 217, 255, 219, 128, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 129, 38, 0, 36, 126, 217, 255, 219, 130, 38, 0, 36, 125, 216, 255, 220, 130, 39, 0, 35, 125, 216, 255, 220, 130, 39, 0, 35 };


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

//  int v=0;
//  for(int n=0;n<SAMPLES_PER_BIT;n++) {data_buffer_one [n] = sin((n*(2*3.141)/16))*((0xFF+1)/2)+128;}
//  for(int n=0;n<SAMPLES_PER_BIT;n++) {data_buffer_zero[n] = sin((n*(2*3.141)/8))*((0xFF+1)/2)+128; }

  int pos=0;

  // fill data buffer
  for(int bit=0;bit<8;bit++) {

    for(int i=0;i<SAMPLES_PER_BIT;i++) {
      int v=0;
      if((b & (1 << bit)) > 0) v =  data_buffer_one[i];
                          else v = data_buffer_zero[i];
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
  TIMER7->regs.bas->PSC = 0x1;// was F
  TIMER7->regs.bas->EGR = 1; // set UG BIT!
  TIMER7->regs.bas->ARR = 0xFF;
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

    // send inputdata...
    modem_send((uint8_t *) inputdata,size);
  }

  modem_deinit();
  flashstorage_log_resume();
  system_geiger->enable_micout();
}
