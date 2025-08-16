# SPIFFS Setup for Prompts System

## Overview

This project now uses SPIFFS (SPI Flash File System) to read prompt files dynamically from the ESP32's flash memory. This allows you to modify prompts directly in the `data/prompts/` directory and have the code automatically use the new content.

## Setup Instructions

### 1. Install ESP32 SPIFFS Upload Tool

You need to install the ESP32 SPIFFS upload tool in Arduino IDE:

1. Open Arduino IDE
2. Go to **Tools** → **Manage Libraries...**
3. Search for "ESP32 SPIFFS"
4. Install "ESP32 SPIFFS" by me-no-dev

### 2. Edit and Upload Prompt Files

1. **Edit the prompt files** directly in the `data/prompts/` directory
2. **Upload to SPIFFS** using Arduino IDE's "ESP32 Sketch Data Upload"
3. This uploads the contents of the `data/` directory to SPIFFS

### 3. Verify SPIFFS Upload

The code will automatically list all files in SPIFFS during initialization. You should see:

```
SPIFFS mounted successfully
SPIFFS files:
  /prompts/system_prompt.md - XXX bytes
  /prompts/iterative_planning_prompt.md - XXX bytes
```

## File Structure

```
arduino-car/
├── data/                       # Files to upload to SPIFFS
│   └── prompts/
│       ├── system_prompt.md
│       └── iterative_planning_prompt.md
├── prompts_manager.h           # SPIFFS file management
├── openai_processor.ino       # Updated to use file-based prompts
└── ... (other project files)
```

## How It Works

1. **At startup**: `initPromptsManager()` mounts SPIFFS and checks for prompt files
2. **When building prompts**: Functions read from SPIFFS files instead of hardcoded strings
3. **Dynamic updates**: Edit files in `data/prompts/`, upload to SPIFFS, restart ESP32

## Troubleshooting

### "SPIFFS Mount Failed"
- Ensure you're using an ESP32 board
- Try uploading sketch data again
- Check if flash memory is sufficient

### "File not found" warnings
- Verify files exist in `data/prompts/` directory
- Run "ESP32 Sketch Data Upload" again
- Check file names match exactly (case-sensitive)

### Prompts not updating
- Restart the ESP32 after uploading new data
- Check Serial Monitor for file reading confirmation
- Verify file sizes in SPIFFS listing

## Benefits

- **Live editing**: Modify prompts without recompiling code
- **Version control**: Track prompt changes separately from code
- **Collaboration**: Non-developers can edit prompts easily
- **Testing**: Try different prompt variations quickly
- **Maintenance**: Centralized prompt management

## Example Workflow

1. Edit `data/prompts/system_prompt.md` directly
2. Upload data to SPIFFS using Arduino IDE
3. Restart ESP32
4. New prompt is automatically used

## Notes

- SPIFFS uses a portion of the ESP32's flash memory
- Files are read-only at runtime (write operations require restart)
- Total SPIFFS size depends on your ESP32 board configuration
- Prompt files should be kept reasonably small (< 10KB each recommended)
