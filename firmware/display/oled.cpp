
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gpio.h"
#include "oled.h"
#include "spi_aux.h"
#include "delay.h"
#include "safecast_config.h"
#include <stdint.h>


#define LCD_SPI      SPI2
#define LCD_DC_GPIO  31
#define LCD_CS_GPIO  33
#define LCD_PWR_GPIO 16
#define LCD_RES_GPIO 17

#define BPP 2 /* 2 bytes per pixel */

#define NOTE(x) Serial1.print(__FILE__); Serial1.print(":"); Serial1.print(__LINE__); Serial1.print(" "); Serial1.println(x);
#define RGB16(r, b, g) ((((r)<<11L)&0x1fL) | (((g)<<5L)&0x3fL) | (((b)<<0L)&0x1fL))

void
write_c(unsigned char out_command)
{	
    gpio_write_bit(PIN_MAP[LCD_DC_GPIO].gpio_device,
                   PIN_MAP[LCD_DC_GPIO].gpio_bit,
                   0);
    spi_aux_write(LCD_SPI,&out_command,1);
    delay_us(70);
}

void
write_d_stream(void *data, unsigned int count)
{
    gpio_write_bit(PIN_MAP[LCD_DC_GPIO].gpio_device,
                   PIN_MAP[LCD_DC_GPIO].gpio_bit,
                   1);
    spi_aux_write(LCD_SPI,(uint8 *)data, count);
    delay_us(10);
}

void write_d(unsigned char out_data)
{
    gpio_write_bit(PIN_MAP[LCD_DC_GPIO].gpio_device,
                   PIN_MAP[LCD_DC_GPIO].gpio_bit,
                   1);
    spi_aux_write(LCD_SPI,&out_data,1);
    delay_us(10);
}


void oled_platform_init(void) {
    spi_aux_enable_device(SPI2,true,SPI_18MHZ,SPI_FRAME_MSB,SPI_MODE_3);

}




//***************************************************
//--------------------------------------
#define Max_Column  0x7f            // 128-1
#define Max_Row     0x7f            // 128-1
#define Brightness  0x0E




//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Instruction Setting
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Column_Address(unsigned char a, unsigned char b)
{
    write_c(0x15);            // Set Column Address
    write_d(a);              //   Default => 0x00 (Start Address)
    write_d(b);              //   Default => 0x7F (End Address)
}


void Set_Row_Address(unsigned char a, unsigned char b)
{
    write_c(0x75);            // Set Row Address
    write_d(a);              //   Default => 0x00 (Start Address)
    write_d(b);              //   Default => 0x7F (End Address)
}

//=========================================================
// Reset GDRAM position
//=========================================================
static void Home(void)
{
    Set_Column_Address(0x00, Max_Column);
    Set_Row_Address(0x00, Max_Row);
}


static void Set_Remap_Format(unsigned char d)
{
    write_c(0xA0);            // Set Re-Map / Color Depth
    write_d(d);              //   Default => 0x40
                        //     Horizontal Address Increment
                        //     Column Address 0 Mapped to SEG0
                        //     Color Sequence: A => B => C
                        //     Scan from COM0 to COM[N-1]
                        //     Disable COM Split Odd Even
                        //     65,536 Colors
}


static void Set_Start_Line(unsigned char d)
{
    write_c(0xA1);            // Set Vertical Scroll by RAM
    write_d(d);              //   Default => 0x00
}


static void Set_Display_Offset(unsigned char d)
{
    write_c(0xA2);            // Set Vertical Scroll by Row
    write_d(d);              //   Default => 0x60
}


static void Set_Display_Mode(unsigned char d)
{
    write_c(0xA4|d);          // Set Display Mode
                        //   Default => 0xA6
                        //     0xA4 (0x00) => Entire Display Off, All Pixels Turn Off
                        //     0xA5 (0x01) => Entire Display On, All Pixels Turn On at GS Level 63
                        //     0xA6 (0x02) => Normal Display
                        //     0xA7 (0x03) => Inverse Display
}


static void Set_Function_Selection(unsigned char d)
{
    write_c(0xAB);            // Function Selection
    write_d(d);              //   Default => 0x01
                        //     Enable Internal VDD Regulator
                        //     Select 8-bit Parallel Interface
}




static void Set_Display_On()
{
    write_c(0xAF);
}



static void Set_Display_Off()
{
    write_c(0xAE);
}



static void Set_Phase_Length(unsigned char d)
{
    write_c(0xB1);            // Phase 1 (Reset) & Phase 2 (Pre-Charge) Period Adjustment
    write_d(d);              //   Default => 0x82 (8 Display Clocks [Phase 2] / 5 Display Clocks [Phase 1])
                        //     D[3:0] => Phase 1 Period in 5~31 Display Clocks
                        //     D[7:4] => Phase 2 Period in 3~15 Display Clocks
}


static void Set_Display_Enhancement(unsigned char d)
{
    write_c(0xB2);      // Display Enhancement
    write_d(d);         //   Default => 0x00 (Normal)
    write_d(0x00);
    write_d(0x00);
}


static void Set_Display_Clock(unsigned char d)
{
    write_c(0xB3);   // Set Display Clock Divider / Oscillator Frequency
    write_d(d);      //   Default => 0x00
                     //     A[3:0] => Display Clock Divider
                     //     A[7:4] => Oscillator Frequency
}


static void Set_VSL(unsigned char d)
{
    write_c(0xB4);   // Set Segment Low Voltage
    write_d(0xA0|d); //   Default => 0xA0
                     //     0xA0 (0x00) => Enable External VSL
                     //     0xA2 (0x02) => Enable Internal VSL (Kept VSL Pin N.C.)
    write_d(0xB5);
    write_d(0x55);
}


static void Set_GPIO(unsigned char d)
{
    write_c(0xB5);            // General Purpose IO
    write_d(d);              //   Default => 0x0A (GPIO Pins output Low Level.)
}


static void Set_Precharge_Period(unsigned char d)
{
    write_c(0xB6);            // Set Second Pre-Charge Period
    write_d(d);              //   Default => 0x08 (8 Display Clocks)
}


static void Set_Precharge_Voltage(unsigned char d)
{
    write_c(0xBB);            // Set Pre-Charge Voltage Level
    write_d(d);              //   Default => 0x17 (0.50*VCC)
}


static void Set_VCOMH(unsigned char d)
{
    write_c(0xBE);            // Set COM Deselect Voltage Level
    write_d(d);              //   Default => 0x05 (0.82*VCC)
}

//=========================================================
// Clear OLED GDRAM
//========================================================= 
void CLS(void) {
  Home();
  write_c(0x5C);    // Enable MCU to Read from RAM

  uint8_t c[256];
  for(uint32_t i=0;i<256;i++) c[i] = 0xff;

  for(uint32_t j=0;j<128;j++) {
    oled_draw_rect(j,127,128,1,c);
  }
}




static void Set_Contrast_Color(unsigned char a, unsigned char b, unsigned char c)
{
    write_c(0xC1);            // Set Contrast Current for Color A, B, C
    write_d(a);              //   Default => 0x8A (Color A)
    write_d(b);              //   Default => 0x51 (Color B)
    write_d(c);              //   Default => 0x8A (Color C)
}


static void Set_Master_Current(unsigned char d)
{
    write_c(0xC7);            // Master Contrast Current Control
    write_d(d);              //   Default => 0x0F (Maximum)
}


static void Set_Multiplex_Ratio(unsigned char d)
{
    write_c(0xCA);            // Set Multiplex Ratio
    write_d(d);              //   Default => 0x7F (1/128 Duty)
}


static void Set_Command_Lock(unsigned char d)
{
    write_c(0xFD);            // Set Command Lock
    write_d(d);              //   Default => 0x12
                        //     0x12 => Driver IC interface is unlocked from entering command.
                        //     0x16 => All Commands are locked except 0xFD.
                        //     0xB0 => Command 0xA2, 0xB1, 0xB3, 0xBB & 0xBE are inaccessible.
                        //     0xB1 => All Commands are accessible.
}




 //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Gray Scale Table Setting (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
static void Set_Gray_Scale_Table()
{
    write_c(0xB8);
    write_d(0x02);           // Gray Scale Level 1
    write_d(0x03);           // Gray Scale Level 2
    write_d(0x04);           // Gray Scale Level 3
    write_d(0x05);           // Gray Scale Level 4
    write_d(0x06);           // Gray Scale Level 5
    write_d(0x07);           // Gray Scale Level 6
    write_d(0x08);           // Gray Scale Level 7
    write_d(0x09);           // Gray Scale Level 8
    write_d(0x0A);           // Gray Scale Level 9
    write_d(0x0B);           // Gray Scale Level 10
    write_d(0x0C);           // Gray Scale Level 11
    write_d(0x0D);           // Gray Scale Level 12
    write_d(0x0E);           // Gray Scale Level 13
    write_d(0x0F);           // Gray Scale Level 14
    write_d(0x10);           // Gray Scale Level 15
    write_d(0x11);           // Gray Scale Level 16
    write_d(0x12);           // Gray Scale Level 17
    write_d(0x13);           // Gray Scale Level 18
    write_d(0x15);           // Gray Scale Level 19
    write_d(0x17);           // Gray Scale Level 20
    write_d(0x19);           // Gray Scale Level 21
    write_d(0x1B);           // Gray Scale Level 22
    write_d(0x1D);           // Gray Scale Level 23
    write_d(0x1F);           // Gray Scale Level 24
    write_d(0x21);           // Gray Scale Level 25
    write_d(0x23);           // Gray Scale Level 26
    write_d(0x25);           // Gray Scale Level 27
    write_d(0x27);           // Gray Scale Level 28
    write_d(0x2A);           // Gray Scale Level 29
    write_d(0x2D);           // Gray Scale Level 30
    write_d(0x30);           // Gray Scale Level 31
    write_d(0x33);           // Gray Scale Level 32
    write_d(0x36);           // Gray Scale Level 33
    write_d(0x39);           // Gray Scale Level 34
    write_d(0x3C);           // Gray Scale Level 35
    write_d(0x3F);           // Gray Scale Level 36
    write_d(0x42);           // Gray Scale Level 37
    write_d(0x45);           // Gray Scale Level 38
    write_d(0x48);           // Gray Scale Level 39
    write_d(0x4C);           // Gray Scale Level 40
    write_d(0x50);           // Gray Scale Level 41
    write_d(0x54);           // Gray Scale Level 42
    write_d(0x58);           // Gray Scale Level 43
    write_d(0x5C);           // Gray Scale Level 44
    write_d(0x60);           // Gray Scale Level 45
    write_d(0x64);           // Gray Scale Level 46
    write_d(0x68);           // Gray Scale Level 47
    write_d(0x6C);           // Gray Scale Level 48
    write_d(0x70);           // Gray Scale Level 49
    write_d(0x74);           // Gray Scale Level 50
    write_d(0x78);           // Gray Scale Level 51
    write_d(0x7D);           // Gray Scale Level 52
    write_d(0x82);           // Gray Scale Level 53
    write_d(0x87);           // Gray Scale Level 54
    write_d(0x8C);           // Gray Scale Level 55
    write_d(0x91);           // Gray Scale Level 56
    write_d(0x96);           // Gray Scale Level 57
    write_d(0x9B);           // Gray Scale Level 58
    write_d(0xA0);           // Gray Scale Level 59
    write_d(0xA5);           // Gray Scale Level 60
    write_d(0xAA);           // Gray Scale Level 61
    write_d(0xAF);           // Gray Scale Level 62
    write_d(0xB4);           // Gray Scale Level 63
}




//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Initialization
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void oled_init(void) {
    /* Set the data/command pin to be a GPIO */
    gpio_set_mode(PIN_MAP[LCD_DC_GPIO].gpio_device,
                  PIN_MAP[LCD_DC_GPIO].gpio_bit,
                  GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[LCD_DC_GPIO].gpio_device,
                   PIN_MAP[LCD_DC_GPIO].gpio_bit,
                   0);

    /* Set chip-select to be a GPIO */
    gpio_set_mode(PIN_MAP[LCD_CS_GPIO].gpio_device,
                  PIN_MAP[LCD_CS_GPIO].gpio_bit,
                  GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[LCD_CS_GPIO].gpio_device,
                   PIN_MAP[LCD_CS_GPIO].gpio_bit,
                   0);

    /* Turn the display on */
    gpio_set_mode(PIN_MAP[LCD_PWR_GPIO].gpio_device,
                  PIN_MAP[LCD_PWR_GPIO].gpio_bit,
                  GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[LCD_PWR_GPIO].gpio_device,
                   PIN_MAP[LCD_PWR_GPIO].gpio_bit,
                   1);

    delay_us(2000); /* Documentation says at least 1ms */

    /* Reset the display */
    gpio_set_mode(PIN_MAP[LCD_RES_GPIO].gpio_device,
                  PIN_MAP[LCD_RES_GPIO].gpio_bit,
                  GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[LCD_RES_GPIO].gpio_device,
                   PIN_MAP[LCD_RES_GPIO].gpio_bit,
                   0);
    delay_us(20); /* Documentation says at least 2us */
    gpio_write_bit(PIN_MAP[LCD_RES_GPIO].gpio_device,
		   PIN_MAP[LCD_RES_GPIO].gpio_bit,
		   1);

    //==============================

    Set_Command_Lock(0x12);         // Unlock Driver IC (0x12/0x16/0xB0/0xB1)
    Set_Command_Lock(0xB1);         // Unlock All Commands (0x12/0x16/0xB0/0xB1)
    Set_Display_Off();
    Set_Display_Clock(0xF1);        // Set Clock as 90 Frames/Sec
    Set_Multiplex_Ratio(0x7F);      // 1/128 Duty (0x0F~0x7F)// 7F
    Set_Display_Offset(0x00);       // Shift Mapping RAM Counter (0x00~0x7F)
    Set_Start_Line(0x00);           // Set Mapping RAM Display Start Line (0x00~0x7F)
    Set_Remap_Format(0x74);         // Set Horizontal Address Increment
                                    //     Column Address 0 Mapped to SEG0
                                    //     Color Sequence D[15:0]=[RRRRR:GGGGGG:BBBBB]
                                    //     Scan from COM127 to COM0
                                    //     Enable COM Split Odd Even
                                    //     65,536 Colors Mode (0x74)
                                    //     * 262,144 Colors Mode (0xB4)
    Set_GPIO(0x00);                 // Disable GPIO Pins Input
    
///M
    Set_Function_Selection(0x01);   // Disable Internal VDD Regulator
                                    // Select 8-bit Parallel Interface
    Set_VSL(0x01);                  // Enable External VSL
    //Set_Contrast_Color(0x80,0x80,0x80); // Set Contrast of Color A (Red)
    Set_Contrast_Color(0xC8,0x80,0xC8); // Set Contrast of Color A (Red)
                                    // Set Contrast of Color B (Green)
                                    // Set Contrast of Color C (Blue)
    Set_Master_Current(Brightness);     // Set Scale Factor of Segment Output Current Control
    Set_Gray_Scale_Table();         // Set Pulse Width for Gray Scale Table
///M
    Set_Phase_Length(0x32);         // Set Phase 1 as 5 Clocks & Phase 2 as 3 Clocks
 //   Set_Display_Enhancement(0xA4);      // Enhance Display Performance
    Set_Precharge_Voltage(0x17);        // Set Pre-Charge Voltage Level as 0.50*VCC
    Set_Precharge_Period(0x01);     // Set Second Pre-Charge Period as 1 Clock
    Set_VCOMH(0x05);                // Set Common Pins Deselect Voltage Level as 0.82*VCC
//M
    Set_Display_Mode(0x02);         // Normal Display Mode (0x00/0x01/0x02/0x03)

    CLS();                          // Clear Screen
 
    delay_us(1000);

    Set_Display_On();

}

void oled_deinit(void) {
    Set_Display_Off();

    // cuts power to the display
    gpio_write_bit(PIN_MAP[LCD_PWR_GPIO].gpio_device,
                   PIN_MAP[LCD_PWR_GPIO].gpio_bit,
                   0);

    delay_us(250000); // give it 250ms to discharge, hard wait; prevent issues with switch bounce
}

void oled_draw_rect(uint8 x, uint8 y, uint8 w, uint8 h, uint8 *data)
{
    Set_Column_Address(x, x+w-1);
    Set_Row_Address(y, y+h-1);
    write_c(0x5c);
    write_d_stream(data, w*h*BPP);
}

void oled_blank(void)
{
    Set_Display_Mode(0);
}

void oled_unblank(void)
{
    Set_Display_Mode(2);
}
