#include "mbed.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <string>

#define MAX_LIGHT 75
#define MIN_LIGHT 0

#define MAX_SOIL 100
#define MIN_SOIL 0

#define MAX_TEMPERATURE 50
#define MIN_TEMPERATURE -10

#define MAX_REL_HUMIDITY 75
#define MIN_REL_HUMIDITY 25

#define TCS34725_CDATAL 0x14
#define TCS34725_RDATAL 0x16
#define TCS34725_GDATAL 0x18
#define TCS34725_BDATAL 0x1A
const int COMMAND_ADDRESS=0X80;
const int TCS34725_ADDRESS = 0x29<<1;

extern DigitalOut redLED;   
extern DigitalOut greenLED;
extern DigitalOut blueLED;

extern uint16_t clear; 
extern uint16_t red; 
extern uint16_t green; 
extern uint16_t blue; 
int counter_red=0;
int counter_green=0;
int counter_blue=0;
 
extern float v_brightness;
float mean_brightness = 0;
float maximum_brightness = 0;
float minimum_brightness = 1000;
int counter_brightness = 0; 

extern float v_temp;
float mean_temperature = 0.0;
float maximum_temperature = 0.0;
float minimum_temperature = 1000.0;
int counter_temperature = 0;

extern float v_humidity;
float mean_humidity = 0.0;
float maximum_humidity = 0.0;
float minimum_humidity = 1000.0;

DigitalOut led1(LED1);
DigitalOut led2(LED2);

char data_L[1];
char data_H[1];
int redValue;

extern bool mode; 
extern bool mode_ant;

I2C i2c(PB_9, PB_8); 
DigitalOut led(PB_7);   //CAMBIAR PIN
//BufferedSerial GPS(PA_9, PA_10);
BufferedSerial GPS(PA_9, PA_10, 9600);


void brightness();
void soilMoisture();
void tempAndHum();
void rgb();
void writeRegister();
void read16();
void location();
void reset_variables();

int counter_GPS=0;
int wait_time=2000;
int counter_hour=0;






void measure(){
    
    //GPS.set_baud(9600);
    while(1){
        soilMoisture();
        brightness();
        tempAndHum();
        rgb();
        location();
        printf("\n");
        if (mode == false){
            led2 = 0;
            led1 = 1;
            ThisThread::sleep_for(2000);
        }
        else {
            led2 = 1;
            led1 = 0;
            if (v_brightness >= MAX_LIGHT || v_brightness <= MIN_LIGHT){
            redLED = 1;
            greenLED = 1;
            blueLED = 0;
        }
        else if (v_temp >= MAX_TEMPERATURE || v_temp <= MIN_TEMPERATURE) {
            redLED = 1;
            greenLED = 0;
            blueLED = 1;
        }
        else if (v_humidity >= MAX_REL_HUMIDITY || v_humidity <= MIN_REL_HUMIDITY) {
            redLED = 1;
            greenLED = 1;
            blueLED = 1;
        }
        else {
            redLED = 0;
            greenLED = 0;
            blueLED = 0; 
        }
            
        counter_hour++;
        if (counter_hour == 5){
            mean_brightness = mean_brightness/counter_brightness;
            mean_temperature = mean_temperature/counter_temperature;
            mean_humidity = mean_humidity/counter_temperature;

            printf("LIGHT: Maximum: %f  Minimum: %f  Mean: %f\n", maximum_brightness, minimum_brightness, mean_brightness);
            printf("TEMPERATURE: Maximum: %f  Minimum: %f  Mean: %f\n", maximum_temperature, minimum_temperature, mean_temperature);
            printf("HUMIDITY: Maximum: %f  Minimum: %f  Mean: %f\n", maximum_humidity, minimum_humidity, mean_humidity);
            if ((max(counter_red,max(counter_green,counter_blue))==counter_red)){
                printf("RGB SENSOR: Dominant: RED, appeared %d times\n", counter_red);
            }
            else if ((max(counter_red,max(counter_green,counter_blue))==counter_green)) {
                printf("RGB SENSOR: Dominant: GREEN, appearred %d times\n", counter_green);
            }
            else if ((max(counter_red,max(counter_green,counter_blue))==counter_blue)) {
                printf("RGB SENSOR: Dominant: BLUE, appeared %d times\n", counter_blue);
            }
            
            
            counter_hour = 0;
            reset_variables();

            
        }
        ThisThread::sleep_for(5000);
        
    }
}

void reset_variables(){
    mean_brightness = 0;
    maximum_brightness = 0;
    minimum_brightness = 1000;
    mean_temperature = 0.0;
    maximum_temperature = 0.0;
    minimum_temperature = 1000.0;
    mean_humidity = 0;
    maximum_humidity = 0;
    minimum_humidity = 1000.0;
    counter_red = 0;
    counter_green = 0;
    counter_blue = 0;
    counter_brightness = 0;
    counter_temperature = 0;

}

void soilMoisture(){
    AnalogIn soil(PA_0);
    float soilValue = 0;
    soilValue = soil.read() * 100;
    printf("SOIL MOISTURE: %.1f%%\n", soilValue);
}

void brightness(){
    AnalogIn brightness(PA_4);
    v_brightness = brightness.read() * 100;
    printf("LIGHT: %.1f%%\n", v_brightness);
    

    if (mode == true) {
        if (mode != mode_ant){
            mean_brightness = 0;
            maximum_brightness = 0;
            minimum_brightness = 1000;
            mode_ant = mode;
         }
        counter_brightness++;
        maximum_brightness = max(maximum_brightness,v_brightness);
        minimum_brightness = min(minimum_brightness,v_brightness);
        mean_brightness = (mean_brightness + v_brightness);
    }
}

void tempAndHum(){
    const int sensorAdd = 0x40 << 1; //sensor addres
    char command[1];
    char data[2];

    command[0] = {0xE5}; //for read humidity
    i2c.write(sensorAdd, command, 1);
    i2c.read(sensorAdd, data, 2);

    uint16_t h = (data[0] << 8) | data[1]; //combine into one 16bit value
    v_humidity = h * 125.0 / 65536 - 6.0;

    command[0] = {0xE3};
    i2c.write(sensorAdd, command, 1); // Send command for reading
    ThisThread::sleep_for(100ms); // to perform the measurement                
    i2c.read(sensorAdd, data, 2); // get the temperature H and L
                
    uint16_t temperature = (data[0]) << 8 | data[1]; // MSB at [0]
    v_temp = ((175.72 * temperature) / 65536) - 46.85;   // formula in datasheet
    printf("TEMP/HUM: Temperature: %.1f,     Relative humidity: %.1f\n", v_temp, v_humidity);


    if (mode == true) {
        if (mode != mode_ant){
            mean_temperature = 0.0;
            maximum_temperature = 0.0;
            minimum_temperature = 1000.0;
            maximum_humidity = 0;
            minimum_humidity = 1000.0;
            mean_humidity = 0;
            mode_ant = mode;
         }
        counter_temperature++;
        maximum_temperature = max(maximum_temperature,v_temp);
        minimum_temperature = min(minimum_temperature,v_temp);
        mean_temperature = (mean_temperature + v_temp);

        maximum_humidity = max(maximum_humidity,v_humidity);
        minimum_humidity = min(minimum_humidity,v_humidity);
        mean_humidity = (mean_humidity + v_humidity);

    }

}


uint16_t read16(uint8_t reg) {
    char color[2];
    char cmd[1];
    cmd[0] = reg;
    i2c.write(TCS34725_ADDRESS, cmd, 1, true);
    i2c.read(TCS34725_ADDRESS, color, 2);
    return (color[1] << 8) | color[0];
}

void rgb(){

    led=1;
    
    //i2c.frequency(400000);  // Set I2C frequency to 400 kHz

    // Initialize TCS34725
  
    char data[2];
    data[0] = COMMAND_ADDRESS;
    data[1] = 0x03;
    i2c.write(TCS34725_ADDRESS, data, 2);

    wait_us(3000);
    
    clear = read16(TCS34725_CDATAL|COMMAND_ADDRESS);
    red = read16(TCS34725_RDATAL|COMMAND_ADDRESS);
    green = read16(TCS34725_GDATAL|COMMAND_ADDRESS);
    blue = read16(TCS34725_BDATAL|COMMAND_ADDRESS);

   

    printf("COLOR SENSOR: Clear: %d  Red: %d  Green: %d  Blue: %d\n", clear, red, green, blue);
    //led=0;

     if (mode == true) {
        if (mode != mode_ant){
            counter_red = 0;
            counter_green = 0;
            counter_blue = 0;
            mode_ant = mode;
         }
        
        if (max(red,max(blue,green))==red){
            counter_red++;
        }
        else if (max(red,max(blue,green))==green) {
            counter_green++;
        }
        else if (max(red,max(blue,green))==blue) {
            counter_blue++;
        }
    }
    


}

void parseAndProcessNMEA(const char* sentence) {
    if (strncmp(sentence, "$GPGGA,", 7) == 0) {
        counter_GPS++;
        if (counter_GPS < 2){
            //printf("\nGPS SENTNECE: %s\n",sentence);
            char* divisions[15];
            int divisionCount = 0;
            //char* token = strtok((char*)sentence, ",");
            char *wordStart = (char*)sentence; //apuntamos a la dirección donde se almacena la sentencia
            
            int i =0;
            while(true){
                i++;
                char currentChar = sentence[i];
                if (currentChar == ',' || currentChar == '\0'){
                    int length = &sentence[i] - wordStart;
                    if(length > 0){
                        divisions[divisionCount] = (char*)malloc(length + 1);
                        strncpy(divisions[divisionCount], wordStart, length);
                        divisions[divisionCount][length] = '\0';
                        divisionCount++;

                        wordStart = (char*)sentence + i + 1; // se mueve al siguiente caracter
                    }else{
                        divisions[divisionCount] = NULL;
                        wordStart = (char*)sentence + i + 1;
                        divisionCount ++;
                    }
                    

                }

                if(currentChar == '\0'){
                    break;
                }
            }


            
            float time = strtof(divisions[1], nullptr);
            float latitude = strtof(divisions[2], nullptr);
            char* NS_ind = divisions[3];
            float longitude = strtof(divisions[4], nullptr);
            char* EW_ind = divisions[5];
            int satellites = strtof(divisions[7], nullptr);
            float altitude = strtof(divisions[9], nullptr);
            char* altitde_unit = divisions[10];
            int totalSeconds = static_cast<int>(time); // Convert to integer (truncate decimal)
            int hours = (totalSeconds / 10000)+1; // Get the hours part (hhmm.ss -> hh)
            int minutes = (totalSeconds % 10000) / 100; // Get the minutes part (hhmm.ss -> mm)
            int seconds = totalSeconds % 100; // Get the remaining seconds (hhmm.ss -> ss)
                
            printf("GPS: #sats: %d  Lat(UTC): %f %s  Long(UTC): %f %s  Altitude: %f %s  %02d:%02d:%02d\n",satellites, latitude, NS_ind, longitude, EW_ind, altitude, altitde_unit, hours, minutes, seconds);
            for (int j = 0; j < divisionCount; j++) {
                    //printf("token: %s\n", divisions[j]);
                    free(divisions[j]); // Don't forget to free allocated memory
            }
        }

        if (counter_GPS == 2){
            counter_GPS = 0;
        }
    }
}

void location() {
    char sentence[120];
    int bytesRead = 0;
    

    while (GPS.readable()) {
        char c;
        if (GPS.read(&c, 1)) {
            if (c == '\n') {
                sentence[bytesRead] = '\0'; // Null-terminate the string
                parseAndProcessNMEA(sentence);
                bytesRead = 0;
            } else {
                sentence[bytesRead] = c;
                bytesRead++;
                if (bytesRead >= sizeof(sentence) - 1) {
                    // Buffer overflow, handle or discard the sentence
                    bytesRead = 0;
                }
            }
        }
    }
}

