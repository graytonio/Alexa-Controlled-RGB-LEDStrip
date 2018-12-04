#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "creds.h"
#include "FS.h"

#define r 16
#define g 5
#define b 4

HTTPClient http;
String id;
 
void setup () {
  //Set lights to off durring boot
  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
  setLight(0, 0, 0);
  
  SPIFFS.begin();
  Serial.begin(115200);

  //Connect to Wifi Connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected Successfully");
  //Resolve device ID
  getID();
}

void getID(){
  if(SPIFFS.exists("/config")){
    File idConfig = SPIFFS.open("/config", "r");
    while(idConfig.available()) id += char(idConfig.read());
    idConfig.close();
    
    http.begin(getPath + id);
    int httpCode = http.GET();
    http.end();
    if(httpCode == 404) newDevice();
  }else{
    Serial.println("File does not exist");
    newDevice();
  }
}

void newDevice(){
  Serial.println("Sending create request to server");
  //Send request for new device
  http.begin(newPath);
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

void setLight(int r_in, int g_in, int b_in){
  analogWrite(r, r_in);
  analogWrite(g, g_in);
  analogWrite(b, b_in);
}

void loop() {
  DynamicJsonBuffer jsonBuffer(512);
  if (WiFi.status() == WL_CONNECTED) { 
    http.begin(getPath + id);
    int httpCode = http.GET();
    if (httpCode > 0) {
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      setLight(root["r"], root["g"], root["b"]);
    }
    http.end();
  }
  delay(3000);
}
