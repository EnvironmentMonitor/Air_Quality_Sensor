/* This provides some easy access to the Arduino ADC's

   ================================
   Arduino Pin  Esp Pin  ESP8266 01
   ================================
   Reset         0          0
   2             2          2
   A4            4          1
   A5            5          3
   

   
   All via 5V to 3V shifter......Arduino 5v side can take more Slaves......
*/
#include <Wire.h>

String StRingV;
String ArduinoID = "A1";  //  A1 Primary slave needs to be present @ address 8 !!!
byte   ArduinoAD = 8;
char msg[22];
volatile boolean RuN = false;



void GetData(){
    StRingV=ArduinoID;
    for(int i=0;i<4;i++){  //A2177077120105111111AA
     yield(); //delay(5);
     unsigned int sensorValue = analogRead(i);
     //delay(1);
     sensorValue=constrain(map(sensorValue, 0, 1023, 0, 999), 0, 999);
     if(sensorValue>=999){sensorValue=998;}
     //Serial.println(sensorValue);
     if(sensorValue<100){StRingV+="0";}
     if(sensorValue<10){StRingV+="0";}
     StRingV+=sensorValue;
     }
    for(int i=6;i<8;i++){  //A2177077120105111111AA
     yield(); //delay(5);
     unsigned int sensorValue = analogRead(i);
     //delay(1);
     sensorValue=constrain(map(sensorValue, 0, 1023, 0, 999), 0, 999);
     if(sensorValue>=999){sensorValue=998;}
     //Serial.println(sensorValue);
     if(sensorValue<100){StRingV+="0";}
     if(sensorValue<10){StRingV+="0";}
     StRingV+=sensorValue;
     } 
  StRingV+="AA"; // Close array, these chars are only written not read.....
  StRingV.toCharArray(msg, 22);
  //Serial.println(StRingV);
}

void ReadSensors() {
  Wire.write(msg);             // Response to Master Request
}

void StaRt() {
  RuN = true;
}

void setup() {
  Wire.begin(ArduinoAD);               // join i2c bus with address #8
  Wire.onRequest(ReadSensors); // register event
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), StaRt, FALLING);
  //Serial.begin(115200);
  //Serial.println("\r\n\r\n");

}

void loop() {
  if (RuN == true) {
  GetData();
  RuN = false;
  }
}
