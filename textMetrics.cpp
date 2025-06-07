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
    if (font == NULL) {
        // This path indicates an initialization order problem.
        // As a temporary workaround/fallback, you could create it here,
        // but it's better to fix the call order.
        font = CreateFont(
            -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | FIXED_PITCH, L"Consolas"
        );
        // If this block runs, you have an initialization order issue to resolve.
    }

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