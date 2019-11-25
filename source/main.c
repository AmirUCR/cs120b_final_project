
/*	Author: amohs002
 *  
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "ADC_H.h"
#include "max7219.h"
#ifndef _SIMULATE_
#include "simAVRHeader.h"
#endif

//----------------UTILITY-----------------------
#pragma region
volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;

	if (_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

unsigned char GetBit(unsigned char p, unsigned char w) {
	return (((0x01 << w) & p) != 0);
}

unsigned long int findGCD(unsigned long int a, unsigned long int b) {
	unsigned long int c;
	while(1) {
		c = a % b;
		if (c == 0) { return b; }
		a = b;
		b = c;
	}

	return 0;
}

void LED_Char_Array_Right_Rotate_By_One(unsigned char arr[], unsigned char n) { 
	unsigned char last = arr[n - 1];
	unsigned char i; 

	for (i = n - 1; i > 0; i--) {
		arr[i] = arr[i - 1];
	}

	arr[0] = last;
} 

#pragma endregion
//--------------END UTILITY---------------------

// TASK TEMPLATE
typedef struct _task {
	unsigned char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;

//---------------Shared variables-----------------------------
unsigned char lcd_updated_flag = 0;
unsigned char arrow_sequence_array[] = { 'L', 'R', 'L' };
const unsigned short numSequence = sizeof(arrow_sequence_array)/sizeof(unsigned char*);
//---------------End shared variables-------------------------

enum LEDMatrix_States { LEDMatrix_wait, LEDMatrix_shift };
int LEDMatrixSMTick(int state) {
	static unsigned char LED_right_arrow[] = { 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static unsigned char LED_left_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//	static unsigned char LED_up_arrow[] = { 0x20, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//	static unsigned char LED_right_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	static unsigned char i = 0;
	static unsigned char numCalls = 0;
	
	numCalls++;

	if (numCalls > 11) {
		if (i < numSequence) {
			i++;
		} else {
			i = 0;
		}

		numCalls = 1;
	}

	unsigned char arrow_direction = arrow_sequence_array[i];

	switch(state) {
		case LEDMatrix_wait:
			state = LEDMatrix_shift;
			break;

		case LEDMatrix_shift:
			state = LEDMatrix_shift;
			break;
	}

	switch (state) {
		case LEDMatrix_wait:
			break;

		case LEDMatrix_shift:
			switch (arrow_direction) {
				case 'L':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_left_arrow[j + 2]);
						max7219_digit(0, 5, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_left_arrow, 11);
					break;

				case 'R':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_right_arrow[j + 2]);
						max7219_digit(0, 5, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_right_arrow, 11);
					break;

				// case 'U':
				// 	max7219_clearDisplay(0);

				// 	for (unsigned char j = 0; j < 8; j++) {
				// 		max7219_digit(0, j, LED_right_arrow[j + 2]);
				// 		max7219_digit(0, 5, 0xFF);
				// 	}

				// 	LED_Char_Array_Right_Rotate_By_One(LED_right_arrow, 11);
				// 	break;
			}

			break;
	}


	return state;
}

enum Joystick_States { Joystick_wait, Joystick_check};
int JoystickSMTick(int state) {
	unsigned short ADC_Value;
	ADC_Value = ADC_Read(0);
	switch (state) {
		case Joystick_wait:
			state = Joystick_check;
			break;

		case Joystick_check:
			break;
		
		default:
			state = Joystick_wait;
			break;
		}

	switch(state) {
		case Joystick_wait:
			break;

		case Joystick_check:
			break; // LEFT OFF HERE
	}

	return state;
}

// enum WriteToLCD_States { writeToLCD_wait, writeToLCD_write };
// int WriteToLCDSMTick(int state) {
// 	switch(state) {
// 		case writeToLCD_wait:
// 			state = pause == 1 ? writeToLCD_wait : writeToLCD_write;
// 			break;

// 		case writeToLCD_write:
// 			state = pause == 1 ? writeToLCD_wait : writeToLCD_write;
// 			break;
// 	}

// 	switch(state) {
// 		case writeToLCD_wait:
// 			break;

// 		case writeToLCD_write:
// 			if (lcd_updated_flag) {
// 				LCD_DisplayString(1, keypad_output);
// 				lcd_updated_flag = 0;
// 			}
			
// 			break;
// 	}

// 	return state;
// }

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	//DDRB = 0xF0; PORTB = 0x0F;
    DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	unsigned char i;

	static task task1;
	task *tasks[] = { &task1 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = LEDMatrix_wait;
	task1.period = 500;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LEDMatrixSMTick;

	unsigned long GCD = tasks[0]->period;
 	for (i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}

	// TimerSet(500);
	// TimerOn();

	unsigned short adc_result = 0x0000;
	char buffer[20];

	TimerSet(GCD);
	TimerOn();

	ADC_init();
	LCD_init();
	max7219_init();


	unsigned char j = 0;

	unsigned short ADC_Value;

    while (1) {

		for (i = 0; i < numTasks; i++) {
			if (tasks[i]->elapsedTime == tasks[i]->period) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}

			tasks[i]->elapsedTime += GCD;
		}

		while(!TimerFlag);
		TimerFlag = 0;

		// ADC_Value = ADC_Read(0);
		// sprintf(buffer, "X=%d   ", ADC_Value);
		// LCD_DisplayString_xy(1, 0, buffer);
		
		// ADC_Value = ADC_Read(1);/* Read the status on Y-OUT pin using channel 0 */
		// sprintf(buffer, "Y=%d   ", ADC_Value);
		// LCD_DisplayString_xy(1, 8, buffer);

		// max7219_clearDisplay(0);

		// for (i = 0; i < 8; i++) {
		// 	max7219_digit(0, i, LED_right_arrow[i + 2]);
		// }

		// while(!TimerFlag);
		// TimerFlag = 0;

		// LED_Char_Array_Right_Rotate_By_One(LED_right_arrow, 11);


		//do test loop for every ic
		// for (unsigned char ic = 0; ic < MAX7219_ICNUMBER; ic++) {
		// 	for (i = 0; i < 8; i++) {
		// 		for (j = 0; j < 8; j++) {
		// 			max7219_digit(ic, i, LED_right_arrow[j + 2]);

					
		// 		}
		// 		//max7219_digit(ic, i, 0);
		// 		while(!TimerFlag);
		// 		TimerFlag = 0;
		// 	}
		// }
		
		

		// max7219_clearDisplay(0);

		//LED_Char_Array_Right_Rotate_By_One(LED_right_arrow, 11);

		
    }

    return 1;
} 