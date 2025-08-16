#include "openai_processor.h"
#include "robot_tools.h"

// Rate limiting
unsigned long lastOpenAIRequest = 0;
const unsigned long OPENAI_RATE_LIMIT_MS = 1000; // 1 second between requests

/**
 * Build the system prompt for OpenAI
 */
String buildSystemPrompt() {
  String prompt = "You are a robot command interpreter. Convert natural language to tool calls.\n\n";
  prompt += "Available tools:\n";
  prompt += "- move_car: Controls movement (forward/backward/left/right/stop + value)\n";
  prompt += "- get_sonar_distance: Measures distance using ultrasonic sensor\n";
  prompt += "- test_sonar: Tests ultrasonic sensor\n";
  prompt += "- get_environment_info: Gathers current environment information\n";
  prompt += "- send_mqtt_message: Sends status updates over MQTT\n\n";
  
  prompt += "Rules:\n";
  prompt += "- Return JSON array of valid tool calls\n";
  prompt += "- Only include tools with confidence > 0.9\n";
  prompt += "- If text doesn't match any tool, mark as 'unknown'\n";
  prompt += "- Be precise with parameters\n";
  prompt += "- Support multiple commands in one message\n\n";
  
  prompt += "Response format:\n";
  prompt += "{\n";
  prompt += "  \"tool_calls\": [\n";
  prompt += "    {\"tool\": \"tool_name\", \"params\": \"parameters\", \"confidence\": 0.95}\n";
  prompt += "  ],\n";
  prompt += "  \"unknown_commands\": [\"unrecognized text\"],\n";
  prompt += "  \"processing_notes\": \"description\"\n";
  prompt += "}\n\n";
  
  prompt += "Examples:\n";
  prompt += "Input: 'move forward for 2 seconds and check distance'\n";
  prompt += "Output: {\"tool_calls\": [{\"tool\": \"move_car\", \"params\": \"forward 2000\", \"confidence\": 0.95}, {\"tool\": \"get_sonar_distance\", \"params\": \"\", \"confidence\": 0.98}], \"unknown_commands\": [], \"processing_notes\": \"Extracted 2 valid tool calls\"}\n";
  
  return prompt;
}

/**
 * Make HTTP request to OpenAI API
 */
String makeOpenAIRequest(String prompt) {
  // Rate limiting
  unsigned long currentTime = millis();
  if (currentTime - lastOpenAIRequest < OPENAI_RATE_LIMIT_MS) {
    return "{\"error\": \"Rate limit exceeded\"}";
  }
  lastOpenAIRequest = currentTime;
  
  if (WiFi.status() != WL_CONNECTED) {
    return "{\"error\": \"WiFi not connected\"}";
  }
  
  HTTPClient http;
  
  // Add timeout and debugging
  http.setTimeout(10000); // 10 second timeout
  
  Serial.println("Connecting to OpenAI API...");
  Serial.println("WiFi Status: " + String(WiFi.status()));
  Serial.println("WiFi RSSI: " + String(WiFi.RSSI()));
  Serial.println("Local IP: " + WiFi.localIP().toString());
  
  http.begin("https://api.openai.com/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(OPENAI_API_KEY));
  
  // Build request payload
  DynamicJsonDocument doc(2048);
  doc["model"] = "gpt-4o-mini";
  doc["max_tokens"] = 500;
  doc["temperature"] = 0.1; // Low temperature for consistent parsing
  
  JsonArray messages = doc.createNestedArray("messages");
  
  JsonObject systemMsg = messages.createNestedObject();
  systemMsg["role"] = "system";
  systemMsg["content"] = buildSystemPrompt();
  
  JsonObject userMsg = messages.createNestedObject();
  userMsg["role"] = "user";
  userMsg["content"] = prompt;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.println("Sending OpenAI request...");
  Serial.println("Payload: " + jsonPayload);
  
  int httpResponseCode = http.POST(jsonPayload);
  String response = "";
  
  Serial.println("HTTP Response Code: " + String(httpResponseCode));
  
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.println("OpenAI Response: " + response);
  } else {
    // Provide more detailed error information
    String errorMsg = "HTTP request failed: " + String(httpResponseCode);
    
    switch (httpResponseCode) {
      case -1:
        errorMsg += " (HTTPC_ERROR_CONNECTION_REFUSED)";
        break;
      case -2:
        errorMsg += " (HTTPC_ERROR_SEND_HEADER_FAILED)";
        break;
      case -3:
        errorMsg += " (HTTPC_ERROR_SEND_PAYLOAD_FAILED)";
        break;
      case -4:
        errorMsg += " (HTTPC_ERROR_NOT_CONNECTED)";
        break;
      case -5:
        errorMsg += " (HTTPC_ERROR_CONNECTION_LOST)";
        break;
      case -6:
        errorMsg += " (HTTPC_ERROR_READ_TIMEOUT)";
        break;
      case -11:
        errorMsg += " (HTTPC_ERROR_CONNECTION_REFUSED)";
        break;
      default:
        errorMsg += " (Unknown error)";
        break;
    }
    
    response = "{\"error\": \"" + errorMsg + "\"}";
    Serial.println("OpenAI request failed: " + errorMsg);
  }
  
  http.end();
  return response;
}

/**
 * Parse OpenAI JSON response into ToolCall array
 */
OpenAIResult parseOpenAIResponse(String jsonResponse) {
  OpenAIResult result;
  result.numToolCalls = 0;
  result.success = false;
  result.error = "";
  result.unknownCommands = "";
  
  // Check for error
  if (jsonResponse.indexOf("\"error\"") != -1) {
    result.error = "OpenAI API error: " + jsonResponse;
    return result;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonResponse);
  
  if (error) {
    result.error = "JSON parsing failed: " + String(error.c_str());
    return result;
  }
  
  // Extract content from OpenAI response
  String content = "";
  if (doc.containsKey("choices") && doc["choices"][0].containsKey("message") && 
      doc["choices"][0]["message"].containsKey("content")) {
    content = doc["choices"][0]["message"]["content"].as<String>();
  } else {
    result.error = "No content found in OpenAI response";
    return result;
  }
  
  Serial.println("OpenAI Content: " + content);
  
  // Parse the content as JSON (it should contain our tool calls)
  DynamicJsonDocument contentDoc(1024);
  DeserializationError contentError = deserializeJson(contentDoc, content);
  
  if (contentError) {
    result.error = "Content JSON parsing failed: " + String(contentError.c_str());
    return result;
  }
  
  // Extract tool calls
  if (contentDoc.containsKey("tool_calls")) {
    JsonArray toolCallsArray = contentDoc["tool_calls"];
    result.numToolCalls = min((int)toolCallsArray.size(), 10); // Max 10 tool calls
    
    for (int i = 0; i < result.numToolCalls; i++) {
      JsonObject toolCall = toolCallsArray[i];
      
      result.toolCalls[i].tool = toolCall["tool"].as<String>();
      result.toolCalls[i].params = toolCall["params"].as<String>();
      result.toolCalls[i].confidence = toolCall["confidence"].as<float>();
      result.toolCalls[i].isValid = (result.toolCalls[i].confidence > 0.9);
    }
  }
  
  // Extract unknown commands
  if (contentDoc.containsKey("unknown_commands")) {
    JsonArray unknownArray = contentDoc["unknown_commands"];
    for (JsonVariant unknown : unknownArray) {
      if (result.unknownCommands.length() > 0) {
        result.unknownCommands += ", ";
      }
      result.unknownCommands += unknown.as<String>();
    }
  }
  
  result.success = true;
  return result;
}

/**
 * Process natural language text through OpenAI to extract tool calls
 */
OpenAIResult processWithOpenAI(String content) {
  Serial.println("=== PROCESSING WITH OPENAI ===");
  Serial.println("Input: " + content);
  
  // Test connectivity first
  if (!testInternetConnectivity()) {
    Serial.println("Internet connectivity test failed - using fallback");
    return createFallbackResponse(content);
  }
  
  String response = makeOpenAIRequest(content);
  OpenAIResult result = parseOpenAIResponse(response);
  
  // If OpenAI failed, try fallback
  if (!result.success && result.error.indexOf("HTTP request failed") != -1) {
    Serial.println("OpenAI request failed - using fallback response");
    return createFallbackResponse(content);
  }
  
  Serial.println("Processing complete. Tool calls: " + String(result.numToolCalls));
  Serial.println("Success: " + String(result.success ? "true" : "false"));
  if (result.error.length() > 0) {
    Serial.println("Error: " + result.error);
  }
  
  return result;
}

/**
 * Execute an array of tool calls with delays between them
 */
String executeToolCalls(OpenAIResult result) {
  if (!result.success) {
    return "Error: " + result.error;
  }
  
  if (result.numToolCalls == 0) {
    if (result.unknownCommands.length() > 0) {
      return "No valid tool calls found. Unknown commands: " + result.unknownCommands;
    } else {
      return "No tool calls to execute";
    }
  }
  
  String executionResults = "Executing " + String(result.numToolCalls) + " tool calls:\n";
  
  for (int i = 0; i < result.numToolCalls; i++) {
    ToolCall call = result.toolCalls[i];
    
    if (!call.isValid) {
      executionResults += "Skipping " + call.tool + " (confidence: " + String(call.confidence) + " < 0.9)\n";
      continue;
    }
    
    Serial.println("Executing tool: " + call.tool + " with params: '" + call.params + "'");
    String toolResult = executeTool(call.tool, call.params);
    
    executionResults += "[" + String(i + 1) + "] " + call.tool + ": " + toolResult + "\n";
    
    // 250ms delay between tool calls
    if (i < result.numToolCalls - 1) {
      delay(250);
    }
  }
  
  if (result.unknownCommands.length() > 0) {
    executionResults += "\nUnknown commands: " + result.unknownCommands;
  }
  
  return executionResults;
}

/**
 * Test basic internet connectivity
 */
bool testInternetConnectivity() {
  Serial.println("Testing internet connectivity...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  http.setTimeout(5000); // 5 second timeout
  
  // Test with a simple HTTP request to a reliable service
  http.begin("http://httpbin.org/get");
  int responseCode = http.GET();
  
  if (responseCode > 0) {
    Serial.println("Internet connectivity test passed");
    http.end();
    return true;
  } else {
    Serial.println("Internet connectivity test failed: " + String(responseCode));
    http.end();
    return false;
  }
}

/**
 * Fallback response when OpenAI is unavailable
 */
OpenAIResult createFallbackResponse(String content) {
  OpenAIResult result;
  result.numToolCalls = 0;
  result.success = false;
  result.error = "OpenAI API unavailable - using fallback response";
  result.unknownCommands = content;
  
  // Try to extract simple commands without AI
  content.toLowerCase();
  
  if (content.indexOf("forward") != -1 || content.indexOf("backward") != -1 || 
      content.indexOf("left") != -1 || content.indexOf("right") != -1 || 
      content.indexOf("stop") != -1) {
    
    // Extract movement command
    String direction = "";
    String duration = "1000"; // Default 1 second
    
    if (content.indexOf("forward") != -1) direction = "forward";
    else if (content.indexOf("backward") != -1) direction = "backward";
    else if (content.indexOf("left") != -1) direction = "left";
    else if (content.indexOf("right") != -1) direction = "right";
    else if (content.indexOf("stop") != -1) direction = "stop";
    
    // Try to extract duration
    int secondsIndex = content.indexOf("second");
    if (secondsIndex != -1) {
      // Look for number before "second"
      for (int i = secondsIndex - 1; i >= 0; i--) {
        if (content.charAt(i) >= '0' && content.charAt(i) <= '9') {
          // Found a number, extract it
          int start = i;
          while (start > 0 && content.charAt(start - 1) >= '0' && content.charAt(start - 1) <= '9') {
            start--;
          }
          String numStr = content.substring(start, i + 1);
          int seconds = numStr.toInt();
          duration = String(seconds * 1000); // Convert to milliseconds
          break;
        }
      }
    }
    
    result.toolCalls[0].tool = "move_car";
    result.toolCalls[0].params = direction + " " + duration;
    result.toolCalls[0].confidence = 0.8;
    result.toolCalls[0].isValid = true;
    result.numToolCalls = 1;
    result.success = true;
    
  } else if (content.indexOf("distance") != -1 || content.indexOf("measure") != -1) {
    result.toolCalls[0].tool = "get_sonar_distance";
    result.toolCalls[0].params = "";
    result.toolCalls[0].confidence = 0.9;
    result.toolCalls[0].isValid = true;
    result.numToolCalls = 1;
    result.success = true;
  }
  
  return result;
}

// ============================================================================
// ITERATIVE PLANNING SYSTEM
// ============================================================================

/**
 * Execute iterative planning for a complex objective
 */
String executeIterativePlanning(String objective) {
  Serial.println("=== STARTING ITERATIVE PLANNING ===");
  Serial.println("Objective: " + objective);
  
  // Send initial status update
  sendMqttMessage("Starting iterative planning for objective: " + objective);
  
  // Initialize planning session
  PlanningSession session;
  session.objective = objective;
  session.currentContext = "Starting fresh. Objective: " + objective;
  session.executionHistory = "";
  session.iterationCount = 0;
  session.isComplete = false;
  session.finalResult = "";
  session.startTime = millis();
  session.lastIterationTime = millis();
  
  const int MAX_ITERATIONS = 10; // Prevent infinite loops
  const unsigned long MAX_PLANNING_TIME = 60000; // 60 seconds max
  
  while (!session.isComplete && 
         session.iterationCount < MAX_ITERATIONS && 
         (millis() - session.startTime) < MAX_PLANNING_TIME) {
    
    session.iterationCount++;
    session.lastIterationTime = millis();
    
    Serial.println("--- Iteration " + String(session.iterationCount) + " ---");
    Serial.println("Current context: " + session.currentContext);
    
    // Send iteration start update
    sendMqttMessage("Starting iteration " + String(session.iterationCount) + " - Context: " + session.currentContext);
    
    // Get planning decision from OpenAI
    PlanningDecision decision = processObjectiveIteratively(session);
    
    if (!decision.shouldContinue) {
      Serial.println("Planning decision: Stop planning");
      sendMqttMessage("Planning decision: Stop planning - " + decision.reasoning);
      session.isComplete = true;
      session.finalResult = decision.reasoning;
      break;
    }
    
    if (decision.objectiveComplete) {
      Serial.println("Planning decision: Objective complete");
      sendMqttMessage("Planning decision: Objective complete - " + decision.reasoning);
      session.isComplete = true;
      session.finalResult = decision.reasoning;
      break;
    }
    
    // Send planning decision update
    sendMqttMessage("Planning decision: " + String(decision.numToolCalls) + " tool calls - " + decision.reasoning);
    
    // Execute the tool calls
    String executionResults = executePlanningToolCalls(decision);
    
    // Send execution results update
    sendMqttMessage("Execution complete: " + String(decision.numToolCalls) + " tools executed");
    
    // Update session with results and check goal completion
    updatePlanningSession(session, decision, executionResults);
    
    // Check if goal was achieved during session update
    if (session.isComplete) {
      sendMqttMessage("SUCCESS: Planning stopped - objective has been achieved!");
      break;
    }
    
    // Small delay between iterations
    delay(500);
  }
  
  // Handle timeout or max iterations
  if (!session.isComplete) {
    if (session.iterationCount >= MAX_ITERATIONS) {
      session.finalResult = "Planning stopped: Maximum iterations reached (" + String(MAX_ITERATIONS) + ")";
      sendMqttMessage("Planning stopped: Maximum iterations reached");
    } else {
      session.finalResult = "Planning stopped: Time limit reached";
      sendMqttMessage("Planning stopped: Time limit reached");
    }
  }
  
  String summary = "=== ITERATIVE PLANNING COMPLETE ===\n";
  summary += "Objective: " + objective + "\n";
  summary += "Iterations: " + String(session.iterationCount) + "\n";
  summary += "Total time: " + String((millis() - session.startTime) / 1000) + " seconds\n";
  summary += "Final result: " + session.finalResult + "\n";
  summary += "Execution history:\n" + session.executionHistory;
  
  // Send final summary
  if (session.isComplete && session.finalResult.indexOf("achieved") != -1) {
    sendMqttMessage("🎉 SUCCESS: Planning completed successfully! " + String(session.iterationCount) + " iterations, " + String((millis() - session.startTime) / 1000) + " seconds - " + session.finalResult);
  } else {
    sendMqttMessage("Planning complete: " + String(session.iterationCount) + " iterations, " + String((millis() - session.startTime) / 1000) + " seconds - " + session.finalResult);
  }
  
  Serial.println(summary);
  return summary;
}

/**
 * Process objective through OpenAI for iterative planning
 */
PlanningDecision processObjectiveIteratively(PlanningSession session) {
  Serial.println("Processing objective iteratively...");
  
  String prompt = buildIterativePlanningPrompt(session);
  String response = makeOpenAIRequest(prompt);
  PlanningDecision decision = parsePlanningResponse(response);
  
  Serial.println("Planning decision - Continue: " + String(decision.shouldContinue ? "true" : "false"));
  Serial.println("Planning decision - Complete: " + String(decision.objectiveComplete ? "true" : "false"));
  Serial.println("Planning decision - Tool calls: " + String(decision.numToolCalls));
  Serial.println("Planning reasoning: " + decision.reasoning);
  
  return decision;
}

/**
 * Build prompt for iterative planning
 */
String buildIterativePlanningPrompt(PlanningSession session) {
  String prompt = "You are an intelligent robot planner. Analyze the current situation and decide what tools to use next.\n\n";
  
  prompt += "ORIGINAL OBJECTIVE: " + session.objective + "\n\n";
  
  prompt += "CURRENT CONTEXT:\n";
  prompt += session.currentContext + "\n\n";
  
  if (session.executionHistory.length() > 0) {
    prompt += "PREVIOUS EXECUTION RESULTS:\n";
    prompt += session.executionHistory + "\n\n";
  }
  
  prompt += "AVAILABLE TOOLS:\n";
  prompt += "- move_car: Controls movement (forward/backward/left/right/stop + value)\n";
  prompt += "- get_sonar_distance: Measures distance using ultrasonic sensor\n";
  prompt += "- test_sonar: Tests ultrasonic sensor\n";
  prompt += "- get_environment_info: Gathers current environment information\n";
  prompt += "- send_mqtt_message: Sends status updates over MQTT\n\n";
  
  prompt += "PLANNING RULES:\n";
  prompt += "- Consider the original objective and current progress\n";
  prompt += "- Use tools strategically to gather information or make progress\n";
  prompt += "- Only include tools with confidence > 0.9\n";
  prompt += "- Be precise with parameters\n";
  prompt += "- Maximum 5 tool calls per iteration\n";
  prompt += "- CRITICAL: After each action, evaluate if the objective is achieved\n";
  prompt += "- For conditional objectives (e.g., 'until X', 'when Y', 'within Z cm'), check completion criteria\n";
  prompt += "- If objective is achieved, mark as complete and send success message\n";
  prompt += "- If no progress can be made, stop planning\n";
  prompt += "- IMPORTANT: Use send_mqtt_message to provide status updates on your thinking and progress\n";
  prompt += "- Send updates before major decisions, after tool executions, and when objectives are complete\n\n";
  
  prompt += "RESPONSE FORMAT:\n";
  prompt += "{\n";
  prompt += "  \"tool_calls\": [\n";
  prompt += "    {\"tool\": \"tool_name\", \"params\": \"parameters\", \"confidence\": 0.95}\n";
  prompt += "  ],\n";
  prompt += "  \"should_continue\": true/false,\n";
  prompt += "  \"objective_complete\": true/false,\n";
  prompt += "  \"reasoning\": \"explanation of decision\",\n";
  prompt += "  \"next_context\": \"updated context for next iteration\"\n";
  prompt += "}\n\n";
  
  prompt += "EXAMPLES:\n";
  prompt += "Input: 'Find the nearest obstacle'\n";
  prompt += "Output: {\"tool_calls\": [{\"tool\": \"get_sonar_distance\", \"params\": \"\", \"confidence\": 0.98}], \"should_continue\": true, \"objective_complete\": false, \"reasoning\": \"Measuring distance to find obstacles\", \"next_context\": \"Checking for obstacles in front\"}\n\n";
  prompt += "Input: 'move forward until within 20cm of obstacle'\n";
  prompt += "Step 1: {\"tool_calls\": [{\"tool\": \"get_sonar_distance\", \"params\": \"\", \"confidence\": 0.98}], \"should_continue\": true, \"objective_complete\": false, \"reasoning\": \"Checking current distance to obstacle\", \"next_context\": \"Measuring distance before moving\"}\n";
  prompt += "Step 2: {\"tool_calls\": [{\"tool\": \"move_car\", \"params\": \"forward 1000\", \"confidence\": 0.95}, {\"tool\": \"get_sonar_distance\", \"params\": \"\", \"confidence\": 0.98}], \"should_continue\": true, \"objective_complete\": false, \"reasoning\": \"Moving forward and checking new distance\", \"next_context\": \"Moving toward obstacle\"}\n";
  prompt += "Step 3: {\"tool_calls\": [{\"tool\": \"send_mqtt_message\", \"params\": \"Goal achieved! Distance is 15cm, which is within 20cm target\", \"confidence\": 0.99}], \"should_continue\": false, \"objective_complete\": true, \"reasoning\": \"Distance is 15cm, which is within the 20cm target. Objective achieved!\", \"next_context\": \"Objective complete - within 20cm of obstacle\"}\n";
  
  return prompt;
}

/**
 * Parse planning response from OpenAI
 */
PlanningDecision parsePlanningResponse(String jsonResponse) {
  PlanningDecision decision;
  decision.numToolCalls = 0;
  decision.shouldContinue = false;
  decision.objectiveComplete = false;
  decision.reasoning = "";
  decision.nextContext = "";
  
  // Check for error
  if (jsonResponse.indexOf("\"error\"") != -1) {
    decision.reasoning = "OpenAI API error: " + jsonResponse;
    return decision;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonResponse);
  
  if (error) {
    decision.reasoning = "JSON parsing failed: " + String(error.c_str());
    return decision;
  }
  
  // Extract content from OpenAI response
  String content = "";
  if (doc.containsKey("choices") && doc["choices"][0].containsKey("message") && 
      doc["choices"][0]["message"].containsKey("content")) {
    content = doc["choices"][0]["message"]["content"].as<String>();
  } else {
    decision.reasoning = "No content found in OpenAI response";
    return decision;
  }
  
  Serial.println("OpenAI Planning Content: " + content);
  
  // Parse the content as JSON
  DynamicJsonDocument contentDoc(1024);
  DeserializationError contentError = deserializeJson(contentDoc, content);
  
  if (contentError) {
    decision.reasoning = "Content JSON parsing failed: " + String(contentError.c_str());
    return decision;
  }
  
  // Extract tool calls
  if (contentDoc.containsKey("tool_calls")) {
    JsonArray toolCallsArray = contentDoc["tool_calls"];
    decision.numToolCalls = min((int)toolCallsArray.size(), 5); // Max 5 tool calls
    
    for (int i = 0; i < decision.numToolCalls; i++) {
      JsonObject toolCall = toolCallsArray[i];
      
      decision.toolCalls[i].tool = toolCall["tool"].as<String>();
      decision.toolCalls[i].params = toolCall["params"].as<String>();
      decision.toolCalls[i].confidence = toolCall["confidence"].as<float>();
      decision.toolCalls[i].isValid = (decision.toolCalls[i].confidence > 0.9);
    }
  }
  
  // Extract planning decision
  decision.shouldContinue = contentDoc.containsKey("should_continue") ? contentDoc["should_continue"].as<bool>() : false;
  decision.objectiveComplete = contentDoc.containsKey("objective_complete") ? contentDoc["objective_complete"].as<bool>() : false;
  decision.reasoning = contentDoc.containsKey("reasoning") ? contentDoc["reasoning"].as<String>() : "";
  decision.nextContext = contentDoc.containsKey("next_context") ? contentDoc["next_context"].as<String>() : "";
  
  return decision;
}

/**
 * Execute tool calls from planning decision
 */
String executePlanningToolCalls(PlanningDecision decision) {
  if (decision.numToolCalls == 0) {
    return "No tool calls to execute in this iteration";
  }
  
  String executionResults = "Iteration tool calls:\n";
  
  for (int i = 0; i < decision.numToolCalls; i++) {
    ToolCall call = decision.toolCalls[i];
    
    if (!call.isValid) {
      executionResults += "Skipping " + call.tool + " (confidence: " + String(call.confidence) + " < 0.9)\n";
      continue;
    }
    
    Serial.println("Executing planning tool: " + call.tool + " with params: '" + call.params + "'");
    String toolResult = executeTool(call.tool, call.params);
    
    executionResults += "[" + String(i + 1) + "] " + call.tool + ": " + toolResult + "\n";
    
    // 250ms delay between tool calls
    if (i < decision.numToolCalls - 1) {
      delay(250);
    }
  }
  
  return executionResults;
}

/**
 * Evaluate if the objective has been achieved based on current context and execution history
 */
bool evaluateGoalCompletion(PlanningSession session, String latestResults) {
  String objective = session.objective;
  String context = session.currentContext;
  String history = session.executionHistory;
  String results = latestResults;
  
  objective.toLowerCase();
  context.toLowerCase();
  history.toLowerCase();
  results.toLowerCase();
  
  // Check for distance-based conditions
  if (objective.indexOf("within") != -1 && objective.indexOf("cm") != -1) {
    // Extract target distance
    int withinIndex = objective.indexOf("within");
    int cmIndex = objective.indexOf("cm");
    if (withinIndex != -1 && cmIndex != -1 && cmIndex > withinIndex) {
      String distanceStr = objective.substring(withinIndex + 6, cmIndex);
      distanceStr.trim();
      int targetDistance = distanceStr.toInt();
      
      // Look for distance measurements in results
      if (results.indexOf("distance:") != -1 || results.indexOf("sonar distance:") != -1) {
        // Extract actual distance from results
        int distanceStart = results.indexOf("distance:");
        if (distanceStart == -1) distanceStart = results.indexOf("sonar distance:");
        if (distanceStart != -1) {
          // Find the number after "distance:"
          for (int i = distanceStart; i < results.length(); i++) {
            if (results.charAt(i) >= '0' && results.charAt(i) <= '9') {
              // Found a number, extract it
              int start = i;
              while (i < results.length() && results.charAt(i) >= '0' && results.charAt(i) <= '9') {
                i++;
              }
              String actualDistanceStr = results.substring(start, i);
              int actualDistance = actualDistanceStr.toInt();
              
              // Check if we're within the target distance
              if (actualDistance <= targetDistance) {
                sendMqttMessage("GOAL ACHIEVED! Distance is " + String(actualDistance) + "cm, which is within " + String(targetDistance) + "cm target");
                return true;
              }
              break;
            }
          }
        }
      }
    }
  }
  
  // Check for "until" conditions
  if (objective.indexOf("until") != -1) {
    // Look for specific "until" conditions in the results
    if (results.indexOf("within") != -1 && results.indexOf("cm") != -1) {
      // This might indicate we've reached a distance condition
      return true;
    }
  }
  
  // Check for "stop" or "halt" conditions
  if (objective.indexOf("stop") != -1 || objective.indexOf("halt") != -1) {
    if (context.indexOf("stopped") != -1 || results.indexOf("stopped") != -1) {
      return true;
    }
  }
  
  return false;
}

/**
 * Update planning session with new results and evaluate goal completion
 */
void updatePlanningSession(PlanningSession &session, PlanningDecision decision, String executionResults) {
  // Add execution results to history
  if (session.executionHistory.length() > 0) {
    session.executionHistory += "\n";
  }
  session.executionHistory += "--- Iteration " + String(session.iterationCount) + " ---\n";
  session.executionHistory += "Reasoning: " + decision.reasoning + "\n";
  session.executionHistory += executionResults;
  
  // Evaluate if goal has been achieved
  bool goalAchieved = evaluateGoalCompletion(session, executionResults);
  if (goalAchieved) {
    session.isComplete = true;
    session.finalResult = "Objective achieved based on goal evaluation";
    sendMqttMessage("SUCCESS: Goal evaluation indicates objective has been achieved!");
  }
  
  // Update context for next iteration
  if (decision.nextContext.length() > 0) {
    session.currentContext = decision.nextContext;
  } else {
    session.currentContext = "Completed iteration " + String(session.iterationCount) + ". " + session.currentContext;
  }
} 