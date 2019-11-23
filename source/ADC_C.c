/*
 * ADC_C.c
 * Adapted from https://www.electronicwings.com/
 * Created: 7/28/2016 4:56:54 PM
 *  Author: User
 */ 
#include <avr/io.h>						/* Include AVR std. library file */
#include <util/delay.h>
#include "ADC_H.h"

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	ADMUX = 0x40;
}

unsigned short ADC_Read(unsigned char channel) {
	int ADC_value;
	
	ADMUX = (0x40) | (channel & 0x07);/* set input channel to read */
	ADCSRA |= (1 << ADSC);	/* start conversion */

	while((ADCSRA & (1<<ADIF)) == 0);	/* monitor end of conversion interrupt flag */
	
	ADCSRA |= (1 << ADIF);	/* clear interrupt flag */

	ADC_value = (unsigned short)ADCL;	/* read lower byte */
	ADC_value = ADC_value + (unsigned short)ADCH * 256;/* read higher 2 bits, Multiply with weightage */

	return ADC_value;		/* return digital value */
}