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
  prompt += "- log_to_webhook: Sends log messages\n";
  prompt += "- test_sonar: Tests ultrasonic sensor\n\n";
  
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
  
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.println("OpenAI Response: " + response);
  } else {
    response = "{\"error\": \"HTTP request failed: " + String(httpResponseCode) + "\"}";
    Serial.println("OpenAI request failed: " + String(httpResponseCode));
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
  
  String response = makeOpenAIRequest(content);
  OpenAIResult result = parseOpenAIResponse(response);
  
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