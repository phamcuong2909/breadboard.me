/*
 Sketch which publishes temperature data from a DS1820 sensor to a MQTT topic.

 This sketch goes in deep sleep mode once the temperature has been sent to the MQTT
 topic and wakes up periodically (configure SLEEP_DELAY_IN_SECONDS accordingly).

 Hookup guide:
 - connect D0 pin to RST pin in order to enable the ESP8266 to wake up periodically
 - DS18B20:
     + connect VCC (3.3V) to the appropriate DS18B20 pin (VDD)
     + connect GND to the appopriate DS18B20 pin (GND)
     + connect D4 to the DS18B20 data pin (DQ)
     + connect a 4.7K resistor between DATA and VCC.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//#define SERIAL_EN             //comment this out when deploying save a few KB of code size
#define SERIAL_BAUD    115200
#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); }
  #define DEBUGln(input) {Serial.println(input);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

#define SLEEP_DELAY_IN_SECONDS  900
#define ONE_WIRE_BUS            2      // DS18B20 pin (D4)

const char* ssid = "Sandiego";
const char* password = "0988807067";

const char* client_id = "TempSensor1";
// use public MQTT broker
//const char* mqtt_server = "broker.hivemq.com";
// use local MQTT broker at home network
const char* mqtt_server = "192.168.1.110";
const char* mqtt_username = "<MQTT_BROKER_USERNAME>";
const char* mqtt_password = "<MQTT_BROKER_PASSWORD>";
const char* temp_topic = "TempSensor1/temperature"; // change to something unique
const char* vdd_topic = "TempSensor1/battery"; // change to something unique

ADC_MODE(ADC_VCC); //vcc read

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperatureString[6];
char vddString[4];
long setupStartMillis;
int retry_count = 15; // we will retry connecting for 15 times only to save battery in case network fails

void setup() {
  setupStartMillis = millis();
  Serial.begin(SERIAL_BAUD);
  DEBUG("Setup start millis: "); DEBUGln(setupStartMillis);

  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup OneWire bus
  DS18B20.begin();
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  DEBUG("Connecting to "); DEBUGln(ssid);

  WiFi.begin(ssid, password);
  //uncomment to use static ip to shorten config time
  WiFi.config(IPAddress(192,168,1,200), IPAddress(192,168,1,100), IPAddress(255,255,255,0));

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    DEBUG(".");
    retry_count--;
    if (retry_count == 0) {
      DEBUG("Entering deep sleep mode for "); DEBUG(SLEEP_DELAY_IN_SECONDS); DEBUGln(" seconds...");
      ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
    }
  }

  long duration = millis()-setupStartMillis;

  DEBUGln("");
  DEBUGln("WiFi connected");
  DEBUG("IP address: ");
  DEBUGln(WiFi.localIP());
  DEBUG("Duration: "); DEBUGln(duration);
}

void callback(char* topic, byte* payload, unsigned int length) {
  DEBUG("Message arrived [");
  DEBUG(topic);
  DEBUG("] ");
  for (int i = 0; i < length; i++) {
    DEBUG((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DEBUG("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      DEBUGln("connected");
    } else {
      DEBUG("failed, rc=");
      DEBUG(client.state());
      retry_count--;
      if (retry_count == 0) {
        DEBUG("Entering deep sleep mode for "); DEBUG(SLEEP_DELAY_IN_SECONDS); DEBUGln(" seconds...");
        ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
      } else {
        DEBUGln(" try again in 5 seconds");
        // Wait 1 seconds before retrying
        delay(1000);
      }
    }
  }
}

float getTemperature() {
  DEBUGln("Requesting DS18B20 temperature...");
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temperature = getTemperature();
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  // send temperature to the serial console
  DEBUG("Sending temperature: "); DEBUGln(temperatureString);
  // send temperature to the MQTT topic
  client.publish(temp_topic, temperatureString);
  // read battery status
  float vdd = ESP.getVcc() / 1000.0;
  // convert battery status to a string
  dtostrf(vdd, 1, 1, vddString);
  // send to the serial console
  DEBUG("Sending battery status: "); DEBUGln(vddString);
  // send vdd status to the MQTT topic
  client.publish(vdd_topic, vddString);
  delay(100);

  DEBUG("Closing MQTT connection...");
  client.disconnect();

  DEBUG("Entering deep sleep mode for "); DEBUG(SLEEP_DELAY_IN_SECONDS); DEBUGln(" seconds...");
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
}
