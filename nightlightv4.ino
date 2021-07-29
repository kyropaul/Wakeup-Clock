#include <ESP8266WiFi.h>
#include <time.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>

//variables
#define MY_NTP_SERVER "time.nist.gov"  //where to get time clock from
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //setting for daylight savings timezone Toronto (https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)
int TimeString = 0;  //working variable for converting time from string time.h to minutes past midnight
int Green = D2;  //Green light soft-wake
int Yellow = D4;  //Yellow Light wake-up
int Blue = D3;  //Blue Light (sleep)
int TCheck=0;

//wifi settings to be removed
const char* ssid = "YourSSID";
const char* password = "YourPassword";

//this is optional, if you don't want a static IP you also need to comment out fixed IP code below
// Set your Static IP address
IPAddress local_IP(192, 168, 0, 9);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
//end of static IP code


//webserver for variables
AsyncWebServer server(80);
const char* PARAM_STRING = "BedTime";
const char* PARAM_INT = "SoftWake";
const char* PARAM_FLOAT = "Wake";
// HTML web page to handle 3 input fields (BedTime, SoftWake, Wake)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>function submitMessage() {alert("Saved value to Alarm Clock");setTimeout(function(){ document.location.reload(false); }, 500);} </script></head><body>
  <p>All times must be entered in minutes past midnight (i.e. 7pm = 1140).</p>
  <form action="/get" target="hidden-form">BedTime (current value %BedTime%): <input type="text" name="BedTime"><input type="submit" value="Submit" onclick="submitMessage()"></form><br>
  <form action="/get" target="hidden-form">
    SoftWake (current value %SoftWake%): <input type="number " name="SoftWake">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Wake (current value %Wake%): <input type="number " name="Wake">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
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

void writeFile(fs::FS &fs, const char * path, const char * message){
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

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "BedTime"){
    return readFile(SPIFFS, "/data/BedTime.txt");
  }
  else if(var == "SoftWake"){
    return readFile(SPIFFS, "/data/SoftWake.txt");
  }
  else if(var == "Wake"){
    return readFile(SPIFFS, "/data/Wake.txt");
  }
  return String();
}


void setup() {
  Serial.begin(115200);
  //set pins to output
  pinMode(Green,OUTPUT);
  pinMode(Yellow,OUTPUT);
  pinMode(Blue,OUTPUT);
  
  //connect to saved wifi
  Serial.println();
  Serial.print("Wifi connecting to ");
  Serial.println( ssid );
  WiFi.begin(ssid,password);
  Serial.println();
  Serial.print("Connecting");
  
  //comment this out if you don't want a static IP
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  //end of static IP code
  
  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      Serial.print(".");        
  }
  Serial.println();
  Serial.println("Wifi Connected Success!");
  Serial.print("NodeMCU IP Address : ");
  Serial.println(WiFi.localIP() );

  
  //webserver code
  if(!SPIFFS.begin()){  //try to initialize file system for stored values
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  // Send a GET request to <ESP_IP>/get?BedTime=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET BedTime value on <ESP_IP>/get?BedTime=<inputMessage>
    if (request->hasParam(PARAM_STRING)) {
      inputMessage = request->getParam(PARAM_STRING)->value();
      writeFile(SPIFFS, "/data/BedTime.txt", inputMessage.c_str());
    }
    // GET SoftWake value on <ESP_IP>/get?SoftWake=<inputMessage>
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/data/SoftWake.txt", inputMessage.c_str());
    }
    // GET Wake value on <ESP_IP>/get?Wake=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT)) {
      inputMessage = request->getParam(PARAM_FLOAT)->value();
      writeFile(SPIFFS, "/data/Wake.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();


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
  int yourBedTime = readFile(SPIFFS, "/data/BedTime.txt").toInt();
//  Serial.print("*** Your BedTime: ");
//  Serial.println(yourBedTime);
  int BedTimeOff = yourBedTime+60;
//  Serial.print("*** Your BedTimeOff: ");
//  Serial.println(BedTimeOff);
  int yourSoftWake = readFile(SPIFFS, "/data/SoftWake.txt").toInt();
//  Serial.print("*** Your SoftWake: ");
//  Serial.println(yourSoftWake);
  int PreWake=yourSoftWake-60;
//  Serial.print("*** Your PreWake: ");
//  Serial.println(PreWake);
  int yourWake = readFile(SPIFFS, "/data/Wake.txt").toInt();
//  Serial.print("*** Your Wake: ");
//  Serial.println(yourWake);
  int AllOff = yourWake+60;
//  Serial.print("*** Your AllOff: ");
//  Serial.println(AllOff);
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

//at 02:24 update time clock
  if( (p_tm->tm_hour == 02) && (p_tm->tm_min ==24) && (TCheck==0)){
    configTime(MY_TZ, MY_NTP_SERVER);
    Serial.println("\nWaiting for Internet time"); 
     while(!time(nullptr)){
       Serial.print("*");
       delay(1000);
     }
     TCheck =1;
   }
   if((p_tm->tm_hour == 00) && (p_tm->tm_min ==01) && (TCheck==1)) {  //reset TCheck after 2:24
     TCheck = 0;
   }
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
     Serial.println("Turn on Green = Wakeup");
   }
   //else Blue=LOW,Yellow=LOW,Green=LOW
   else {
     digitalWrite(Blue,LOW);
     digitalWrite(Yellow,LOW);
     digitalWrite(Green,LOW);
     Serial.println("turn all off");
   }
  delay(10000); //delay in loop (approx 10 seconds)
}
