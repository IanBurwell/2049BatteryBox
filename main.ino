#include <Wire.h>

#define check_sec 200 
#define charge_thresh 0.9
#define max_in 1000.f
#define dc_thresh 15
//may be that tru/false swapped
// A0 A2 A4 A6   true/HIGH [0]
// 2  3  4  5
// A1 A3 A5 A7   false/LOW [1]

//Digital 2,3,4,5 for switching, 10-11 for display
//A0-A7 battery 
//TwoWire (maybe sda11, scl12) for oled
//digital 6 for button
//                    lower|higher
uint16_t charge[4][2] = {{0,0},
                         {0,0},
                         {0,0},
                         {0,0}};
bool state[4] = {false, false, false, false};
//Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup(){
    Serial.begin(9600);
    pinMode(13, OUTPUT);
    pinMode(6, INPUT_PULLUP);
    for(int pin = 2; pin <= 5; pin++)
        pinMode(pin, OUTPUT);
    for(int i = 0; i < 4; i++)
        digitalWrite(i+2, state[i]);

    //display setup
    //Wire.begin(11, 12);//needs more research, but TwoWire should suppoert software pins
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);// initialize with the I2C addr 0x3C (for the 128x32)
    //display.clearDisplay();


}

double lastCheck = millis();
double lastUpdate = millis();
void loop(){
    //every 10 seconds switch, check/switch, switch and charge update
    //flash # of charged
    if(digitalRead(6) == LOW || millis() - lastCheck >= check_sec*1000UL){
        lastCheck = millis();
        cycleBatt();

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

    //debug
    if(millis() - lastUpdate >= 5000){
        //dispVoltages();
    }

}

void cycleBatt(){
        updateCharge();

        //update charge array and swap battery if charged
        for(int i = 0; i < 4; i++){
            if(charge[i][0] <= dc_thresh && 
               charge[i][1] <= dc_thresh)//if both disconnected default idx#1
                state[i] = true;
            else if(charge[i][0]/max_in >= charge_thresh && 
                    charge[i][1] <= dc_thresh)//idx#0 charged, swap to unplugged
                state[i] = false;//idx#1
            else if(charge[i][1]/max_in >= charge_thresh && 
                    charge[i][0] <= dc_thresh)//idx#1 charged, swap to unplugged
                state[i] = true;//idx#0
            else if(charge[i][0] >= charge[i][1] && 
                    charge[i][0]/max_in < charge_thresh) //if idx#0 batt highest+not charged
                state[i] = true;//idx#0
            else 
                state[i] = false;//idx#1
        }

        updatePowered();
        blinkCharged();
}

void updateCharge(){
    //flip
    for(int i = 0; i < 4; i++)
        state[i] = !state[i];
    updatePowered();
    
    //updateCharges
    for(int i = 0; i < 4; i++){
        if(!state[i])//check opposites hopefully
            charge[i][0] = analogRead(i*2);
        else
            charge[i][1] = analogRead(i*2+1);
    }
    
    //flip
    for(int i = 0; i < 4; i++)
        state[i] = !state[i];
    updatePowered();

    //updateCharges
    for(int i = 0; i < 4; i++){
        if(!state[i])//check opposites hopefully
            charge[i][0] = analogRead(i*2);
        else
            charge[i][1] = analogRead(i*2+1);
    }
}

void updatePowered(){
    for(int i = 0; i < 4; i++)
        digitalWrite(i+2, state[i]);
    delay(100);//allow relays to swap
}

void blinkCharged(){
    //blink number of charged batteries
    uint8_t tot = 0, max = 8;
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 2; j++){
            if(charge[i][j]/max_in >= charge_thresh){
                tot++;
                digitalWrite(13, HIGH);
                delay(100);
                digitalWrite(13, LOW);
                delay(400);
            }else if(charge[i][j] < dc_thresh)//dont count disconnected
                max--;
        }
    //solid if all done
    if(tot >= max)
        digitalWrite(13, HIGH);
    else
        digitalWrite(13, LOW);
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

