# Architecture Changes - Unified Prompt System

## Overview

The Arduino Car project has been simplified from a dual-prompt system to a single, unified iterative planning approach.

## What Changed

### Before (Dual-Prompt System)
- **System Prompt**: Handled simple commands like "move forward 2 seconds"
- **Iterative Planning Prompt**: Handled complex goals like "move until < 20cm from obstacle"
- **Two different workflows** with different response formats
- **Complex routing logic** to decide which prompt to use

### After (Unified System)
- **Single Prompt**: `iterative_planning_prompt.md` handles ALL commands
- **One workflow**: Every command goes through iterative planning
- **Simplified architecture**: No more dual-path complexity
- **Better functionality**: Can handle both simple and complex goals

## Why This Change Makes Sense

### 1. **Your Use Case**
All your commands are actually complex goals that require:
- Multiple tool calls
- Progress assessment
- Iterative execution
- Goal completion checking

### 2. **Simpler Architecture**
- **One prompt file** to maintain
- **One workflow** to debug
- **One response format** to parse
- **Eliminates complexity** of choosing between approaches

### 3. **Better Functionality**
- **Simple commands** (like "move forward 2 seconds") work perfectly with iterative planning
- **Complex goals** (like "move until < 20cm from obstacle") work as intended
- **No artificial limitations** on what the robot can do

## Code Changes Made

### 1. **`arduino-car.ino`**
- **Removed**: Simple command processing path
- **Added**: `executeIterativePlanning()` function
- **Modified**: `executeCommand()` now always uses iterative planning
- **Added**: Prompts manager initialization

### 2. **`openai_processor.ino`**
- **Kept**: All iterative planning functions
- **Deprecated**: Simple command processing functions
- **Updated**: To work with the new unified approach

### 3. **Prompt Files**
- **`system_prompt.md`**: Marked as deprecated
- **`iterative_planning_prompt.md`**: Now the main prompt for everything

## New Workflow

```
MQTT Command → executeCommand() → executeIterativePlanning() → OpenAI Planning → Tool Execution → Progress Assessment → Repeat until Goal Complete
```

## Benefits

1. **Simplified Maintenance**: One prompt file to edit and maintain
2. **Better Functionality**: All commands get the full planning treatment
3. **Cleaner Code**: No more complex routing logic
4. **Easier Debugging**: One workflow to trace through
5. **More Robust**: Iterative planning can handle edge cases better

## Migration Notes

- **Existing simple commands** will still work but now get iterative planning
- **Complex goals** work exactly as before
- **No functionality lost** - only complexity removed
- **Better user experience** - all commands get intelligent planning

## Future Considerations

- **Remove `system_prompt.md`** entirely in next version
- **Clean up deprecated functions** in `openai_processor.ino`
- **Optimize iterative planning** for very simple commands
- **Add more sophisticated goal completion logic**
