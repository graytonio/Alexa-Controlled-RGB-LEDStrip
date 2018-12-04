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

#define initArgs "{\"args\": {\"name\": \"LED Strip\",\"type\": \"led-strip\", \"r\": 255, \"g\": 255, \"b\": 255}}";

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
  //Set lights to off durring boot
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
  setLight(0, 0, 0);

  SPIFFS.begin();
  Serial.begin(115200);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP("LED STRIP");

  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/connect", HTTP_POST, handleConnection);
  webServer.begin();

  File root = SPIFFS.open("/index.html", "r");
  while(root.available()) portal+= char(root.read());
  root.close

  //Resolve device ID
  getID();
}

void handleRoot(){
  webServer.send(200, "text/html", portal);
}

void handleConnection(){
  ssid = webServer.arg("ssid");
  pass = webServer.arg("pass");
  hub = webServer.arg("hub");
  server.sendHeader("Location", "/");
  server.send(303);
  connectToNetwork();
}

//Resolve ID with Server
void getID(){
  if(SPIFFS.exists("/config")){
    File idConfig = SPIFFS.open("/config", "r");
    while(idConfig.available()) id += char(idConfig.read());
    idConfig.close();

    http.begin("http://" + hub + "/get/" + id);
    int httpCode = http.GET();
    http.end();
    if(httpCode == 404) newDevice();
  }else{
    Serial.println("File does not exist");
    newDevice();
  }
}

//Connect to Wifi Connection
void connectToNetwork(){
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected Successfully");
  wifi=true;
}

//Create a new device on the server
void newDevice(){
  Serial.println("Sending create request to server");
  //Send request for new device
  http.begin("http://" + hub + "/new");
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
      http.begin("http://" + hub + "/get/" + id);
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
