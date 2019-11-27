
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

#define button1 ((~PINA & 0x08) >> 3)
#define button2 ((~PINA & 0x10) >> 4)
#define button3 ((~PINA & 0x20) >> 5)
#define button4 ((~PINA & 0x40) >> 6)

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
unsigned char gameInProgressFlag = 0;

unsigned char menuShownFlag = 0;
unsigned char lcdMenuChoice = 0;
unsigned char lcdMenuChoiceConfirm = 0;
unsigned char displayedScore = 0;
unsigned char score = 0;

unsigned char lcdRow1Col = 1;
unsigned char lcdRow1String[] = "Score: ";

unsigned char lcdRow2Col = 1;
unsigned char lcdRow2String[] = "";

unsigned char matrixDifficultyBar = 3;

unsigned char numCalls = 0; // Use this to keep track of
									   // how many elements in LED_right/left_arrow
									   // we've gone through. We move to the next arrow when
									   // we've gone through all of the current array's elements (rows)

unsigned char lastJoystickMove = 'M'; // L for left, R for right, M for middle
unsigned char arrow_sequence_array[] = { 'L', 'R', 'L' };
unsigned char sequenceIndex = 0;
const unsigned short numSequence = sizeof(arrow_sequence_array)/sizeof(unsigned char*);

//---------------End shared variables-------------------------

void GameReset() {
	gameInProgressFlag = 0;
	score = 0;
	numCalls = 0;
	sequenceIndex = 0;
	lastJoystickMove = 'M';

	menuShownFlag = 0;
	lcdMenuChoice = 0;
	lcdMenuChoiceConfirm = 0;
	displayedScore = 0;
}

enum Logic_States { Logic_wait, Logic_check, Logic_CheckWait, Logic_wait_wait };
int LogicSMTick(int state) {
	switch(state) {
		case Logic_wait:
			if (!gameInProgressFlag || numCalls < 6) {
				state = Logic_wait;
			}
			else {
				state = Logic_check;
			}
			break;

		case Logic_wait_wait:
			if (lastJoystickMove != 'M') {
				state = Logic_wait_wait;
			}
			else {
				if (numCalls < 6) {
					state = Logic_wait;
				} else {
					state = Logic_check;
				}
			}
			break;

		case Logic_check:
			if (numCalls >= 6) {
				state = Logic_check;
			} else {
				state = Logic_wait;
			}
			break;

		case Logic_CheckWait:
			if (numCalls >= 6) {
				state = Logic_CheckWait;
			}
			else {
				state = Logic_wait;
			}
			break;
	}

	switch(state) {
		case Logic_wait:
			if (lastJoystickMove != 'M') {
				if (score > 0) {
						score--;
					} else {
						score = 0;
				}

				state = Logic_wait_wait;
			}
			break;
		
		case Logic_check:
			if (lastJoystickMove != 'M') {
				if (lastJoystickMove == arrow_sequence_array[sequenceIndex]) {
					score++;
				}
				else {
					if (score > 0) {
						score--;
					} else {
						score = 0;
					}
				}

				state = Logic_CheckWait;
			}

			break;
		
		case Logic_CheckWait:
			break;

		case Logic_wait_wait:
			break;
	}

	return state;
}

enum LEDMatrix_States { LEDMatrix_wait, LEDMatrix_shift };
int LEDMatrixSMTick(int state) {
	static unsigned char LED_right_arrow[] = { 0x02, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static unsigned char LED_box[] = { 0x0f, 0x09, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static unsigned char LED_left_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//	static unsigned char LED_up_arrow[] = { 0x40, 0xE0, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//  static unsigned char LED_down_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	unsigned char arrow_direction;

	switch(state) {
		case LEDMatrix_wait:
			if (!gameInProgressFlag) {
				state = LEDMatrix_wait;
			} else {
				state = LEDMatrix_shift;
			}
			break;

		case LEDMatrix_shift:
			if (!gameInProgressFlag) {
				state = LEDMatrix_wait;
			} else {
				state = LEDMatrix_shift;
			}
			break;
	}

	switch (state) {
		case LEDMatrix_wait:
			break;

		case LEDMatrix_shift:
			numCalls++;

			if (numCalls > 11) {
				if (sequenceIndex <= numSequence) {
					sequenceIndex++;
				} else {
					sequenceIndex = 0;
				}

				numCalls = 1;
			}

			arrow_direction = arrow_sequence_array[sequenceIndex];

			switch (arrow_direction) {
				case 'L':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_left_arrow[j + 2]);
						max7219_digit(0, matrixDifficultyBar, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_left_arrow, 11);
					break;

				case 'R':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_right_arrow[j + 2]);
						max7219_digit(0, matrixDifficultyBar, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_right_arrow, 11);
					break;
			}

			break;
	}


	return state;
}

enum Joystick_States { Joystick_wait, Joystick_check};
int JoystickSMTick(int state) {

	// Max Y 1008
	// Min Y 31
	// Default Y 540
	unsigned short ADC_Value;
	ADC_Value = ADC_Read(0);

	switch (state) {
		case Joystick_wait:
			if (!gameInProgressFlag) {
				state = Joystick_wait;
			} else {
				state = Joystick_check;
			}
			
			break;

		case Joystick_check:
			if (!gameInProgressFlag) {
				state = Joystick_wait;
			} else {
				state = Joystick_check;
			}
			break;
		
		default:
			state = Joystick_wait;
			break;
		}

	switch(state) {
		case Joystick_wait:
			break;

		case Joystick_check:
			if (ADC_Value > 900) {
				lastJoystickMove = 'R';
			}
			else if (ADC_Value < 300) {
				lastJoystickMove = 'L';
			}
			else {
				lastJoystickMove = 'M';
			}
			break;
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

enum LCDDisplay_States { LCDDisplay_wait, LCDDisplay_showMenu, LCDDisplay_gameInProgress };
int LCDDisplaySMTick(int state) {

	switch(state) {
		case LCDDisplay_wait:
			if (!menuShownFlag) {
				state = LCDDisplay_showMenu;
			} else {
				state = LCDDisplay_gameInProgress;
			}
			break;

		case LCDDisplay_showMenu:
			if (lcdMenuChoiceConfirm == 1) {
				state = LCDDisplay_gameInProgress;
			}
			break;

		case LCDDisplay_gameInProgress:
			break;
	}

	switch(state) {
		case LCDDisplay_wait:
			break;

		case LCDDisplay_showMenu:
			if (!menuShownFlag && !gameInProgressFlag) {
				LCD_Cursor(7);
				LCD_DisplayString(1, "> Start           Option 2");
				menuShownFlag = 1;
				lcdMenuChoice = 1;
			}
			else if (menuShownFlag && button2) {
				LCD_Cursor(7);
				LCD_DisplayString(1, "  Start         > Option 2");
				lcdMenuChoice = 2;
			}
			else if (menuShownFlag && button1) {
				LCD_Cursor(7);
				LCD_DisplayString(1, "> Start           Option 2");
				lcdMenuChoice = 1;
			}
			else if (menuShownFlag && button3) {
				if (lcdMenuChoice == 1) {
					lcdMenuChoiceConfirm = 1;
				}
				else {
					// TODO
				}
			}
			
			break;
		
		case LCDDisplay_gameInProgress:
			gameInProgressFlag = 1;
			if (!displayedScore) {
				LCD_Cursor(1);
				LCD_DisplayString(1, lcdRow1String);
				displayedScore = 1;
			}

			if (button4) {
				GameReset();
				break;
			}

			LCD_Cursor(8);
			LCD_WriteData(score + '0');
			break;
	}


	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	//DDRB = 0xF0; PORTB = 0x0F;
    DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = LEDMatrix_wait;
	task1.period = 200;
	task1.elapsedTime = task1.period;
	task1.TickFct = &LEDMatrixSMTick;

	task2.state = Joystick_wait;
	task2.period = 100;
	task2.elapsedTime = task2.period;
	task2.TickFct = &JoystickSMTick;

	task3.state = Logic_wait;
	task3.period = 100;
	task3.elapsedTime = task3.period;
	task3.TickFct = &LogicSMTick;

	task4.state = LCDDisplay_wait;
	task4.period = 100;
	task4.elapsedTime = task4.period;
	task4.TickFct = &LCDDisplaySMTick;

	unsigned long GCD = tasks[0]->period;
 	for (unsigned char i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}

	// unsigned short adc_result = 0x0000;
	// char buffer[20];

	TimerSet(GCD);
	TimerOn();

	ADC_init();
	LCD_init();
	max7219_init();

	// unsigned char j = 0;

	// unsigned short ADC_Value;

    while (1) {

		for (unsigned char i = 0; i < numTasks; i++) {
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