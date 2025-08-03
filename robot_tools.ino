#include "robot_tools.h"

// Global sonar object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Array of available tools
Tool tools[] = {
  {"get_sonar_distance", "Measures distance using ultrasonic sensor in centimeters", getSonarDistance},
  {"log_to_webhook", "Sends a log message via HTTP POST to webhook endpoint", logToWebhook},
  {"move_car", "Controls car movement. Format: 'direction duration' or 'direction degrees'. Examples: 'forward 1000', 'backward 2000', 'left 90', 'right 180', 'stop'", moveCar},
  {"test_sonar", "Tests ultrasonic sensor with detailed diagnostics", testSonar}
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
  Serial.println("=== GET SONAR DISTANCE ===");
  
  // Take multiple readings and average them for better accuracy
  int readings[5];
  int validReadings = 0;
  
  Serial.println("Taking 5 distance readings...");
  
  for (int i = 0; i < 5; i++) {
    int reading = sonar.ping_cm();
    Serial.println("Reading " + String(i + 1) + ": " + String(reading) + " cm");
    
    // Only count readings that are reasonable (not 0 and not > 400)
    if (reading > 0 && reading <= 400) {
      readings[validReadings] = reading;
      validReadings++;
    } else {
      Serial.println("  -> Skipping invalid reading: " + String(reading));
    }
    
    delay(50); // Small delay between readings
  }
  
  Serial.println("Valid readings: " + String(validReadings) + "/5");
  
  if (validReadings == 0) {
    Serial.println("No valid readings obtained");
    return "Distance: Out of range (>400cm or no echo)";
  }
  
  // Calculate average of valid readings
  int total = 0;
  for (int i = 0; i < validReadings; i++) {
    total += readings[i];
  }
  int averageDistance = total / validReadings;
  
  Serial.println("Average distance: " + String(averageDistance) + " cm");
  
  String result = "Distance: " + String(averageDistance) + " cm (avg of " + String(validReadings) + " readings)";
  Serial.println("Final result: " + result);
  Serial.println("=== SONAR DISTANCE COMPLETE ===");
  
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
  Serial.println("Stopping all wheels: IN1=LOW, IN2=LOW, IN3=LOW, IN4=LOW");
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
  Serial.println("=== GO FORWARD EXECUTING ===");
  Serial.println("Duration: " + String(milliseconds) + "ms");
  Serial.println("Setting IN1=HIGH, IN2=LOW, IN3=HIGH, IN4=LOW");
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  Serial.println("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  Serial.println("Stopping wheels...");
  stopWheels();
  Serial.println("=== GO FORWARD COMPLETE ===");
}

/**
 * Move car backward for specified duration
 * @param milliseconds Duration in milliseconds
 */
void goBackward(int milliseconds) {
  Serial.println("=== GO BACKWARD EXECUTING ===");
  Serial.println("Duration: " + String(milliseconds) + "ms");
  Serial.println("Setting IN1=LOW, IN2=HIGH, IN3=LOW, IN4=HIGH");
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  Serial.println("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  Serial.println("Stopping wheels...");
  stopWheels();
  Serial.println("=== GO BACKWARD COMPLETE ===");
}

/**
 * Turn car left for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnLeft(int milliseconds) {
  Serial.println("=== TURN LEFT EXECUTING ===");
  Serial.println("Duration: " + String(milliseconds) + "ms");
  Serial.println("Setting IN1=HIGH, IN2=LOW, IN3=LOW, IN4=HIGH");
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  Serial.println("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  Serial.println("Stopping wheels...");
  stopWheels();
  Serial.println("=== TURN LEFT COMPLETE ===");
}

/**
 * Turn car right for specified duration
 * @param milliseconds Duration in milliseconds
 */
void turnRight(int milliseconds) {
  Serial.println("=== TURN RIGHT EXECUTING ===");
  Serial.println("Duration: " + String(milliseconds) + "ms");
  Serial.println("Setting IN1=LOW, IN2=HIGH, IN3=HIGH, IN4=LOW");
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  Serial.println("Motors activated, waiting " + String(milliseconds) + "ms...");
  delay(milliseconds);
  
  Serial.println("Stopping wheels...");
  stopWheels();
  Serial.println("=== TURN RIGHT COMPLETE ===");
}

/**
 * Turn car left by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnLeftDegrees(int degrees) {
  Serial.println("=== TURN LEFT DEGREES ===");
  Serial.println("Degrees: " + String(degrees));
  int duration = 600 * (degrees / 90);
  Serial.println("Calculated duration: " + String(duration) + "ms");
  turnLeft(duration);
}

/**
 * Turn car right by specified degrees
 * @param degrees Degrees to turn (90 degrees = 600ms)
 */
void turnRightDegrees(int degrees) {
  Serial.println("=== TURN RIGHT DEGREES ===");
  Serial.println("Degrees: " + String(degrees));
  int duration = 600 * (degrees / 90);
  Serial.println("Calculated duration: " + String(duration) + "ms");
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
  Serial.println("=== MOVE CAR TOOL CALLED ===");
  Serial.println("Raw params: '" + params + "'");
  
  params.trim();
  Serial.println("Trimmed params: '" + params + "'");
  
  if (params.length() == 0) {
    Serial.println("Error: No movement command provided");
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
  
  Serial.println("Parsed command: '" + command + "'");
  Serial.println("Parsed value: '" + valueStr + "'");
  
  command.toLowerCase();
  Serial.println("Lowercase command: '" + command + "'");
  
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
 * Tool: Test Sonar
 * Comprehensive test of the ultrasonic sensor
 * @param params Optional parameters (not used currently)
 * @return String containing test results
 */
String testSonar(String params) {
  Serial.println("=== SONAR SENSOR TEST ===");
  
  // Test 1: Check pin configuration
  Serial.println("Pin Configuration:");
  Serial.println("  TRIGGER_PIN: " + String(TRIGGER_PIN));
  Serial.println("  ECHO_PIN: " + String(ECHO_PIN));
  Serial.println("  MAX_DISTANCE: " + String(MAX_DISTANCE) + " cm");
  
  // Test 2: Take multiple raw readings
  Serial.println("\nTaking 10 raw readings:");
  int readings[10];
  int validCount = 0;
  int invalidCount = 0;
  
  for (int i = 0; i < 10; i++) {
    int reading = sonar.ping_cm();
    readings[i] = reading;
    
    if (reading > 0 && reading <= 400) {
      validCount++;
      Serial.println("  Reading " + String(i + 1) + ": " + String(reading) + " cm (VALID)");
    } else {
      invalidCount++;
      Serial.println("  Reading " + String(i + 1) + ": " + String(reading) + " cm (INVALID)");
    }
    
    delay(100);
  }
  
  // Test 3: Calculate statistics
  Serial.println("\nStatistics:");
  Serial.println("  Valid readings: " + String(validCount) + "/10");
  Serial.println("  Invalid readings: " + String(invalidCount) + "/10");
  
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
    Serial.println("  Min reading: " + String(minReading) + " cm");
    Serial.println("  Max reading: " + String(maxReading) + " cm");
    Serial.println("  Average reading: " + String(average) + " cm");
    Serial.println("  Range: " + String(maxReading - minReading) + " cm");
  }
  
  // Test 4: Recommendations
  Serial.println("\nTroubleshooting Recommendations:");
  if (invalidCount > validCount) {
    Serial.println("  ⚠️  Many invalid readings - check wiring and sensor placement");
  }
  if (validCount > 0) {
    Serial.println("  ✅ Sensor is working, but may need calibration");
  } else {
    Serial.println("  ❌ Sensor not working - check connections");
  }
  
  Serial.println("\nCommon Issues:");
  Serial.println("  1. Wrong pin connections (TRIGGER vs ECHO swapped)");
  Serial.println("  2. Loose wiring or bad connections");
  Serial.println("  3. Sensor too close to ground or other objects");
  Serial.println("  4. Power supply issues (sensor needs 5V)");
  Serial.println("  5. Interference from other electronics");
  
  String result = "Sonar test complete. Valid: " + String(validCount) + "/10, Invalid: " + String(invalidCount) + "/10";
  Serial.println("=== SONAR TEST COMPLETE ===");
  
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