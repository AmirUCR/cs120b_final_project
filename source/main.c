
/*	Author: amohs002
 *  
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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

// Frequencies for the song
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
#define REST 0	// No note


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

// Shift the contents of a character array to the right - This will make the illusion that the arrows/blocks are 
// cascading down on the matrix

// Example: LED_right_arrow[] = { 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
// LED_Char_Array_Right_Rotate_By_One(LED_right_arrow)
// will result in LED_right_arrow[] = { 0x00, 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
// where every element in shifted to the right once
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
unsigned char gameEnded = 0;

unsigned char menuShownFlag = 0;
unsigned char updateScoreFlag = 0;
unsigned char score = 0;
unsigned char highScore = 0;

unsigned char countup = 20;
char countdown = 5;	// Initial 5 second countdown

// The red line that appears on the matrix
unsigned char matrixDifficultyBar = 3;

unsigned char numCalls = 0; // Use this to keep track of
									   // how many elements in LED_right/left_arrow
									   // we've gone through. We move to the next arrow when
									   // we've gone through all of the current array's elements (rows)

unsigned char lastJoystickMove = 'M'; // L for left, R for right, M for middle - B for Block
unsigned char arrow_sequence_array[] = { 'L', 'R', 'L', 'A', 'R', 'R', 'L', 'B', 'L', 'L', 'R', 'L', 'L',
										'B', 'A', 'B', 'L', 'L', 'L', 'R', 'L', 'L', 'R', 'R', 'L', 'L', 'R', 'R', 'L', 'A', 'L', 'B' };
unsigned char sequenceIndex = 0;
const unsigned short numSequence = sizeof(arrow_sequence_array)/sizeof(arrow_sequence_array[0]);

unsigned char LED_array_size = 11;

// Countdown numbers that appear before game start
unsigned char five[] = { 0x00, 0x7C, 0x60, 0x7E, 0x06, 0x06, 0x66, 0x3C };
unsigned char four[] = { 0x00, 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C };
unsigned char three[] = { 0x00, 0x3C, 0x66, 0x06, 0x1C, 0x0C, 0x66, 0x3C };
unsigned char two[] = { 0x00, 0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E };
unsigned char one[] = { 0x00, 0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7E };

// Arrow shapes - These arrays are not modified.
unsigned char LED_right_arrow_default[] = { 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box_right_default[] = { 0x0F, 0x09, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box_left_default[] = { 0xF0, 0x90, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_left_arrow_default[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Arrow shapes - We shift these values in order to move the arrows on the matrix
unsigned char LED_right_arrow[] = { 0x02, 0x0F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box_right[] = { 0x0F, 0x09, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_box_left[] = { 0xF0, 0x90, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char LED_left_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// unsigned char LED_up_arrow[] = { 0x40, 0xE0, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// unsigned char LED_down_arrow[] = { 0x40, 0xF0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


//---------------End shared variables-------------------------
// Only update if conditions are met
void UpdateHighscore() {
	if ((highScore == 0) || (score > highScore)) {
		highScore = score;
		eeprom_update_byte((const char *) 1, highScore);	// Save to EEPROM
	}
}

void GameReset() {
	gameInProgressFlag = 0;
	gameEnded = 0;
	score = 0;
	numCalls = 0;
	sequenceIndex = 0;
	countup = 20;
	countdown = 5;
	lastJoystickMove = 'M';

	// Reset arrow arrays
	for (unsigned char i = 0; i < LED_array_size; i++) {
		LED_right_arrow[i] = LED_right_arrow_default[i];
		LED_left_arrow[i] = LED_left_arrow_default[i];
		LED_box_left[i] = LED_box_left_default[i];
		LED_box_right[i] = LED_box_right_default[i];
	}
}

enum Logic_States { Logic_wait, Logic_check, Logic_CheckWait, Logic_wait_wait };
int LogicSMTick(int state) {
	switch(state) {
		case Logic_wait:
			if ((!gameInProgressFlag) || (numCalls < 6)) {
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

				// Moved the joystick in the correct direction
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

enum LEDMatrix_States { LEDMatrix_wait, LEDMatrix_countdown, LEDMatrix_shift };
int LEDMatrixSMTick(int state) {
	unsigned char arrow_direction;

	switch(state) {
		case LEDMatrix_wait:
			if (!gameInProgressFlag) {
				state = LEDMatrix_wait;
			}
			else {
				state = LEDMatrix_countdown;
			}
			break;

		case LEDMatrix_countdown:
			if (!gameInProgressFlag) {
				state = LEDMatrix_wait;
			}
			else if (countdown >= 0) {
				state = LEDMatrix_countdown;
			} 
			else {
				state = LEDMatrix_shift;
			}
			break;

		case LEDMatrix_shift:
			if (!gameInProgressFlag) {
				state = LEDMatrix_wait;
			}
			else {
				state = LEDMatrix_shift;
			}
			break;
	}

	switch (state) {
		case LEDMatrix_wait:
			max7219_clearDisplay(0);
			break;

		case LEDMatrix_countdown:
			if (countup >= 20) {
				countup = 0;

				switch(countdown) {
					case 5:
						for (unsigned char j = 0; j < 8; j++) {
							max7219_digit(0, j, five[j]);
						}
					break;
					
					case 4:
						for (unsigned char j = 0; j < 8; j++) {
							max7219_digit(0, j, four[j]);
						}
					break;

					case 3:
						for (unsigned char j = 0; j < 8; j++) {
							max7219_digit(0, j, three[j]);
						}
					break;

					case 2:
						for (unsigned char j = 0; j < 8; j++) {
							max7219_digit(0, j, two[j]);
						}
					break;
					
					case 1:
						for (unsigned char j = 0; j < 8; j++) {
							max7219_digit(0, j, one[j]);
						}
					break;
				}

				countdown--;
			}

				countup++;

		break;

		case LEDMatrix_shift:
			numCalls++;

			if (numCalls > 11) {
				if (sequenceIndex < (numSequence - 1)) {
					sequenceIndex++;
				} else {
					sequenceIndex = 0;
					gameEnded = 1;
					break;
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

				case 'B':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_box_right[j + 2]);
						max7219_digit(0, matrixDifficultyBar, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_box_right, 11);
				break;

				case 'A':
					max7219_clearDisplay(0);

					for (unsigned char j = 0; j < 8; j++) {
						max7219_digit(0, j, LED_box_left[j + 2]);
						max7219_digit(0, matrixDifficultyBar, 0xFF);
					}

					LED_Char_Array_Right_Rotate_By_One(LED_box_left, 11);
				break;
			}

			break;
	}

	return state;
}

enum PWMSpeaker_States { PWMSpeaker_wait, PWMSpeaker_play };
int PWMSpeakerSMTick(int state) {
	//static double song1[] = { G4, G4, A4s, C5, G4, G4, F4, Fs, G4, G4, A4s, C5, G4, G4, F4, Fs };
	static double song1[] = { E4, E4, E4, REST, E4, E4, E4, REST, E4, G4, C4, D4, E4, REST, REST, REST, F4, F4, F4, F4, F4, E4, E4, E4, E4, D4, D4, E4, D4, REST, G4, REST };

	switch(state) {
		case PWMSpeaker_wait:
			if ((lastJoystickMove != 'M') && !(countdown >= 0) && (numCalls >= 6)) {
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

enum Joystick_States { Joystick_wait, Joystick_check };
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
						 LCDDisplay_showHighscore, LCDDisplay_selectErase, LCDDisplay_showEraseConfirmation, LCDDisplay_youWin,
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
				menuShownFlag = 0;

				if ((score > 0) && (score > highScore)) {
					UpdateHighscore();
				}

				GameReset();
				state = LCDDisplay_wait;
			}
			else if (gameEnded) {
				menuShownFlag = 0;
				state = LCDDisplay_youWin;
			}
			break;

		case LCDDisplay_youWin:
			if (!button1 && !button2 && button3 && !button4) {
				menuShownFlag = 0;
				state = LCDDisplay_selectStartHold;
			} else {
				state = LCDDisplay_youWin;
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

				if (highScore == 0) {
					LCD_DisplayString(1, "No highscore yet> Back         v");
				} else {
					unsigned char buffer[33];

					if ((highScore / 10) > 0) {
						sprintf(buffer, "Highscore: %d   > Back         v", highScore);
					} else {
						sprintf(buffer, "Highscore: %d    > Back         v", highScore);
					}


					LCD_DisplayString(1, buffer);
				}

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

				if (highScore != 0) {
					eeprom_update_byte((const char*) 1, 0xFF);
					highScore = 0;
				}

				LCD_DisplayString(1, "Highscore reset > Back");
				LCD_Cursor(17);
			}
			break;
		
		case LCDDisplay_gameInProgress:
			gameInProgressFlag = 1;

			if (!menuShownFlag) {
				menuShownFlag = 1;
				LCD_DisplayString(1, "Score: ");
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

		case LCDDisplay_youWin:
			if (!menuShownFlag) {
				menuShownFlag = 1;
				
				if (score == 0) {
					LCD_DisplayString(1, "Try again!      > Back");
					GameReset();
					LCD_Cursor(17);
				} else {
					LCD_DisplayString(1, "You won!        > Back");	
					LCD_Cursor(17);
					
					UpdateHighscore();
					GameReset();
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
	task1.period = 50;
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

	if (eeprom_read_byte((const char*) 1) != 0xFF) {
		highScore = eeprom_read_byte((const char*) 1);
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
