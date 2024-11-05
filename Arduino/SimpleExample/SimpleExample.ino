#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <FirebaseJson.h>
#include <LittleFS.h>

#include "html_page.h"

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point Aodhan";
const char* password = "password";

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

volatile int interruptCounterA = 0;
volatile int interruptCounterB = 0;
String correct_name = "user";
String user_name;
String proficiency;
String correct_proficiency = "pass";
bool name_received = false;
bool proficiency_received = false;
bool device_connected = 0;
bool time_out = 0;
String JSON_string;
String comp_string;
String str1, str2, str3, str4, str5, str6, str7, str8, str9;



FirebaseJson json;




portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;   //"timerMux"

void IRAM_ATTR onTimer() {       //ISR for timer
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounterA = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR onTimer2() {       //ISR for timer 2
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounterB = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}




void notifyClients() {
  /*
  ws.textAll(String(blueledState));
  ws.textAll(String(redledState));
  */
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  /*if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
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
  */
}


void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
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





void setupServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){    //when requested by connecting to esp32 wi-fi
    request->send_P(200, "text/html", index_html);     //this sends the initial login page
    Serial.println("Client Connected");
    time_out = 0;
  });

  server.on("/aceon", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/aceon.png", "image/png");
  });
}






void setup(){
  
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(115200);


  WiFi.mode(WIFI_AP); 
  WiFi.softAP(ssid); //, password);

  
  ws.onEvent(eventHandler);
  server.addHandler(&ws);

  setupServer();
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();

  LittleFS.begin(true);

  // 3 minute timer to check the connection is still live
  hw_timer_t *timer = timerBegin(0, 80, true);                //using timer 0, prescaler of 80, count up
  timerAttachInterrupt(timer, &onTimer, true);    //  hardware timer, address of ISR, edge type
  timerAlarmWrite(timer, 180000000, true);        // use timer, count up to (180000000 u seconds = 3 mins), reload
  timerAlarmEnable(timer);  

  // 1 second timer to "refresh" the info on the page
  hw_timer_t *timer2 = timerBegin(1, 80, true);                //using timer 1, prescaler of 80, count up
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
      WiFi.softAP(ssid); //, password);                            //update title
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

    if(true){   //if any BMS readings have changed
      str1 = "{\"Q\":";
      str2 = String(1);
      str3 = ", \"V\":";
      str4 = String(2);    //build up string allowing placement of variables 
      str5 = ", \"I\":";
      str6 = String(3);
      str7 = ", \"T\":";
      str8 = String(4);
      str9 = "}";
      comp_string = str1 + str2 + str3 + str4 + str5 + str6 + str7 + str8 + str9;   //put all bits together
      
      json.setJsonData(comp_string);
      json.toString(JSON_string, false);//, true /* prettify option */);
      
      // Serial.print(JSON_string);
      ws.textAll(JSON_string);   //send string of JSON data through websocket
    }
  }
  

  ws.cleanupClients();
  
  dnsServer.processNextRequest();
  if(name_received && proficiency_received){
      name_received = false;
      proficiency_received = false;
    }
    
}
