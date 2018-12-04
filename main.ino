#include <Wire.h>
#include "image.cpp"
#define check_sec 60 //HOW MANY SECONDS BEFORE NEXT cycleBatt()
#define charge_thresh 14
#define max_in 15
#define dc_thresh 5

// A0 A2 A4 A6   PIN true/HIGH [coil is off]
// 2  3  4  5
// A1 A3 A5 A7   PIN false/LOW [coil is on]

//Digital 2,3,4,5 for switching, 10-11 for display
//A0-A7 battery 
//TwoWire (maybe sda11, scl12) for oled
//digital 6 for button

//         PIN LOW, COIL ON|PIN HIGH, COIL OFF
uint16_t charge[4][2] = {{0,0},  // A0,A1
                         {0,0},  // A2,A3
                         {0,0},  // A4,A5
                         {0,0}}; // A6,A7
                         
bool state[4] = {false, false, false, false};
//Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup(){
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(6, INPUT_PULLUP);
    for(uint8_t pin = 2; pin <= 5; pin++)
        pinMode(pin, OUTPUT);

    *digitalPinToPCMSK(6) |= bit (digitalPinToPCMSKbit(6));
    PCIFR  |= bit (digitalPinToPCICRbit(6));
    PCICR  |= bit (digitalPinToPCICRbit(6));
    
    //display setup
    //Wire.begin(11, 12);//needs more research, but TwoWire should suppoert software pins
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);// initialize with the I2C addr 0x3C (for the 128x32)
    //display.clearDisplay();

    //DRAW STARTUP IMAGE
    cycleBatt();
    delay(1000); //for people to see it

    //dispVoltages();
}

void loop(){
    for(int i = 0; i < check_sec; i++){
        delay(1000);
        Serial.println();
        for(int i = 0; i < 4; i++){
            Serial.print(state[i]);
            Serial.print(", ");
        }
        Serial.println();
        for(int i = 0; i < 4; i++){
            Serial.print(charge[i][0]);
            Serial.print("|");
            Serial.print(charge[i][1]);
            Serial.print(", ");
        }
        Serial.println();
    }
    
    //every minute switch, check/switch, switch and charge update
    //flash # of charged
    cycleBatt();
    //dispVoltages();
}

ISR(PCINT2_vect){
    cycleBatt();
}

void cycleBatt(){
    noInterrupts();
    updateCharge();

    //update charge array and swap battery if charged
    for(int i = 0; i < 4; i++){
        if(charge[i][0] <= dc_thresh && charge[i][1] <= dc_thresh)//if both disconnected turn coil off to save power/wear on coil idx#0
            state[i] = true;
        else if(charge[i][0]/1024.f*max_in >= charge_thresh && charge[i][1] <= dc_thresh)//idx#0 charged, swap to unplugged
            state[i] = false;//idx#1
        else if(charge[i][1]/1024.f*max_in >= charge_thresh && charge[i][0] <= dc_thresh)//idx#1 charged, swap to unplugged
            state[i] = true;//idx#0
        else if(charge[i][0] >= charge[i][1] && charge[i][0]/1024.f*max_in < charge_thresh) //if idx#0 batt highest+not charged
            state[i] = true;//idx#0
        else 
            state[i] = false;//idx#1

    }

    updatePowered();
    blinkCharged();
    interrupts();
}

void updateCharge(){
    //flip
    for(int i = 0; i < 4; i++)
        state[i] = !state[i];
    updatePowered();
    
    //updateCharges
    for(uint8_t i = 0; i < 4; i++)
        charge[i][(uint8_t)(state)] = analogRead(i*2+state);
        //reduced to one line
        /*if(!state[i])//check opposites hopefully
            charge[i][0] = analogRead(i*2);
        else
            charge[i][1] = analogRead(i*2+1);*/
    
    //flip
    for(int i = 0; i < 4; i++)
        state[i] = !state[i];
    updatePowered();

    //updateCharges
    for(uint8_t i = 0; i < 4; i++)
        charge[i][(uint8_t)(state)] = analogRead(i*2+state);
}

void updatePowered(){
    for(int i = 0; i < 4; i++)
        digitalWrite(i+2, state[i]);
    delay(50);//allow relays to swap
}

void blinkCharged(){
    //blink number of charged batteries
    uint8_t tot = 0, maximum = 8; //max can't be a variable name
    for(uint8_t i = 0; i < 4; i++)
        for(uint8_t j = 0; j < 2; j++){
            if(charge[i][j]/1024.f*max_in >= charge_thresh){
                tot++;
                digitalWrite(LED_BUILTIN, HIGH);
                delay(100);
                digitalWrite(LED_BUILTIN, LOW);
                delay(400);
            }else if(charge[i][j] < dc_thresh)//dont count disconnected
                maximum--;
        }
    //solid if all done
    if(tot >= maximum)
        digitalWrite(LED_BUILTIN, HIGH);
    else
        digitalWrite(LED_BUILTIN, LOW);
}

// void dispVoltages(){
//     display.clearDisplay();
//     display.setTextColor(WHITE);
//     display.setTextSize(1);
//     display.setCursor(0,0);
    
//     display.print(charge[0] + " " + charge[1] + " " + charge[2]);
//     display.setCursor(0,10);
//     display.print(charge[3] + " " + charge[4] + " " + charge[5]);
//     display.setCursor(0,20);
//     display.print(charge[6] + " " + charge[7]);

//     display.display();
// }

