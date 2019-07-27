// based on this paste from @martinbateman
// https://pastebin.com/LNUvv1En
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>


#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_BL          4  // Dispaly backlight control pin


// Replace with your network credentials
const char* ssid     = "xxxxxx";
const char* password = "yyyyyy";
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
  Serial.begin(115200);

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
  }

  timeClient.begin();
  timeClient.setTimeOffset(3600);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
    
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  char timeString[25];
  char colourString2[25];
  sprintf (timeString, "%02i:%02i:%02i", timeClient.getHours (), timeClient.getMinutes (), timeClient.getSeconds ());
  colourString.toCharArray(colourString2,25);
  //sprintf (colourString2, "%d", sixteenBitHex);


  tft.setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
  tft.drawString("88:88:88",10,10,7); // Overwrite the text to clear it
  tft.setTextColor(rgb565Decimal, TFT_BLACK);
  tft.drawString (timeString, 10, 10, 7);
  tft.drawString (colourString2, 0, 80, 4);
  
  delay(1000);
}
