/*	Author: amohs002
 *  Partner(s) Name: Matthew Walsh
 *	Lab Section:
 *	Assignment: Lab #11  Exercise #0
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

//----------------UTILITY-----------------------
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

//-----------------END UTILITY----------------------

// TASK TEMPLATE
typedef struct _task {
	unsigned char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;

//---------------Shared variables-----------------------------
unsigned char led0_output = 0x00;
unsigned char led1_output = 0x00;
unsigned char pause = 0;
//---------------End shared variables-------------------------

enum pauseButtonSM_States { pauseButton_wait, pauseButton_press, pauseButton_pressRest };

// Monitors button connected to PA0
// When button is pressed, shared variable "pause" is toggled
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

enum toggleLED0_States { toggleLED0_wait, toggleLED0_blink };
int toggleLED0SMTick(int state) {
	switch (state) {
		case toggleLED0_wait:
			state = (pause == 0) ? toggleLED0_blink: toggleLED0_wait;
			break;

		case toggleLED0_blink:
			state = pause ? toggleLED0_wait : toggleLED0_blink;
			break;

		default:
			state = toggleLED0_wait;
			break;
	}

	switch(state) {
		case toggleLED0_wait:
			break;

		case toggleLED0_blink:
			led0_output = (led0_output == 0x00) ? 0x01 : 0x00;
			break;
	}

	return state;
}

enum toggleLED1_States { toggleLED1_wait, toggleLED1_blink };
int toggleLED1SMTick(int state) {
	switch (state) {
		case toggleLED1_wait:
			state = !pause ? toggleLED1_blink : toggleLED1_wait;
			break;

		case toggleLED1_blink:
			state = pause ? toggleLED1_wait : toggleLED1_blink;
			break;

		default:
			state = toggleLED1_wait;
			break;
	}

	switch(state) {
		case toggleLED1_wait:
			break;

		case toggleLED1_blink:
			led1_output = (led1_output == 0x00) ? 0x01 : 0x00;
			break;
	}

	return state;
}

enum display_States { display_display };
int displaySMTick(int state) {
	unsigned char output;

	switch (state) {
		case display_display:
			state = display_display;
			break;

		default:
			state = display_display;
			break;
	}
	
	switch(state) {
		case display_display:
			output = led0_output | (led1_output << 1);
			break;
	}

	PORTB = output;

	return state;
}

unsigned long int findGCD(unsigned long int a, unsigned long int b) {
	unsigned long int c;
	while(1) {
		c = a % b;
		if (c == 0) {
			return b;
		}
		a = b;
		b = c;
	}

	return 0;
}

int main(void) {
    /* Insert DDR and PORT initializations */
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;

	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	unsigned short i;

	// Task 1 pauseButtonSM
	task1.state = pauseButton_wait;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &pauseButtonSMTick;

	// Task 2 toggleLED0SM
	task2.state = toggleLED0_wait;
	task2.period = 500;
	task2.elapsedTime = task2.period;
	task2.TickFct = &toggleLED0SMTick;

	// Task 3 toggleLED1SM
	task3.state = toggleLED1_wait;
	task3.period = 1000;
	task3.elapsedTime = task3.period;
	task3.TickFct = &toggleLED1SMTick;

	// Task 4 displaySM
	task4.state = display_display;
	task4.period = 10;
	task4.elapsedTime = task4.period;
	task4.TickFct = &displaySMTick;

 	unsigned long GCD = tasks[0]->period;
 	for (i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}

	TimerSet(GCD);
	TimerOn();
	
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
