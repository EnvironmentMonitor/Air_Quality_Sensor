/* This provides some easy access to the Arduino peripherals and Libraries via a 16 byte transfer
   run any sketch and include this snippet to provide readings to the ESP8266 Via I2C using the array.
   ESP8266 connected to I2C via pins 4(SDA) and 5(SCL), the SHT21 is directly connected to ESP8266 pins
   Arduino pins 4 and 5 are connected via 5v to 3v converter to the ESP8266 pins 4 and 5.
   The 16 bytes are arranged - bytes 0+1 are a 2 char ID, 3 byte blocks 2-4, 5-7, 8-11, 12-14 are Arduino A0 to A3 reading
   presenting data as byte allows very simple transfer.
   ESP8266 pin0 Used to Reset this Arduino and ESP8266 pin2 to send Request for data (RTS) to this Arduino.....

   ================================
   Arduino Pin  Esp Pin  ESP8266 01
   ================================
   Reset         0          0
   2             2          2
   A4            4          1
   A5            5          3
   
For ESP8266-01 Serial MUST Be Removed from the ESP8266 Master Sketch !!!

Optional Ultrasound Ping commented out add 3 chars to transfer....
   
   All via 5V to 3V shifter......Arduino 5v side can take more Slaves......

#include <Wire.h>
#include <NewPing.h>

#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
*/
#include <Wire.h>
String StRingV;
String ArduinoID = "A1";  //  A1 Primary slave needs to be present @ address 8 !!!
byte   ArduinoAD = 8;
char msg[32];
unsigned int sensorValue;
volatile boolean RuN = false;


void GetData(){
    StRingV=ArduinoID;
    for(int i=0;i<4;i++){  //A2177077120105A
     yield();
     sensorValue = analogRead(i);
     if(sensorValue>=999){sensorValue=998;}
     //Serial.println(sensorValue);
     if(sensorValue<100){StRingV+="0";}
     if(sensorValue<10){StRingV+="0";}
     StRingV+=sensorValue;
     } 
 /*  // 29ms should be the shortest delay between pings.
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  sensorValue=uS / US_ROUNDTRIP_CM;
  if(sensorValue<100){StRingV+="0";}
  if(sensorValue<10){StRingV+="0";}  
  StRingV+=sensorValue;
*/
  StRingV+="AA"; // Close array, these chars are only written not read.....
  StRingV.toCharArray(msg, 16);
  //Serial.println(StRingV);
}

void ReadSensors() {
  Wire.write(msg);             // Response to Master Request
}

void StaRt() {
  RuN = true;
}

void setup() {
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);       // 5V Reference
  pinMode(12, OUTPUT);
  analogWrite(12, 127);        // Square Wave
  Wire.begin(ArduinoAD);       // join i2c bus with address #8
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
