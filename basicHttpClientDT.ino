#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

//Enter your secrets here
const char* apiKey = "<apikey>";
const char* ssid = "<ssid>";
const char* password = "<password>";
const char* sslFingerprint = "90 00 99 74 2C 04 49 2A 86 69 E9 0A 2E CC F5 36 0B C1 CE E1";

String thingId = "<thingId>";

void setup() {
  //Set NTP server to GMT +1
  configTime(1 * 3600, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  delay(3000);
  
  //Configure ESP8266 onboard LED
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  for(uint8_t t = 4; t > 0; t--) {
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }

  WiFiMulti.addAP(ssid, password);
}

void loop() {
  if((WiFiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;
    
    //Subscribe to sensor data
    http.begin("api.disruptive-technologies.com", 443, "/v1/subscribe?thing_ids=" + thingId, sslFingerprint);
    http.addHeader("Accept", "application/json");
    http.addHeader("Authorization", "ApiKey " + String(apiKey));
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Connection", "keep-alive");

    int httpCode = http.GET();
    
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
  
      // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      // create buffer for read
      char buff[1024] = { 0 };

      // get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // read all data from server
      while(http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if(size) {
          // read up to 350 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
  
          // write it to Serial
          //Serial.write(buff, c);

          //Just to remove some artefacts
          char * jsonString = strchr(buff, '{');
          Serial.println(jsonString); 
          
          DynamicJsonBuffer jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(jsonString);

          if (!root.success()) {
            Serial.println("parseObject() failed");
          }

          String result = root["result"];
          String eventNumber = root["result"]["version"];
          String temperature = root["result"]["state_changed"]["temperature"];

          if (eventNumber.toInt() % 2 == 0) {
            digitalWrite(LED_BUILTIN, LOW);
          } else {
            digitalWrite(LED_BUILTIN, HIGH);
          }

          if(len > 0) {
              len -= c;
          }
        }
        delay(1);
      }
      Serial.println();
      Serial.print("[HTTP] connection closed or file end.\n");
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  }
}

