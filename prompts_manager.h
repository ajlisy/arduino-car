#ifndef PROMPTS_MANAGER_H
#define PROMPTS_MANAGER_H

#include <Arduino.h>
#include "robot_tools.h"
#include "prompts_data.h"

class PromptsManager {
public:
  PromptsManager() {}
  
  /**
   * Initialize the prompts manager
   * Note: No SPIFFS initialization needed anymore
   */
  bool begin() {
    logToRobotLogs("Prompts Manager initialized successfully");
    logToRobotLogs("All prompts are now embedded in code - no file system needed");
    return true;
  }
  
  /**
   * Get the planning prompt template
   * Now returns the embedded prompt directly
   */
  String getPlanningPromptTemplate() {
    return String(ITERATIVE_PLANNING_PROMPT);
  }
  
  /**
   * Format the planning prompt with session data
   * Uses the embedded prompt from prompts_data.h
   */
  String formatPlanningPrompt(const String& objective, const String& context, const String& executionHistory) {
    return ::formatPlanningPrompt(objective, context, executionHistory);
  }
  
  /**
   * Check if prompts are available
   * Always returns true since prompts are embedded
   */
  bool promptsAvailable() {
    return true;
  }
  
  /**
   * Get prompt information for debugging
   */
  void logPromptInfo() {
    logToRobotLogs("Prompts Manager: All prompts embedded in code");
    logToRobotLogs("Planning prompt length: " + String(strlen(ITERATIVE_PLANNING_PROMPT)) + " characters");
  }
};

#endif // PROMPTS_MANAGER_H
