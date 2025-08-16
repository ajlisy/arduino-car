# Prompts Workflow

## Unified Iterative Planning System

This project now uses a single, unified approach: **all commands are processed through iterative planning**. This eliminates the need for separate simple vs. complex command handling.

## Directory Structure

```
arduino-car/
├── data/prompts/              # Edit prompts here
│   └── iterative_planning_prompt.md # MAIN PROMPT - handles all commands
├── prompts_manager.h           # Handles reading from SPIFFS
└── ... (other project files)
```

## How to Update Prompts

1. **Edit** the `iterative_planning_prompt.md` file directly in `data/prompts/`
2. **Upload** to SPIFFS using Arduino IDE: **Tools** → **ESP32 Sketch Data Upload**
3. **Restart** the ESP32
4. **Check** Serial Monitor for confirmation



## Template Variables

The planning prompt supports these placeholders:
- `{{OBJECTIVE}}` - The main goal to achieve
- `{{CONTEXT}}` - Current robot state/context
- `{{EXECUTION_HISTORY}}` - Results from previous tool calls

## Benefits

- **Unified system** - One prompt handles all command types
- **Simplified architecture** - No more dual-path approach
- **Better functionality** - Iterative planning can handle both simple and complex goals
- **Live updates** - Modify prompts without recompiling code
- **Simple workflow** - Edit → Upload → Restart
- **Version control** - Track prompt changes in git

## Example

1. Edit `data/prompts/iterative_planning_prompt.md`
2. Save the file
3. Upload to SPIFFS in Arduino IDE
4. Restart ESP32
5. New prompt is automatically used

## How It Works

1. **MQTT command received** (e.g., "move forward until < 20cm from obstacle")
2. **Iterative planning starts** - OpenAI analyzes the goal and creates a plan
3. **Tool execution** - Robot executes the planned tool calls
4. **Progress assessment** - Robot re-evaluates position and progress
5. **Iteration continues** - Planning and execution repeat until goal is reached
6. **Completion** - Robot reports success or timeout
