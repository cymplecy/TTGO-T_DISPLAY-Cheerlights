// based on @martinbateman clock code
// https://t.co/1gRc56wNOE
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>


#include <TFT_eSPI.h>
#include <SPI.h>

#include <TimeLib.h>

#define TFT_BL          4  // Display backlight control pin

// Replace with your network credentials
const char* ssid     = "xxxxx";
const char* password = "yyyyy";
const char* mqtt_server = "simplesi.cloud";

int cheer_red = 0;
int cheer_green = 0;
int cheer_blue = 0;
unsigned int rgb565Decimal = 0x8410;
unsigned int newrgb565Decimal;
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

  timeClient.begin();
  timeClient.setTimeOffset(3600);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


  xTaskCreate (updateNTP, "NTP Client", 4096, NULL, 2, NULL);
  xTaskCreate (updateScreen, "Screen", 4096, NULL, 1, NULL);
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
