#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MONITORWIFI_PERIOD 5000
#define MYTASK_PERIOD 2000
#define STARTUP_DELAY 3000

QueueHandle_t publish_queue;

WiFiClient espClient;

void initWiFi() {
const char* ssid = "LIB-7671512"; 
const char* password = "nF9DGYHGXdQK"; 

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) { 
Serial.print(".");
delay(1000);
}
Serial.println(WiFi.localIP());
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



int main(){
// Initialize serial port
//Serial.begin(115200);
Serial.begin(9600);
// Small Delay so that we don't miss the first few items printed
vTaskDelay(STARTUP_DELAY / portTICK_PERIOD_MS);
// Initialize WiFi and Connection to MQTT Broker
Serial.println("Connecting to WiFi");
initWiFi();

Serial.println("Starting All Tasks");
// Create a task that monitors the WiFi hotspots
xTaskCreate(&monitor_wifi_task, "monitor_wifi_task", 2048,NULL,5,NULL );

// Start Task and a Queue that will receive the events from the wifi monitoring
publish_queue = xQueueCreate(10, sizeof(int));
xTaskCreate(&my_task, "my_task", 2048,NULL,5,NULL );

}



void setup() { main(); }
void loop(){}