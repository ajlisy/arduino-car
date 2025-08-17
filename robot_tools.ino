#include "robot_tools.h"

// Global sonar object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Array of available tools
Tool tools[] = {
  {"get_sonar_distance", "Measures distance using ultrasonic sensor in centimeters", getSonarDistance},
  {"move_car", "Controls car movement. Format: 'direction duration' or 'direction degrees'. Examples: 'forward 1000', 'backward 2000', 'left 90', 'right 180', 'stop'", moveCar},
  {"test_sonar", "Tests ultrasonic sensor with detailed diagnostics", testSonar},
  {"get_environment_info", "Gathers current environment information (distance, position, etc.) for planning", getEnvironmentInfo},
  {"send_mqtt_message", "Sends a message over MQTT. Format: 'message text'. Example: 'send_mqtt_message Planning next step...'", sendMqttMessage}
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
String executeTool(String toolName, String params) {
  for (int i = 0; i < NUM_TOOLS; i++) {
    if (tools[i].name == toolName) {
      logToRobotLogs("Executing tool: " + toolName);
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
  logToRobotLogs("=== GET SONAR DISTANCE ===");
  
  // Send MQTT update that we're starting distance measurement
  sendMqttMessage("Starting sonar distance measurement...");
  
  // Take multiple readings and average them for better accuracy
  int readings[5];
  int validReadings = 0;
  
  logToRobotLogs("Taking 5 distance readings...");
  
  for (int i = 0; i < 5; i++) {
    int reading = sonar.ping_cm();
    logToRobotLogs("Reading " + String(i + 1) + ": " + String(reading) + " cm");
    
    // Send individual reading over MQTT
    sendMqttMessage("Sonar reading " + String(i + 1) + ": " + String(reading) + " cm");
    
    // Only count readings that are reasonable (not 0 and not > 400)
    if (reading > 0 && reading <= 400) {
      readings[validReadings] = reading;
      validReadings++;
    } else {
      logToRobotLogs("  -> Skipping invalid reading: " + String(reading));
      sendMqttMessage("Invalid sonar reading " + String(i + 1) + ": " + String(reading) + " cm (skipped)");
    }
    
    delay(50); // Small delay between readings
  }
  
  logToRobotLogs("Valid readings: " + String(validReadings) + "/5");
  sendMqttMessage("Sonar measurement complete: " + String(validReadings) + "/5 valid readings");
  
  if (validReadings == 0) {
    logToRobotLogs("No valid readings obtained");
    sendMqttMessage("Sonar result: Out of range (>400cm or no echo)");
    return "Distance: Out of range (>400cm or no echo)";
  }
  
  // Calculate average of valid readings
  int total = 0;
  for (int i = 0; i < validReadings; i++) {
    total += readings[i];
  }
  int averageDistance = total / validReadings;
  
  logToRobotLogs("Average distance: " + String(averageDistance) + " cm");
  
  // Send final result over MQTT
  String mqttResult = "Sonar distance: " + String(averageDistance) + " cm (avg of " + String(validReadings) + " readings)";
  sendMqttMessage(mqttResult);
  
  String result = "Distance: " + String(averageDistance) + " cm (avg of " + String(validReadings) + " readings)";
  logToRobotLogs("Final result: " + result);
  logToRobotLogs("=== SONAR DISTANCE COMPLETE ===");
  
  return result;
}



// ==========================================
// MOTOR CONTROL HELPER FUNCTIONS
// ==========================================

/**
 * Stop all wheels
 */
void stopWheels() {
  logToRobotLogs("Stopping all wheels: IN1=LOW, IN2=LOW, IN3=LOW, IN4=LOW");
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
  logToRobotLogs("=== GO FORWARD EXECUTING ===");
  logToRobotLogs("Duration: " + String(milliseconds) + "ms");
  logToRobotLogs("Setting IN1=HIGH, IN2=LOW, IN3=HIGH, IN4=LOW");
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  logToRobotLogs("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  logToRobotLogs("Stopping wheels...");
  stopWheels();
  logToRobotLogs("=== GO FORWARD COMPLETE ===");
}

/**
 * Move car backward for specified duration
 * @param milliseconds Duration in milliseconds
 */
void goBackward(int milliseconds) {
  logToRobotLogs("=== GO BACKWARD EXECUTING ===");
  logToRobotLogs("Duration: " + String(milliseconds) + "ms");
  logToRobotLogs("Setting IN1=LOW, IN2=HIGH, IN3=LOW, IN4=HIGH");
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  logToRobotLogs("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  logToRobotLogs("Stopping wheels...");
  stopWheels();
  logToRobotLogs("=== GO BACKWARD COMPLETE ===");
}

/**
 * Turn car left for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnLeft(int milliseconds) {
  logToRobotLogs("=== TURN LEFT EXECUTING ===");
  logToRobotLogs("Duration: " + String(milliseconds) + "ms");
  logToRobotLogs("Setting IN1=HIGH, IN2=LOW, IN3=LOW, IN4=HIGH");
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  logToRobotLogs("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  logToRobotLogs("Stopping wheels...");
  stopWheels();
  logToRobotLogs("=== TURN LEFT COMPLETE ===");
}

/**
 * Turn car right for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnRight(int milliseconds) {
  logToRobotLogs("=== TURN RIGHT EXECUTING ===");
  logToRobotLogs("Duration: " + String(milliseconds) + "ms");
  logToRobotLogs("Setting IN1=LOW, IN2=HIGH, IN3=HIGH, IN4=LOW");
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  logToRobotLogs("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  logToRobotLogs("Stopping wheels...");
  stopWheels();
  logToRobotLogs("=== TURN RIGHT COMPLETE ===");
}

/**
 * Turn car left by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnLeftDegrees(int degrees) {
  logToRobotLogs("=== TURN LEFT DEGREES ===");
  logToRobotLogs("Degrees: " + String(degrees));
  int duration = 600 * (degrees / 90);
  logToRobotLogs("Calculated duration: " + String(duration) + "ms");
  turnLeft(duration);
}

/**
 * Turn car right by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnRightDegrees(int degrees) {
  logToRobotLogs("=== TURN RIGHT DEGREES ===");
  logToRobotLogs("Degrees: " + String(degrees));
  int duration = 600 * (degrees / 90);
  logToRobotLogs("Calculated duration: " + String(duration) + "ms");
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
  logToRobotLogs("=== MOVE CAR TOOL CALLED ===");
  logToRobotLogs("Raw params: '" + params + "'");
  
  params.trim();
  logToRobotLogs("Trimmed params: '" + params + "'");
  
  if (params.length() == 0) {
    logToRobotLogs("Error: No movement command provided");
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
  
  logToRobotLogs("Parsed command: '" + command + "'");
  logToRobotLogs("Parsed value: '" + valueStr + "'");
  
  command.toLowerCase();
  logToRobotLogs("Lowercase command: '" + command + "'");
  
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
  
  logToRobotLogs("Move car result: " + result);
  return result;
}

/**
 * Tool: Test Sonar
 * Comprehensive test of the ultrasonic sensor
 * @param params Optional parameters (not used currently)
 * @return String containing test results
 */
String testSonar(String params) {
  logToRobotLogs("=== SONAR SENSOR TEST ===");
  
  // Send MQTT update that we're starting sonar test
  sendMqttMessage("Starting comprehensive sonar sensor test...");
  
  // Test 1: Check pin configuration
  logToRobotLogs("Pin Configuration:");
  logToRobotLogs("  TRIGGER_PIN: " + String(TRIGGER_PIN));
  logToRobotLogs("  ECHO_PIN: " + String(ECHO_PIN));
  logToRobotLogs("  MAX_DISTANCE: " + String(MAX_DISTANCE) + " cm");
  
  sendMqttMessage("Sonar pins: TRIGGER=" + String(TRIGGER_PIN) + ", ECHO=" + String(ECHO_PIN) + ", MAX_DIST=" + String(MAX_DISTANCE) + "cm");
  
  // Test 2: Take multiple raw readings
  logToRobotLogs("\nTaking 10 raw readings:");
  sendMqttMessage("Taking 10 raw sonar readings for testing...");
  
  int readings[10];
  int validCount = 0;
  int invalidCount = 0;
  
  for (int i = 0; i < 10; i++) {
    int reading = sonar.ping_cm();
    readings[i] = reading;
    
    if (reading > 0 && reading <= 400) {
      validCount++;
      logToRobotLogs("  Reading " + String(i + 1) + ": " + String(reading) + " cm (VALID)");
      sendMqttMessage("Sonar test reading " + String(i + 1) + ": " + String(reading) + " cm (VALID)");
    } else {
      invalidCount++;
      logToRobotLogs("  Reading " + String(i + 1) + ": " + String(reading) + " cm (INVALID)");
      sendMqttMessage("Sonar test reading " + String(i + 1) + ": " + String(reading) + " cm (INVALID)");
    }
    
    delay(100);
  }
  
  // Test 3: Calculate statistics
  logToRobotLogs("\nStatistics:");
  logToRobotLogs("  Valid readings: " + String(validCount) + "/10");
  logToRobotLogs("  Invalid readings: " + String(invalidCount) + "/10");
  
  sendMqttMessage("Sonar test statistics: " + String(validCount) + "/10 valid, " + String(invalidCount) + "/10 invalid");
  
  if (validCount > 0) {
    int minReading = 400;
    int maxReading = 0;
    int total = 0;
    
    for (int i = 0; i < 10; i++) {
      if (readings[i] > 0 && readings[i] <= 400) {
        if (readings[i] < minReading) minReading = readings[i];
        if (readings[i] > maxReading) maxReading = readings[i];
        total += readings[i];
      }
    }
    
    int average = total / validCount;
    logToRobotLogs("  Min reading: " + String(minReading) + " cm");
    logToRobotLogs("  Max reading: " + String(maxReading) + " cm");
    logToRobotLogs("  Average reading: " + String(average) + " cm");
    logToRobotLogs("  Range: " + String(maxReading - minReading) + " cm");
    
    sendMqttMessage("Sonar test results: Min=" + String(minReading) + "cm, Max=" + String(maxReading) + "cm, Avg=" + String(average) + "cm, Range=" + String(maxReading - minReading) + "cm");
  }
  
  // Test 4: Recommendations
  logToRobotLogs("\nTroubleshooting Recommendations:");
  if (invalidCount > validCount) {
    logToRobotLogs("  ⚠️  Many invalid readings - check wiring and sensor placement");
  }
  if (validCount > 0) {
    logToRobotLogs("  ✅ Sensor is working, but may need calibration");
  } else {
    logToRobotLogs("  ❌ Sensor not working - check connections");
  }
  
  logToRobotLogs("\nCommon Issues:");
  logToRobotLogs("  1. Wrong pin connections (TRIGGER vs ECHO swapped)");
  logToRobotLogs("  2. Loose wiring or bad connections");
  logToRobotLogs("  3. Sensor too close to ground or other objects");
  logToRobotLogs("  4. Power supply issues (sensor needs 5V)");
  logToRobotLogs("  5. Interference from other electronics");
  
  String result = "Sonar test complete. Valid: " + String(validCount) + "/10, Invalid: " + String(invalidCount) + "/10";
  
  // Send final test result over MQTT
  sendMqttMessage("Sonar test complete: " + String(validCount) + "/10 valid readings");
  
  logToRobotLogs("=== SONAR TEST COMPLETE ===");
  
  return result;
}

/**
 * Tool: Get Environment Info
 * Gathers comprehensive environment information for planning purposes
 * @param params Optional parameters (not used currently)
 * @return String containing environment information
 */
String getEnvironmentInfo(String params) {
  logToRobotLogs("=== GET ENVIRONMENT INFO ===");
  
  // Send MQTT update that we're gathering environment info
  sendMqttMessage("Gathering comprehensive environment information...");
  
  String info = "Environment Information:\n";
  info += "========================\n";
  
  // Get distance reading
  logToRobotLogs("Getting distance reading...");
  int distance = sonar.ping_cm();
  info += "Distance ahead: " + String(distance) + " cm";
  
  // Send distance reading over MQTT
  sendMqttMessage("Environment distance: " + String(distance) + " cm");
  
  // Get WiFi signal strength
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    info += "WiFi signal: " + String(rssi) + " dBm\n";
    info += "WiFi SSID: " + String(WiFi.SSID()) + "\n";
    sendMqttMessage("Environment WiFi: " + String(rssi) + " dBm, SSID: " + String(WiFi.SSID()));
  } else {
    info += "WiFi: Not connected\n";
    sendMqttMessage("Environment WiFi: Not connected");
  }
  
  // Get MQTT connection status
  if (client.connected()) {
    info += "MQTT: Connected\n";
    sendMqttMessage("Environment MQTT: Connected");
  } else {
    info += "MQTT: Disconnected\n";
    sendMqttMessage("Environment MQTT: Disconnected");
  }
  
  // Get system uptime
  unsigned long uptime = millis() / 1000; // Convert to seconds
  info += "Uptime: " + String(uptime) + " seconds\n";
  
  // Get free memory
  info += "Free memory: " + String(ESP.getFreeHeap()) + " bytes\n";
  
  sendMqttMessage("Environment system: Uptime " + String(uptime) + "s, Free memory " + String(ESP.getFreeHeap()) + " bytes");
  
  logToRobotLogs("Environment info gathered");
  sendMqttMessage("Environment information gathering complete");
  
  return info;
}

/**
 * Tool: Send MQTT Message
 * Sends a message over MQTT for status updates and planning communication
 * @param params The message text to send
 * @return String confirmation of message sent
 */
String sendMqttMessage(String params) {
  logToRobotLogs("=== SEND MQTT MESSAGE ===");
  logToRobotLogs("Message: " + params);
  
  if (params.length() == 0) {
    return "Error: No message provided";
  }
  
  if (!client.connected()) {
    return "Error: MQTT not connected";
  }
  
  // Create JSON message
  DynamicJsonDocument doc(512);
  doc["robot_id"] = "arduino_car";
  doc["message_type"] = "status_update";
  doc["content"] = params;
  doc["timestamp"] = String(millis());
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to the MQTT topic
  bool success = client.publish(MQTT_TOPIC, jsonString.c_str());
  
  if (success) {
    logToRobotLogs("MQTT message sent successfully");
    return "Message sent: " + params;
  } else {
    logToRobotLogs("Failed to send MQTT message" + jsonString);
    return "Error: Failed to send message";
  }
}

/**
 * Log message to serial console and optionally to MQTT
 * @param message The message text to log
 * @return String confirmation of message logged
 */
String logToRobotLogs(String message) {
  if (message.length() == 0) {
    return "Error: No message provided";
  }
  
  // Log to serial console
  Serial.println("[ROBOT_LOG] " + message);

  // Optionally send to MQTT robotlogs topic for remote monitoring
  if (client.connected()) {
    // Publish to robotlogs topic
    client.publish("ajlisy/robotlogs", message.c_str());
  }
  
  return "Log message printed to serial: " + message;
}

/**
 * Initialize the robot tools system
 * Call this from setup()
 */
void initRobotTools() {
  logToRobotLogs("Robot Tools System Initialized");
  logToRobotLogs("Use listTools() to see available tools");
  logToRobotLogs(listTools());
} 