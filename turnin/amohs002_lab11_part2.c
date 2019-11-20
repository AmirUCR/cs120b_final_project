/*	Author: amohs002
 *  Partner(s) Name: Matthew Walsh
 *	Lab Section:
 *	Assignment: Lab #11  Exercise #2
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

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))
          
/*-------------------------------------------------------------------------*/

#define DATA_BUS PORTC		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTD	// port connected to pins 4 and 6 of LCD disp.
#define RS 6			// pin number of uC connected to pin 4 of LCD disp.
#define E 7			// pin number of uC connected to pin 6 of LCD disp.

/*-------------------------------------------------------------------------*/

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
unsigned char pause = 1;
unsigned char *string;
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

enum WriteToLCD_States { writeToLCD_wait, writeToLCD_write };
int WriteToLCDSMTick(int state) {
	static unsigned char lcd_write_index = 0;
	const unsigned char length = strlen(string) + 1;
	unsigned char *temp = malloc(length);

	if (!pause) {
		strncpy(temp, string + lcd_write_index, 16);
		if (lcd_write_index < length - 17) {
			lcd_write_index++;
		} else {
			lcd_write_index = 0;
		}
	}

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
			LCD_DisplayString(1, temp);
			break;
	}

	free(temp);

	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	unsigned char i;

	static task task1, task2;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = pauseButton_wait;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &pauseButtonSMTick;

	task2.state = writeToLCD_wait;
	task2.period = 500;
	task2.elapsedTime = task2.period;
	task2.TickFct = &WriteToLCDSMTick;

	unsigned long GCD = tasks[0]->period;
 	for (i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}

	TimerSet(GCD);
	TimerOn();

	LCD_init();

	string = "David is legen... wait for it... DARY!";

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
