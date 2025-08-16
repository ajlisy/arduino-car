#ifndef PROMPTS_MANAGER_H
#define PROMPTS_MANAGER_H

#include <SPIFFS.h>
#include <Arduino.h>

class PromptsManager {
private:
  bool spiffsInitialized;
  
  // File paths for prompts
  const char* PLANNING_PROMPT_FILE = "/prompts/iterative_planning_prompt.md";
  
public:
  PromptsManager() : spiffsInitialized(false) {}
  
  /**
   * Initialize SPIFFS file system
   */
  bool begin() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    spiffsInitialized = true;
    Serial.println("SPIFFS mounted successfully");
    
    // List available files for debugging
    listFiles();
    
    return true;
  }
  
  /**
   * Read a file from SPIFFS and return as String
   */
  String readFile(const char* filename) {
    if (!spiffsInitialized) {
      Serial.println("SPIFFS not initialized");
      return "";
    }
    
    if (!SPIFFS.exists(filename)) {
      Serial.println("File not found: " + String(filename));
      return "";
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
      Serial.println("Failed to open file: " + String(filename));
      return "";
    }
    
    String content = file.readString();
    file.close();
    
    Serial.println("Read file: " + String(filename) + " (" + String(content.length()) + " bytes)");
    return content;
  }
  

  
  /**
   * Get the planning prompt template from file
   */
  String getPlanningPromptTemplate() {
    return readFile(PLANNING_PROMPT_FILE);
  }
  
  /**
   * Format the planning prompt with session data
   */
  String formatPlanningPrompt(const String& objective, const String& context, const String& executionHistory) {
    String template_str = getPlanningPromptTemplate();
    
    // Replace placeholders with actual values
    template_str.replace("{{OBJECTIVE}}", objective);
    template_str.replace("{{CONTEXT}}", context);
    template_str.replace("{{EXECUTION_HISTORY}}", executionHistory);
    
    return template_str;
  }
  
  /**
   * List all files in SPIFFS for debugging
   */
  void listFiles() {
    if (!spiffsInitialized) return;
    
    Serial.println("SPIFFS files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
      Serial.println("  " + String(file.name()) + " - " + String(file.size()) + " bytes");
      file = root.openNextFile();
    }
    
    root.close();
  }
  
  /**
   * Check if a prompt file exists
   */
  bool promptFileExists(const char* filename) {
    if (!spiffsInitialized) return false;
    return SPIFFS.exists(filename);
  }
  
  /**
   * Get file size for a prompt file
   */
  size_t getPromptFileSize(const char* filename) {
    if (!spiffsInitialized) return 0;
    
    if (SPIFFS.exists(filename)) {
      File file = SPIFFS.open(filename, "r");
      size_t size = file.size();
      file.close();
      return size;
    }
    return 0;
  }
};

#endif // PROMPTS_MANAGER_H
