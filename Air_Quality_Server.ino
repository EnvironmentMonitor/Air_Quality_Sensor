
/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* 
   This provides some easy access to the Arduino peripherals and Libraries via a 16 byte Int->String->Int transfer
   ESP8266 connected to I2C via pins 4(SDA) and 5(SCL), the SHT21 is directly connected to ESP8266 pins
   Arduino pins A4 and A5 are connected via 5v to 3v converter to the ESP8266 pins 4 and 5.
   The 16 bytes are arranged - bytes 0+1 are a 2 char ID, 3 byte blocks 2-4, 5-7, 8-11, 12-14 are Arduino A0 to A3 reading, 15 terminator
   presenting data as byte allows very simple transfer.
   ESP8266 pin0 Used to Reset the Arduino and ESP8266 pin2 to send Request for data (RTS) assigned to Arduino interrupt 0 (D2).....

   Webpages with SVG Charts on / for Sensors and /diag for Temperature, Internals and Counters...........

   The Arduino data fetch routine takes about 12ms to populate msg[] Slave Sketch at the bottom of this Sketch

   The option to use Multiple I2C busses can be used by initialising Each Bus and Device before Use....3 I2C Buses on 5 pins.....

   Wire.begin(4,5);
   Wire.beginTransmission(8);   // Address 8 on Bus 0
   Wire.requestFrom(8, 16);
   Wire.endTransmission();
   delay(50);
   Wire.begin(1,3);             // Use this for ESP8266-01 BUT No Serial !!!!! Use LCD For Wiring/Adaptation Debug......
   Wire.beginTransmission(8);   // Address 8 on Bus 1
   Wire.requestFrom(8, 16);
   Wire.endTransmission();
   delay(50);
   myHTU21D.begin();            // Start Sensor 1 Bus 1
   delay(50);
   Hum = myHTU21D.readCompensatedHumidity();  
   Temp = myHTU21D.readTemperature();
   delay(50);
   Wire.begin(12,5);            // Option to Share a Clock pin
   Wire.beginTransmission(8);   // Address 8 on Bus 2
   Wire.requestFrom(8, 16);
   Wire.endTransmission();
   delay(50);
   myHTU21D.begin();            // Start Sensor 2 Bus 2
   delay(50);
   Hum = myHTU21D.readCompensatedHumidity();
   Temp = myHTU21D.readTemperature();

   
   Pin Numbers are Arduino interpretation.......
   ================================
   Arduino Pin  Esp Pin  ESP8266 01
   ================================
   Reset         0          0
   D2            2          2
   A4            4          1
   A5            5          3
   
   For ESP8266-01 Serial MUST Be Removed from the ESP8266 Master Sketch !!!
   
   All via 4 way 5V to 3V shifter......Arduino 5v side can take many more Slaves......

   Log the data to Spiffs or SDCard, in the example below I have used 3 SRAM Array's to feed the 3 SVG Charts
   adapt these to feed SPIFFS or SD Recording.........
   Thingspeak Channel........  https://thingspeak.com/channels/209159
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
//#include "ThingSpeak.h"
#include "HTU21D.h"

HTU21D myHTU21D;

char ssid[] = "YourSSID";      // your network SSID (name) 
char pass[] = "YourPassword";  // your network password




const char * myWriteAPIKey = "YourWriteKey";
const char* hostts = "api.thingspeak.com";
unsigned long ulNextTSMeas_ms=30000;

extern "C" 
{
#include "user_interface.h"
}

ADC_MODE(ADC_VCC);
const int led = 16;
int I1,y,y2,iicdata,SensInt,FCoUnt,ReSet,sensorReading,TSMeasCount,EnD,ArrAy;
int *pfTemp;
int *pfTemp1;
int *pfTemp2;
int *aDcData;
int COAlrm=0;
int SmAlrm=0;

unsigned long sAcnt=0;
unsigned long cAcnt=0;
unsigned long sAcnt2=0;
unsigned long cAcnt2=0;
unsigned long ulMeasCount=0;
unsigned long ulNextMeas_ms=0;
unsigned long ulReqcount,ulEnd,CoUnt;
float pfVcC,SensF,Hum,Temp;
String duration1,New0,New1,New2,New3;
char hostString[16] = {0};
char aRRaY[16] = {0};

int    ScAle = 1;    // Scale the X axis of the graph.....
int     siZe = 450;  // The Width of the SVG chart & size of arrays......
int RTSDelay = 25;   // Set this for the response time of Your Arduino Data Fetch routine......
int ReFresh  = 500;  // Set this for the delay between I2C Requests, min 50ms......Server needs space to run....

ESP8266WebServer server(80);
WiFiClient client;
IPAddress ServerIP; 



void ThingPost(){
  pfVcC =  ESP.getVcc(); 
// Set TS Post data from array's.....
  float sensorReading1 = (aDcData[4] * (5.0 / 1023.0))*1000;
  //if(sensorReading1>1000){sensorReading1=sensorReading1/1000;}
  float sensorReading2 = (aDcData[5] * (5.0 / 1023.0))*1000;
 // if(sensorReading2>1000){sensorReading2=sensorReading2/1000, 3;}
  float sensorReading3 = aDcData[6] * (5.0 / 1023.0);
  float sensorReading4 = aDcData[7] * (5.0 / 1023.0);
//if(sensorReading1>=15){sensorReading1-=15;}else{sensorReading1=0;}
//if(sensorReading2>=150){sensorReading2-=150;}else{sensorReading2=0;}
//Serial.println(String(sensorReading1) + " : " + String(sensorReading2/1000, 4) + " : " + String(sensorReading3) + " : " + String(sensorReading4) + " : Time : " + duration1 );  
// Use WiFiClient class to create Thingspeak Post
  WiFiClient cli;
  if (!cli.connect(hostts, 80)) {
    return;
  }
  String url = "/update?key=";
  url += myWriteAPIKey;
  url += "&field1=";
  url += sensorReading1;
  url += "&field2=";
  url += String(sensorReading2/1000, 4);
  url += "&field3=";
  url += Temp;
  url += "&field4=";
  url += Hum;
  url += "&field5=";
  url += pfVcC/1000, 2;
  url += "&field6=";
  url += sensorReading3;
  url += "&field7=";
  url += sensorReading4;
  url += "&field8=";
  url += FCoUnt+(ReSet * 10);
// This will send the request to the server
  cli.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + hostts + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(20);
  cli.stop();

 
  
}





void GetNano(){
  int lOop=0;
  New0="";
  New2="";
// ================================== Activate Arduino Interrupt, only 1 Interrupt needed, supply to all Slaves ============================

  digitalWrite(2, LOW);       // When Set Request To Send Active(interrupt FALLING) Slave will refresh data....
  delay(RTSDelay);            // This will make sure the Arduino is in the loop with fresh data......  
  digitalWrite(2, HIGH);      // Set Request idle fresh data now in msg[]....
// ================================== DE-Activate Arduino Interrupt & Start of Arduino 1 I2C transfer ======================================
  Wire.beginTransmission(8);
  Wire.requestFrom(8, 16);    // request 16 bytes from slave device #8 bus 0 (Arduino Mini Pro costs £8 for 5 from China)
  while (Wire.available()) {  // slave may send more Bytes than requested, embedded Respose Bytes - 8:15
  char c = Wire.read();       // receive a byte as character 
  New0+=c;
  }
  Wire.endTransmission();
lOop=0;
New1=New0.substring(0,2);    // Very Simple error checking - error if not detected........
if(New1=="A1"){   
iicdata=4;
for(int i=2;i<12;i+=3){
char inChar = New0.charAt(i+2);
 int newt=inChar-48;        // Convert Numerical Char to int.....
     inChar = New0.charAt(i+1);
 int newt1=inChar-48; 
     inChar = New0.charAt(i);
 int newt2=inChar-48;
 int newt3=(newt2*100)+(newt1*10)+newt;
 if(newt3==998){newt3+=25;}
 aDcData[iicdata]=newt3;
 iicdata++;
 }
}else{ FCoUnt++; }
   Hum = myHTU21D.readCompensatedHumidity();
   Temp = myHTU21D.readTemperature();
   CoUnt++;
   ulMeasCount++;
   if(ulMeasCount>siZe){ulMeasCount=1;}
   pfTemp[ulMeasCount] = Temp;
   pfTemp1[ulMeasCount] = aDcData[0];
   pfTemp2[ulMeasCount] = aDcData[4];
   pfVcC =  ESP.getVcc(); 
   duration1 = "";
   int hr,mn,st;
   st = millis() / 1000;
   mn = st / 60;
   hr = st / 3600;
   st = st - mn * 60;
   mn = mn - hr * 60;
   if (hr<10) {duration1 += ("0");}
   duration1 += (hr);
   duration1 += (":");
   if (mn<10) {duration1 += ("0");}
   duration1 += (mn);
   duration1 += (":");
   if (st<10) {duration1 += ("0");}
   duration1 += (st);
}




void AQS()
  { 
    String PageStr="";
    pfVcC = ESP.getVcc();
    digitalWrite ( led, 1 );
    WiFiClient client = server.client(); 
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 20");        // refresh the page automatically every 30 sec
          client.println();
          PageStr+=("<!DOCTYPE HTML>");
          PageStr+=("<html><title>Environment Monitor AQS</title>");            
          client.println(PageStr);
          PageStr="";          
          PageStr+=("<div style=\"float:left; text-align: center; \"><font color=\"#000000\"><body bgcolor=\"#a0dFfe\"><h1><p>Environment Monitor<BR>Air Quality Sensors</h1></p><br>");        
          PageStr+=("MQ-2 (Arduino 1) Trend<BR><img src=\"/test1.svg\" />");
          if(New3=="A2"){
            PageStr+=("<BR>MQ-2 (Arduino 2) Trend<BR><img src=\"/test2.svg\" />");
            client.println(PageStr);
            PageStr="";
          }
          PageStr+=("<BR><a href=\"/diag\">AQS System Info Page</a>");
          PageStr+=("<BR>");
           client.println(PageStr);
          PageStr="<div style=\"float:inherit; text-align: center; \"> <h2>";           
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 2; analogChannel++) {
            if(analogChannel==0){
              SensF = (aDcData[4] * (5.0 / 1023.0))*1000;
              PageStr+=("<BR> <h2> Arduino One<BR> MQ-2 Alarm Count = ");
              PageStr+=(sAcnt);
              PageStr+=("<BR> CH4/Smoke");
              PageStr+=(" Value is : ");
              PageStr+=String(SensF);
              PageStr+=(" mV<BR> ");
            }else{
              SensF = (aDcData[5] * (5.0 / 1023.0))*1000;
              PageStr+=(" MQ-7 Alarm Count = ");
              PageStr+=(cAcnt);  
              PageStr+=("<BR> CO");
              PageStr+=(" Value is : ");
              PageStr+=String(SensF/1000, 4);
              PageStr+=(" V<BR> ");
            }
          }
          client.println(PageStr);
          PageStr="</div>";
          PageStr+=("</h2></div><BR><BR></html>");
          client.println(PageStr);
    
    PageStr="";            
    ulReqcount++;
    // and stop the client
    delay(1);
    client.stop();    
    digitalWrite ( led, 0 );
    client.flush();
  }


void diag()
  {   digitalWrite ( led, 1 );
      WiFiClient client = server.client(); 
     pfVcC = ESP.getVcc();
     long int spdcount = ESP.getCycleCount();
     delay(1);
     long int spdcount1 = ESP.getCycleCount();
     long int speedcnt = spdcount1-spdcount; 
     FlashMode_t ideMode = ESP.getFlashChipMode();
     ulReqcount++;
                                duration1 = " ";
                                int hr,mn,st;
                                st = millis() / 1000;
                                mn = st / 60;
                                hr = st / 3600;
                                st = st - mn * 60;
                                mn = mn - hr * 60;
                                if (hr<10) {duration1 += ("0");}
                                duration1 += (hr);
                                duration1 += (":");
                                if (mn<10) {duration1 += ("0");}
                                duration1 += (mn);
                                duration1 += (":");
                                if (st<10) {duration1 += ("0");}
                                duration1 += (st);     
                                client.println("HTTP/1.1 200 OK"); 
                                client.println("Content-Type: text/html");
                                client.println("Connection: close");
                                client.println("Refresh: 20");        // refresh the page automatically every 20 sec
                                client.println();
                                String PageStr="";
                                PageStr+=("<!DOCTYPE HTML>");
                                PageStr+=("<html><head><title>Environment Monitor AQS</title></head><body>");// 
                                PageStr+=("<div style=\"float:left; \">");
                                PageStr+=("<p>Temperature Trend<BR><img src=\"/test3.svg\" /><BR><a href=\"/\">Gas Sensor Page</a></p></div>");
                                PageStr+=("<font color=\"#0000FF\"><body bgcolor=\"#A0DFFE\">");
                                PageStr+=("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">");
                                PageStr+=("<h2>Environment Monitor AQS<BR>System Information</h2><BR>");
                                client.println(PageStr);
                                PageStr="";                               
                                PageStr+=(" I2C Arduino ID : ");
                                PageStr+=(New1);
                                PageStr+=("<BR> SHT21 Temperature : ");
                                PageStr+=(Temp);
                                PageStr+=(" C<BR> SHT21 Humidity : ");
                                PageStr+=(Hum);
                                PageStr+=(" %RH<BR> Arduino MQ-2 (A0) :  ");
                                float sensorReading1 = (aDcData[4] * (5.0 / 1023.0))*1000;
                                PageStr+=sensorReading1;// millivots
                                PageStr+=("mV<BR> Arduino MQ-7 (A1) : ");
                                float sensorReading2 = aDcData[5] * (5.0 / 1023.0)*1000;
                                PageStr+=String(sensorReading2/1000, 4);
                                PageStr+=("V<BR> Arduino I2C 3V (A2) : ");
                                float sensorReading3 = aDcData[6] * (5.0 / 1023.0);
                                SensF = sensorReading3;
                                PageStr+=SensF;
                                PageStr+=("V<BR> Arduino I2C 5V (A3) : ");
                                float sensorReading4 = aDcData[7] * (5.0 / 1023.0);
                                SensF = sensorReading4;
                                PageStr+=SensF;
                                PageStr+=("V<BR> Arduino I2C Requests : ");
                                PageStr+=(CoUnt);
                                PageStr+=("<BR> Arduino I2C Errors : ");
                                PageStr+=(FCoUnt + (ReSet*10) );
                                PageStr+=("<BR> Arduino Resets : ");
                                PageStr+=(ReSet);
                                client.println(PageStr);
                                PageStr="";
                               String diagdat="";
                                diagdat+="<BR>  Web Page Requests = ";
                                diagdat+=ulReqcount;
                                diagdat+="<BR>  WiFi Station Hostname = ";
                                diagdat+=wifi_station_get_hostname();
                                diagdat+="<BR>  Free RAM = ";
                                client.print(diagdat);
                                client.println((uint32_t)system_get_free_heap_size()/1024);
                                diagdat=" KBytes<BR>";                               
                                diagdat+="  SDK Version = ";                                 
                                diagdat+=ESP.getSdkVersion();
                                diagdat+="<BR>  Boot Version = ";
                                diagdat+=ESP.getBootVersion();
                                diagdat+="<BR>  Free Sketch Space  = ";
                                diagdat+=ESP.getFreeSketchSpace()/1024;
                                diagdat+=" KBytes<BR>  Sketch Size  = ";
                                diagdat+=ESP.getSketchSize()/1024;
                                diagdat+=" KBytes<BR>";
                                client.println(diagdat);
                                client.printf("  Flash Chip id = %08X\n", ESP.getFlashChipId());
                                client.print("<BR>");
                                client.printf("  Flash Chip Mode = %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
                                diagdat="<BR>  Flash Size By ID = ";
                                diagdat+=ESP.getFlashChipRealSize()/1024;
                                diagdat+=" KBytes<BR>  Flash Size (IDE) = "; 
                                diagdat+=ESP.getFlashChipSize()/1024;
                                diagdat+=" KBytes<BR>  Flash Speed = ";
                                diagdat+=ESP.getFlashChipSpeed()/1000000;
                                diagdat+=" MHz<BR>  ESP8266 CPU Speed = ";
                                diagdat+=ESP.getCpuFreqMHz();
                                diagdat+=" MHz<BR>";
                                client.println(diagdat);
                                client.printf("  ESP8266 Chip id = %08X\n", ESP.getChipId());
                                diagdat="<BR>  System Instruction Cycles Per Second = ";
                                diagdat+=speedcnt*1000;                                                               
                                diagdat+="<BR>  System VCC = ";
                                diagdat+=pfVcC/1000, 2;
                                diagdat+=" V ";
                                diagdat+="<BR>  System Uptime =";
                                diagdat+=duration1;
                                diagdat+="<BR>  Last System Restart Reason = <FONT SIZE=-1>";
                                diagdat+=ESP.getResetInfo();
                                client.println(diagdat);
                                client.println("<BR><FONT SIZE=-2><a href=\"mailto:environmental.monitor.log@gmail.com\">environmental.monitor.log@gmail.com</a><BR><FONT SIZE=-2>ESP8266 With I2C Arduino ADC Slave<BR><FONT SIZE=-2>Compiled Using ver. 2.3.0-rc1, Jan, 2017<BR>");
                                client.println("<IMG SRC=\"https://raw.githubusercontent.com/genguskahn/ESP8266-For-DUMMIES/master/SoC/DimmerDocs/organicw.gif\" WIDTH=\"250\" HEIGHT=\"151\" BORDER=\"1\"></body></html>");
  PageStr="";
  diagdat="";
  // and stop the client
  delay(10);
  client.stop();
  digitalWrite ( led, 0 );
  client.flush();
  }


void handleNotFound() {
  ulReqcount++;
	digitalWrite ( led, 1 );
	String message = "<html><head><title>404 Not Found</title></head><body bgcolor=\"#A0DFFE\"><font color=\"#0000FF\"><h1>";
  message += "<img src=\"http://82.5.78.180:5050/img/flame.gif\" style=\"width:50px;position:relative;top:8px;\">Not";
  message += "<img src=\"http://82.5.78.180:5050/img/flame.gif\" style=\"width:50px;position:relative;top:8px;\">";
  message += "<BR>:-( Here )-:</h1><p>The requested URL was not found on this server, What did you ask for?.</p>";
	message += "<BR>URI (What you typed to end up here...): ";
	message += server.uri();
	message += "<BR>Method (How the access was made...): ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "<BR>Arguments (Did you ask a Question?): ";
	message += server.args();
	message += "<BR>";
	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}
  message += " </body></html>";
	server.send ( 200, "text/html", message );
	digitalWrite ( led, 0 );
}

void setup ( void ) {
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 1 );
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.begin(115200);
  delay(50);
  Serial.println("\r\n\r\nRe-Starting I2C Arduino Slave.......\r\n");
  digitalWrite(0, HIGH);
  pinMode(2, OUTPUT);     // I2C Request To Send, ESP8266 to Slave, 0 = Request Ready, 1 = idle
  digitalWrite(2, HIGH);
  delay(3000);            // allow all to settle before sending reset to Arduino.....
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);  // Using Fash button
  delay(10);
  digitalWrite(0, LOW);
  delay(5);               // 5 millisec pulse to reset Arduino.....
  digitalWrite(0, HIGH);
  delay(3000);            // Arduino Startup delay
  Wire.begin(4,5);            // I2C Bus 0
  delay(70);
  digitalWrite(2, LOW);       // When Set Request To Send Active Slave will refresh data....
  delay(RTSDelay);                
  digitalWrite(2, HIGH);      // Set Request idle
  Wire.beginTransmission(8);
  Wire.requestFrom(8, 6);
  Wire.endTransmission();
  Serial.println("Started I2C Arduino Slave.....\r\n");
  delay(50);
  Serial.println("Starting WfFi...\r\n");
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);
  delay(50);
  Serial.print(F("\r\n\r\nConnecting to "));
  Serial.println(ssid);
	WiFi.begin ( ssid, pass );
	Serial.println ( "" );

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

	Serial.println ( "" );
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	if ( MDNS.begin ( "esp8266" ) ) {
		Serial.println ( "MDNS responder started" );
	}

	server.on ( "/", HTTP_GET, AQS );
  server.on ( "/test1.svg", []() { drawGraph0(1); } );
  server.on ( "/test2.svg", []() { drawGraph0(2); } );
  server.on ( "/test3.svg", []() { drawGraph0(3); } );
  server.on("/diag", HTTP_GET, diag); 
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );
  
  aDcData = new int[32];
  pfTemp = new int[siZe];
  pfTemp1 = new int[siZe];
  pfTemp2 = new int[siZe];    
  if (pfTemp==NULL || pfTemp2==NULL || aDcData==NULL)
  {
    Serial.println("Error in memory allocation!");
  }
 myHTU21D.begin();
 delay(200);
  Hum = myHTU21D.readCompensatedHumidity();
  Temp = myHTU21D.readTemperature();
  Serial.println("\r\nI2C T/H Sensor Data on BUS 0 : " + String(Temp) + " : "+ String(Hum) + "\r\n"); 

  Serial.println("I2C Arduino Slave on BUS 0....Requesting Data...\r\n\r\n"); 
  digitalWrite ( led, 0 );
}

void loop( void ) {
	server.handleClient();
  if(FCoUnt>10){           // Reset Arduino after 10 errors from I2C......
  FCoUnt=0;
  ReSet++;                 // Reset ESP after 50 Errors......
  if(ReSet==5){ESP.restart();}
  Serial.println("\r\nResetting I2C Arduino Slave.......Time : " + duration1 + " Uptime\r\n");
  delay(10);
  digitalWrite(0, LOW);
  delay(5);               // 5 millisec pulse to reset Arduino.....
  digitalWrite(0, HIGH);
  delay(3000);            // Arduino Startup delay

}

if(SmAlrm==1 || COAlrm==1){

  digitalWrite ( led, 1 );    // turn on the LED 
  delay(1000); 
  float pinVoltage = aDcData[4] / 2;
  float pinVoltage1 = aDcData[5 / 2];
  if(pinVoltage<=30 && pinVoltage1<=150){
    COAlrm=0;
    SmAlrm=0;
  }
}
if (millis()>=ulNextMeas_ms) 
 {
    ulNextMeas_ms = millis()+ReFresh; 
    GetNano(); 
 }
if (millis()>=ulNextTSMeas_ms) 
 {
  TSMeasCount++;
    if (TSMeasCount>=720) {// Set the point for reboot....resfresh Timers, DNS, DHCP Etc......
     delay(100);
     ESP.restart();     
     }
  ulNextTSMeas_ms = millis()+120000;
  ThingPost(); 
 }

}



void drawGraph0(int ArrAy) {
  digitalWrite ( led, 1 );
  WiFiClient cl = server.client(); 
  String out = ""; 
  ulEnd=1;
  char temp[200];
  out +="HTTP/1.1 200 OK\r\n";
  out += "Content-Type: image/svg+xml\r\n\r\n";
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"";
  out +=siZe+10;
  out +="\" height=\"150\">\n";
  out += "<rect width=\"";
  out +=siZe+10;
  out +="\" height=\"150\" fill=\"rgb(25, 30, 110)\" stroke-width=\"1\" stroke=\"rgb(255, 255, 255)\" />\n";
  out += "<g stroke=\"white\">\n"; 
  cl.println(out);
  out = ""; 
  if(ArrAy==1){
   y = pfTemp2[ulEnd] * 2; 
  }else if(ArrAy==2){
   y = pfTemp1[ulEnd] * 2;        
  }else if(ArrAy==3){
   y = pfTemp[ulEnd] * 2;
  }
  for (int x = 0; x < ulMeasCount*ScAle; x+=ScAle) {
  if(ArrAy==1){
   y2 = pfTemp2[ulEnd++] * 2; 
  }else if(ArrAy==2){
   y2 = pfTemp1[ulEnd++] * 2;        
  }else if(ArrAy==3){
   y2 = pfTemp[ulEnd++] * 2; 
  }
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + ScAle, 140 - y2);
    cl.print(temp);
    y = y2;
  }
  cl.println("</g>\n</svg>");
  out = "";
  delay(1);
  cl.stop();    
  digitalWrite ( led, 0 );
  cl.flush();
}
