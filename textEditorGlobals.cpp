#include "textEditorGlobals.h" 


std::vector<std::wstring> textBuffer; 
int caretLine = 0;
int caretCol = 0;
int scrollOffsetY = 0; // Vertical scroll offset (in lines)
int scrollOffsetX = 0; // Horizontal scroll offset (in pixels)
int maxLineWidthPixels = 0; // Maximum pixel width of any line in textBuffer