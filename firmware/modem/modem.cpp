#include "modem.h"
#include "Geiger.h"
#include "flashstorage.h"
#include "log_read.h"

using namespace std;


void modem_init() {

}

void modem_send_one() {
  
}

void modem_send_zero() {
  
}

void modem_send(unsigned char*data,int size) {

  for(int n=0;n<size) {
  
    for(int b=0;b<8;b++) {
      if((data[n] & (1 << b)) > 0) modem_send_one ();
                              else modem_send_zero();
    }

  }
}

void modem_logxfer() {
  system_geiger->enable_micout();

  flashstorage_log_pause();

  int id_pos =0;
  char inputdata[1024];
  inputdata[0]=0;
  
  log_read_start();
  modem_init();
  for(int z=0;;z++) {

    // fill data
    int size = log_read_block(inputdata);
    if(size == 0) break;

    // send inputdata...
    modem_send(inputdata,size);
  }

  flashstorage_log_resume();
  system_geiger->disable_micout();
}
