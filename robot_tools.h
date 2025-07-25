#ifndef ROBOT_TOOLS_H
#define ROBOT_TOOLS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <NewPing.h>
#include <string>

// Pin definitions for ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 400

// Global sonar object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Tool structure definition
struct Tool {
  String name;
  String description;
  String (*execute)(String params);
};

// Forward declarations of tool functions
String getSonarDistance(String params);
String logToWebhook(String params);

// Array of available tools
Tool tools[] = {
  {"get_sonar_distance", "Measures distance using ultrasonic sensor in centimeters", getSonarDistance},
  {"log_to_webhook", "Sends a log message via HTTP POST to webhook endpoint", logToWebhook}
};

const int NUM_TOOLS = sizeof(tools) / sizeof(tools[0]);

/**
 * Lists all available tools with their descriptions
 * @return String containing formatted list of all tools
 */
String listTools() {
  String result = "Available Robot Tools:\n";
  result += "=====================\n";
  
  for (int i = 0; i < NUM_TOOLS; i++) {
    result += String(i + 1) + ". " + tools[i].name + "\n";
    result += "   Description: " + tools[i].description + "\n\n";
  }
  
  return result;
}

/**
 * Execute a tool by name
 * @param toolName Name of the tool to execute
 * @param params Parameters to pass to the tool
 * @return String result from the tool execution
 */
String executeTool(String toolName, String params = "") {
  for (int i = 0; i < NUM_TOOLS; i++) {
    if (tools[i].name == toolName) {
      Serial.println("Executing tool: " + toolName);
      return tools[i].execute(params);
    }
  }
  
  return "Error: Tool '" + toolName + "' not found. Use listTools() to see available tools.";
}

/**
 * Get the number of available tools
 * @return Number of tools available
 */
int getToolCount() {
  return NUM_TOOLS;
}

/**
 * Get tool information by index
 * @param index Tool index (0-based)
 * @return Tool structure or empty tool if index invalid
 */
Tool getToolByIndex(int index) {
  if (index >= 0 && index < NUM_TOOLS) {
    return tools[index];
  }
  
  Tool emptyTool = {"", "", nullptr};
  return emptyTool;
}

// ==========================================
// TOOL IMPLEMENTATIONS
// ==========================================

/**
 * Tool: Get Sonar Distance
 * Measures distance using the ultrasonic sensor
 * @param params Optional parameters (not used currently)
 * @return String containing distance measurement
 */
String getSonarDistance(String params) {
  int distance = sonar.ping_cm();
  
  if (distance == 0) {
    return "Distance: Out of range (>400cm or no echo)";
  }
  
  String result = "Distance: " + String(distance) + " cm";
  Serial.println(result);
  return result;
}

/**
 * Tool: Log to Webhook
 * Sends a log message via HTTP POST to the webhook endpoint
 * @param params The message to log
 * @return String indicating success or failure
 */
String logToWebhook(String params) {
  if (WiFi.status() != WL_CONNECTED) {
    return "Error: WiFi not connected";
  }
  
  if (params.length() == 0) {
    params = "Robot log entry - " + String(millis());
  }
  
  HTTPClient http;
  http.begin("https://webhook.site/82b13278-9f0a-463c-ac50-d21bce1e36e3");
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  String jsonPayload = "{\"message\":\"" + params + "\",\"timestamp\":" + String(millis()) + ",\"robot_id\":\"arduino_car\"}";
  
  int httpResponseCode = http.POST(jsonPayload);
  String result;
  
  if (httpResponseCode > 0) {
    result = "Log sent successfully. Response code: " + String(httpResponseCode);
    Serial.println(result);
  } else {
    result = "Error sending log. Response code: " + String(httpResponseCode);
    Serial.println(result);
  }
  
  http.end();
  return result;
}

/**
 * Initialize the robot tools system
 * Call this from setup()
 */
void initRobotTools() {
  Serial.println("Robot Tools System Initialized");
  Serial.println("Use listTools() to see available tools");
  Serial.println(listTools());
}

#endif // ROBOT_TOOLS_H 