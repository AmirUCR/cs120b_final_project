/*
max7219 lib 0x02

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef MAX7219_H_
#define MAX7219_H_

#include <avr/io.h>


//setup ports
#define MAX7219_DINDDR DDRD
#define MAX7219_DINPORT PORTD
#define MAX7219_DININPUT PD2

#define MAX7219_CLKDDR DDRD
#define MAX7219_CLKPORT PORTD
#define MAX7219_CLKINPUT PD3

#define MAX7219_LOADDDR DDRD
#define MAX7219_LOADPORT PORTD
#define MAX7219_LOADINPUT PD4

//setup number of chip attached to the board
#define MAX7219_ICNUMBER 2

//define registers
#define MAX7219_REGNOOP 0x00
#define MAX7219_REGDIGIT0 0x01
#define MAX7219_REGDIGIT1 0x02
#define MAX7219_REGDIGIT2 0x03
#define MAX7219_REGDIGIT3 0x04
#define MAX7219_REGDIGIT4 0x05
#define MAX7219_REGDIGIT5 0x06
#define MAX7219_REGDIGIT6 0x07
#define MAX7219_REGDIGIT7 0x08
#define MAX7219_REGDECODE 0x09
#define MAX7219_REGINTENSITY 0x0A
#define MAX7219_REGSCANLIMIT 0x0B
#define MAX7219_REGSHUTDOWN 0x0C
#define MAX7219_REGTEST 0x0F


//functions
void max7219_send(unsigned char icnum, unsigned char reg, unsigned char data);
void max7219_shutdown(unsigned char icnum, unsigned char value);
void max7219_intensity(unsigned char icnum, unsigned char value);
void max7219_test(unsigned char icnum, unsigned char value);
void max7219_scanlimit(unsigned char icnum, unsigned char value);
void max7219_clearDisplay(unsigned char icnum);

void max7219_decode(unsigned char icnum, unsigned char value);
void max7219_digit0(unsigned char icnum, unsigned char value);
void max7219_digit1(unsigned char icnum, unsigned char value);
void max7219_digit2(unsigned char icnum, unsigned char value);
void max7219_digit3(unsigned char icnum, unsigned char value);
void max7219_digit4(unsigned char icnum, unsigned char value);
void max7219_digit5(unsigned char icnum, unsigned char value);
void max7219_digit6(unsigned char icnum, unsigned char value);
void max7219_digit7(unsigned char icnum, unsigned char value);
void max7219_digit(unsigned char icnum, unsigned char digit, unsigned char value);

unsigned char max7219_getdigit0(unsigned char icnum);
unsigned char max7219_getdigit1(unsigned char icnum);
unsigned char max7219_getdigit2(unsigned char icnum);
unsigned char max7219_getdigit3(unsigned char icnum);
unsigned char max7219_getdigit4(unsigned char icnum);
unsigned char max7219_getdigit5(unsigned char icnum);
unsigned char max7219_getdigit6(unsigned char icnum);
unsigned char max7219_getdigit7(unsigned char icnum);
unsigned char max7219_getdigit(unsigned char icnum, unsigned char digit);
void max7219_init();

#endif
