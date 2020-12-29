# DDR or Dance Dance Riverside
Dance Dance Riverside is an embedded systems game based on the ATmega1284

Here it is [in action](https://www.youtube.com/watch?v=EeVlqUXmgbs)

You can find the wiring and documentation for the breadboard [here](https://docs.google.com/document/d/1c6faT4JMFR15S9H-j21YKbM7iYtD3dLnPAnaw_kQdL8)


# What is This?

This is a music rhythm game much similar to Dance Dance Revolution. You start the game by selecting Start on the main menu. After a countdown, the track starts. Musical notes, in the form of directed arrows, start cascading down on the LED matrix. You move the analog joystick in the direction of the arrows. In order to win points, you move the analog joystick when an arrow is directly under the difficulty bar. A counter keeps track of your score.


# Tech and Components

Tech:

* Written in C language
* Compiled on Ubuntu 18.04 using Makefile

Components:

* At the heart of this game is an AVR ATmega1284 microcontroller
* LCD Module 1602A
* Piezo Buzzer
* Max7219 LED Matrix
* Joystick Module
* HW 131 Power Supply
* Atmel ICE PCBA Kit
* Additional wires and 4 pushdown buttons. The buttons can be skipped if you are using the joystick for menu navigation.


# Code

* ADC_C.c: Provides the ADC (Analog to Digital Converter) functionality for the Joystick [1]
* io.c: Provides the functions to write to the LCD [2]
* max7219.c: Provides the functions to write to the 8x8 LED matrix [3]
* pwm.c: Provides the PWM (Pulse Width Modulator) functions to make the piezo buzzer play tunes [4]


# How to get this thing working

You need:
1. An ATmega1284 microcontroller
1. A joystick module
1. An embedded systems LCD (I used a 1602A)
1. A max7219 8x8 led matrix
1. A piezo buzzer/speaker
1. A breadboard, wires, push buttons
1. A power supply for the breadboard (I used an HW-131 module)
1. Linux OS or Atmel Studio (I used Ubuntu and Makefile and this tutorial is only for those)

Now, time to install the AVR tool chain. Instructions can be found [here](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/)

```
sudo apt-get update
sudo apt-get upgrade all

sudo apt-get install gcc-avr binutils-avr avr-libc

Optional (if you wish to mess with a debugger): sudo apt-get install gdb-avr
```

After that, download or clone this repo.

The wiring and documentation for the breadboard can be found [here](https://docs.google.com/document/d/1c6faT4JMFR15S9H-j21YKbM7iYtD3dLnPAnaw_kQdL8)

Make sure you connect the AREF wire to the microcontroller!

After wiring the board, open a terminal at the root of this project. Cross your fingers and do:
```
make program
```
If everything goes right, you can play the game now! Have fun!

# Some Images

![Bird's Eye View Image](https://github.com/AmirUCR/dance_dance_riverside/blob/master/images/birdseye.png)
![Image 1](https://github.com/AmirUCR/dance_dance_riverside/blob/master/images/1.png)
![Image 2](https://github.com/AmirUCR/dance_dance_riverside/blob/master/images/2.png)
![Image 3](https://github.com/AmirUCR/dance_dance_riverside/blob/master/images/3.png)

# Sources
[1]. "Analog Joystick Interface with AVR ATmega16/ATmega32." ElectronicWings, https://electronicwings.com/avr-atmega/analog-joystick-interface-with-atmega-16-32. Accessed 2 Dec. 2019. <br />
[2]. Kindly provided by UC Riverside<br />
[3]. Gironi, Davide. "AVR Atmega MAX7219 7-segment / Led Matrix Display Driver Library." Davide Gironi, 22 July 2013, https://davidegironi.blogspot.com/2013/07/avr-atmega-max7219-7-segment-led-matrix.html. Accessed 2 Dec. 2019. <br />
[4]. Kindly Provided by UC Riverside :)
