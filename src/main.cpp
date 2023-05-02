#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PubSubClient.h"

#define MONITORWIFI_PERIOD 5000
#define MYTASK_PERIOD 2000
#define GETLOCATION_PERIOD 5500
#define STARTUP_DELAY 3000
#define LED 2

QueueHandle_t publish_queue;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

const char* ssid = "balboa271apartments"; 
const char* password = "balboa271";
String building = "";
const char* MQTT_Broker = "35.173.206.112";
const char* topic = "config";
const int MQTT_Port = 1883;
bool InLandmark = false;

void blink() {
digitalWrite(LED, HIGH);
delay(500);
digitalWrite(LED, LOW);
delay(500);
}

void initWiFi() { 
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) { 
Serial.print(".");
delay(1000);
blink();
}
Serial.println(WiFi.localIP());
digitalWrite(LED, HIGH);
}

void monitor_wifi_task(void *p) {

int n;

while(1) {
//WiFi.disconnect();
//delay(100);
n = WiFi.scanNetworks();
Serial.println("scan done");
if (n == 0) {
Serial.println("no networks found");
} else {
Serial.print(n);
Serial.println(" networks found");
for (int i = 0; i < n; ++i) {
// Print SSID for each network found
Serial.print(i + 1);
Serial.print(": ");
Serial.println(WiFi.SSID(i));
// Verification if we are close to any of these buildings
if(WiFi.SSID(i) == "Stefanni" || WiFi.SSID(i) == "Chardon") building = WiFi.SSID(i);
delay(10);
}
}
Serial.println("");
xQueueSend(publish_queue, &n, (TickType_t) 0);
//WiFi.reconnect();
vTaskDelay(MONITORWIFI_PERIOD / portTICK_PERIOD_MS);
}
}

void my_task(void *p) {

int n;
while (1) {
if ( xQueueReceive(publish_queue, &n, (TickType_t) 0) == pdTRUE ) {
Serial.print("In my_task got the message that we discovered this number of hotspots: "); 
Serial.println(n);
}
vTaskDelay(MYTASK_PERIOD / portTICK_PERIOD_MS);
}

}

void getLocation(void *p) {
  while(1) {
    delay(5000);
      if((building == "Stefanni" || building == "Chardon") && InLandmark) {
      Serial.print("Currently at ");
      Serial.print(building);
      building = "";
    }
    else if((building == "Stefanni" || building == "Chardon") && !InLandmark) {
      Serial.print("Just arrived at ");
      Serial.print(building);
      InLandmark = true;

    } else if ((building != "Stefanni" || building != "Chardon") && InLandmark) {
      Serial.println("Just left landmark");
      InLandmark = false;
    }
    else Serial.println("Not in a current landmark");

    vTaskDelay(GETLOCATION_PERIOD / portTICK_PERIOD_MS);
  }
}

void connectToMQTT() {
  //Tries to connect to MQTT every 2 seconds 
  while(!MQTTclient.connected()) {
    if(MQTTclient.connect("ESP32clientID")) MQTTclient.subscribe(topic);
    else delay(2000);
  }
}

void initMQTT() {
  MQTTclient.setServer(MQTT_Broker, MQTT_Port);
  connectToMQTT();
}

int main(){
// Initialize serial port
Serial.begin(115200);

// Small Delay so that we don't miss the first few items printed
vTaskDelay(STARTUP_DELAY / portTICK_PERIOD_MS);
// Initialize WiFi and Connection to MQTT Broker
pinMode(LED, OUTPUT); //Turn on 2nd LED
Serial.println("Connecting to WiFi");
initWiFi();

Serial.println("Starting All Tasks");
// Create a task that monitors the WiFi hotspots
xTaskCreate(&monitor_wifi_task, "monitor_wifi_task", 2048,NULL,5,NULL );

// Start Task and a Queue that will receive the events from the wifi monitoring
publish_queue = xQueueCreate(10, sizeof(int));
xTaskCreate(&my_task, "my_task", 2048,NULL,5,NULL );

// Create a task that monitors whether we are in a landmark or not
xTaskCreate(&getLocation, "getLocation", 2048, NULL, 5, NULL);
}

void setup() { main(); }
void loop(){}