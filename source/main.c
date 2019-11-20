
/*	Author: amohs002
 *  Partner(s) Name: Matthew Walsh
 *	Lab Section:
 *	Assignment: Lab #11  Exercise #3
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#ifndef _SIMULATE_
#include "simAVRHeader.h"
#endif

#define KEYPADPORT PORTB
#define KEYPADPIN PINB

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
unsigned char pause = 0;
unsigned char *keypad_output = " ";
unsigned char lcd_updated_flag = 0;
//---------------End shared variables-------------------------

enum pauseButtonSM_States { pauseButton_wait, pauseButton_press, pauseButton_pressRest };

// Monitors button connected to PA0
// When button is pressed, shared variable "pause" is togged
int pauseButtonSMTick(int state) {
	unsigned char press = ~PINA & 0x01;

	switch(state) {
		case pauseButton_wait:
			if (!press) {
				state = pauseButton_wait;
			} else if (press) {
				state = pauseButton_press;
			}
			break;

		case pauseButton_press:
			state = pauseButton_pressRest;
			break;

		case pauseButton_pressRest:
			state = (press == 0x00) ? pauseButton_wait : pauseButton_pressRest;
			break;

		default:
			state = pauseButton_wait;
			break;
	}
	
	switch(state) {
		case pauseButton_wait:
			break;

		case pauseButton_press:
			pause = (pause == 0) ? 1 : 0;
			break;
			
		case pauseButton_pressRest:
			break;
	}

	return state;
}

enum keypadSM_States { getKeypadKey_get };
int GetKeypadKeySMTick(int state) {

	switch(state) {
		case getKeypadKey_get:
			state = getKeypadKey_get;
			break;

		default:
			state = getKeypadKey_get;
			break;
	}

	switch(state) {
		case getKeypadKey_get:

			// Check keys in column 1.
			KEYPADPORT = 0xEF;	// 1110 1111
			asm("nop");
			if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = "1"; lcd_updated_flag = 1; }
			else if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = "4"; lcd_updated_flag = 1; }
			else if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = "7"; lcd_updated_flag = 1; }
			else if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = "*"; lcd_updated_flag = 1; }
			else {
				// Check keys in column 2.
				KEYPADPORT = 0xDF;	// 1101 1111
				asm("nop");
				if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = "2"; lcd_updated_flag = 1; }
				else if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = "5"; lcd_updated_flag = 1; }
				else if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = "8"; lcd_updated_flag = 1; }
				else if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = "0"; lcd_updated_flag = 1; }
				else {
					// Check keys in column 3.
					KEYPADPORT = 0xBF;	// 1011 1111
					asm("nop");
					if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = "3"; lcd_updated_flag = 1; }
					else if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = "6"; lcd_updated_flag = 1; }
					else if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = "9"; lcd_updated_flag = 1; }
					else if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = "#"; lcd_updated_flag = 1; }
					else {
						// Check keys in column 4.
						KEYPADPORT = 0x7F;	// 0111 1111
						asm("nop");
						if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = "A"; lcd_updated_flag = 1; }
						else if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = "B"; lcd_updated_flag = 1; }
						else if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = "C"; lcd_updated_flag = 1; }
						else if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = "D"; lcd_updated_flag = 1; }
					}
				}
			}
			
			break;
	}

	return state;
}

enum WriteToLCD_States { writeToLCD_wait, writeToLCD_write };
int WriteToLCDSMTick(int state) {
	switch(state) {
		case writeToLCD_wait:
			state = pause == 1 ? writeToLCD_wait : writeToLCD_write;
			break;

		case writeToLCD_write:
			state = pause == 1 ? writeToLCD_wait : writeToLCD_write;
			break;
	}

	switch(state) {
		case writeToLCD_wait:
			break;

		case writeToLCD_write:
			if (lcd_updated_flag) {
				LCD_DisplayString(1, keypad_output);
				lcd_updated_flag = 0;
			}
			
			break;
	}

	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xF0; PORTB = 0x0F;
    DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	unsigned char i;

	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2, &task3 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = pauseButton_wait;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &pauseButtonSMTick;

	task2.state = writeToLCD_wait;
	task2.period = 50;
	task2.elapsedTime = task2.period;
	task2.TickFct = &WriteToLCDSMTick;

	task3.state = getKeypadKey_get;
	task3.period = 50;
	task3.elapsedTime = task3.period;
	task3.TickFct = &GetKeypadKeySMTick;

	unsigned long GCD = tasks[0]->period;
 	for (i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}

	TimerSet(GCD);
	TimerOn();

	LCD_init();

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
    }

    return 1;
}
