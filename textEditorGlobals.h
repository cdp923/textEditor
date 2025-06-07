#pragma once

#include <vector>
#include <string>

// Global text buffer
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
extern int bufferZone;