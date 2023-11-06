#include "mbed.h"
#include <cstdint>
#include <stdio.h>
#include <cmath>

DigitalOut redLED(PB_14);   
DigitalOut greenLED(PB_15);
DigitalOut blueLED(PB_13);

DigitalOut led1(LED1);
DigitalOut led2(LED2);

BufferedSerial serial(USBTX, USBRX, 9600);
InterruptIn button(PB_2);

bool mode = false;
bool mode_ant = false;

Thread measureThread;
int16_t red;
int16_t green;
int16_t blue;
int16_t clear;

extern void measure();

void button_rise(){
    mode = !mode;
}

// main() runs in its own thread in the OS
int main()
{
    button.rise(button_rise);
    button.mode(PullUp);
    measureThread.start(measure);
    while (true) {
        if (mode == false){ // TEST MODE
            led2 = 0;
            led1 = 1;
            // Set the RGB LED to the dominant colour
            if (max(red,max(blue,green))==red){
                redLED=1;
                greenLED=0;
                blueLED=0;
            }
            else if (max(red,max(blue,green))==green) {
                redLED=0;
                greenLED=1;
                blueLED=0;
            }
            else if (max(red,max(blue,green))==blue) {
                redLED=0;
                greenLED=0;
                blueLED=1;
            }
        }
        else {  // NORMAL MODE
            led2 = 1;
            led1 = 0;
        }
        ThisThread::sleep_for(500);
    }
}

