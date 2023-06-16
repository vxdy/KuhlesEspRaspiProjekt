/**
  Author: Marco B
  Github: https://github.com/vxdy/
  E-Mail: m.bajtl@centrapi.net
  License: MIT
*/

#include <DHT.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <Preferences.h>


#define TFT_PIN_CS 5
#define TFT_PIN_DC 2
#define TFT_PIN_RST 4
#define DHT_SENSOR_PIN 21
#define DHT_SENSOR_TYPE DHT11

const char* WIFI_SSID = "FBIT.IoT.Router4";
const char* WIFI_PASW = "WueLoveIoT";
const char* MQTT_BROKER = "192.168.104.230";
const char* mqttUser = "";
const char* mqttPassword = "";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

long lastMsg = 0;
char msg[50];
int value = 0;
bool updated = false;
int CurrentScreen = 1;

String gTemp = "";
String gHumi = "";
String gLehrer = "";
String gFach = "";
String gRaum = "";
String currentTime = "";

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

WiFiClient espClient;
PubSubClient mqttclient(espClient);

DynamicJsonDocument doc(1024);

Preferences preferences;

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (byte i = 0; i < length; i++) {
    char tmp = char(payload[i]);
    msg += tmp;
  }

  String strTopic = topic;

  if (strTopic == "esp/in/temp") {
    gTemp = msg;
  } else if (strTopic == "esp/in/humi") {
    gHumi = msg;
  } else if (strTopic == "esp/in/data") {
     deserializeJson(doc, msg);
     JsonObject obj = doc.as<JsonObject>();
    gLehrer = obj["lehrer"].as<String>();
    gRaum = obj["raum"].as<String>();
    gFach = obj["fach"].as<String>();
    if(gLehrer != NULL && gLehrer != ""){
      preferences.putString("lehrer", gLehrer);
      preferences.putString("raum", gRaum);
      preferences.putString("fach", gFach);
    }
  }

  if (msg == "NaN") {
    Serial.println("Error");
    return;
  }

  Serial.println(msg);
  Serial.println(strTopic);
  setDisplay();
}


void setup(void) {
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1);


  Serial.begin(9600);
  dht_sensor.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WIFI Connected");
  preferences.begin("gpio", false);
  mqttclient.setServer(MQTT_BROKER, 1883);


  while (!mqttclient.connected()) {
    Serial.println("Connecting to MQTT...");

    if (mqttclient.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(mqttclient.state());
      delay(2000);
    }
  }

  String lehrerPref = preferences.getString("lehrer", "");
  String raumPref =  preferences.getString("raum", "");
  String fachPref =   preferences.getString("fach", "");

  if(lehrerPref != NULL && lehrerPref != "") {
    gLehrer = lehrerPref;
    gRaum = raumPref;
    gFach = fachPref;
  } 

  mqttclient.setCallback(callback);
  mqttclient.subscribe("esp/in/temp");
  mqttclient.subscribe("esp/in/data");
  mqttclient.subscribe("esp/in/humi");
}

void loop() {
  if (mqttclient.state() != 0) {
    while (!mqttclient.connected()) {
      Serial.println("Connecting to MQTT...");

      if (mqttclient.connect("ESP32Client", mqttUser, mqttPassword)) {
        Serial.println("connected");

      } else {

        Serial.print("failed with state ");
        Serial.print(mqttclient.state());
        delay(2000);
      }
    }
  }
  float humi = dht_sensor.readHumidity();
  float tempC = dht_sensor.readTemperature();
  String msgHumi = String(humi);
  String msgTemp = String(tempC);

  char tempString[8];
  dtostrf(tempC, 1, 1, tempString);
  char humString[8];
  dtostrf(humi, 1, 1, humString);

  if (updated == false) {
    gTemp = tempString;
    gHumi = humString;
    setDisplay();
  }


  mqttclient.publish("esp/out/temp", tempString);
  mqttclient.publish("esp/out/humi", humString);

  if (CurrentScreen == 1) {
    CurrentScreen = 2;
  } else {
    CurrentScreen = 1;
  }

  setDisplay();
  delay(5000);

  mqttclient.loop();
}


void setDisplay() {
  tft.fillScreen(ST7735_BLACK);

    tft.setTextSize(4);
    tft.setCursor(1, 10);
    tft.setTextColor(ST7735_WHITE);

  if (CurrentScreen == 1) {

    tft.print("T:" + String(gTemp));
    tft.setCursor(1, 40);
    tft.print("H:" + String(gHumi));
  }
  
   if(CurrentScreen == 2) {
     JsonObject obj = doc.as<JsonObject>();
    tft.print("R:" + gRaum);
    tft.setCursor(1, 40);
    tft.print("L:" + gLehrer);
    tft.setCursor(1, 80);
    tft.print("F:");
    tft.setCursor(50, 80);
    tft.setTextSize(3);
    tft.print(gFach);
  
  }

  updated = true;
}