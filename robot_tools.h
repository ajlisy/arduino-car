#ifndef ROBOT_TOOLS_H
#define ROBOT_TOOLS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <NewPing.h>
#include <string>
#include <PubSubClient.h>

// Pin definitions for motors
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

// Pin definitions for ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 400

// Global sonar object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Global MQTT client (optional, for logging)
extern PubSubClient client;

// Tool structure definition
struct Tool {
  String name;
  String description;
  String (*execute)(String params);
};

// Forward declarations of tool functions
String getSonarDistance(String params);
String logToWebhook(String params);
String moveCar(String params);

// Array of available tools
Tool tools[] = {
  {"get_sonar_distance", "Measures distance using ultrasonic sensor in centimeters", getSonarDistance},
  {"log_to_webhook", "Sends a log message via HTTP POST to webhook endpoint", logToWebhook},
  {"move_car", "Controls car movement. Format: 'direction duration' or 'direction degrees'. Examples: 'forward 1000', 'backward 2000', 'left 90', 'right 180', 'stop'", moveCar}
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

// ==========================================
// MOTOR CONTROL HELPER FUNCTIONS
// ==========================================

/**
 * Stop all wheels
 */
void stopWheels() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

/**
 * Move car forward for specified duration
 * @param milliseconds Duration in milliseconds
 */
void goForward(int milliseconds) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(milliseconds);
  stopWheels();
}

/**
 * Move car backward for specified duration
 * @param milliseconds Duration in milliseconds
 */
void goBackward(int milliseconds) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(milliseconds);
  stopWheels();
}

/**
 * Turn car left for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnLeft(int milliseconds) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(milliseconds);
  stopWheels();
}

/**
 * Turn car right for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnRight(int milliseconds) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(milliseconds);
  stopWheels();
}

/**
 * Turn car left by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnLeftDegrees(int degrees) {
  int duration = 600 * (degrees / 90);
  turnLeft(duration);
}

/**
 * Turn car right by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnRightDegrees(int degrees) {
  int duration = 600 * (degrees / 90);
  turnRight(duration);
}

// ==========================================
// TOOL IMPLEMENTATIONS (continued)
// ==========================================

/**
 * Tool: Move Car
 * Controls car movement with various commands
 * @param params Movement command in format: "direction value"
 *              Examples: "forward 1000", "backward 2000", "left 90", "right 180", "stop"
 * @return String indicating the action performed
 */
String moveCar(String params) {
  params.trim();
  
  if (params.length() == 0) {
    return "Error: No movement command provided. Use: forward/backward/left/right/stop + value";
  }
  
  // Find the first space to separate command from value
  int spaceIndex = params.indexOf(' ');
  String command = params;
  String valueStr = "";
  
  if (spaceIndex != -1) {
    command = params.substring(0, spaceIndex);
    valueStr = params.substring(spaceIndex + 1);
  }
  
  command.toLowerCase();
  
  String result = "";
  
  if (command == "stop") {
    stopWheels();
    result = "Car stopped";
  }
  else if (command == "forward") {
    if (valueStr.length() == 0) {
      return "Error: Forward command requires duration in milliseconds";
    }
    int duration = valueStr.toInt();
    if (duration <= 0) {
      return "Error: Duration must be positive";
    }
    goForward(duration);
    result = "Car moved forward for " + String(duration) + "ms";
  }
  else if (command == "backward") {
    if (valueStr.length() == 0) {
      return "Error: Backward command requires duration in milliseconds";
    }
    int duration = valueStr.toInt();
    if (duration <= 0) {
      return "Error: Duration must be positive";
    }
    goBackward(duration);
    result = "Car moved backward for " + String(duration) + "ms";
  }
  else if (command == "left") {
    if (valueStr.length() == 0) {
      return "Error: Left command requires degrees or duration";
    }
    int value = valueStr.toInt();
    if (value <= 0) {
      return "Error: Value must be positive";
    }
    
    // If value is 90 or 180, treat as degrees, otherwise as milliseconds
    if (value == 90 || value == 180 || value == 270 || value == 360) {
      turnLeftDegrees(value);
      result = "Car turned left " + String(value) + " degrees";
    } else {
      turnLeft(value);
      result = "Car turned left for " + String(value) + "ms";
    }
  }
  else if (command == "right") {
    if (valueStr.length() == 0) {
      return "Error: Right command requires degrees or duration";
    }
    int value = valueStr.toInt();
    if (value <= 0) {
      return "Error: Value must be positive";
    }
    
    // If value is 90 or 180, treat as degrees, otherwise as milliseconds
    if (value == 90 || value == 180 || value == 270 || value == 360) {
      turnRightDegrees(value);
      result = "Car turned right " + String(value) + " degrees";
    } else {
      turnRight(value);
      result = "Car turned right for " + String(value) + "ms";
    }
  }
  else {
    return "Error: Unknown command '" + command + "'. Use: forward/backward/left/right/stop";
  }
  
  // Log the movement (optional MQTT logging)
  if (client.connected()) {
    char message[50];
    sprintf(message, "%s", result.c_str());
    client.publish("car", message);
  }
  
  Serial.println("Move car result: " + result);
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