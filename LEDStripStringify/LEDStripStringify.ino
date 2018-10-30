#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


#define r 16
#define g 5
#define b 4

const char* ssid = "2C";
const char* password = "";
HTTPClient http;
 
void setup () {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  pinMode(r, OUTPUT);
  pinMode(g, OUTPUT);
  pinMode(b, OUTPUT);
}

void setLight(int r_in, int g_in, int b_in){
  analogWrite(r, r_in);
  analogWrite(g, g_in);
  analogWrite(b, b_in);
}

void loop() {
  DynamicJsonBuffer jsonBuffer(512);
  if (WiFi.status() == WL_CONNECTED) { 
    http.begin("http://pers.conheart.com/led");
    int httpCode = http.GET();
    if (httpCode > 0) {
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      setLight(root["r"], root["g"], root["b"]);
    }
    http.end();
  }
  delay(3000);
}
