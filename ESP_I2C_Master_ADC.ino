 /* 
   This provides some easy access to the Arduino peripherals and Libraries via a 22 byte char/Int->String->/char/Int transfer
   ESP8266 connected to I2C via pins 4(SDA) and 5(SCL), the SHT21 is directly connected to ESP8266 pins
   Arduino pins A4 and A5 are connected via 5v to 3v converter to the ESP8266 pins 4 and 5.
   The 22 bytes are arranged - bytes 0+1 are a 2 char ID, 3 byte blocks 2-4, 5-7, 8-11, 12-14.... are Arduino A0 to A3 & A6 + A7 reading plus terminator(AA)
   presenting data as char allows very simple transfer.
   ESP8266 pin0 Used to Reset the Arduino and ESP8266 pin2 to send Request for data (RTS) assigned to Arduino interrupt 0 (D2).....

   Webpage with SVG Charts on / for all available ADC's (A4,A5 I2C)

   The charts can be used to verify wiring, use 3v ESP VCC displayed as Reference and connect ADC's to 3V or if 5v Arduino to 5V, to see FSD

   The Arduino data fetch routine takes about 12ms to populate msg[] in the Slave Sketch.....

   Use a 3V Aruino to eliminate the need for the convertor, results obviously 3V Max.....

   For ESP8266-01 Serial MUST Be Removed from the ESP8266 Master Sketch or edited for No Reset and Set_TX(2) for Serial with pin 0 generating Interrupt
   
   Pin Numbers are Arduino interpretation.......
   ===================================================
   Arduino Pin  Esp Pin  ESP8266 01  Serial.set_tx(2);
   ===================================================
   Reset         0          0            N/A
   D2            2          2             0
   A4            4          1             1
   A5            5          3             3
   
   
   
   All via 4 way 5V to 3V shifter......Arduino 5v side can take many more Slaves......

   Log the data to Spiffs or SDCard, in the example below I have used 6 SRAM Array's to feed the 6 SVG Charts
   adapt these to feed SPIFFS or SD Recording.........
   Thingspeak Channel........  https://thingspeak.com/channels/209159
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>



char ssid[] = "YourSSID";      // your network SSID (name) 
char pass[] = "YourPASSWORD";  // your network password


extern "C" 
{
#include "user_interface.h"
}

ADC_MODE(ADC_VCC);


const int led = 16;
int I1,y,y2,iicdata,FCoUnt,ArrAy;
int *pfTemp0;
int *pfTemp1;
int *pfTemp2;
int *pfTemp3;
int *pfTemp4;
int *pfTemp5;
int *aDcData;

unsigned long ulMeasCount=0;
unsigned long ulNextMeas_ms=0;
unsigned long ulReqcount,ulEnd,CoUnt;
String duration1,New0,New1,out;
boolean FlIp = false;

int    sCale = 2;     // Scale the chart X axis display
int     siZe = 600;   // The Width of the SVG chart & size of arrays......1400(*6) leaves 4Kb of memory.....
int RTSDelay = 12;    // Set this for the response time of Your Arduino Data Fetch routine, 6 delay's of Xms + Analogue reads.....
int  ReFresh = 50;    // Set this for the delay between I2C Requests, min 50ms......Server needs space to run....Serial Seriously SLOWs things down !!!!.....

WiFiServer server(80);
WiFiClient client;



///////////////////
// (re-)start WiFi
///////////////////
void WiFiStart()
{
 // ulReconncount++;
  // Connect to WiFi network
  WiFi.begin();
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
  // Start the server
  server.begin();
}





void GetNano(){
  int lOop=0;
  New0="";
  New1="";
// ================================== Activate Arduino Interrupt, only 1 Interrupt needed, supply to all Slaves ============================

  digitalWrite(2, LOW);       // When Set Request To Send Active(interrupt FALLING) Slave will refresh data....
  delay(RTSDelay);            // This will make sure the Arduino is in the loop with fresh data......  
  digitalWrite(2, HIGH);      // Set Request idle fresh data now in msg[]....
// ================================== DE-Activate Arduino Interrupt & Start of Arduino 1 I2C transfer ======================================
  Wire.beginTransmission(8);
  Wire.requestFrom(8, 22);    // request 22 bytes from slave device #8 bus 0 (Arduino Mini Pro costs £8 for 5 from China)
  while (Wire.available()) {  // slave may send more Bytes than requested, embedded Respose Bytes - 24:32
  char c = Wire.read();       // receive a byte as character 
  New0+=c;
  }
  Wire.endTransmission();

lOop=0;
New1=New0.substring(0,2);    // Very Simple error checking - error if not detected........
if(New1=="A1"){   
iicdata=4;
for(int i=2;i<18;i+=3){
char inChar = New0.charAt(i+2);
 int newt=inChar-48;        // Convert Numerical Char to int.....
     inChar = New0.charAt(i+1);
 int newt1=inChar-48; 
     inChar = New0.charAt(i);
 int newt2=inChar-48;
 int newt3=(newt2*100)+(newt1*10)+newt;
 if(newt3==998){newt3+=25;}// Correct transfer for Full Scale clipping 1023-25.......
   aDcData[iicdata]=newt3;
   iicdata++; 
 }
   CoUnt++;
   ulMeasCount++;
   if(ulMeasCount>siZe-1){ulMeasCount=1;}
   pfTemp0[ulMeasCount] = aDcData[4];
   pfTemp1[ulMeasCount] = aDcData[5];
   pfTemp2[ulMeasCount] = aDcData[6];
   pfTemp3[ulMeasCount] = aDcData[7];
   pfTemp4[ulMeasCount] = aDcData[8];
   pfTemp5[ulMeasCount] = aDcData[9];
}else{ FCoUnt++; }
  /* duration1 = "";
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
Serial.println("|_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_¬");
Serial.print("\r\n0  Device ID Value : ");
Serial.println(New1);
Serial.print("\r\n1 Arduino Nano A0 Value  ");
Serial.print(aDcData[4]);
Serial.print("\r\n2 Arduino Nano A1 Value  ");
Serial.print(aDcData[5]);  
Serial.print("\r\n3 Arduino Nano A2 Value  ");
Serial.print(aDcData[6]);
Serial.print("\r\n4 Arduino Nano A3 Value  ");
Serial.print(aDcData[7]);
Serial.print("\r\n5 Arduino Nano A5 Value  ");
Serial.print(aDcData[8]);
Serial.print("\r\n6 Arduino Nano A6 Value  ");
Serial.print(aDcData[9]);
Serial.print("\r\n7    SHT21 Humidity ");
Serial.print(Hum);
Serial.print(" RH %\r\n8 SHT21 Temperature ");
Serial.print(Temp);
Serial.print(" Deg C\r\n     Running Time : ");
Serial.print(duration1);
Serial.print("\r\nLoops ");
Serial.print(CoUnt);
Serial.print("\r\nSingle Errors ");
Serial.print(FCoUnt);
Serial.print("\r\nArduino Resets(+2 errors) ");
Serial.println(ReSet);

if(CoUnt==100){
  Serial.print("\r\nArduino Requests at 100 ");
  Serial.println(duration1);
}
if(CoUnt==500){
  Serial.print("\r\nArduino Requests at 500 ");
  Serial.println(duration1);
}
if(CoUnt==1000){
  Serial.print("\r\nArduino Requests at 1000 ");
  Serial.println(duration1);
  for(int i =1;i<500;i++){
    Serial.println(String(pfTemp0[i]) + " : " + String(pfTemp1[i]) + " : " + String(pfTemp2[i]) + " : " + String(pfTemp3[i]) + " : " + String(pfTemp4[i]) + " : " + String(pfTemp5[i]));
  }
}*/
}




void setup () {
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 1 );
  pinMode ( 12, OUTPUT );
  analogWrite ( 12, 511 );    // PWM Square wave output......
  Serial.begin(115200);
  delay(50);
  Serial.println("\r\n\r\nRe-Starting I2C Arduino Slave.......\r\n");
  digitalWrite(0, HIGH);
  pinMode(2, OUTPUT);         // I2C Request To Send, ESP8266 to Slave, 0 = Request Ready, 1 = idle
  digitalWrite(2, HIGH);
  delay(3000);                // allow all to settle before sending reset to Arduino.....
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);      // Using Fash button
  delay(10); 
  digitalWrite(0, LOW);
  delay(5);                   // 5 millisec pulse to reset Arduino.....
  digitalWrite(0, HIGH);
  delay(3000);                // Arduino Startup delay
  Wire.begin(4,5);            // I2C Bus 0
  delay(70);
  digitalWrite(2, LOW);       // When Set Request To Send (Interrupt) Active Slave will refresh data....
  delay(RTSDelay);                
  digitalWrite(2, HIGH);      // Set Request idle
  Wire.beginTransmission(8);
  Wire.requestFrom(8, 22);
  Wire.endTransmission();
  Serial.println("Started I2C Arduino Slave.....\r\n");
  delay(50);

  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
      Serial.print(".");
  }
    Serial.println();
  server.begin();
    Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  aDcData = new int[32];
  pfTemp0 = new int[siZe];
  pfTemp1 = new int[siZe];
  pfTemp2 = new int[siZe];    
  pfTemp3 = new int[siZe];
  pfTemp4 = new int[siZe];
  pfTemp5 = new int[siZe];    
  if (pfTemp0==NULL || pfTemp5==NULL || aDcData==NULL)
  {
    Serial.println("Error in memory allocation!");
  }

  Serial.println("I2C Arduino Slave on BUS 0....Requesting Data...\r\n\r\n"); 
  digitalWrite ( led, 0 );
}

void loop() {

if (millis()>=ulNextMeas_ms) 
 {
    ulNextMeas_ms = millis()+ReFresh; 
    GetNano(); 
 }

  ///////////////////////////////
  // check if WLAN is connected//
  ///////////////////////////////
  
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiStart();
  }
  
  ///////////////////////////////////
  // Check if a client has connected
  ///////////////////////////////////
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  
  // Wait until the client sends some data
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    return; 
  }
  
  ///////////////////////////////////////
  // Read the first line of the request//
  ///////////////////////////////////////
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?show=1234 HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
 //Serial.println(sPath);
  
  /////////////////////////////
  // format the html response//
  /////////////////////////////
 if(sPath=="/") {
    float pfVcC = ESP.getVcc();
          digitalWrite ( led, 1 );
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");        // refresh the page automatically every 20 sec
          client.println();
   String PageStr=("<!DOCTYPE HTML>");
          PageStr+=("<html><title>Environment Monitor ADC</title>");
          PageStr+=("<BR><font color=\"#000000\"><body bgcolor=\"#a0dFfe\"><h1><p>Arduino ADC Page<br>");
          PageStr+="<FONT SIZE=-2>I2C Errors : ";
          PageStr+=FCoUnt;
          PageStr+="  :  ESP8266 VCC : ";
          PageStr+=pfVcC/1000, 2;          
          PageStr+="V : Free Memory : ";
          PageStr+=((uint32_t)system_get_free_heap_size()/1024);
          PageStr+=" KBytes<BR></p><div style=\"float:left; text-align: center; \">";  
          client.println(PageStr);
          PageStr="";

for(ArrAy=1;ArrAy<7;ArrAy++){
  out="";
  if( ArrAy==1 ){out+="<hr>";}
  ulEnd=1;
  char temp[200];
  out += "Arduino Nano ADC Pin : A";
  if(ArrAy<5){
    out += ArrAy-1;
  }else{
    out += ArrAy+1;
  }
  out += "<br><svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"";
  out +=(siZe*sCale)+10;
  out +="\" height=\"150\">\n";
  out += "<rect width=\"";
  out +=(siZe*sCale)+10;
  out +="\" height=\"150\" fill=\"rgb(25, 30, 110)\" stroke-width=\"1\" stroke=\"rgb(255, 255, 255)\" />\n";
  out += "<g stroke=\"white\">\n"; 
  client.println(out);
  out = ""; 
  if(ArrAy==1){
   y = pfTemp0[ulEnd] / 10; 
  }else if(ArrAy==2){
   y = pfTemp1[ulEnd] / 10;        
  }else if(ArrAy==3){
   y = pfTemp2[ulEnd] / 10;
  }else if(ArrAy==4){
   y = pfTemp3[ulEnd] / 10; 
  }else if(ArrAy==5){
   y = pfTemp4[ulEnd] / 10;        
  }else if(ArrAy==6){
   y = pfTemp5[ulEnd] / 10;
  }
  for (int x = 0; x < ulMeasCount*sCale; x+=sCale) {
  if(ArrAy==1){
   y2 = pfTemp0[ulEnd++] / 10; 
  }else if(ArrAy==2){
   y2 = pfTemp1[ulEnd++] / 10;        
  }else if(ArrAy==3){
   y2 = pfTemp2[ulEnd++] / 10; 
  }else if(ArrAy==4){
   y2 = pfTemp3[ulEnd++] / 10; 
  }else if(ArrAy==5){
   y2 = pfTemp4[ulEnd++] / 10;        
  }else if(ArrAy==6){
   y2 = pfTemp5[ulEnd++] / 10; 
  }
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + sCale, 140 - y2);
    client.print(temp);
    y = y2;
  }
  client.println("</g>\n</svg><BR>");
  out = "Last Value : ";
  float LaSt=y * 10;
  if (LaSt>=1020){LaSt+=3;}// Correcting error from chart conversion Max 1020.....Use Array's for actual values only for display here.....
  out += LaSt * (5.0 / 1023.0);
  out+="V<hr>";
  client.println(out);
  out = "";


                      
 }
 PageStr+=("</div><BR></html>");
 client.println(PageStr);
 digitalWrite ( led, 0 );      
 }
// and stop the client
client.stop();
}
