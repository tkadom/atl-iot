 
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
extern "C" {
#include "user_interface.h"
}

#define WLAN_SSID       "twiot"
#define WLAN_PASS       "tworker01"


#define TW_SERVER      "atliot.com"
#define TW_SERVERPORT  1883                   // use 8883 for SSL
#define TW_USERNAME    "xxxxx"
#define TW_KEY         "xxxxx"
#define PIR_INPUT_PIN 12

WiFiClient client;

//WiFiClientSecure client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = TW_SERVER;
const char MQTT_USERNAME[] PROGMEM  = TW_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = TW_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, TW_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);


const char MOTION[] PROGMEM = "/motion";
Adafruit_MQTT_Publish motionSensor = Adafruit_MQTT_Publish(&mqtt, MOTION);

void turnOnLED() { 
  digitalWrite(BUILTIN_LED, LOW);
}

void turnOffLED(){
  digitalWrite(BUILTIN_LED, HIGH);
}
void MQTT_connect() {
  int8_t ret;
  
  if (mqtt.connected()) {
    return;
  }

  Serial.print("MQTT Connection: ");

  uint8_t retries = 20;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retry in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
          abort();  // lets reset and try this show again...
       }
  }
  Serial.println("MQTT Connected!");
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}


void sendMotionReport(int elapsedTime) {
  char sendBuffer[128];

  // Generate client name based on MAC address 
  String clientName;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);

    if (elapsedTime > 0 ) {
           sprintf(sendBuffer,"{\"id\":\"%s\",\"name\":\"%s\",\"motion\":\"%s\",\"time\":\"%d\"}",clientName.c_str(), "Entry","End",elapsedTime/1000);
    }
    else {
            sprintf(sendBuffer,"{\"id\":\"%s\",\"name\":\"%s\",\"motion\":\"%s\",\"time\":\"%d\"}",clientName.c_str(), "Entry","Start",0);
   
    }
  Serial.println(sendBuffer);
  if (! motionSensor.publish(sendBuffer)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
}

void reportMotion (int secondsToTimeout) {
   static int motionTimer;
   static int startMotionTime;
   static int oldSensorValue = 0;
   static bool currentlyReadingMotion = false;
   bool newSensorValue = digitalRead(PIR_INPUT_PIN) && digitalRead(PIR_INPUT_PIN);  //try to avoid noise by reading twice...

  if ( newSensorValue != oldSensorValue )
  {
    motionTimer = millis();
    if (!currentlyReadingMotion)
    { 
      Serial.write("detected new motion ");
      sendMotionReport(0);
      startMotionTime = motionTimer;
    }
    oldSensorValue = newSensorValue;
    currentlyReadingMotion=true;
  }
  if (currentlyReadingMotion)
  {
    turnOnLED();
     if ( (millis() - motionTimer) > (secondsToTimeout)) /* provided seconds that have passed since last motion detected */
     {
        currentlyReadingMotion = false;
        Serial.write("motion has stopped ");
        sendMotionReport(millis() - (startMotionTime + (secondsToTimeout)));
    }
  }
  else
  {
    turnOffLED();
  }
}

void setup() {
  system_phy_set_max_tpw(0.25); 
  pinMode(PIR_INPUT_PIN, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi access point.

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
}

void loop() {
  
  MQTT_connect();
  reportMotion(20 * 1000);  /* milliseconds */
  
}


