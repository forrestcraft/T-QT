/*
  An example showing rainbow colours on a 1.8" TFT LCD screen
  and to show a basic example of font use.

  Make sure all the display driver and pin connections are correct by
  editing the User_Setup.h file in the TFT_eSPI library folder.

  Note that yield() or delay(0) must be called in long duration for/while
  loops to stop the ESP8266 watchdog triggering.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

#define WIFI_SSID "Jonesmo"
#define WIFI_PASSWORD "Rossydog11"
#define MQTT_HOST "192.168.68.132"
#define MQTT_PORT 1883

#define MQTT_TOPIC_RECV "sensor/co2_2/received" // MQTT topic to publish received IR codes
#define MQTT_TOPIC_SEND "sensor/co2_2/send" // MQTT topic to subscribe for sending IR codes
void callback(char* topic, byte* payload, unsigned int length) {}
#define CO2_SDA      17
#define CO2_SCL      16

WiFiClient espClient;
PubSubClient client(MQTT_HOST, MQTT_PORT, callback, espClient);
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  // Serial.println();
  // Serial.print("Connecting to ");
  // Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }
  randomSeed(micros());
  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());

}

long lastReconnectAttempt = 0;
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // String clientId = c;
    // clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect("co2_2", "homeassistant", "Ohlighu3zeerieyo8Ual6uephu2quoh2saache0aet3reexee3oogh5iehuPhoh4")) {
      // Serial.println("connected");
      client.subscribe(MQTT_TOPIC_RECV);
    } else {
      // Serial.print("failed, rc=");
      // Serial.print(client.state());
      // Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
SensirionI2CScd4x scd4x;

unsigned long targetTime = 0;
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11;

void setup(void) {
  
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Wire.begin(CO2_SDA, CO2_SCL);
  uint16_t error;
  char errorMessage[256];
  scd4x.begin(Wire);
  targetTime = millis() + 1000;

    // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
      // Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      // errorToString(error, errorMessage, 256);
      // Serial.println(errorMessage);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
      // Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      // errorToString(error, errorMessage, 256);
      // Serial.println(errorMessage);
  }

  // Serial.println("Waiting for first measurement... (5 sec)");
  
  setup_wifi();
  client.setServer(MQTT_HOST, 1883);
  client.setCallback(callback);
  tft.setFreeFont(&FreeMono24pt7b);
}

char data[500];
uint8_t i = 0;
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (targetTime < millis()) {
    targetTime = millis() + 10000;
    i %= 4;
    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool isDataReady = false;
    
    uint16_t error;
    char errorMessage[256];
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        // Serial.print("Error trying to execute getDataReadyFlag(): ");
        // errorToString(error, errorMessage, 256);
        // Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    temperature = (temperature*9/5)+32;
    if (error) {
        // Serial.print("Error trying to execute readMeasurement(): ");
        // errorToString(error, errorMessage, 256);
        // Serial.println(errorMessage);
    } else if (co2 == 0) {
        // Serial.println("Invalid sample detected, skipping.");
    } else {
        // Serial.print("Co2:");
        // Serial.print(co2);
        // Serial.print("\t");
        // Serial.print("Temperature:");
        // Serial.print(temperature);
        // Serial.print("\t");
        // Serial.print("Humidity:");
        // Serial.println(humidity);
        String payload = "{ \"ppm\": " + String(co2) + ","+
                          "\"temp\": " + String(temperature) + ","+
                          "\"humidity\": " + String(humidity) + "}";
        payload.toCharArray(data, (payload.length() + 1));
        client.publish(MQTT_TOPIC_SEND, data);
    }

    // Colour changing state machine
    for (int i = 0; i < 160; i++) {
      tft.drawFastVLine(i, 0, tft.height(), colour);
      switch (state) {
        case 0:
          green += 2;
          if (green == 64) {
            green = 63;
            state = 1;
          }
          break;
        case 1:
          red--;
          if (red == 255) {
            red = 0;
            state = 2;
          }
          break;
        case 2:
          blue ++;
          if (blue == 32) {
            blue = 31;
            state = 3;
          }
          break;
        case 3:
          green -= 2;
          if (green == 255) {
            green = 0;
            state = 4;
          }
          break;
        case 4:
          red ++;
          if (red == 32) {
            red = 31;
            state = 5;
          }
          break;
        case 5:
          blue --;
          if (blue == 255) {
            blue = 0;
            state = 0;
          }
          break;
      }
      colour = red << 11 | green << 5 | blue;
    }



    // The new larger fonts do not use the .setCursor call, coords are embedded
    tft.setTextColor(TFT_BLACK, TFT_BLACK); // Do not plot the background colour

    //tft.drawCentreString("Font size 2",81,12,2); // Draw text centre at position 80, 12 using font 2
    tft.drawCentreString("T:"+String(temperature), 62, 0, 4); // Draw text centre at position 80, 24 using font 4
    tft.drawCentreString(String(co2), 52, 30, 6); // Draw text centre at position 80, 24 using font 4
    tft.drawCentreString("Hum:"+String(humidity), 62, 80, 4); // Draw text centre at position 80, 24 using font 4



  }
}






