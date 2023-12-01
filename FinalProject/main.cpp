#include "mbed.h"
#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <cmath>
#include "Mutex.h"

DigitalOut redLED(PB_14);   
DigitalOut greenLED(PB_15);
DigitalOut blueLED(PB_13);

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);


BufferedSerial serial(USBTX, USBRX, 9600);
InterruptIn button(PB_2);

int mode = 0;
int mode_ant = 0;

Thread measureThread;
int16_t red;
int16_t green;
int16_t blue;
int16_t clear;
float distance_sound = 0.0;
float blink_time;
Timeout sound;
bool no_rep = false;
float currentDistance=0.0;
Mutex distanceMutex;

extern void measure();



void button_rise(){
    mode++;
}

void blink_led(){
    led2 = !led2;
    no_rep = false;
}

// main() runs in its own thread in the OS
int main()
{
    
    button.rise(button_rise);
    button.mode(PullUp);
    measureThread.start(measure);
    while (true) {
        if (mode == 0){ // TEST MODE
            led3 = 0;
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
        else if (mode == 1) {  // NORMAL MODE
            led3 = 0;
            led2 = 1;
            led1 = 0;
        }

        else if (mode == 2) {
            led3=1;
            led1=0;
            float distance_range;
            if (distance_sound!=0){
            distanceMutex.lock(); // Lock the Mutex before accessing distance_sound
            if (!no_rep) {
                if (distance_sound > 99.0) {
                    distance_range = 99;
                } else {
                    distance_range = distance_sound;
                }
                //printf("%f\n", blink_time);
                blink_time = distance_range * 1.0 / 99.0;
                sound.attach_us(blink_led, blink_time * 1000000);
                no_rep = true;
            }
            
            distanceMutex.unlock();
            }
            


        }

        else if (mode>2) {
            mode = 0;

        }
        ThisThread::sleep_for(2);
    }
}

