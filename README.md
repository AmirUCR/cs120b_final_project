# DDR or Dance Dance Riverside
Dance Dance Riverside is an embedded systems game based on the ATmega1284

The wiring and documentation for the breadboard can be found [here](https://docs.google.com/document/d/1c6faT4JMFR15S9H-j21YKbM7iYtD3dLnPAnaw_kQdL8)

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

After that, download or clone this repo.

The wiring and documentation for the breadboard can be found [here](https://docs.google.com/document/d/1c6faT4JMFR15S9H-j21YKbM7iYtD3dLnPAnaw_kQdL8)
Make sure you connect the AREF wire to the microcontroller!

After wiring the board, open a terminal at the root of this project. Cross your fingers and do:
```
make program
```
If everything goes right, you can play the game now! Have fun!
