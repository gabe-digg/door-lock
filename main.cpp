/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "keypad.h"
#include "mbed.h"
#include "SLCD.h"
//#include <array>
#include "Password.h"
#include <string>
// Blinking rate in milliseconds
#define BLINKING_RATE 600ms
#define BLINKING_RATE2 200ms
using ThisThread::sleep_for;

Keypad MyKeypad( PTC8,PTA5, PTA4, PTA12, PTD3, PTA2, PTA1);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
Thread thread1;
Thread thread2;
Thread thread3;
SLCD screen;
unsigned char pressedkey;   
void reset() {
  screen.clear();
  screen.Home();
}
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
 string keypress_to_string(char input[9]){
    while (true){
        char input[9]; // 8 digits + '#' + null terminator
        int index = 0;
        pressedkey = MyKeypad.ReadKey();
        if (pressedkey == '#') { 
        input[index] = '\0'; // Null-terminate the password
        //printf("Password entered: %s\n", input);
        index = 0; // Reset for next input
    } 
        else if (pressedkey >= '0' && pressedkey <= '9') {
    input[index++] = pressedkey;
    return(input);
    }}}



char input[9]; // 8 digits + null terminator
int main() {
  sleep_for(500ms); // Give time for  chip  to start
  reset();
  Password pass;

  
  keypress_to_string(input);


  if (pass.check_password(input)) {
    screen.putc(1);//temporay till i figure out letter output
    } else {
    screen.putc(0);
    }

}
 