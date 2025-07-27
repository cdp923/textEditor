# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Compile the application:
```bash
# Navigate to project directory first
cd /path/to/textEditor

# Compile resource file
windres textEditor.rc -O coff -o textEditor.res

# Compile and link all components
g++ wWinMain.cpp WindowProc.cpp textEditorGlobals.cpp textMetrics.cpp updateCaretAndScroll.cpp fileOperations.cpp undoStack.cpp characterCase.cpp isModified.cpp cursorControls.cpp searchMode.cpp infoBar.cpp textEditor.res -o textEditor.exe -mwindows -municode -lcomdlg32

# Run the application
textEditor.exe
```

## Architecture Overview

This is a Windows text editor built using the Win32 API with a modular C++ architecture:

### Core Components:
- **wWinMain.cpp**: Application entry point and window creation
- **WindowProc.cpp/h**: Main window message handler
- **textEditorGlobals.cpp/h**: Global state management (text buffer, caret position, selection)

### Key Modules:
- **textMetrics.cpp/h**: Font metrics and text measurement calculations
- **updateCaretAndScroll.cpp/h**: Caret positioning and scrollbar management
- **fileOperations.cpp/h**: File I/O operations (open, save, new)
- **undoStack.cpp/h**: Undo/redo functionality
- **searchMode.cpp/h**: Text search functionality with search bar UI
- **infoBar.cpp/h**: Status bar showing line/column position and document stats
- **cursorControls.cpp/h**: Keyboard navigation and text editing operations
- **characterCase.cpp/h**: Text case conversion utilities
- **isModified.cpp/h**: Document modification tracking

### Global State:
- `textBuffer`: Vector of wide strings representing document lines
- `caretLine/caretCol`: Current cursor position
- `scrollOffsetX/Y`: Viewport scrolling offsets
- `selection`: Text selection state with start/end positions
- `currentFilePath`: Path of currently opened file
- `documentModified`: Tracks unsaved changes

### Key Features:
- Text editing with undo/redo support
- File operations (New, Open, Save, Save As)
- Text search with dedicated search bar
- Status bar with cursor position and document statistics
- Text selection and case conversion
- Scrollable viewport with caret tracking