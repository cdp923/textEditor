#include "textEditorGlobals.h" 


std::vector<std::wstring> textBuffer; 
int caretLine = 0;
int caretCol = 0;
int scrollOffsetY = 0; // Vertical scroll offset (in lines)
int scrollOffsetX = 0; // Horizontal scroll offset (in pixels)
int maxLineWidthPixels = 0; // Maximum pixel width of any line in textBuffer

bool trackCaret = true; //scroll bars not working because autoscroll
int caretHiddenCount = 0;
int padding = 3;    //blank space before the end of the right side
int bufferZone = 2; //visible lines when scrolling back up

std::wstring currentFilePath;
bool documentModified = false;