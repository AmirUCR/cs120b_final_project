/*
max7219 lib 0x02

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.

https://davidegironi.blogspot.com/
*/


#include <stdio.h>
#include <avr/io.h>

#include "max7219.h"


static unsigned char max7219_values[MAX7219_ICNUMBER][8];

/*
 * shift out a byte
 */
void max7219_shiftout(unsigned char bytedata) {
	unsigned char j = 0;

	//the shift is made in reverse order for this ic
	for(j = 8; j > 0; j--){
		unsigned char val = (bytedata & (1<<(j-1)))>>(j-1);

		MAX7219_CLKPORT &= ~(1 << MAX7219_CLKINPUT); //set the serial-clock pin low

		if (val) {
			MAX7219_DINPORT |= (1 << MAX7219_DININPUT);
		}
		else {
			MAX7219_DINPORT &= ~(1 << MAX7219_DININPUT);
		}

		MAX7219_CLKPORT |= (1 << MAX7219_CLKINPUT); //set the serial-clock pin high
	}
}


/*
 * Clear display
 */
void max7219_clearDisplay(unsigned char icnum) {
	if (icnum < 0 || icnum >= MAX7219_ICNUMBER) {
		return;
	}

	for(unsigned char i = 0; i < 8; i++) {
		max7219_digit(icnum, i, 0);
	}
}


/*
 * shift out data to a selected number
 */
void max7219_send(unsigned char icnum, unsigned char reg, unsigned char data) {
	unsigned char i = 0;

	if (icnum < MAX7219_ICNUMBER) {
		MAX7219_LOADPORT &= ~(1 << MAX7219_LOADINPUT); //load down

		//send no op to following ic
		for (i = icnum; i < (MAX7219_ICNUMBER - 1); i++) {
			max7219_shiftout(MAX7219_REGNOOP); //no op reg
			max7219_shiftout(MAX7219_REGNOOP); //no op data
		}

		//send info to current ic
		max7219_shiftout(reg); //send reg
		max7219_shiftout(data); //send data
		//send no op to previous ic

		for (i = 0; i < icnum; i++) {
			max7219_shiftout(MAX7219_REGNOOP); //no op reg
			max7219_shiftout(MAX7219_REGNOOP); //no op data
		}

		MAX7219_LOADPORT |= (1<<MAX7219_LOADINPUT); //load up
	}
}


/*
 * set shutdown for a selected ic
 */
void max7219_shutdown(unsigned char icnum, unsigned char value) {
	if (value == 0 || value == 1) {
		max7219_send(icnum, MAX7219_REGSHUTDOWN, value);
	}
}


/*
 * set brightness for a selected ic
 */
void max7219_intensity(unsigned char icnum, unsigned char value) {
	if (value < 16) {
		max7219_send(icnum, MAX7219_REGINTENSITY, value);
	}
}


/*
 * set test mode for a selected ic
 */
void max7219_test(unsigned char icnum, unsigned char value) {
	if (value == 0 || value == 1) {
		max7219_send(icnum, MAX7219_REGTEST, value);
	}
}


/*
 * set active outputs for a selected ic
 */
void max7219_scanlimit(unsigned char icnum, unsigned char value) {
	if (value < 8)
		max7219_send(icnum, MAX7219_REGSCANLIMIT, value);
}


/*
 * set decode mode for a selected ic
 */
void max7219_decode(unsigned char icnum, unsigned char value) {
	max7219_send(icnum, MAX7219_REGDECODE, value);
}

/*
 * set output 0 for a selected ic
 */
void max7219_digit0(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][0] = value;
	max7219_send(icnum, MAX7219_REGDIGIT0, value);
}


/*
 * set output 1 for a selected ic
 */
void max7219_digit1(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][1] = value;
	max7219_send(icnum, MAX7219_REGDIGIT1, value);
}


/*
 * set output 2 for a selected ic
 */
void max7219_digit2(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][2] = value;
	max7219_send(icnum, MAX7219_REGDIGIT2, value);
}


/*
 * set output 3 for a selected ic
 */
void max7219_digit3(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][3] = value;
	max7219_send(icnum, MAX7219_REGDIGIT3, value);
}

/*
 * set output 4 for a selected ic
 */
void max7219_digit4(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][4] = value;
	max7219_send(icnum, MAX7219_REGDIGIT4, value);
}


/*
 * set ouput 5 for a selected ic
 */
void max7219_digit5(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][5] = value;
	max7219_send(icnum, MAX7219_REGDIGIT5, value);
}

/*
 * set output 6 for a selected ic
 */
void max7219_digit6(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][6] = value;
	max7219_send(icnum, MAX7219_REGDIGIT6, value);
}


/*
 * set output 7 for a selected ic
 */
void max7219_digit7(unsigned char icnum, unsigned char value) {
	max7219_values[icnum][7] = value;
	max7219_send(icnum, MAX7219_REGDIGIT7, value);
}


/*
 * set output digit for a selected ic
 */
void max7219_digit(unsigned char icnum, unsigned char digit, unsigned char value) {
	switch(digit) {
		case 0:
			max7219_digit0(icnum, value);
			break;
		case 1:
			max7219_digit1(icnum, value);
			break;
		case 2:
			max7219_digit2(icnum, value);
			break;
		case 3:
			max7219_digit3(icnum, value);
			break;
		case 4:
			max7219_digit4(icnum, value);
			break;
		case 5:
			max7219_digit5(icnum, value);
			break;
		case 6:
			max7219_digit6(icnum, value);
			break;
		case 7:
			max7219_digit7(icnum, value);
			break;
	}
}


/*
 * get digit 0 value for a selected ic
 */
unsigned char max7219_getdigit0(unsigned char icnum) {
	return max7219_values[icnum][0];
}


/*
 * get digit 1 value for a selected ic
 */
unsigned char max7219_getdigit1(unsigned char icnum) {
	return max7219_values[icnum][1];
}


/*
 * get digit 2 value for a selected ic
 */
unsigned char max7219_getdigit2(unsigned char icnum) {
	return max7219_values[icnum][2];
}


/*
 * get digit 3 value for a selected ic
 */
unsigned char max7219_getdigit3(unsigned char icnum) {
	return max7219_values[icnum][3];
}


/*
 * get digit 4 value for a selected ic
 */
unsigned char max7219_getdigit4(unsigned char icnum) {
	return max7219_values[icnum][4];
}


/*
 * get digit 5 value for a selected ic
 */
unsigned char max7219_getdigit5(unsigned char icnum) {
	return max7219_values[icnum][5];
}


/*
 * get digit 6 value for a selected ic
 */
unsigned char max7219_getdigit6(unsigned char icnum) {
	return max7219_values[icnum][6];
}


/*
 * get digit 7 value for a selected ic
 */
unsigned char max7219_getdigit7(unsigned char icnum) {
	return max7219_values[icnum][7];
}


/*
 * get digit value for a selected ic
 */
unsigned char max7219_getdigit(unsigned char icnum, unsigned char digit) {
	unsigned char value = 0;
	switch(digit) {
		case 0:
			value = max7219_getdigit0(icnum);
			break;
		case 1:
			value = max7219_getdigit1(icnum);
			break;
		case 2:
			value = max7219_getdigit2(icnum);
			break;
		case 3:
			value = max7219_getdigit3(icnum);
			break;
		case 4:
			value = max7219_getdigit4(icnum);
			break;
		case 5:
			value = max7219_getdigit5(icnum);
			break;
		case 6:
			value = max7219_getdigit6(icnum);
			break;
		case 7:
			value = max7219_getdigit7(icnum);
			break;
	}
	return value;
}


/*
 * init the shift register
 */
void max7219_init() {
	//output
	MAX7219_DINDDR |= (1 << MAX7219_DININPUT);
	MAX7219_CLKDDR |= (1 << MAX7219_CLKINPUT);
	MAX7219_LOADDDR |= (1 << MAX7219_LOADINPUT);
	//low
	MAX7219_DINPORT &= ~(1 << MAX7219_DININPUT);
	MAX7219_CLKPORT &= ~(1 << MAX7219_CLKINPUT);
	MAX7219_LOADPORT &= ~(1 << MAX7219_LOADINPUT);

	//init ic
	for (unsigned char ic = 0; ic < MAX7219_ICNUMBER; ic++) {
		max7219_shutdown(ic, 1); //power on
		max7219_test(ic, 0); //test mode off
		max7219_decode(ic, 0); //use led matrix
		max7219_intensity(ic, 2); //intensity
		max7219_scanlimit(ic, 7); //set number of digit to drive
	}
}

