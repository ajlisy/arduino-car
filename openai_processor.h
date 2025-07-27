#ifndef OPENAI_PROCESSOR_H
#define OPENAI_PROCESSOR_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Tool call structure
struct ToolCall {
  String tool;
  String params;
  float confidence;
  bool isValid;
};

// OpenAI processing result
struct OpenAIResult {
  ToolCall toolCalls[10];  // Max 10 tool calls
  int numToolCalls;
  String unknownCommands;
  bool success;
  String error;
};

/**
 * Process natural language text through OpenAI to extract tool calls
 * @param content The natural language text to process
 * @return OpenAIResult containing parsed tool calls
 */
OpenAIResult processWithOpenAI(String content);

/**
 * Execute an array of tool calls with delays between them
 * @param result The OpenAI processing result
 * @return String containing execution results
 */
String executeToolCalls(OpenAIResult result);

/**
 * Build the system prompt for OpenAI
 * @return String containing the system prompt
 */
String buildSystemPrompt();

/**
 * Make HTTP request to OpenAI API
 * @param prompt The complete prompt to send
 * @return String containing OpenAI response
 */
String makeOpenAIRequest(String prompt);

/**
 * Parse OpenAI JSON response into ToolCall array
 * @param jsonResponse The JSON response from OpenAI
 * @return OpenAIResult containing parsed tool calls
 */
OpenAIResult parseOpenAIResponse(String jsonResponse);

#endif // OPENAI_PROCESSOR_H 