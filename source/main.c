
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
#include "pwm.h"
#include "max7219.h"
#ifndef _SIMULATE_
#include "simAVRHeader.h"
#endif

#define button1 ((~PINA & 0x08) >> 3)
#define button2 ((~PINA & 0x10) >> 4)
#define button3 ((~PINA & 0x20) >> 5)
#define button4 ((~PINA & 0x40) >> 6)

#define C4 261.63
#define C4s 277.18
#define D4 293.66
#define Eb 311.13
#define E4 329.63
#define F4 349.23
#define Fs 369.00
#define G4 392.00
#define A4 440.00
#define A4s 466.16
#define B4 493.88
#define C5 523.25
#define A5s 932.33
#define C5s 554.37
#define D5 587.33
#define G5 783.99

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
unsigned char updateScoreFlag = 0;
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
unsigned char arrow_sequence_array[] = { 'L', 'L', 'R', 'R', 'R' };
unsigned char sequenceIndex = 0;
const unsigned short numSequence = sizeof(arrow_sequence_array)/sizeof(arrow_sequence_array[0]);

unsigned char LED_array_size = 11;

unsigned char LED_right_arrow_default[] = { 0x02, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box_default[] = { 0x0f, 0x09, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_left_arrow_default[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


unsigned char LED_right_arrow[] = { 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box[] = { 0x0F, 0x09, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_left_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// unsigned char LED_up_arrow[] = { 0x40, 0xE0, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// unsigned char LED_down_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


//---------------End shared variables-------------------------

void GameReset() {
	gameInProgressFlag = 0;
	score = 0;
	numCalls = 0;
	sequenceIndex = 0;
	lastJoystickMove = 'M';

	for (unsigned char i = 0; i < LED_array_size; i++) {
		LED_right_arrow[i] = LED_right_arrow_default[i];
		LED_left_arrow[i] = LED_left_arrow_default[i];
		LED_box[i] = LED_box_default[i];
	}

	menuShownFlag = 0;
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
			} else {
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
				updateScoreFlag = 1;

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
				updateScoreFlag = 1;

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
			max7219_clearDisplay(0);
			break;

		case LEDMatrix_shift:
			numCalls++;

			if (numCalls > 11) {
				if (sequenceIndex < (numSequence - 1)) {
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

enum PWMSpeaker_States { PWMSpeaker_wait, PWMSpeaker_play };
int PWMSpeakerSMTick(int state) {
	static double song1[] = { G4, G4, A4s, C5, G4, G4,
							F4, Fs, G4, G4, A4s, C5, G4,
							G4, F4, Fs };


	switch(state) {
		case PWMSpeaker_wait:
			if (lastJoystickMove != 'M') {
				state = PWMSpeaker_play;
			} else {
				state = PWMSpeaker_wait;
			}
			break;

		case PWMSpeaker_play:
			if (lastJoystickMove == 'M') {
				state = PWMSpeaker_wait;
			} else {
				state = PWMSpeaker_play;
			}
			break;
		
		default:
			state = PWMSpeaker_wait;
			break;
	}

	switch(state) {
		case PWMSpeaker_wait:
			set_PWM(0);
			break;
	
		case PWMSpeaker_play:
			if (lastJoystickMove == arrow_sequence_array[sequenceIndex]) {
				set_PWM(song1[sequenceIndex]);
			}

			break; 

	default:
		break;
	}
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

enum LCDDisplay_States { LCDDisplay_wait, LCDDisplay_selectStart, LCDDisplay_selectOptions,
						 LCDDisplay_showHighscore, LCDDisplay_selectErase, LCDDisplay_showEraseConfirmation,
						 LCDDisplay_showHighscoreHold, LCDDisplay_selectStartHold, LCDDisplay_showEraseConfirmationHold,
						 LCDDisplay_gameInProgress };
int LCDDisplaySMTick(int state) {
	switch(state) {
		case LCDDisplay_wait:
			menuShownFlag = 0;
			state = LCDDisplay_selectStart;
			break;
		
		case LCDDisplay_selectStart:
			if (!button1 && button2 && !button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectOptions;
			}
			else if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_gameInProgress;
			}
			else {
				state = LCDDisplay_selectStart;
			}
			break;

		case LCDDisplay_selectOptions:
			if (button1 && !button2 && !button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectStart;
			}
			else if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_showHighscoreHold;
			}
			else {
				state = LCDDisplay_selectOptions;
			}
			break;

		case LCDDisplay_showHighscoreHold:
			if (button3) {
				state = LCDDisplay_showHighscoreHold;
			} else {
				state = LCDDisplay_showHighscore;
			}
			break;
		
		case LCDDisplay_showHighscore:
			if (!button1 && button2 && !button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectErase;
			}
			else if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectStartHold;
			}
			else {
				state = LCDDisplay_showHighscore;
			}
			break;

		case LCDDisplay_selectStartHold:
			if (button3) {
				state = LCDDisplay_selectStartHold;
			} else {
				state = LCDDisplay_selectStart;
			}
			break;

		case LCDDisplay_selectErase:
			if (button1 && !button2 && !button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_showHighscore;
			}
			else if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_showEraseConfirmationHold;
			}
			else {
				state = LCDDisplay_selectErase;
			}
			break;

		case LCDDisplay_showEraseConfirmationHold:
			if (button3) {
				state = LCDDisplay_showEraseConfirmationHold;
			} else {
				state = LCDDisplay_showEraseConfirmation;
			}
			break;
			break;

		case LCDDisplay_showEraseConfirmation:
			if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectStartHold;
			} else {
				state = LCDDisplay_showEraseConfirmation;
			}
			break;

		case LCDDisplay_gameInProgress:
			if (button4) {
				// EEPROM SAVE SCORE
				GameReset();
				state = LCDDisplay_wait;
				break;
			}
			break;
	}

	// ACTIONS
	switch(state) {
		case LCDDisplay_wait:
			break;

		case LCDDisplay_selectStart:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, "> Start           Highscore");
				LCD_Cursor(1);
			}
			break;

		case LCDDisplay_selectOptions:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, "  Start         > Highscore");
				LCD_Cursor(17);
			}
			break;

		case LCDDisplay_showHighscore:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				// EEPROM READ SCORE
				// DISPLAY ON LCD
				LCD_DisplayString(1, "  HIGHSCORE HERE");
				LCD_DisplayString(17, "> Back         v");
				LCD_Cursor(17);
			}
			break;

		case LCDDisplay_selectErase:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, "  Back         ^> ERASE SCORE");
				LCD_Cursor(17);
			}
			break;

		case LCDDisplay_showEraseConfirmation:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, "Highscore reset > Back");
				LCD_Cursor(17);
			}
			break;
		
		case LCDDisplay_gameInProgress:
			gameInProgressFlag = 1;

			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, lcdRow1String);
				LCD_Cursor(8);
				LCD_WriteData('0');
			}

			if (updateScoreFlag) {
				updateScoreFlag = 0;

				if (score == 0) {
					LCD_Cursor(8);
					LCD_WriteData('0');
				}
				else if (score < 10) {
					LCD_Cursor(8);
					LCD_WriteData(score + '0');
					LCD_WriteData(' ');
					LCD_Cursor(9);
				}
				else {
					LCD_Cursor(8);
					unsigned char temp = score / 10;
					LCD_WriteData(temp + '0');
					temp = score % 10;
					LCD_WriteData(temp + '0');
				}
			}
			break;

			case LCDDisplay_showHighscoreHold:
				break;

			case LCDDisplay_selectStartHold:
				break;

			case LCDDisplay_showEraseConfirmationHold:
				break;
	}

	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	static task task1, task2, task3, task4, task5;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5 };
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

	task5.state = PWMSpeaker_wait;
	task5.period = 100;
	task5.elapsedTime = task5.period;
	task5.TickFct = &PWMSpeakerSMTick;

	unsigned long GCD = tasks[0]->period;
 	for (unsigned char i = 1; i < numTasks; i++) {
 		GCD = findGCD(GCD, tasks[i]->period);
 	}
	 
	TimerSet(GCD);
	TimerOn();

	ADC_init();
	LCD_init();
	max7219_init();
	PWM_on();

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
    }

    return 1;
} 