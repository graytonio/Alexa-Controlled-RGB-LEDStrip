#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "DNSServer.h"
#include "FS.h"

//Wiring Pins
#define r 16
#define g 5
#define b 4

#define initArgs "{\"args\": {\"name\": \"LED Strip\",\"type\": \"led-strip\", \"r\": 255, \"g\": 255, \"b\": 255}}"

const byte DNS_PORT = 53;
IPAddress apIP(10,10,10,1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

HTTPClient http;
bool wifi = false;
String ssid;
String password;
String hub;
String id;
String portal;

void setup () {
  WiFi.softAPdisconnect(true);
  
  //Set lights to off durring boot
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
  setLight(0, 0, 0);

  SPIFFS.begin();
  Serial.begin(115200);
  
  getWifi();
}

void initPortal(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP("LED STRIP");
  dnsServer.start(DNS_PORT, "*", apIP);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/connect", HTTP_POST, handleConnection);
  webServer.begin();
  File root = SPIFFS.open("/index.html", "r");
  while(root.available()) portal+= char(root.read());
  root.close();
}

void handleRoot(){
  webServer.send(200, "text/html", portal);
}

void handleConnection(){
  ssid = webServer.arg("ssid");
  password = webServer.arg("pass");
  hub = webServer.arg("hub");
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(hub);
  webServer.send(200);
  connectToNetwork();
  ESP.reset();
}

//Resolve ID with Server
void getID(){
  if(SPIFFS.exists("/config")){
    File idConfig = SPIFFS.open("/config", "r");
    while(idConfig.available()) id += char(idConfig.read());
    idConfig.close();

    http.begin("http://" + String(hub) + "/get/" + id);
    int httpCode = http.GET();
    http.end();
    if(httpCode == 404) newDevice();
  }else{
    Serial.println("File does not exist");
    newDevice();
  }
}

void getWifi(){
  if(SPIFFS.exists("/wifi")){
    Serial.println("Loading Wifi Configuration");
    File wifiCon=SPIFFS.open("/wifi", "r");
    int line = 0;
    while(wifiCon.available()){
      char in = char(wifiCon.read());
      if(in == '\n'){
        line++;
        continue;
      }
      if(line == 0) ssid += in;
      if(line == 1) password += in;
      if(line == 2) hub += in;
    }
    wifiCon.close();

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(hub);
    Serial.println("Connecting");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    for(int i=0; i<5; i++){
      delay(1000);
    }
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Connected");
      wifi = true;
      getID();
    }else{
      Serial.println("Connection Failed Using captive portal");
      WiFi.disconnect();
      initPortal();
    }
  }else{
    initPortal();
  }
  
}

//Connect to Wifi Connection
void connectToNetwork(){
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println(String(ssid));
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected Successfully");
  wifi=true;
  File wifiCon = SPIFFS.open("/wifi", "w");
  wifiCon.print(ssid + '\n');
  wifiCon.print(password + '\n');
  wifiCon.print(hub);
  wifiCon.close();
  getID();
}

//Create a new device on the server
void newDevice(){
  Serial.println("Sending create request to server");
  //Send request for new device
  http.begin("http://" + String(hub) + "/new");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(initArgs);

  //Save response ID
  DynamicJsonBuffer jsonBuffer(512);
  JsonObject& root = jsonBuffer.parseObject(http.getString());
  id = root["id"].as<String>();
  http.end();

  //Write to internal file system
  File idConfig = SPIFFS.open("/config", "w");
  idConfig.print(id);
  idConfig.close();
}

//Set Light Channels
void setLight(int r_in, int g_in, int b_in){
  analogWrite(r, r_in);
  analogWrite(g, g_in);
  analogWrite(b, b_in);
}

void loop() {
  if(wifi){
    DynamicJsonBuffer jsonBuffer(512);
    if (WiFi.status() == WL_CONNECTED) {
      http.begin("http://" + String(hub) + "/get/" + id);
      int httpCode = http.GET();
      if (httpCode > 0) {
        JsonObject& root = jsonBuffer.parseObject(http.getString());
        setLight(root["r"], root["g"], root["b"]);
      }
      http.end();
    }
    delay(3000);
  }else{
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
}
