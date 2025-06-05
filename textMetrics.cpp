#define NOMINMAX 

#include "TextMetrics.h" 
#include "TextEditorGlobals.h" // For textBuffer and maxLineWidthPixels

#include <algorithm> 
#include <vector>    
HFONT font = NULL; 
TEXTMETRICW textMetrics;
int charWidth = 0;       
int charHeight = 0;      
int linesPerPage = 0;    
int maxCharWidth = 0;  
void calcTextMetrics(HWND hwnd){
    HDC hdc = GetDC(hwnd);

    // Create or get a font. Planning on adding custom fonts
    font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Select the font into the device context
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    GetTextMetricsW(hdc, &textMetrics);
    
    charWidth = textMetrics.tmAveCharWidth;
    charHeight = textMetrics.tmHeight + textMetrics.tmExternalLeading;
    maxCharWidth = textMetrics.tmMaxCharWidth;
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    // Calculate how many lines can fit vertically
    if (charHeight > 0) {
        linesPerPage = clientRect.bottom / charHeight;
    } else {
        linesPerPage = 1; // Avoid division by zero
    }
    // Ensure at least one line is always shown
    if (linesPerPage == 0) {linesPerPage = 1;}
    maxLineWidthPixels = 0;
    SIZE size;
    for (const auto& line : textBuffer) {
        GetTextExtentPoint32W(hdc, line.c_str(), line.length(), &size);
        if (size.cx > maxLineWidthPixels) {
            maxLineWidthPixels = size.cx;
        }
    }
    maxLineWidthPixels = std::max(maxLineWidthPixels, (int)clientRect.right); // Ensure at least client width
    // Select the old font back into the device context
    SelectObject(hdc, hOldFont);

    ReleaseDC(hwnd, hdc); // Release the device context
}