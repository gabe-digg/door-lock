/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "keypad.h"
#include "mbed.h"
#include "SLCD.h"
#include <iostream>
// Blinking rate in milliseconds
#define BLINKING_RATE 600ms
#define BLINKING_RATE2 200ms
using ThisThread::sleep_for;

Keypad MyKeypad(PTA12, PTA4, PTA5, PTC8, PTD3, PTA2, PTA1);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
Thread thread1;
Thread thread2;
Thread thread3;
SLCD screen;
char pressedkey;        
void led1_thread()
{
    while (true) {
 led1 = !led1;
 sleep_for(BLINKING_RATE);
}
}
void led2_thread()
{
    while (true) {
 led2 = !led2;
 ThisThread::sleep_for(BLINKING_RATE2); 
}
}
void keypad_thread(){
    while (true){
        pressedkey = MyKeypad.ReadKey();
        if (pressedkey!=NO_KEY) {
            screen.printf("I-%02x", pressedkey);
            std::cout << pressedkey << std::endl;
        }
        sleep_for(BLINKING_RATE);
    }
}
int main(void) {
thread1.start(led1_thread);
thread2.start(led2_thread);
thread3.start(keypad_thread);
 while (true) { // Added infinite loop to keep main running
        sleep_for(1s); // Prevent main from consuming too much CPU
}
}