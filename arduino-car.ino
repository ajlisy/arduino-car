// Arduino Car MQTT Command Receiver
// Listens for JSON commands on MQTT and executes robot tools

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "robot_tools.h"
#include "openai_processor.h"
#include "config.h"

// Pin definitions for motors
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

WiFiClient espClient;
PubSubClient client(espClient);

// Declare the MQTT client as external for robot_tools.h
extern PubSubClient client;

void setup() {
  // Initialize motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(115200);
  
  // Initialize robot tools
  initRobotTools();
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  setupMQTT();
  
  Serial.println("Arduino Car MQTT Command Receiver Ready!");
  Serial.println("Listening for commands on topic: " + String(MQTT_TOPIC));
}

void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Small delay to prevent overwhelming the system
  delay(100);
}

void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      
      // Subscribe to the robot command topic
      client.subscribe(MQTT_TOPIC);
      Serial.println("Subscribed to topic: " + String(MQTT_TOPIC));
      
      // Send initial status message
      sendStatusMessage("Robot connected and ready to receive commands");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message received on topic: " + String(topic));
  
  // Convert payload to string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Raw message: " + message);
  
  // Parse JSON message
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    sendStatusMessage("Error: Invalid JSON format");
    return;
  }
  
  // Check if this message is from the robot itself (ignore to prevent loops)
  if (doc.containsKey("robot_id") && doc["robot_id"] == "arduino_car") {
    Serial.println("Ignoring message from self");
    return;
  }
  
  // Extract command content
  if (!doc.containsKey("content")) {
    sendStatusMessage("Error: No 'content' field in JSON");
    return;
  }
  
  String content = doc["content"].as<String>();
  String sender = doc.containsKey("sender") ? doc["sender"].as<String>() : "unknown";
  String id = doc.containsKey("id") ? doc["id"].as<String>() : "unknown";
  
  Serial.println("Command from " + sender + " (ID: " + id + "): " + content);
  
  // Execute the command
  String result = executeCommand(content);
  
  // Send response back
  sendStatusMessage("Command executed: " + content + " | Result: " + result);
}

String executeCommand(String command) {
  command.trim();
  
  Serial.println("=== EXECUTING COMMAND ===");
  Serial.println("Input: " + command);
  
  // Check if this is a complex objective that needs iterative planning
  if (isComplexObjective(command)) {
    Serial.println("Detected complex objective - using iterative planning");
    return executeIterativePlanning(command);
  } else {
    Serial.println("Detected simple command - using direct execution");
    // Process the command through OpenAI
    OpenAIResult result = processWithOpenAI(command);
    
    // Execute the tool calls
    String executionResult = executeToolCalls(result);
    
    Serial.println("=== COMMAND EXECUTION COMPLETE ===");
    return executionResult;
  }
}

bool isComplexObjective(String command) {
  // Keywords that suggest complex, multi-step objectives
  String complexKeywords[] = {
    "explore", "find", "search", "navigate", "avoid", "reach", "locate",
    "discover", "investigate", "map", "survey", "patrol", "monitor",
    "follow", "track", "chase", "escape", "hide", "seek"
  };
  
  command.toLowerCase();
  
  for (String keyword : complexKeywords) {
    if (command.indexOf(keyword) != -1) {
      return true;
    }
  }
  
  // Also check for multiple actions or conditional language
  if (command.indexOf("if") != -1 || 
      command.indexOf("when") != -1 || 
      command.indexOf("until") != -1 ||
      command.indexOf("while") != -1 ||
      command.indexOf("then") != -1) {
    return true;
  }
  
  // Check for multiple commands separated by common conjunctions
  if (command.indexOf(" and ") != -1 || 
      command.indexOf(" or ") != -1 || 
      command.indexOf(" but ") != -1) {
    return true;
  }
  
  return false;
}

void sendStatusMessage(String message) {
  if (client.connected()) {
    // Create JSON response
    DynamicJsonDocument doc(512);
    doc["robot_id"] = "arduino_car";
    doc["status"] = message;
    doc["timestamp"] = String(millis());
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publish to the same topic
    client.publish(MQTT_TOPIC, jsonString.c_str());
    
    Serial.println("Status sent: " + message);
  }
} 