#include "mbed.h"
#include "USBSerial.h"
#include <I2C.h>

#define LEDDIR (uint32_t*) 0x50000514 
#define SETTEMPERATURE (1UL << 8)
#define SETPROXIMITY (1UL << 8)

unsigned int readReg = 0xEF; //Read and write registers for temperature sensor
unsigned int writeReg = 0xEE; 

unsigned int readRegProx = (0x39<<1) + 1; //Read and write registers for proximity sensor
unsigned int writeRegProx = 0x39<<1;

Ticker interruptTicker;
USBSerial test;
Thread thread, thread1, thread2;
EventFlags PTEvent;
I2C i2c(p31, p2); //(SDA, SCL)
DigitalOut port22(p22);
DigitalOut pullupResistor(p32); //Pin 32 = P1_0
DigitalOut redLED(LED2);
DigitalOut blueLED(LED4);
DigitalOut greenLED(LED3);


I2C colors(I2C_SDA1, I2C_SCL1); //p0.14, p0.15
DigitalOut SetHigh(P1_0); //P1.0
DigitalOut SetHigh1(p22); //GPIO P0.22;


void read_temperature(){
    char temperatureData[8] = {0, 0, 0, 0, 0, 0, 0, 0}; //char == uint8
    unsigned short AC5, AC6; //Initializing constants and variables
    short MC, MD;
    long X1 = 0, X2 = 0, B5 = 0, currentT = 0, setT = 0;
    uint16_t MSB, LSB;
    int i = 0;

    //string command;
    //scanf("%s", command);
    //setT = PLACEHOLDER FOR COMMAND ENTERED; //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temperatureData[0] = 0xD0;
    i2c.write(writeReg, temperatureData, 1, true); //Send 0xD0 = 208
    i2c.read(readReg, temperatureData, 1); //Expect a 0x55 = 85 back

    //test.printf("1: Array[0] = %i \r\n\r\n", temperatureData[0]); //Check for signal from sensor

    if(temperatureData[0] == 85){

        //AC5
        temperatureData[0] = 0xB2;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        MSB = temperatureData[0];
        temperatureData[0] = 0xB3;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        LSB = temperatureData[0];
        AC5 = ((LSB<<8)|MSB);

        //AC6
        temperatureData[0] = 0xB4;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        MSB = temperatureData[0];
        temperatureData[0] = 0xB5;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        LSB = temperatureData[0];
        AC6 = ((MSB<<8)|LSB);

        //MC
        temperatureData[0] = 0xBC;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        MSB = temperatureData[0];
        temperatureData[0] = 0xBD;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        LSB = temperatureData[0];
        MC = ((MSB<<8)|LSB);

        //MD
        temperatureData[0] = 0xBE;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        MSB = temperatureData[0];
        temperatureData[0] = 0xBF;
        i2c.write(writeReg, temperatureData, 1, true);
        i2c.read(readReg, temperatureData, 1);
        LSB = temperatureData[0];
        MD = ((MSB<<8)|LSB);

        while(true){
            int waitTemp = PTEvent.wait_any(SETTEMPERATURE); //Wait for event flag
        
            temperatureData[0] = 0xF4;
            temperatureData[1] = 0x2E;
            i2c.write(writeReg, (const char *)temperatureData, 2); //Send 0xF4 = 244 and 0x2E = 46 to start temperature reading

            thread_sleep_for(5); //Wait for 4.5 ~= 5 milliseconds

            temperatureData[0] = 0xF6;
            i2c.write(writeReg, (const char *)temperatureData, 1, true); //Send 0xF6 = 246 to read MSB
            i2c.read(readReg, temperatureData, 1);

            MSB = temperatureData[0];

            temperatureData[0] = 0xF7; //Send 0xF7 = 247 to read LSB
            i2c.write(writeReg, (const char *)temperatureData, 1, true);
            i2c.read(readReg, temperatureData, 1);

            LSB = temperatureData[0];
            currentT = ((MSB<<8)|LSB); //Combine two bytes

            X1 = (currentT - AC6) * (AC5 / (32768));
            X2 = (MC * 2048) / (X1 + MD);
            B5 = X1 + X2;
            currentT = (B5 + 8)/16;
            currentT = currentT/10;

            setT = 40; //FOR TESTING ONLY

            test.printf("Reading temperature: %i degrees C\r\n\r\n", currentT);

            /* //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            scanf("%s", command);
            
            if(FARENHEIT COMMAND == TRUE){ 
                T = (1.8 * T) + 32;
                test.printf("Reading temperature: %i degrees F\r\n\r\n", T);
            }
            else{
                test.printf("Reading temperature: %i degrees C\r\n\r\n", T);
            }
            if(FARENEIT COMMAND == TRUE){
                if(currentT >= (setT + 1)){ //If detected temp is higher than set temp, LED = red
                    greenLED = 1;
                    blueLED = 1;
                    redLED = 0;
                }
                else if(currentT <= (setT - 1)){ //If detected temp is lower than set temp, LED = blue
                    greenLED = 1;
                    redLED = 1;
                    blueLED = 0;
                }
                else if (currentT > (setT - 1) || currentT < (setT + 1)){ //If detected temp is set temp, LED = green
                    redLED = 1;
                    blueLED = 1;
                    greenLED = 0;
                }
            }
            else if(CELCIUS COMMAND == TRUE){*/
                if(currentT >= (setT + 0.5)){ //If detected temp is higher than set temp, LED = red
                    greenLED = 1;
                    blueLED = 1;
                    redLED = 0;
                }
                else if(currentT <= (setT - 0.5)){ //If detected temp is lower than set temp, LED = blue
                    greenLED = 1;
                    redLED = 1;
                    blueLED = 0;
                }
                else if (currentT > (setT - 0.5) || currentT < (setT + 0.5)){ //If detected temp is set temp, LED = green
                    redLED = 1;
                    blueLED = 1;
                    greenLED = 0;
                }/*
            }
            */
        }
    }
}

void proximity_sensor(){
    char Data[8] = {0, 0, 0, 0, 0, 0, 0, 0}; //char == uint8
    int i = 0;
    long setProx = 0; //Set to result from speech to text

    //setProx = result; //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    thread_sleep_for(10);

    Data[0] = 0x92; //146
    i2c.write(writeRegProx, Data, 1, true); //Check ID
    i2c.read(readRegProx, Data, 1); //Expect a 0xAB = 171 back

    //test.printf("The sensor ID is: %d\r\n\r\n", Data[0]); //Check for connection

    if(Data[0] == 171){
            Data[0] = 0x90; //Increasing distance
            Data[1] = 0x30;
            i2c.write(writeRegProx, (const char *)Data, 2);

            Data[0] = 0x8F; //Increasing gain = improves readings of distant signals
            Data[1] = 0x0C;
            i2c.write(writeRegProx, (const char *)Data, 2);

        while(true){
            int waitTemp = PTEvent.wait_any(SETPROXIMITY); //Wait for event flag, MIGHT need to be before while loop
            thread_sleep_for(50); //5 millisecond buffer

            Data[0] = 0x80; //Send start signal
            Data[1] = 0x25;
            i2c.write(writeRegProx, (const char *)Data, 2);

            thread_sleep_for(15); //Additional buffer time

            Data[0] = 0x9C; //Read data obtained
            i2c.write(writeRegProx, (const char *)Data, 1, true);
            i2c.read(readRegProx, Data, 1);

            //test.printf("PDATA is: %d\r\n", Data[i]); //Data from chip

            if((Data[0] < (setProx + 1)) && (Data[0] > (setProx - 1))) { //Print if detected distance is close to set distance
                test.printf("%i cm reached\r\n", setProx);
            }
        }
    }
}

void color_sensor(){
   
   // MyMessage.printf("Shift write %d\n\r", writeaddr);
   // MyMessage.printf("Shift read %d\n\r", readaddr);

    uint8_t data[2];
    char hold[1];
    char test1; //test ints to see if I am writing or reading properly
    char test2;

    uint16_t Red = 0;
    uint16_t Blue = 0;
    uint16_t Green = 0;
    uint16_t Clear = 0;

    uint16_t MSB = 0;
    uint16_t LSB = 0;
    uint16_t RedCombo = 0;
    uint16_t GreenCombo = 0;
    uint16_t BlueCombo = 0;
    uint16_t ClearCombo = 0;


    hold[0] = 0x00;
    data[0] = 0x92; //address of the ID register, output says 0xA8

    thread_sleep_for(1000);
    test1 = colors.write(writeaddr, (const char*) data, 1, true);
    test2 = colors.read(readaddr, hold, 1);
    //MyMessage.printf("is it writing properly? %d\n\r", test1);
    //MyMessage.printf("is it reading properly? %d\n\r", test2);
   // MyMessage.printf("Communication with sensor established\n\r");
   // MyMessage.printf("Is the output 0xA8? %d\n\r", hold[0]); //A8 = 168, AB = 171

//Turning on the sensor
    data[0] = 0x80; //write to this address
    data[1] = 0x13; //what is being written to address
    colors.write(writeaddr, (const char*) data, 2, true);
   // MyMessage.printf("Power to sensor on\n\r");


    while(true)
    {
    //Reading red lower and upper bit
        thread_sleep_for(100);
        data[0] = 0x96;
        test1 = colors.write(writeaddr, (const char*) data, 1, true);
        test2 = colors.read(readaddr, hold, 1, false);
        MSB = hold[0];
        

        data[0] = 0x97;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        LSB = hold[0];
        RedCombo = ((MSB<<8)|LSB);
   // MyMessage.printf("Red Ouptut: %d\n\r", RedCombo);


    //Reading Green lower and upper bit
        thread_sleep_for(100);
        data[0] = 0x98;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        MSB = hold[0];

        data[0] = 0x99;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        LSB = hold[0];
        GreenCombo = ((MSB<<8)|LSB);
      // MyMessage.printf("Green Ouptut: %d\n\r", GreenCombo);

        //Reading Blue lower and upper bit
        thread_sleep_for(100);
        data[0] = 0x9A;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        MSB = hold[0];

        data[0] = 0x9B;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        LSB = hold[0];
        BlueCombo = ((MSB<<8)|LSB);
     // MyMessage.printf("Blue Ouptut: %d\n\r", BlueCombo);

        //Reading clear lower and upper bit
        thread_sleep_for(100);
        data[0] = 0x94;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        MSB = hold[0];

        data[0] = 0x95;
        colors.write(writeaddr, (const char*) data, 1, true);
        colors.read(readaddr, hold, 1, false);
        LSB = hold[0];
        ClearCombo = ((MSB<<8)|LSB);
    

    }
}

void setEFlag(){ //Send event flag to read_temperature
    //if(result == 'Activate temperature sensor'){
        PTEvent.set(SETTEMPERATURE);
    //}
    //else if(result == 'Activate proximity sensor'){
    /*    PTEvent.set(SETPROXIMITY);
    }
    else if(result == 'Activate color sensor'){
     PTEvent.set(SETCOLOR);
    }
    else if(result == null){
        thread_sleep_for(1);
    }
    //else(){
        test.printf("Please activate a valid sensor\r\n\n");
        test.printf("VALID COMMANDS:\r\n");
        test.printf("'Activate temperature sensor'\r\n'Activate proximity sensor'\r\n'Activate color sensor'\r\n");
    //}*/
}

int main() {
    thread.start(read_temperature);
    thread1.start(proximity_sensor);
    thread2.start(color_sensor);
	//*LEDDIR = *LEDDIR | (1 << 6);
    //*LEDDIR = *LEDDIR | (1 << 16);
    //*LEDDIR = *LEDDIR | (1 << 24);

    redLED = 1; //Turns LED off when first setting bits, causes a short blink
    blueLED = 1;
    greenLED = 1;

    pullupResistor = 1;
    port22 = 1;

    interruptTicker.attach(&setEFlag, 1.0); //Check for command every 1 second, may need to slow down

    while (true) {
        thread_sleep_for(1000);
    }
}
