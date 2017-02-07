# Air Quality Sensor
Using ESP8266-12E(3V) with I2C Slave Arduino(5V) Nano/Uno Plus MQ-2, MQ-7 and SHT21 Sensors Using 4 Way 3V to 5V Convertor


This is using an Interrupt Driven routine on the Arduino to refresh the data from the available ADC's


The sketches use 4 pins on both the Arduino(Reset,D2,A4+A5) and the ESP8266(0,2,4+5 (or 1+3 for -01))

The Air Quality Server -


This is a host server to provide access to the realtime data 

This also posts the results to Thinsgspeak from the data gathered

(A0)  MQ-2  Millivolt (2 Decimals)

(A1)  MQ-7  Voltage   (4 Decimals)

Temperature Degrees C (2 Decimals)

Humidity % RH      (2 Decimals)

ESP8266 Vcc Voltage   (2 Decimals)

(A2) I2C 3V Voltage   (2 Decimals)

(A3) I2C 5V Voltage   (2 Decimals)

I2C Error counter

   For ESP8266-01 Serial MUST Be Removed from the Air Quality Server Sketch
   
   or edited for No Reset and Serial.set_tx(2) for Serial
   
   with pin 0 generating the Interrupt to the Arduino
   
   
   Pin Numbers are Arduino interpretation.......
   
   =====================================================
   
   Arduino Pin____Esp Pin___ESP8266-01___Serial.set_tx(2);
   
   =====================================================
   
   Reset____________0_____________0_____________N/A
   
   D2______________2_____________2_____________0
   
   A4______________4_____________1_____________1
   
   A5______________5_____________3_____________3



Master ADC


This will provide a basic 6 channel oscilloscope(Very Basic)

A0 to A7(Not suitable for UNO) Data Values are displayed via SVG generated charts

Plus the display of the Last Voltage recorded

Please feel free to correct the timing,

I only needed the ADC's for the MQ sensors....

There is a Square Wave output on ESP8266 Pin 12 to allow the Arduino to be tested

This should display the 1Khz signal clearly on the ADC SVG Chart



