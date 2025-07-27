# Text Editor

This is a text editor similar to Notepad built using the Win32 API. 
I built this to practice GUI components and Windows programming.

## Features

- Text editing with undo/redo support
- File operations (New, Open, Save, Save As)
- Text search functionality
- Status bar showing cursor position and document statistics
- Text selection and case conversion
- Scrollable viewport with caret tracking

## Prerequisites

### Windows
- MinGW-w64 or MSYS2 with GCC
- Windows SDK (for Win32 API headers)

### macOS
- **Note**: This project uses Win32 API which is Windows-specific. To run on macOS, you'll need:
- Wine (to run Windows executables)
- MinGW cross-compiler for Windows targets

## Compilation Instructions

### Windows (Native)

#### Using MinGW/MSYS2:

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 terminal and install required packages:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-tools-git
   ```

3. Navigate to the project directory:
   ```bash
   cd /path/to/textEditor
   ```

4. Compile the resource file:
   ```bash
   windres textEditor.rc -O coff -o textEditor.res
   ```

5. Compile and link the application:
   ```bash
   g++ wWinMain.cpp WindowProc.cpp textEditorGlobals.cpp textMetrics.cpp updateCaretAndScroll.cpp fileOperations.cpp undoStack.cpp characterCase.cpp isModified.cpp cursorControls.cpp searchMode.cpp infoBar.cpp textEditor.res -o textEditor.exe -mwindows -municode -lcomdlg32
   ```

6. Run the application:
   ```bash
   ./textEditor.exe
   ```

### macOS (Cross-compilation)

#### Option 1: Using Homebrew with MinGW

1. Install Homebrew if not already installed:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. Install MinGW cross-compiler:
   ```bash
   brew install mingw-w64
   ```

3. Navigate to the project directory:
   ```bash
   cd /path/to/textEditor
   ```

4. Compile the resource file:
   ```bash
   x86_64-w64-mingw32-windres textEditor.rc -O coff -o textEditor.res
   ```

5. Cross-compile the application:
   ```bash
   x86_64-w64-mingw32-g++ wWinMain.cpp WindowProc.cpp textEditorGlobals.cpp textMetrics.cpp updateCaretAndScroll.cpp fileOperations.cpp undoStack.cpp characterCase.cpp isModified.cpp cursorControls.cpp searchMode.cpp infoBar.cpp textEditor.res -o textEditor.exe -mwindows -municode -lcomdlg32
   ```

6. Install Wine to run Windows executables:
   ```bash
   brew install --cask wine-stable
   ```

7. Run the application using Wine:
   ```bash
   wine textEditor.exe
   ```

#### Option 2: Using MacPorts with MinGW

1. Install MacPorts from https://www.macports.org/

2. Install MinGW cross-compiler:
   ```bash
   sudo port install x86_64-w64-mingw32-gcc
   ```

3. Follow the same compilation steps as Option 1, but use the MacPorts paths if needed.

## Troubleshooting

### Windows
- Ensure MinGW/MSYS2 is properly installed and in your PATH
- If you get "command not found" errors, restart your terminal after installation
- Make sure you're using the MinGW64 terminal, not the MSYS2 terminal

### macOS
- If cross-compilation fails, ensure MinGW is properly installed: `x86_64-w64-mingw32-gcc --version`
- Wine may require additional setup for first-time use
- Some Windows-specific features may not work perfectly under Wine

## Development

The project is structured with modular components:
- Main window handling in `WindowProc.cpp`
- Global state management in `textEditorGlobals.cpp`
- Feature-specific modules for search, undo, file operations, etc.

See `CLAUDE.md` for detailed architecture information.
