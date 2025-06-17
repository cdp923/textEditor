#pragma once

#include "resource.h"
#include <vector>
#include <string>


// Global text buffer and stack
extern std::vector<std::wstring> textBuffer;


// Caret and scroll variables
extern int caretLine;
extern int caretCol;
extern int scrollOffsetY;
extern int scrollOffsetX;
extern int maxLineWidthPixels;
extern bool trackCaret; 
extern int caretHiddenCount;
extern int padding;
extern int bufferZoneY;
extern int bufferZoneX;

extern std::wstring currentFilePath;
extern bool documentModified;

struct Selection {
    int startLine, startCol;
    int endLine, endCol;
    bool active;

    void Clear() { 
    startLine = startCol = endLine = endCol = -1; 
    active = false; 
    }
};
extern Selection selection;
extern std::wstring selectedText;
