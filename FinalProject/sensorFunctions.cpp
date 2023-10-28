#include "mbed.h"
#include <cstdint>
#include <stdint.h>



#define TCS34725_CDATAL 0x14
#define TCS34725_RDATAL 0x16
#define TCS34725_GDATAL 0x18
#define TCS34725_BDATAL 0x1A
const int COMMAND_ADDRESS=0X80;
const int TCS34725_ADDRESS = 0x29<<1;
extern uint16_t clear; 
extern uint16_t red; 
extern uint16_t green; 
extern uint16_t blue; 
 
extern float v_brightness;
extern float v_temp;
extern float v_humidity;

char data_L[1];
char data_H[1];
int redValue;

I2C i2c(PB_9, PB_8);
//BufferedSerial gps_serial(PA_9, PA_10, 9600);
DigitalOut led(PA_8);

char c[256];
int value;

void brightness();
void tempAndHum();
void rgb();
void writeRegister();
void read16();
void gps();




void measure(){
    value = 0;
    gps();
    /*
    while(1){
        brightness();
        tempAndHum();
        rgb();
        gps();
        printf("\n");
        ThisThread::sleep_for(2000);
        
    }*/
    
}

void brightness(){
    AnalogIn brightness(PA_4);
    v_brightness = brightness.read() * 100;
    printf("Brightness: %.1f\n", v_brightness);
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
    printf("Humidity: %.1f\n", v_humidity);


    command[0] = {0xE3};
    i2c.write(sensorAdd, command, 1); // Send command for reading
    ThisThread::sleep_for(100ms); // to perform the measurement                
    i2c.read(sensorAdd, data, 2); // get the temperature H and L
                
    uint16_t temperature = (data[0]) << 8 | data[1]; // MSB at [0]
    v_temp = ((175.72 * temperature) / 65536) - 46.85;   // formula in datasheet
    printf("Temperature: %.1f\n", v_temp);

}

void writeRegister(uint8_t reg, uint8_t value) {
    char data[2];
    data[0] = reg;
    data[1] = value;
    i2c.write(TCS34725_ADDRESS, data, 2);
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
    writeRegister(COMMAND_ADDRESS, 0x03);  // Enable the device
    wait_us(3000);  //POR QUE????
    
    clear = read16(TCS34725_CDATAL|COMMAND_ADDRESS);
    red = read16(TCS34725_RDATAL|COMMAND_ADDRESS);
    green = read16(TCS34725_GDATAL|COMMAND_ADDRESS);
    blue = read16(TCS34725_BDATAL|COMMAND_ADDRESS);


    printf("Rojo: %d\n", red);
    printf("Green: %d\n", green);
    printf("Blue: %d\n", blue);
    printf("Clear: %d\n", clear);

}


void gps(){
    value = 5;
    //serial.set_baud(9600);
    BufferedSerial gps_serial(PA_9, PA_10, 9600);
    char c[256];
    while (true) {
        
        if(gps_serial.readable()){
            gps_serial.read(c, 256);
            printf("\n\n%s", c);
        }

        if (strncmp(c, "$PGGA", 5) == true) {
            printf("COINCIDE");
        }

        /*
        if (strncmp(c, "GPGGA", 5) == 0) {
            // Split the sentence using ',' as a delimiter
            char* tokens[15];
            char* token = strtok((char*)c, ",");
            int i = 0;
            while (token != nullptr) {
                tokens[i] = token;
                token = strtok(nullptr, ",");
                i++;
            }

            printf("GPS Hora: %s\n", token[0]);
        }
        //printf("\n%s\n", c);
        */
        
        
    }


}

/*

void gps(){
    value = 5;
    
    char c[256];
    if(gps_serial.readable()){
        gps_serial.read(c, 256);
        printf("%s", c);
        
        if (strncmp(c, "$GPGGA", 6) == 0) {
            // Split the sentence using ',' as a delimiter
            char* tokens[15];
            char* token = strtok((char*)c, ",");
            int i = 0;
            while (token != nullptr) {
                tokens[i] = token;
                token = strtok(nullptr, ",");
                i++;
            }

            printf("GPS Hora: %s\n", &token[0]);
        }
        printf("\n%s\n", c);
        
        

    }

}
*/



