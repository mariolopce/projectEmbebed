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
#define COMMAND_ADDRESS 0X80
#define TCS34725_ADDRESS 0x29<<1    // RGB sensor address
#define Si7021_ADDRESS 0x40<<1      // Temperature humidity address

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
 
float v_brightness;
float mean_brightness = 0;
float maximum_brightness = 0;
float minimum_brightness = 1000;
int counter_brightness = 0; 

float v_temp;
float mean_temperature = 0.0;
float maximum_temperature = 0.0;
float minimum_temperature = 1000.0;
int counter_temperature = 0;

float v_humidity;
float mean_humidity = 0.0;
float maximum_humidity = 0.0;
float minimum_humidity = 1000.0;

float v_soilMoisture;
float mean_soil = 0.0;
float maximum_soil = 0.0;
float minimum_soil = 1000.0;
int counter_soil = 0;


float timeGPS, latitude,  longitude, altitude;
char* NS_ind, *EW_ind, *altitude_unit;
int satellites, totalSeconds, hours, minutes, seconds;


extern bool mode; 
extern bool mode_ant;

I2C i2c(PB_9, PB_8); 
DigitalOut led(PB_7);  
BufferedSerial GPS(PA_9, PA_10, 9600);

AnalogIn soil(PA_0);
AnalogIn Brightness(PA_4);


void brightness();
void soilMoisture();
void tempAndHum();
void rgb();
void read_colour();
void location();
void parseSentenceGPS();
void reset_variables();
void print_result();
void print_stats();

int counter_GPS=0;
int counter_hour=0;

bool flag_30 = false;
bool repeat_30 = false;
Timeout to;

void count_30(){
    flag_30 = true;
    repeat_30 = false;
}


void measure(){
    
    //GPS.set_baud(9600);
    while(1){
        
        
        if (mode == false){
            // If TEST MODE --> Measures are taken each 2 seconds
            soilMoisture();
            brightness();
            tempAndHum();
            rgb();
            location();
            print_result();
            ThisThread::sleep_for(2000);
        }
        else {
            // If NORMAL MODE --> We check the limits, we show the maximum, minimum and average
            //                    and measures are taken each 30 seconds
            if (repeat_30 == false){
                to.attach_us(count_30, 5000000);
                repeat_30 = true;
            }
            if (flag_30 == true){
                soilMoisture();
                brightness();
                tempAndHum();
                rgb();
                location();
                print_result();
                
                if (v_brightness >= MAX_LIGHT || v_brightness <= MIN_LIGHT){
                    // YELLOW COLOUR
                    redLED = 1;
                    greenLED = 1;
                    blueLED = 0;
                }
                else if (v_soilMoisture >= MAX_SOIL || v_soilMoisture <= MIN_SOIL){
                    // CYAN COLOUR
                    redLED = 0;
                    greenLED = 1;
                    blueLED = 1;
                }
                else if (v_temp >= MAX_TEMPERATURE || v_temp <= MIN_TEMPERATURE) {
                    // PURPLE COLOUR
                    redLED = 1;
                    greenLED = 0;
                    blueLED = 1;
                }
                else if (v_humidity >= MAX_REL_HUMIDITY || v_humidity <= MIN_REL_HUMIDITY) {
                    // WHITE COLOUR
                    redLED = 1;
                    greenLED = 1;
                    blueLED = 1;
                }
                else {
                    // RGB LED switched off
                    redLED = 0;
                    greenLED = 0;
                    blueLED = 0; 
                }
                
                counter_hour++;
                if (counter_hour == 5){ // It should be 120
                    // Stats shown each 30 seconds * 120 --> 1 hour
                    mean_soil = mean_soil/counter_soil;
                    mean_brightness = mean_brightness/counter_brightness;
                    mean_temperature = mean_temperature/counter_temperature;
                    mean_humidity = mean_humidity/counter_temperature;

                    print_stats();
                    
                    counter_hour = 0;
                    reset_variables();
                    

                    
                }
                flag_30 = false;
            }
            
        }
    }
}

void print_result()
{
    printf("SOIL MOISTURE: %.1f%%\n", v_soilMoisture);
    printf("LIGHT: %.1f%%\n", v_brightness);
    printf("TEMP/HUM: Temperature: %.1f,     Relative humidity: %.1f\n", v_temp, v_humidity);
    printf("COLOR SENSOR: Clear: %d  Red: %d  Green: %d  Blue: %d\n", clear, red, green, blue);
    printf("GPS: #sats: %d  Lat(UTC): %f %s  Long(UTC): %f %s  Altitude: %f %s  %02d:%02d:%02d\n",satellites, latitude, NS_ind, longitude, EW_ind, altitude, altitude_unit, hours, minutes, seconds);
    printf("\n");
}

void print_stats()
{
    printf("RESULTS \n");
    printf("SOIL MOISTURE: Maximum: %f  Minimum: %f  Mean: %f\n", maximum_soil, minimum_soil, mean_soil);
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
    printf("\n");
}

void reset_variables(){
    // Function used to reset the stats variables for new computation each hour
    mean_brightness = 0;
    maximum_brightness = 0;
    minimum_brightness = 1000;
    mean_temperature = 0.0;
    maximum_temperature = 0.0;
    minimum_temperature = 1000.0;
    mean_humidity = 0;
    maximum_humidity = 0;
    minimum_humidity = 1000.0;
    mean_soil = 0;
    maximum_soil = 0;
    minimum_soil = 1000.0;
    counter_red = 0;
    counter_green = 0;
    counter_blue = 0;
    counter_brightness = 0;
    counter_temperature = 0;
    counter_soil = 0;

}

void soilMoisture(){
    // Analog sensor
    v_soilMoisture = soil.read() * 100;

    if (mode == true) {
        // Only if mode = NORMAL we update maximum, minimum and average values
        if (mode != mode_ant){
            mean_soil = 0;
            maximum_soil = 0;
            minimum_soil = 1000;
            mode_ant = mode;
         }
        counter_soil++;
        maximum_soil = max(maximum_soil,v_soilMoisture);
        minimum_soil = min(minimum_soil,v_soilMoisture);
        mean_soil = (mean_soil + v_soilMoisture);
        
    }
}

void brightness(){
    // Analog sensor
    v_brightness = Brightness.read() * 100;
    
    if (mode == true) {
        // Only if mode = NORMAL we update maximum, minimum and average values
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
    // I2C sensor
    char command[1];
    char data[2];
    // HUMIDITY
    command[0] = {0xE5};                        //for read humidity
    i2c.write(Si7021_ADDRESS, command, 1);
    i2c.read(Si7021_ADDRESS, data, 2);               // we save the result in two bytes (L and H)

    uint16_t h = (data[0] << 8) | data[1];      // MSB at [0]
    v_humidity = h * 125.0 / 65536 - 6.0;       // formula in datasheet
    // TEMPERATURE
    command[0] = {0xE3};
    i2c.write(Si7021_ADDRESS, command, 1);           // Send command for reading
    ThisThread::sleep_for(100ms);               // to perform the measurement                
    i2c.read(Si7021_ADDRESS, data, 2);               // we save the result in two bytes (L and H)
                
    uint16_t temperature = (data[0]) << 8 | data[1];    // MSB at [0]
    v_temp = ((175.72 * temperature) / 65536) - 46.85;  // formula in datasheet
    


    if (mode == true) {
        // Only if mode = NORMAL we update maximum, minimum and average values
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


uint16_t read_colour(uint8_t reg) {
    // Funtion used to read the value of a colour channel
    char colour_buffer[2];      // Array to save the value
    char colour_register[1];        // In this array it is saved the register of the low byte of each colour
    colour_register[0] = reg;
    i2c.write(TCS34725_ADDRESS, colour_register, 1);
    i2c.read(TCS34725_ADDRESS, colour_buffer, 2);
    return (colour_buffer[1] << 8) | colour_buffer[0];  
}

void rgb(){
    // LED in the sensor ON
    led=1;
    // Initialize TCS34725
    char configuration[2];
    configuration[0] = 0x00;              // 
    configuration[1] = 0x03;                         // PON (bit 0) and AEN (bit 1) set to 1
    i2c.write(TCS34725_ADDRESS, configuration, 2);
    
    //  To ensure the data is read correctly, 
    //  a two-byte read I2C transaction should be used with a read word protocol bit set in the command register

    clear = read_colour(TCS34725_CDATAL|COMMAND_ADDRESS);
    red = read_colour(TCS34725_RDATAL|COMMAND_ADDRESS);
    green = read_colour(TCS34725_GDATAL|COMMAND_ADDRESS);
    blue = read_colour(TCS34725_BDATAL|COMMAND_ADDRESS);


     if (mode == true) {
         // Only if mode = NORMAL we update maximum, minimum and average values
        if (mode != mode_ant){
            counter_red = 0;
            counter_green = 0;
            counter_blue = 0;
            mode_ant = mode;
         }
        
        if (max(red,max(blue,green))==red){             // If the maximum value is the red one 
            counter_red++;
        }
        else if (max(red,max(blue,green))==green) {     // If the maximum value is the green one
            counter_green++;
        }
        else if (max(red,max(blue,green))==blue) {      // If the maximum value is the blue one
            counter_blue++;
        }
    }
    


}

void parseSentenceGPS(const char* sentence) {
    if (strncmp(sentence, "$GPGGA,", 7) == 0) {
        // If the sentence does not start with "$GPGGA," we are not interested
        counter_GPS++;
        if (counter_GPS < 2){
            //  In order not to show twice the value of the GPS
            char* divisions[15];
            int divisionCount = 0;
            char *wordStart = (char*)sentence; // we point to address where it is saved the sentence
            
            int i =0;
            while(true){
                i++;
                char currentChar = sentence[i];    

                // Check for the comma (',') or end of string ('\0')
                if (currentChar == ',' || currentChar == '\0'){
                    int length = &sentence[i] - wordStart;          // Calculate the length of the data segment
                    // If the segment is non-empty, store it in divisions array
                    if(length > 0){
                        divisions[divisionCount] = (char*)malloc(length + 1);   // Save space for the segment with the end of string
                        strncpy(divisions[divisionCount], wordStart, length);   // Copy in divisions the segment
                        divisions[divisionCount][length] = '\0';                // Add the end of string
                        divisionCount++;

                        wordStart = (char*)sentence + i + 1; // Move to the next character

                    }else{
                        // If the segment is empty, store NULL in divisions array
                        divisions[divisionCount] = NULL;
                        wordStart = (char*)sentence + i + 1;
                        divisionCount ++;
                    }
                    

                }

                if(currentChar == '\0'){
                    break;
                }
            }


            
            timeGPS = strtof(divisions[1], nullptr);
            latitude = strtof(divisions[2], nullptr);
            NS_ind = divisions[3];
            longitude = strtof(divisions[4], nullptr);
            EW_ind = divisions[5];
            satellites = strtof(divisions[7], nullptr);
            altitude = strtof(divisions[9], nullptr);
            altitude_unit = divisions[10];
            totalSeconds = static_cast<int>(timeGPS);      // Convert to integer (truncate decimal)
            hours = (totalSeconds / 10000)+1;           // Get the hours part (hhmm.ss -> hh)
            minutes = (totalSeconds % 10000) / 100;     // Get the minutes part (hhmm.ss -> mm)
            seconds = totalSeconds % 100;               // Get the remaining seconds (hhmm.ss -> ss)
                
           
            
            for (int j = 0; j < divisionCount; j++) {     
                free(divisions[j]); // Free allocated memory
            }
        }

        if (counter_GPS == 2){
            counter_GPS = 0;
        }
    }
}

void location() {
    char sentence[200];
    int bytesRead = 0;  // To track the number of bytes stored in the sentence
    
    while (GPS.readable()) {
        char c;
        if (GPS.read(&c, 1)) {                  // Read one character at a time from the GPS
            if (c == '\n') {                    // If the character is the end of a sentence    
                sentence[bytesRead] = '\0';     // Null-terminate the string
                parseSentenceGPS(sentence);  // Parse the sentence
                bytesRead = 0;                  // Reset for the next sentence
            } else {
                sentence[bytesRead] = c;        // Store c in sentence
                bytesRead++;                    // Increment the counter of the index
                if (bytesRead >= sizeof(sentence) - 1) {
                    // Buffer overflow --> Discard the sentence
                    bytesRead = 0;
                }
            }
        }
    }
}



