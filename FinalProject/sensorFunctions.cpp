#include "mbed.h"
#include <cstdint>
#include <cstdio>
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
BufferedSerial gps_serial(PA_9, PA_10, 9600);
DigitalOut led(PA_8);


char c[256];
int value;
int counter_GPS = 0;

void brightness();
void tempAndHum();
void rgb();
void writeRegister();
void read16();
void gps();
void soilMoisture();




void measure(){
    value = 0;
    
    while(1){
        brightness();
        tempAndHum();
        rgb();
        soilMoisture();
        gps();
        printf("\n");
        ThisThread::sleep_for(2000);
        
    }
    
    
}


void soilMoisture(){
    AnalogIn soil(PA_0);
    float soilValue = 0;
    soilValue = soil.read() * 100;
    printf("Soil Moisture: %.1f\n", soilValue);
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





void parseAndProcessNMEA(const char* sentence) {
    if (strncmp(sentence, "$GPGGA,", 7) == 0) {
        counter_GPS++;
        if (counter_GPS < 2){
            printf("\nGPS SENTNECE: %s\n",sentence);
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

            for (int j = 0; j < divisionCount; j++) {
                printf("token: %s\n", divisions[j]);
                free(divisions[j]); // Don't forget to free allocated memory
            }
            
            /*
            int i = 0;
            while (i<15) {
                
                divisions[i] = token;
                printf("token: %s\n",  divisions[i]);
                token = strtok(NULL, ",");
                i++;
            }
            printf("Size: %d\n", i);

            float time = strtof(divisions[1], nullptr);
            float latitude = strtof(divisions[2], nullptr);
            char* NS_ind = divisions[3];
            float longitude = strtof(divisions[4], nullptr);
            char* EW_ind = divisions[5];
            int satellites = strtof(divisions[7], nullptr);
            float altitude = strtof(divisions[9], nullptr);
            char* altitde_unit = divisions[10];
            int totalSeconds = static_cast<int>(time); // Convert to integer (truncate decimal)
            int hours = totalSeconds / 10000; // Get the hours part (hhmm.ss -> hh)
            int minutes = (totalSeconds % 10000) / 100; // Get the minutes part (hhmm.ss -> mm)
            int seconds = totalSeconds % 100; // Get the remaining seconds (hhmm.ss -> ss)


            printf("GPS: #sats: %d  Lat(UTC): %f %s  Long(UTC): %f %s  Altitude: %f %s  %02d:%02d:%02d\n",satellites, latitude, NS_ind, longitude, EW_ind, altitude, altitde_unit, hours, minutes, seconds);
            */
        }

        if (counter_GPS == 2){
            counter_GPS = 0;
        }
    }
}

void gps() {
    char sentence[120];
    int bytesRead = 0;
    

    while (gps_serial.readable()) {
        char c;
        if (gps_serial.read(&c, 1)) {
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