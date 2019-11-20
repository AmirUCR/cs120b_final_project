/*	Author: amohs002
 *  Partner(s) Name: Matthew Walsh
 *	Lab Section:
 *	Assignment: Lab #11  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#define KEYPADPORT PORTC
#define KEYPADPIN PINC

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

unsigned char GetBit(unsigned char p, unsigned char w) {
	return (((0x01 << w) & p) != 0);
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
unsigned char keypad_output = 0x00;
//---------------End shared variables-------------------------

// Returns '\0' if no key pressed, else returns char '1', '2' , ..., '9', 'A', ...
// If multiple keys pressed, returns leftmost-topmost one
// Keypad must be connected to PORTC

/* Keypad arrangement

		PC4	PC5	PC6	PC7
	col 1	2	3	4
row
PC0	 1  1 | 2 | 3 | A
PC1  2  4 | 5 | 6 | B
PC2  3  7 | 8 | 9 | C
PC3  4  * | 0 | # | D
*/

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
			if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = '1'; break; }
			if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = '4'; break; }
			if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = '7'; break; }
			if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = '*'; break; }

			// Check keys in column 2.
			KEYPADPORT = 0xDF;	// 1101 1111
			asm("nop");
			if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = '2'; break; }
			if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = '5'; break; }
			if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = '8'; break; }
			if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = '0'; break; }

			// Check keys in column 3.
			KEYPADPORT = 0xBF;	// 1011 1111
			asm("nop");
			if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = '3'; break; }
			if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = '6'; break; }
			if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = '9'; break; }
			if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = '#'; break; }

			// Check keys in column 4.
			KEYPADPORT = 0x7F;	// 0111 1111
			asm("nop");
			if (GetBit(KEYPADPIN, 0) == 0) { keypad_output = 'A'; break; }
			if (GetBit(KEYPADPIN, 1) == 0) { keypad_output = 'B'; break; }
			if (GetBit(KEYPADPIN, 2) == 0) { keypad_output = 'C'; break; }
			if (GetBit(KEYPADPIN, 3) == 0) { keypad_output = 'D'; break; }

			// Return NULL otherwise.
			else {
				keypad_output = '\0';
			}

			break;
	}

	return state;
}

enum displayKeypadOnLEDs_States { displayKeypadOnLEDs_display };
int displayKeypadOnLEDsSMTick(int state) {
	switch(state) {
		case displayKeypadOnLEDs_display:
			state = displayKeypadOnLEDs_display;
			break;
	}

	switch(state) {
		case displayKeypadOnLEDs_display:
			switch(keypad_output) {
				case '\0': PORTB = 0x1F; break;
				case '1': PORTB = 0x01; break;
				case '2': PORTB = 0x02; break;
				case '3': PORTB = 0x03; break;
				case '4': PORTB = 0x04; break;
				case '5': PORTB = 0x05; break;
				case '6': PORTB = 0x06; break;
				case '7': PORTB = 0x07; break;
				case '8': PORTB = 0x08; break;
				case '9': PORTB = 0x09; break;
				case 'A': PORTB = 0x0A; break;
				case 'B': PORTB = 0x0B; break;
				case 'C': PORTB = 0x0C; break;
				case 'D': PORTB = 0x0D; break;
				case '*': PORTB = 0x0E; break;
				case '0': PORTB = 0x00; break;
				case '#': PORTB = 0x0F; break;
			}
		
		break;
	}

	return state;
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

int main(void) {
    /* Insert DDR and PORT initializations */
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;

	static task task1, task2;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	unsigned short i;

	// Task 1 GetKeypadKeySM
	task1.state = getKeypadKey_get;
	task1.period = 10;
	task1.elapsedTime = task1.period;
	task1.TickFct = &GetKeypadKeySMTick;

	// Task 2 displaySM
	task2.state = displayKeypadOnLEDs_display;
	task2.period = 10;
	task2.elapsedTime = task2.period;
	task2.TickFct = &displayKeypadOnLEDsSMTick;

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
