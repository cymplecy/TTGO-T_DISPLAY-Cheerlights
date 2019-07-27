/*
  Cheerlights Clock for TTGO-T-DISPLAY
 based on @martinbateman clock code
 https://t.co/1gRc56wNOE
 https://pastebin.com/4Ec6d4xY
 
ported to TTGO T2 Board by LeRoy Miller July 25, 2019 @kd8bxp

 
added Geolocate Timezone code July 25, 2019

merged back into cheerlights version by Simon Walters @cymplecy
 
*/
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>


#include <TFT_eSPI.h>
#include <SPI.h>

#include <TimeLib.h>

#include <ArduinoJson.h>  //use version 5.13.5
#include <HTTPClient.h>
#include <DNSServer.h>

#define TFT_BL          4  // Display backlight control pin

//TimeZone variables
String TZAPIKEY="xxxxxxx";  //http://timezonedb.com used to get gmtOffset
String payload, TZone, lastUpdate;
String coordinate; // = lat,lon
String coordinateTZ; //lat=xx.xxx&lng=xxx.xxx
long  gmtOffset_sec;
const int   daylightOffset_sec = 0;


// Replace with your network credentials
const char* ssid     = "xxxxxx";
const char* password = "xxxxxx";
const char* mqtt_server = "simplesi.cloud";

int cheer_red = 0;
int cheer_green = 0;
int cheer_blue = 0;
unsigned int rgb565Decimal = 0x8410;
unsigned int newrgb565Decimal;
String rgb565Hex;
String colourString = "not set yet";
String newColourString;

String strData;
String topicStr;



WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient espClient;
PubSubClient client(espClient);

TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t serialMutex = NULL;
void Task1 (void *pvParams);
void Task2 (void *pvParams);

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  topicStr = topic;

  Serial.print("Message:");
  
  strData = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    strData += (char)payload[i];
  }
  
  Serial.println();
  Serial.println("-----------------------");

  if (topicStr.endsWith("cheerlights/rgb565Hex")) {
   
    //colourString = newColourString;
    rgb565Hex = strData;//.toInt();
    Serial.println("Hex");
  
    Serial.println(rgb565Hex);
  }  
  
  if (topicStr.endsWith("cheerlights/rgb565Decimal")) {
   
    colourString = newColourString;
    rgb565Decimal = strData.toInt();
    Serial.println("*******");
  
    Serial.println(rgb565Decimal);
  }  
  if (topicStr.endsWith("cheerlights")) {
   
    newColourString = "Cheerlights:" + strData + "                         ";
    //sixteenBitHex = newSixteenBitHex;
    Serial.println(strData);
  }  
} // end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("cheerlights",1);
      client.subscribe("cheerlights/rgb565Decimal",1);
      client.subscribe("cheerlights/rgb565Hex",1);      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin (115200);
  serialMutex = xSemaphoreCreateMutex ();

  if (TFT_BL > 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println (".");
  }
  Serial.println(WiFi.localIP());

  geolocation(); //get latitude/longitude from external IP address (used to find Time zone)
  timeZone(); //used to get time zone
 
  timeClient.begin();
  timeClient.setTimeOffset(gmtOffset_sec); //timeClient.setTimeOffset(3600);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


  xTaskCreate (updateNTP, "NTP Client", 4096, NULL, 2, NULL);
  xTaskCreate (updateScreen, "Screen", 4096, NULL, 1, NULL);
}

void getJson(String url) {
 
   if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
     HTTPClient http;  //Declare an object of class HTTPClient
     http.begin(url);  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
     if (httpCode > 0) { //Check the returning code
       payload = http.getString();   //Get the request response payload
     
    }
 
    http.end();   //Close connection
 
  }
}
 
 
void geolocation(){
 
    String url;
    url = "http://ip-api.com/json";
    getJson(url);
 
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(payload);
  root.printTo(Serial);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
 
  // Extract values
  Serial.println(("Response:"));
  Serial.println(root["lat"].as<char*>());
  Serial.println(root["lon"].as<char*>());
  coordinate = String(root["lat"].as<char*>()) + "," + String(root["lon"].as<char*>());
  //coordinatePrev = "lat=" + String(root["lat"].as<char*>()) + "&lon=" + String(root["lon"].as<char*>()); //do we use this anywhere (?)
  coordinateTZ = "lat=" + String(root["lat"].as<char*>()) + "&lng=" + String(root["lon"].as<char*>());
  //mylat = root["lat"];
  //mylon = root["lon"];
 // M5.Lcd.println("My Lat/Lon: "+coordinate);
 Serial.println("My Lat/Lon: "+coordinate);
}
 
/*
 * Used to get the GMT offset for your timezone
 */
 
 void timeZone() {
 String url;
 url = "http://api.timezonedb.com/v2/get-time-zone?key=" + TZAPIKEY + "&format=json&fields=gmtOffset,abbreviation&by=position&"+coordinateTZ;
 getJson(url);
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  gmtOffset_sec = root["gmtOffset"];
  const char* temp1 = root["abbreviation"];
  TZone = (String)temp1;
  Serial.print("GMT Offset = ");
  Serial.print(gmtOffset_sec);
  Serial.println(" " + TZone);
  //M5.Lcd.println("GMT Offset = "+(String)gmtOffset_sec);
}
 
 

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void updateNTP (void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (xSemaphoreTake (serialMutex, (TickType_t)10) == pdTRUE) {
      while(!timeClient.update()) {
        timeClient.forceUpdate();
      }
      setTime (timeClient.getEpochTime ());
      xSemaphoreGive (serialMutex);
    }
    vTaskDelay ((1000/portTICK_PERIOD_MS) * 60 * 60);  // update every hour
  }
}

void updateScreen (void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (xSemaphoreTake (serialMutex, (TickType_t)10) == pdTRUE) {
      char timeString[25];
      char colourString2[25];
      colourString.toCharArray(colourString2,25);

      time_t t = now (); 
      sprintf (timeString, "%02i:%02i:%02i", hour (t), minute (t), second (t));
      xSemaphoreGive (serialMutex);

      tft.setTextColor(0x39C4, TFT_BLACK);
      tft.drawString("88:88:88",10,10,7);
      tft.setTextColor(rgb565Decimal, TFT_BLACK);
      tft.drawString (timeString, 10, 10, 7);
      tft.drawString (colourString2, 0, 80, 4);
    }
    vTaskDelay (1000/portTICK_PERIOD_MS);
  }
}
