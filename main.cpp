/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#undef __ARM_FP
#include "keypad.h"
#include "mbed.h"
#include "SLCD.h"
// Blinking rate in milliseconds
#define BLINKING_RATE 600ms
#define BLINKING_RATE2 200ms
Keypad mykeypad(PTA12, PTA4, PTA5, PTC8, PTD3, PTA2, PTA1);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
Thread thread1;
Thread thread2;
void led1_thread()
{
    while (true) {
 led1 = !led1;
 ThisThread::sleep_for(BLINKING_RATE); 
}
}
void led2_thread()
{
    while (true) {
 led2 = !led2;
 ThisThread::sleep_for(BLINKING_RATE2); 
}
}
SLCD slcd;

int main()
{
thread1.start(led1_thread);
thread2.start(led2_thread);
char pressedkey; 
pressedkey = mykeypad.ReadKey();
while (1)
    if (pressedkey!=NO_KEY) {
     slcd.printf("%c",pressedkey);
    ThisThread::sleep_for(BLINKING_RATE);
} 

}