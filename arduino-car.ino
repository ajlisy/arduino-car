// Arduino Car MQTT Command Receiver
// Listens for JSON commands on MQTT and executes robot tools

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "robot_tools.h"
#include "openai_processor.h"
#include "config.h"
#include "prompts_manager.h"

// Pin definitions for motors
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

WiFiClient espClient;
PubSubClient client(espClient);

// Declare the MQTT client as external for robot_tools.h
extern PubSubClient client;

// Function declarations for iterative planning
String executeIterativePlanning(String objective);

void setup() {
  // Initialize motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(115200);
  
  
  // Initialize robot tools
  initRobotTools();
  
  // Initialize prompts manager
  PromptsManager promptsManager;
  if (!promptsManager.begin()) {
    logToRobotLogs("Warning: Prompts Manager initialization failed");
  } else {
    promptsManager.logPromptInfo();
  }
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  setupMQTT();
  
  logToRobotLogs("Arduino Car MQTT Command Receiver Ready!");
  logToRobotLogs("Listening for commands on topic: " + String(MQTT_TOPIC));
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
  logToRobotLogs("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logToRobotLogs(".");
  }
  
  logToRobotLogs("");
  logToRobotLogs("WiFi connected");
  logToRobotLogs("IP address: " + String(WiFi.localIP()));
}

void setupMQTT() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    logToRobotLogs("Attempting MQTT connection...");
    
    if (client.connect(MQTT_CLIENT_ID)) {
      logToRobotLogs("connected");
      
      // Subscribe to the robot command topic
      client.subscribe(MQTT_TOPIC);
      logToRobotLogs("Subscribed to topic: " + String(MQTT_TOPIC));
      
      // Send initial status message
      sendStatusMessage("Robot connected and ready to receive commands");
      
    } else {
      logToRobotLogs("failed, rc=" + String(client.state()));
      logToRobotLogs(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Only process messages from the ajlisy/robot topic
  if (String(topic) != MQTT_TOPIC) {
    return; // Silently ignore messages from other topics
  }
  
  
  // Convert payload to string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Parse JSON message
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    logToRobotLogs("JSON parsing failed: ");
    logToRobotLogs(error.c_str());
    sendStatusMessage("Error: Invalid JSON format");
    return;
  }
  
  // Check if this message is from the robot itself (ignore to prevent loops)
  if (doc.containsKey("robot_id") && doc["robot_id"] == "arduino_car") {
    //logToRobotLogs("Ignoring message from self");
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
  
  
  // Execute the command
  String result = executeCommand(content);
  
  // Send response back
  sendStatusMessage("Command executed: " + content + " | Result: " + result);
}

String executeCommand(String command) {
  command.trim();
  
  logToRobotLogs("=== EXECUTING COMMAND ===");
  logToRobotLogs("Input: " + command);
  
  // All commands are now processed through iterative planning
  logToRobotLogs("Processing command through iterative planning");
  return executeIterativePlanning(command);
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
    
    logToRobotLogs("Status sent: " + message);
  }
} 