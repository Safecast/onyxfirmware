// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

//#include "wirish.h"
#include "safecast_config.h"
#include "mpr121.h"
#include "i2c.h"
#include "exti.h"
#include <stdio.h>

#define CAPTOUCH_ADDR 0x5A
#define CAPTOUCH_I2C I2C1
#define CAPTOUCH_GPIO 30

// "WASD" cluster as defined by physical arrangement of touch switches
#define W_KEY (1 << 3)
#define A_KEY (1 << 6)
#define S_KEY (1 << 4)
#define D_KEY (1 << 2)
#define Q_KEY (1 << 8)
#define E_KEY (1 << 0)

static struct i2c_dev *i2c;
static uint8 touchInit = 0;
static uint8 touchService = 0;
static uint16 touchList =  1 << 9 | 1 << 8 | 1 << 6 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 0;

static void
mpr121Write(uint8 addr, uint8 value)
{
    struct i2c_msg msg;
    uint8 bytes[2];
    int result;

    bytes[0] = addr;
    bytes[1] = value;

    msg.addr = CAPTOUCH_ADDR;
    msg.flags = 0;
    msg.length = sizeof(bytes);
    msg.xferred = 0;
    msg.data = bytes;

    result = i2c_master_xfer(i2c, &msg, 1, 100);
/*
    if (!result) {
        Serial1.print(addr, 16); Serial1.print(" -> "); Serial1.print(value); Serial1.print("\r\n");
    }
    else {
        Serial1.print(addr, 16); Serial1.print(" err "); Serial1.print(result); Serial1.print("\r\n");
    }
*/
    return;
}

static uint8
mpr121Read(uint8 addr)
{
    struct i2c_msg msgs[2];
    uint8 byte;

    byte = addr;
    msgs[0].addr   = msgs[1].addr   = CAPTOUCH_ADDR;
    msgs[0].length = msgs[1].length = sizeof(byte);
    msgs[0].data   = msgs[1].data   = &byte;
    msgs[0].flags = 0;
    msgs[1].flags = I2C_MSG_READ;
    i2c_master_xfer(i2c, msgs, 2, 100);
    return byte;
}

int last_key;

int cap_lastkey() {
  return last_key;
}

int cap_clearlastkey() {
  last_key = -1;
}


char c[100];
char *diag_data() {

 // int elec2h = mpr121Read(0x08);
 // int elec2l = mpr121Read(0x09);

 // int elec2_base = mpr121Read(0x20);
 // int bs = mpr121Read(TCH_STATL);
c[0]=0;
  //sprintf(c,"%d %d %d %d",elec2h,elec2l,elec2_base,bs);

  return c; 
}

static void cap_change(void) {
    /*
    if( digitalRead(MANUAL_WAKEUP_GPIO) == LOW ) {
        touchInit = 0;
        return; // don't initiate service if the unit is powered down
    }
    */
/*
  gpio_set_mode (GPIOB,9, GPIO_OUTPUT_PP);
  for(int n=0;n<100;n++) {
    gpio_toggle_bit(GPIOB,9);
    delay_us(1000);
  }
  gpio_write_bit(GPIOB,9,0);
*/


    if(touchInit) {
        touchService = 1; // flag that we're ready to be serviced

    }
    
    int board_state=0;
    board_state = mpr121Read(TCH_STATL);
    board_state |= mpr121Read(TCH_STATH) << 8;

    for (int key=0; key<16; key++) {
      if (board_state&(1<<key)) {
        last_key = key;
        return;
      }
    }


    touchService = 0;

    return;
}

void cap_init(void) {
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,1);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,1);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_OUTPUT_PP);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[9].gpio_bit,1);
    gpio_write_bit(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,1);
    delay_us(1000);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_INPUT_PD);
    gpio_set_mode(PIN_MAP[9].gpio_device,PIN_MAP[5].gpio_bit,GPIO_INPUT_PD);

    i2c = CAPTOUCH_I2C;
    i2c_init(i2c);
    i2c_master_enable(i2c, 0);
    //Serial1.print(".");

    mpr121Write(ELE_CFG, 0x00);   // disable electrodes for config
    delay_us(100);

    // Section A
    // This group controls filtering when data is > baseline.
    mpr121Write(MHD_R, 0x01);
    mpr121Write(NHD_R, 0x01);
    //    mpr121Write(NCL_R, 0x50);
    //    mpr121Write(FDL_R, 0x50);
    mpr121Write(NCL_R, 0x00);
    mpr121Write(FDL_R, 0x00);

    // Section B
    // This group controls filtering when data is < baseline.
    mpr121Write(MHD_F, 0x01);
    mpr121Write(NHD_F, 0x01);
    mpr121Write(NCL_F, 0xFF);
    //    mpr121Write(FDL_F, 0x52);
    mpr121Write(FDL_F, 0x02);

    // Section C
    // This group sets touch and release thresholds for each electrode
    mpr121Write(ELE0_T, TOU_THRESH);
    mpr121Write(ELE0_R, REL_THRESH);
    mpr121Write(ELE1_T, TOU_THRESH);
    mpr121Write(ELE1_R, REL_THRESH);
    mpr121Write(ELE2_T, TOU_THRESH);
    mpr121Write(ELE2_R, REL_THRESH);
    mpr121Write(ELE3_T, TOU_THRESH);
    mpr121Write(ELE3_R, REL_THRESH);
    mpr121Write(ELE4_T, TOU_THRESH);
    mpr121Write(ELE4_R, REL_THRESH);
    mpr121Write(ELE5_T, TOU_THRESH);
    mpr121Write(ELE5_R, REL_THRESH);
    mpr121Write(ELE6_T, TOU_THRESH);
    mpr121Write(ELE6_R, REL_THRESH);
    mpr121Write(ELE7_T, TOU_THRESH);
    mpr121Write(ELE7_R, REL_THRESH);
    mpr121Write(ELE8_T, TOU_THRESH);
    mpr121Write(ELE8_R, REL_THRESH);
    mpr121Write(ELE9_T, TOU_THRESH);
    mpr121Write(ELE9_R, REL_THRESH);
    mpr121Write(ELE10_T, TOU_THRESH);
    mpr121Write(ELE10_R, REL_THRESH);
    mpr121Write(ELE11_T, TOU_THRESH);
    mpr121Write(ELE11_R, REL_THRESH);

    // Section D
    // Set the Filter Configuration
    // Set ESI2
    mpr121Write(FIL_CFG, 0x03);  // set CDT to 32us, ESI (sampling interval) to 8 ms
    mpr121Write(AFE_CONF, 0x3F); // 6 samples, 63uA <-- will be overridden by auto-config i think

    // Section F
    mpr121Write(ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V
    mpr121Write(ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    mpr121Write(ATO_CFGT, 0xB5);    // Target = 0.9*USL = 0xB5 @3.3V

    // mpr121Write(ATO_CFGU, 0xC0);  // VSL = (Vdd-0.7)/vdd*256 = 0xC0 @2.8V
    // mpr121Write(ATO_CFGL, 0x7D);  // LSL = 0.65*USL = 0x7D @2.8V
    // mpr121Write(ATO_CFGT, 0xB2);    // Target = 0.9*USL = 0xB2 @2.8V

    //    mpr121Write(ATO_CFGU, 0x9C);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @1.8V
    //    mpr121Write(ATO_CFGL, 0x65);  // LSL = 0.65*USL = 0x82 @1.8V
    //    mpr121Write(ATO_CFGT, 0x8C);    // Target = 0.9*USL = 0xB5 @1.8V

    // Enable Auto Config and auto Reconfig
    mpr121Write(ATO_CFG0, 0x3B); // must match AFE_CONF setting of 6 samples, retry enabled

    delay_us(100);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);   // Enables all 12 Electrodes
    //mpr121Write(ELE_CFG, 0x06);     // Enable first 6 electrodes
    delay_us(100);

//    pinMode(CAPTOUCH_GPIO, INPUT);
    gpio_set_mode(PIN_MAP[CAPTOUCH_GPIO].gpio_device,PIN_MAP[CAPTOUCH_GPIO].gpio_bit,GPIO_INPUT_PU);
    
    exti_attach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit), gpio_exti_port(PIN_MAP[CAPTOUCH_GPIO].gpio_device), cap_change, EXTI_FALLING);
//    exti_attach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit), gpio_exti_port(PIN_MAP[CAPTOUCH_GPIO].gpio_device), cap_change, EXTI_RISING);

//    attachInterrupt(CAPTOUCH_GPIO, cap_change, FALLING);
//    attachInterrupt(CAPTOUCH_GPIO, cap_change, RISING);

    mpr121Read(TCH_STATL);
    mpr121Read(TCH_STATH);

    touchInit = 1;
    return;

}

void
cap_deinit(void)
{
 //   detachInterrupt(CAPTOUCH_GPIO);
    exti_detach_interrupt((afio_exti_num)(PIN_MAP[CAPTOUCH_GPIO].gpio_bit));


    // Disable MPR121 scanning, in case the chip is on
    mpr121Write(ELE_CFG, 0x00);

    return;
}


int
cap_setkeydown(void (*new_keydown)(int key))
{
    return 0;
}

int
cap_setkeyup(void (*new_keyup)(int key))
{
    return 0;
}


/*
void
cap_debug(void)
{
    uint8 bytes[2];
    uint16 temp;
    int i;

    bytes[0] = mpr121Read(TCH_STATL);
    temp = mpr121Read(FIL_CFG);
    Serial1.print( temp, 16 );
    Serial1.println( "\r" );

    Serial1.print("AFE_CONF: ");
    temp = mpr121Read(AFE_CONF);
    Serial1.print( temp, 16 );
    Serial1.println( "\r" );

    for( i = 0; i < 13; i++ ) {
        temp = 0;
        if( touchList & (1 << i) ) {
            temp = mpr121Read(ELE0_T + i * 2);
            Serial1.print( "   TT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );

            temp = mpr121Read(ELE0_R + i * 2);
            Serial1.print( "   RT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );
            
            temp = mpr121Read(0x5F + i);
            Serial1.print( "   CUR" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );
            
            temp = mpr121Read(0x1e + i);
            Serial1.print( "   ELEBASE" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp << 2, 16 );

            temp |= mpr121Read(0x4 + i);
            temp |= (mpr121Read(0x5 + i) & 0x3) << 8;
            Serial1.print( "   ELEFILT" );
            Serial1.print( i );
            Serial1.print( " " );
            Serial1.print( temp, 16 );

            Serial1.println( "\r" );
            
            temp = 0;
        }
    }
    Serial1.print( "CHG TIME: ");
    for( i = 0; i < 5; i++ ) {
        temp = mpr121Read(0x6c + i);
        Serial1.print( temp, 16 );
        Serial1.print( " " );
    }
    Serial1.println( "\r" );
    Serial1.println( "\r" );

    delay(500);
}
*/
