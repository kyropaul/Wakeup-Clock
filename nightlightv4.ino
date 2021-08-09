#include <time.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <LittleFS.h>

//variables
#define MY_NTP_SERVER "time.nist.gov"  //where to get time clock from
const char* MY_TZ = ""; //was define "EST5EDT,M3.2.0,M11.1.0" //setting for daylight savings timezone Toronto (https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)
int TimeString = 0;  //working variable for converting time from string time.h to minutes past midnight
int Green = D2;  //Green light wakeup
int Yellow = D4;  //Yellow Light (almost up)
int Blue = D3;  //Blue Light (sleep)
//int TCheck=0;
unsigned long lastConnectionTime = 10 * 60 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postInterval = 60 * 60 * 1000;  // update interval hourly
//String setTZ ="";

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;
WiFiClient wifiClient;


String readFile(fs::FS &fs, const char * path){
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    //Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  //Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, String message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}



void setup() {
  Serial.begin(115200);
  //set pins to output
  pinMode(Green,OUTPUT);
  pinMode(Yellow,OUTPUT);
  pinMode(Blue,OUTPUT);
  
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
    // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  server.begin();

  
  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      Serial.print(".");        
  }
  Serial.println();
  Serial.println("Wifi Connected Success!");
  Serial.println(WiFi.localIP() );
   //get IP address and flash led so end user can see IP to get portal
  String IPAdd = WiFi.localIP().toString();
  int p1 =IPAdd.indexOf(".");
  int p2 = IPAdd.indexOf(".",p1+1);
  int p3 = IPAdd.indexOf(".",p2+1); //p3 will be the starting place of the last part of IP
  int d1=IPAdd.substring(p3+1,p3+2).toInt();
  int d2=IPAdd.substring(p3+2,p3+3).toInt();
  int d3=IPAdd.substring(p3+3,p3+4).toInt();
  //flash Red for the first digit
  for (int x=1; x<=d1; x+=1){
    analogWrite(Green,500);
    analogWrite(Yellow,500);
    analogWrite(Blue,500);
    delay(1000);
    analogWrite(Green,0);
    analogWrite(Yellow,0);
    analogWrite(Blue,0);
    delay(1000);    
  }
  delay(500);
  //flash Green for the first digit
  for (int x=1; x<=d2; x+=1){
    analogWrite(Green,500);
    analogWrite(Yellow,500);
    analogWrite(Blue,500);
    delay(1000);
    analogWrite(Green,0);
    analogWrite(Yellow,0);
    analogWrite(Blue,0);
    delay(1000);    
  }
  delay(500);
  //flash Blue for the first digit
  for (int x=1; x<=d3; x+=1){
    analogWrite(Green,500);
    analogWrite(Yellow,500);
    analogWrite(Blue,500);
    delay(1000);
    analogWrite(Green,0);
    analogWrite(Yellow,0);
    analogWrite(Blue,0);
    delay(1000);    
  }

  
  //initialize littlefs
  if(!LittleFS.begin()){  //try to initialize file system for stored values
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }  
  //load time zone
  MY_TZ = readFile(LittleFS, "/data/TZ.txt").c_str();
  //setTZ = readFile(LittleFS, "/data/TZ.txt");
  Serial.println(MY_TZ);
  //connect to time clock
  configTime(MY_TZ, MY_NTP_SERVER);
  Serial.println("\nWaiting for Internet time");

  while(!time(nullptr)){
     Serial.print("*");
     delay(1000);
  }
  Serial.println("\nTime response....OK");
  //ensure all LED are off
  digitalWrite(Green,LOW);
  digitalWrite(Yellow,LOW);
  digitalWrite(Blue,LOW);   
}

void loop() {
  // To access your stored values on BedTime, SoftWake, Wake
  String SetTZ = readFile(LittleFS, "/data/TZ.txt");
  int yourBedTime = readFile(LittleFS, "/data/BedTime.txt").toInt();
//  Serial.print("*** Your BedTime: ");
//  Serial.println(yourBedTime);
  int BedTimeOff = yourBedTime+60;
//  Serial.print("*** Your BedTimeOff: ");
//  Serial.println(BedTimeOff);
  int yourSoftWake = readFile(LittleFS, "/data/SoftWake.txt").toInt();
//  Serial.print("*** Your SoftWake: ");
//  Serial.println(yourSoftWake);
  int PreWake=yourSoftWake-60;
//  Serial.print("*** Your PreWake: ");
//  Serial.println(PreWake);
  int yourWake = readFile(LittleFS, "/data/Wake.txt").toInt();
//  Serial.print("*** Your Wake: ");
//  Serial.println(yourWake);
  int AllOff = yourWake+60;
//  Serial.print("*** Your AllOff: ");
//  Serial.println(AllOff);
  WiFiClient client = server.available();   // Listen for incoming clients
   if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if(header.indexOf("GET /config.html?") >= 0){ //if the header contains /config.html
              int A = header.indexOf("SetBed="); //find start of setBed
              int B = header.indexOf("&SetSoftWake="); //find start of setSoftWake
              String SetBed = header.substring(A+7,B);
              Serial.print("Bed Time = ");
              Serial.println(SetBed);
              //write to file
              writeFile(LittleFS, "/data/BedTime.txt", SetBed);
              yourBedTime = SetBed.toInt();
              A = header.indexOf("&SetWake=");
              String SetSoftWake = header.substring(B+13,A);
              yourSoftWake = SetSoftWake.toInt();
              Serial.print("Soft Wake = ");
              Serial.println(SetSoftWake);
              //write to file
              writeFile(LittleFS, "/data/SoftWake.txt", SetSoftWake);
              B= header.indexOf("&SetTZ=");
              String SetWake = header.substring(A+9,B);
              yourWake = SetWake.toInt();
              Serial.print("Wake = ");
              Serial.println(SetWake);
              //write to file
              writeFile(LittleFS, "/data/Wake.txt", SetWake);
              A= header.indexOf(" HTTP/1.1");
              SetTZ = header.substring(B+7,A);
              Serial.print("TimeZone = ");
              SetTZ.replace("%2C",",");
              Serial.println(SetTZ);
              MY_TZ=SetTZ.c_str();
              //write to file
              writeFile(LittleFS, "/data/TZ.txt", SetTZ);
              configTime(MY_TZ, MY_NTP_SERVER);
              Serial.println("\nWaiting for Internet time"); 
              

            } else { Serial.println("No Data");}
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            //css was borrowed from another project don't need buttons but left css alone for now
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            //html content and form
            client.println("<body><h1>ESP Sleep Clock</h1><p>All times are minutes past midnight (i.e. 6am = 360)</p>");
            client.println("<p>Current Time: ");
            client.println(TimeString);
            client.println("</p>");
            client.println("<form action=\"/config.html\" method=\"get\">");
            client.println("<label for=\"SetBed\">BedTime (blue light will turn on now):</label>");
            client.println("<input type=\"text\" id=\"SetBed\" name=\"SetBed\" value=\"");
            client.println(yourBedTime); //load current value into box
            client.println("\"><br><br>");
            client.println("<label for=\"SetSoftWake\">SoftWake (Green light turns on):</label>");
            client.println("<input type=\"text\" id=\"SetSoftWake\" name=\"SetSoftWake\" value=\"");
            client.println(yourSoftWake); //load current value into box
            client.println("\"><br><br>");
            client.println("<label for=\"SetWake\">Wake Time (Sun will turn on and stay on for 1 hour):</label>");
            client.println("<input type=\"text\" id=\"SetWake\" name=\"SetWake\" value=\"");
            client.println(yourWake); //load current value into box
            client.println("\"><br><br>");
            client.println("<label for=\"SetTZ\">Set Time Zone (<a href=\"https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv\">https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv</a>):</label>");
            client.println("<input type=\"text\" id=\"SetTZ\" name=\"SetTZ\" value=\"");
            client.println(SetTZ); //load current value into box
            client.println("\"><br><br>");
            client.println("<input type=\"submit\" value=\"Submit\">");
            client.println("</form>");
            client.println("</body></html>");
            client.println();
            // Break out of the while loop
            break;
            } else { // if you got a newline, then clear currentLine
            currentLine = "";
            }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  //delay to prevent exceeding openweather poling requirement for free account
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println("making webcall");
      if (WiFi.status() == WL_CONNECTED) {
        configTime(MY_TZ, MY_NTP_SERVER);
        Serial.println("\nWaiting for Internet time"); 
        while(!time(nullptr)){
          Serial.print("*");
          delay(1000);
        }
      }
  }

//at 02:24 update time clock
//  if( (p_tm->tm_hour == 02) && (p_tm->tm_min ==24) && (TCheck==0)){
//    configTime(MY_TZ, MY_NTP_SERVER);
//    Serial.println("\nWaiting for Internet time"); 
//     while(!time(nullptr)){
//       Serial.print("*");
//       delay(1000);
//     }
//     TCheck =1;
//   }
//   if((p_tm->tm_hour == 00) && (p_tm->tm_min ==01) && (TCheck==1)) {  //reset TCheck after midnight
//     TCheck = 0;
//   }
   //calculate time in minutes past midnight
   TimeString = (p_tm->tm_hour*60)+p_tm->tm_min;
   Serial.print(p_tm->tm_hour);
   Serial.print(":");
   Serial.print(p_tm->tm_min);
   Serial.print(" (");
   Serial.print(TimeString);
   Serial.println(")");
   
   //between yourBedTime and BedTimeOff Blue = HIGH, Yellow = LOW, Green=LOW
   if (TimeString >= yourBedTime && TimeString < BedTimeOff){
     digitalWrite(Blue,HIGH);
     digitalWrite(Yellow,LOW);
     digitalWrite(Green,LOW);
     Serial.println("Turn on Blue = BedTime");
   }
   //between PreWake and yourSoftWake Blue = HIGH, Yellow = LOW, Green= LOW
   else if (TimeString >= PreWake && TimeString < yourSoftWake){
     digitalWrite(Blue,HIGH);
     digitalWrite(Yellow,LOW);
     digitalWrite(Green,LOW);
     Serial.println("Turn on Blue = PreWake");
   }
   //between yourSoftWake and YourWake Blue=LOW,Yellow=LOW, Green=HIGH
   else if (TimeString >= yourSoftWake && TimeString < yourWake){
     digitalWrite(Blue,LOW);
     digitalWrite(Yellow,LOW);
     digitalWrite(Green,HIGH);
     Serial.println("Turn on Yellow = SoftWake");
   }
   //between YourWake and AllOff Blue=LOW,Yellow=HIGH,Green=HIGH
   else if (TimeString >= yourWake && TimeString < AllOff){
     digitalWrite(Blue,LOW);
     digitalWrite(Yellow,HIGH);
     digitalWrite(Green,HIGH);
     Serial.println("Turn on Green & Yellow = Wakeup");
   }
   //else Blue=LOW,Yellow=LOW,Green=LOW
   else {
     digitalWrite(Blue,LOW);
     digitalWrite(Yellow,LOW);
     digitalWrite(Green,LOW);
     Serial.println("turn all off");
   }
  delay(1000); //delay in loop (approx 10 seconds)
}

