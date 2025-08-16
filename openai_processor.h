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

// Iterative planning session
struct PlanningSession {
  String objective;           // Original objective
  String currentContext;      // Current state/context
  String executionHistory;    // Results from previous tool calls
  int iterationCount;         // Current iteration number
  bool isComplete;           // Whether objective is achieved
  String finalResult;        // Final summary when complete
  unsigned long startTime;   // When planning started
  unsigned long lastIterationTime; // Last iteration timestamp
};

// Planning decision result
struct PlanningDecision {
  ToolCall toolCalls[5];     // Reduced max for iterative planning
  int numToolCalls;
  bool shouldContinue;       // Whether to continue planning
  bool objectiveComplete;    // Whether objective is achieved
  String reasoning;          // Why this decision was made
  String nextContext;        // Updated context for next iteration
};



// Function declarations for current system
OpenAIResult processWithOpenAI(String content);
String executeToolCalls(OpenAIResult result);
String buildSystemPrompt();
String makeOpenAIRequest(String prompt);
OpenAIResult parseOpenAIResponse(String jsonResponse);
bool testInternetConnectivity();
OpenAIResult createFallbackResponse(String content);

// Prompts manager - now embedded in code
// No initialization function needed

// Iterative planning function declarations
String executeIterativePlanning(String objective);
PlanningDecision processObjectiveIteratively(PlanningSession session);
String buildIterativePlanningPrompt(PlanningSession session);
PlanningDecision parsePlanningResponse(String jsonResponse);
String executePlanningToolCalls(PlanningDecision decision);
bool evaluateGoalCompletion(PlanningSession session, String latestResults);
void updatePlanningSession(PlanningSession &session, PlanningDecision decision, String executionResults);



#endif // OPENAI_PROCESSOR_H 