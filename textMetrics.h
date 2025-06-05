#pragma once

#include <windows.h> 

// Global font and metrics variables
extern HFONT font; 
extern TEXTMETRICW textMetrics;
extern int charWidth;
extern int charHeight;
extern int linesPerPage;
extern int maxCharWidth; // Max char width (useful for monospace, not used in code)

// Function to calculate and update text metrics
void calcTextMetrics(HWND hwnd);