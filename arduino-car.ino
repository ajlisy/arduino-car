// Example integration of robot_tools.h into arduino-car.ino
// This shows how to modify your existing code to use the new tools system

#include <WiFi.h>
#include <HTTPClient.h>
#include <string>
#include <PubSubClient.h>
#include <stdio.h>
#include "robot_tools.h"  // Include the new tools system

//Pins for motors (unchanged)
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

#define ATTEMPT_MQTT 0

const char* ssid = "gavroche";
const char* password = "***REMOVED***";
String instructionsURL = "https://pastebin.com/raw/ydRLi8ad";

const char* mqtt_server = "192.168.4.200";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  //Loop until we've connected to Wifi
  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize the robot tools system
  initRobotTools();

  //Loop until we're connected to MQTT
  if (ATTEMPT_MQTT) {
    client.setServer(mqtt_server, mqtt_port);
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    // Use the new tools system for distance measurement
    String distanceResult = executeTool("get_sonar_distance");
    Serial.println(distanceResult);
    
    // Log to webhook that robot started
    String logResult = executeTool("log_to_webhook", "Robot initialized and connected to MQTT");
    Serial.println(logResult);
  }

  //Download instructions (unchanged)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.println("Instructions URL: ");
    Serial.print(instructionsURL);
    http.begin(instructionsURL.c_str());
    int httpResponseCode = http.GET();

    //Process Response
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      http.end();
      Serial.println("Payload received ------------->>");
      Serial.println(payload);
      Serial.println("<< -------------");
      
      // Log the received instructions
      executeTool("log_to_webhook", "Instructions received: " + payload.substring(0, 50) + "...");
      
      tokenize(payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      executeTool("log_to_webhook", "Error downloading instructions: " + String(httpResponseCode));
      http.end();
    }
  } else {
    Serial.println("WiFi Disconnected");
    executeTool("log_to_webhook", "WiFi connection failed");
  }
}

void loop() {
  if (ATTEMPT_MQTT) client.loop();
  
  // Example: Check distance every 10 seconds and log if something is close
  static unsigned long lastDistanceCheck = 0;
  if (millis() - lastDistanceCheck > 10000) {
    String distance = executeTool("get_sonar_distance");
    
    // Parse distance value (simple example)
    int distValue = distance.substring(distance.indexOf(":") + 2, distance.indexOf(" cm")).toInt();
    if (distValue > 0 && distValue < 20) {
      executeTool("log_to_webhook", "Object detected at close range: " + String(distValue) + "cm");
    }
    
    lastDistanceCheck = millis();
  }
  
  delay(5000);
}

// Example function to demonstrate tools usage
void demonstrateTools() {
  Serial.println("=== Robot Tools Demonstration ===");
  
  // List all available tools
  Serial.println(listTools());
  
  // Execute individual tools
  String distance = executeTool("get_sonar_distance");
  Serial.println("Distance measurement: " + distance);
  
  String logResult = executeTool("log_to_webhook", "Tools demonstration completed");
  Serial.println("Log result: " + logResult);
  
  // Get tool count
  Serial.println("Total tools available: " + String(getToolCount()));
  
  // Get tool by index
  Tool firstTool = getToolByIndex(0);
  Serial.println("First tool: " + firstTool.name + " - " + firstTool.description);
}

// Include all your existing functions here (unchanged):
// - reconnect()
// - tokenize()
// - parse_and_route()
// - wait()
// - go_forward()
// - go_backward()
// - turn_90_right()
// - turn_90_left()
// - turn_degrees_left()
// - turn_degrees_right()
// - stop_wheels() 