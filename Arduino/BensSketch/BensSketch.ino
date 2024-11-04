#include <DNSServer.h>
#include <WiFi.h>
#include <Wire.h>  //I2C library
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
//#include <ArduinoJson.h>
#include <FirebaseJson.h>
#include <SPIFFS.h>
#include "LittleFS.h"


#define BQ34Z100 0x55


DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

volatile int interruptCounterA = 0;
volatile int interruptCounterB = 0;
String BMS_data;
String correct_name = "user";
String user_name;
String proficiency;
String correct_proficiency = "pass";
bool name_received = false;
bool proficiency_received = false;
bool device_connected = 0;
bool time_out = 0;
bool lock_login_page = 0;
bool LED = 0;
int SOC;
int old_SOC;
int voltage;
int old_voltage;
int current;
int old_current;
float temp;
float old_temp;
int count = 0;
bool blueledState = 0;
byte redledState = 2;
String SOC_string;
String voltage_string;
String current_string;
String temp_string;
String trial;
String mytitle = "CUB_001 - ";
String percent = "%";
String device_title;
String toggle1 = "toggle";
String JSON_string;
String comp_string;
String str1, str2, str3, str4, str5, str6, str7, str8, str9;


//StaticJsonBuffer<300> jsonBuff;        //create a JSON buffer to store an object 
//StaticJsonBuffer<300> jsonBuffer;               //create a JSON buffer to store an object
//JsonObject& root = jsonBuffer.createObject();   //create a JSON object called root   
//JsonObject& myData = jsonBuff.createObject();   //create a JSON object called root

//JsonObject root = doc.to<JsonObject>();
//char json[] = "{\"charge\":\100\,\"volts\":\3713\,\"amps\":13518\}"; 

FirebaseJson json;
//FirebaseJsonData result;



String processor(const String& var){    //this will replace placeholders with values
  //Serial.println(var);
  if(var == "CHARGE"){
    return SOC_string;
  }
  else if(var == "VOLTAGE"){
    return voltage_string;
  }
  else if(var == "CURRENT"){
    return current_string;
  }
  else if(var == "TEMPERATURE"){
    return temp_string;
  }
  else if(var == "STATE"){
    if (blueledState==1){
      return "ON";
    }
    else{
      return "OFF";
    }
    if (redledState==3){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}




hw_timer_t * timer = NULL;   //"timer"
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;   //"timerMux"
 
void IRAM_ATTR onTimer() {       //ISR for timer
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounterA = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}



hw_timer_t * timer2 = NULL;   //"timer2"
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;   //"timerMux"
 
void IRAM_ATTR onTimer2() {       //ISR for timer 2
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounterB = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}







const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>
  
<html>   
<head>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
<title> Login Page </title>  
<style>   
Body {  
  font-family: Calibri, Helvetica, sans-serif;  
  background-color: pink;  
}  
button {   
       background-color: #4CAF50;   
       width: 100%;  
        color: white;   
        padding: 15px;   
        margin: 10px 0px;   
        border: none;   
        cursor: pointer;   
         }   
 form {   
        border: 3px solid #f1f1f1;   
    }   
 input[type=text], input[type=password] {   
        width: 100%;   
        margin: 8px 0;  
        padding: 12px 20px;   
        display: inline-block;   
        border: 2px solid green;   
        box-sizing: border-box;   
    }  
 button:hover {   
        opacity: 0.7;   
    }   
  .cancelbtn {   
        width: auto;   
        padding: 10px 18px;  
        margin: 10px 5px;  
    }   
        
     
 .container {   
        padding: 25px;   
        background-color: lightblue;  
    }

  img {
    height: auto; 
    max-width: 100%;
  }
       
</style>   
</head>    
<body>    
    <center> <h1> CUB_00001 Login Form </h1> </center>   
    
        <div class="container">   
          <form action="/get">   
            <label>Username : </label>   
            <input type="text" placeholder="Enter Username" name="username" required>  
            <label>Password : </label>   
            <input type="password" placeholder="Enter Password" name="password" required>  
            <button type="submit">Submit</button> 
            <input type="checkbox" checked="checked"> Remember me   
            <button type="button" class="cancelbtn"> Cancel</button>   
            Forgot <a href="#"> password? </a>  
          </form> 
        </div>   
        <img src="aceon"> 
        
</body>     
</html>  
</html>)rawliteral";





const char display_page[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>
  
<html>   
<head>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
<title> Info Page </title>  
<link rel="icon" href="data:,">
<style>   
Body {  
  font-family: Calibri, Helvetica, sans-serif;  
  background-color: lightblue;  
}  

  
button {   
       background-color: #4CAF50;   
       width: 100%;  
        color: orange;   
        padding: 15px;   
        margin: 30px 0px;   
        border: none;   
        cursor: pointer; 
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);  
         }   
 form {   
        border: 3px solid #f1f1f1;   
    }   
 input[type=text], input[type=password] {   
        width: 100%;   
        margin: 8px 0;  
        padding: 12px 20px;   
        display: inline-block;   
        border: 2px solid green;   
        box-sizing: border-box;   
    }  
 button:hover {   
        opacity: 0.7;   
    } 
 .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }  
  .cancelbtn {   
        width: auto;   
        padding: 10px 18px;  
        margin: 10px 5px;  
    }   
        
     
 .container {   
        padding: 25px;   
        background-color: lightgreen;  
    }   

  
</style> 
<link rel="icon" href="data:,">  
</head>    
<body>    
    <center> <h1> Battery information </h1> </center>   
    
        <div class="container">   
          <p> 
            <span class="charge">State of charge:</span> 
            <span id="charge">%CHARGE%</span> 
            <span id="charge">&#37</span>
          </p>
          <p>
            <span class="voltage">Voltage:</span> 
            <span id="voltage">%VOLTAGE%</span> 
            <span id="voltage">mV</span>
          </p>
          <p>
            <span class="current">Current:</span> 
            <span id="current">%CURRENT%</span> 
            <span id="current">mA</span>
          </p>
          <p>
            <span class="temp">Temperature:</span> 
            <span id="temp">%TEMPERATURE%</span> 
            <span id="temp">degC</span>
          </p>
        </div> 
        <p><button id="bluebutton" class="button">Blue</button>
          <span class="state">Blue LED:</span>
          <span id="bluestate">%STATE%</span>
        </p>
        <p><button id="redbutton" class="button">Red</button>
          <span class="state">Red LED:</span>
          <span id="redstate">%STATE%</span>
        </p>
        <img src="aceon2" style="max-width: 100%; height:auto;"> 
        <form action="/about">
          <button type="Submit">About AceOn</button>
        </form>
        <form action="/unit image">
          <button type="Submit">Unit layout</button>
        </form>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed'); 
  }
  function onMessage(event) {
    
    var bluestate;
    var redstate;
    var SOC;
    var voltage;
    var current;
    var temp;
    var info = 0;
    if (event.data == "1"){
      bluestate = "ON";
      document.getElementById('bluestate').innerHTML = bluestate;
    }
    else if(event.data == "0"){
      bluestate = "OFF";
      document.getElementById('bluestate').innerHTML = bluestate;
    }
    else if(event.data == "2"){
      redstate = "OFF";
      document.getElementById('redstate').innerHTML = redstate;
    }
    else if(event.data == "3"){
      redstate = "ON";
      document.getElementById('redstate').innerHTML = redstate;
    }

    else{
             
       info = JSON.parse(event.data);    //extract individual data from json string
       SOC = info["charge"];
       voltage = info["volts"];          //put each data type into javascript variable 
       current = info["amps"];
       temp = info["temp"];
       
       document.getElementById('charge').innerHTML = SOC;
       document.getElementById('voltage').innerHTML = voltage;    //replace values on page with new data values
       document.getElementById('current').innerHTML = current; 
       document.getElementById('temp').innerHTML = temp; 
       }
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('bluebutton').addEventListener('click', toggleblue);
    document.getElementById('redbutton').addEventListener('click', togglered);
  }
  function toggleblue(){
    websocket.send('toggleblue');
  }
  function togglered(){
    websocket.send('togglered');
  }
</script>        
</body>     
</html>  
</html>)rawliteral";





const char about_aceon[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>
  
<html>   
<head>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
<title> About AceOn </title>  
<style>   
Body {  
  font-family: Calibri, Helvetica, sans-serif;  
  background-color: lightgreen;  
}  
button {   
       background-color: #4CAF50;   
       width: 100%;  
        color: white;   
        padding: 15px;   
        margin: 10px 0px;   
        border: none;   
        cursor: pointer;   
         }   
 form {   
        border: 3px solid #f1f1f1;   
    }   
          
 .container {   
        padding: 25px;   
        background-color: lightblue;  
    }
 .font-style{
        font-weight:bold;
 }

  img {
    height: auto; 
    max-width: 100%;
  }
       
</style>   
</head>    
<body>    
    <center> <h1> About AceOn </h1> </center>   
    
        <div class="container"> 
          <form action="/return"> 
           <p><span class="font-style">Address:</span></p>
           <p>Unit 9B<br>Stafford Park 12<br>Telford<br>Shropshire<br>TF3 3BJ</p>
           <p><span class="font-style">Telephone: </span>01952 293388</p>
           <a href="https://www.aceongroup.com">Our website</a>
           <p> </p>
           <button type="Submit">Back</button> 
          </form>           
        </div>   
        
</body>     
</html>  
</html>)rawliteral";




const char device_image[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>
  
<html>   
<head>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
<title> Unit layout </title>  
<style>   
Body {  
  font-family: Calibri, Helvetica, sans-serif;  
  background-color: lightgreen;  
}  
button {   
       background-color: #4CAF50;   
       width: 100%;  
        color: white;   
        padding: 15px;   
        margin: 10px 0px;   
        border: none;   
        cursor: pointer;   
         }   
 form {   
        border: 3px solid #f1f1f1;   
    }   
      
 .container {   
        padding: 25px;   
        background-color: lightblue;  
    }
 .font-style{
        font-weight:bold;
 }

  img {
    height: auto; 
    max-width: 100%;
  }
       
</style>   
</head>    
<body>    
    <center> <h1> Unit layout </h1> </center>   
         <p>PUT IMAGE OF UNIT HERE</p> 
         <form action="/return"> 
            <button type="Submit">Back</button> 
         </form>           
</body>     
</html>  
</html>)rawliteral";



void notifyClients() {
  ws.textAll(String(blueledState));
  ws.textAll(String(redledState));
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggleblue") == 0) {    //if "toggleblue" string was received
      
      if(blueledState==1){
        digitalWrite(2, LOW);
        blueledState = 0;
      }
      else{
        digitalWrite(2, HIGH);
        blueledState = 1;
      }
      notifyClients();

    }
    else if (strcmp((char*)data, "togglered") == 0) {    //if "togglered" string was received
      
      if(redledState==3){
        digitalWrite(4, LOW);
        redledState = 2;
      }
      else if(redledState==2){
        digitalWrite(4, HIGH);
        redledState = 3;
      }
      notifyClients();
    }
  }
}


void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void initWebSocket() {
  ws.onEvent(eventHandler);
  server.addHandler(&ws);
}



class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html); 
  }
};




int get_SOC(){
  Wire.beginTransmission( BQ34Z100);
  Wire.write( 0x2C);       // command for SOC
  Wire.endTransmission();
  Wire.requestFrom( BQ34Z100, 2);  // two bytes
  uint16_t x = Wire.read();  // lowest byte first
  uint16_t y = Wire.read();  // MSB next
  x |= y << 8 ;
  return (int)x;
}


int get_voltage(){
  Wire.beginTransmission( BQ34Z100);
  Wire.write( 0x08);       // command for voltage
  Wire.endTransmission();
  Wire.requestFrom( BQ34Z100, 2);  // two bytes
  uint16_t x = Wire.read();  // lowest byte first
  uint16_t y = Wire.read();  // MSB next
  x |= y << 8 ;
  return (int)x;
}


int get_current(){
  Wire.beginTransmission( BQ34Z100);
  Wire.write( 0x14);       // command for average current
  Wire.endTransmission();
  Wire.requestFrom( BQ34Z100, 2);  // two bytes
  uint16_t x = Wire.read();  // lowest byte first
  uint16_t y = Wire.read();  // MSB next
  x |= y << 8 ;
  if (x<65536 and x>32767) x = 65536 - x;
  return (int)x;
}


float get_temp() {
  Wire.beginTransmission( BQ34Z100);
  Wire.write( 0x06);       // command for temperature
  Wire.endTransmission();
  Wire.requestFrom( BQ34Z100, 2);  // two bytes
  uint16_t x = Wire.read();  // lowest byte first
  uint16_t y = Wire.read();  // MSB next
  x |= y << 8 ;
  float temp = (float( x) / 10.0) - 273.15;
  return temp;
}




void setupServer(){
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){    //when requested by connecting to esp32 wi-fi

    if(lock_login_page==0){
      request->send_P(200, "text/html", index_html);     //this sends the initial login page
      //Serial.println("Client Connected");
      time_out = 0;
    }
  });


  server.on("/about", HTTP_GET, [](AsyncWebServerRequest *request){    //when requested by pressing about aceon button
  
      if(time_out==0){
        request->send_P(200, "text/html", about_aceon);     //this sends the about aceon page
      }
      else{
        lock_login_page = 0;
        device_connected = 0;
        request->send(200, "text/html", "This connection has timed out <br>");//<a href=\"/\">Return to Home Page</a>");
      }
    });


  server.on("/unit image", HTTP_GET, [](AsyncWebServerRequest *request){    //when requested by pressing about aceon button
  
      if(time_out==0){
        request->send_P(200, "text/html", device_image);     //this sends the unit image page
      }
      else{
        lock_login_page = 0;
        device_connected = 0;
        request->send(200, "text/html", "This connection has timed out <br>");//<a href=\"/\">Return to Home Page</a>");
      }
    });
    
      
  server.on("/return", HTTP_GET, [](AsyncWebServerRequest *request){    //when requested by pressing back button
  
      if(time_out==0){
        request->send_P(200, "text/html", display_page, processor);    //request display page and processor functions
      }
      else{
        lock_login_page = 0;
        device_connected = 0;
        request->send(200, "text/html", "This connection has timed out <br>");//<a href=\"/\">Return to Home Page</a>");
      }
    });


  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {    //when requested by form action (/get) 
      String inputMessage;                                            
      String inputParam;
      
      if (request->hasParam("username")) {      //if a username has been entered
        inputMessage = request->getParam("username")->value();   
        inputParam = "username";
        user_name = inputMessage;
        if(user_name=="user"){                //if username is correct
          Serial.println(inputMessage);
          name_received = true;
        }
      }
    
      if (request->hasParam("password")) {     //if a password has been entered
        inputMessage = request->getParam("password")->value();
        inputParam = "password";
        proficiency = inputMessage;
        if(proficiency=="pass"){           //if password is correct
          Serial.println(inputMessage);
          proficiency_received = true;
          
        }
      }

      if(name_received && proficiency_received and time_out==0){   //if both username and password correct and page not timed out
        SOC = get_SOC();
        voltage = get_voltage();
        current = get_current();
        temp = get_temp();
        SOC_string = String(SOC);
        voltage_string = String(voltage);
        current_string = String(current);
        temp_string = String(temp);
        request->send_P(200, "text/html", display_page, processor);    //request display page and processor functions
        lock_login_page = 1;
        device_connected = 1;   
        timerRestart(timer);
        
      }
      else if(name_received && proficiency_received and time_out==1){
        lock_login_page = 0;
        device_connected = 0;
        request->send(200, "text/html", "This connection has timed out <br>");//<a href=\"/\">Return to Home Page</a>");
      }
      else if(name_received && proficiency_received !=true){
        request->send(200, "text/html", "The details entered are incorrect, please try again <br><a href=\"/\">Return to Home Page</a>");
      }
  });


  
    server.on("/aceon", HTTP_GET, [](AsyncWebServerRequest *request){
      // request->send(SPIFFS, "/aceon.png", "image/png");
      request->send(LittleFS, "/aceon.png", "image/png");
    });
    server.on("/aceon2", HTTP_GET, [](AsyncWebServerRequest *request){
      // request->send(SPIFFS, "/aceon2.png", "image/png");
      request->send(LittleFS, "/aceon2.png", "image/png");
    }); 
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}


void test() {
  // Request the DeviceName from the BMS
  Wire.beginTransmission(BQ34Z100);
  Wire.write(0x21); // Send register address
  Wire.endTransmission(false); // Send repeated start condition
  
  Wire.requestFrom(BQ34Z100, 21); // Request the ASCII block
  char deviceName[21 + 1]; // Create buffer to store device name
  int i = 0;

  // Read each byte into the device name buffer
  while (Wire.available() && i < 21) {
    deviceName[i++] = Wire.read();
  }
  deviceName[i] = '\0'; // Null-terminate the string

  Serial.print("Device Name: ");
  Serial.println(deviceName);

  delay(1000); // Wait before next read for stability
}

void print_8bit(uint8_t x) {
  String binaryString = "";
  for (int i = 7; i >= 0; i--) {
    binaryString += (x & (1 << i)) ? '1' : '0';
  }
  Serial.println(binaryString);
}
void print_16bit(uint16_t x) {
  String binaryString = "";
  for (int i = 15; i >= 0; i--) {
    binaryString += (x & (1 << i)) ? '1' : '0';
  }
  Serial.println(binaryString);
}

void setup(){
  
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  Wire.begin(22,21,100000);  //connect I2C
  Serial.begin(115200);

  Wire.beginTransmission( BQ34Z100);
  byte error = Wire.endTransmission();
  if( error == 0)
  {
    Serial.println("The chip is found");
  }
  else
  {
    Serial.println("Error, not connected to the chip");
  }


  WiFi.mode(WIFI_AP); 
  SOC = get_SOC();                                      //read SOC from BMS
  SOC_string = String(SOC);                             //convert SOC to string
  device_title = mytitle + SOC_string + percent; 
  WiFi.softAP(device_title);
  initWebSocket();
  setupServer();
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();
  // SPIFFS.begin();
  initLittleFS();

  // 3 minute timer to check the connection is still live
  timer = timerBegin(0, 80, true);                //using timer 0, prescaler of 80, count up
  timerAttachInterrupt(timer, &onTimer, true);    //  hardware timer, address of ISR, edge type
  timerAlarmWrite(timer, 180000000, true);        // use timer, count up to (180000000 u seconds = 3 mins), reload
  timerAlarmEnable(timer);  

  // 1 second timer to "refresh" the info on the page
  timer2 = timerBegin(1, 80, true);                //using timer 1, prescaler of 80, count up
  timerAttachInterrupt(timer2, &onTimer2, true);    //  hardware timer, address of ISR, edge type
  timerAlarmWrite(timer2, 1000000, true);        // use timer, count up to (1000000 u seconds = 1 sec), reload
  timerAlarmEnable(timer2);                 
}




void loop(){

  if (interruptCounterA > 0) {      //if interrupt has fired (flag has set)

    portENTER_CRITICAL(&timerMux);
    interruptCounterA--;              //reset interrupt A flag
    portEXIT_CRITICAL(&timerMux);

    if(device_connected==0){
       
      SOC = get_SOC();                                      //read SOC from BMS
      SOC_string = String(SOC);                             //convert SOC to string
      device_title = mytitle + SOC_string + percent;        //create title with SOC in it
      WiFi.softAP(device_title);                            //update title
      
    }
    else{

      time_out = 1;    //stop page from reloading if requested
      device_connected = 0;
      ws.closeAll();   //close websocket
      
    }
  }

  if(interruptCounterB > 0){

    portENTER_CRITICAL(&timerMux);
    interruptCounterB--;              //reset interrupt B flag
    portEXIT_CRITICAL(&timerMux);

    SOC = get_SOC();
    voltage = get_voltage();
    current = get_current();
    temp = get_temp();

    if(SOC != old_SOC or voltage != old_voltage or current != old_current or temp != old_temp){   //if any BMS readings have changed
            
      str1 = "{\"charge\":";
      str2 = String(SOC);
      str3 = ", \"volts\":";
      str4 = String(voltage);    //build up string allowing placement of variables 
      str5 = ", \"amps\":";
      str6 = String(current);
      str7 = ", \"temp\":";
      str8 = String(temp);
      str9 = "}";
      comp_string = str1 + str2 + str3 + str4 + str5 + str6 + str7 + str8 + str9;   //put all bits together
      
      json.setJsonData(comp_string);
      json.toString(JSON_string, false);//, true /* prettify option */);
      
      ws.textAll(JSON_string);   //send string of JSON data through websocket
      //Serial.print(JSON_string);
      old_SOC = SOC;
      old_voltage = voltage;
      old_current = current;
      old_temp = temp;
    }
  }
  

  ws.cleanupClients();
  
  dnsServer.processNextRequest();
  if(name_received && proficiency_received){
      
      name_received = false;
      proficiency_received = false;
           
    }
    
}
