#define NOMINMAX 

#include "updateCaretAndScroll.h" 
#include "textEditorGlobals.h" // For textBuffer, caretLine, caretCol, scrollOffsetX, scrollOffsetY
#include "textMetrics.h"    // For charHeight, linesPerPage, font, maxLineWidthPixels

#include <windows.h>
#include <algorithm> // For std::max, std::min
void UpdateCaretPosition(HWND hwnd){
    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    int x=0;
    if(caretLine<textBuffer.size()){
        SIZE size;
        // Get the width of the text up to the cursor position to ensure accurate X axis cursor movement
        GetTextExtentPoint32W(hdc, textBuffer[caretLine].c_str(), caretCol, &size);
        x = size.cx;
    }
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    //horizontal auto scroll
    int padding = 5;
    if(x<=scrollOffsetX+padding){
        scrollOffsetX = std::max(0, x - padding);
    }else if (x > scrollOffsetX + clientRect.right - padding) {
            scrollOffsetX = x - clientRect.right + padding;
        }
    //Vertical autoscroll
    int bufferZone = 2;
    if(caretLine<scrollOffsetY+bufferZone){
        if((caretLine<=1)){
            scrollOffsetY =  0;
        }else{
            scrollOffsetY = caretLine-2;
        }
    }else if(caretLine >=scrollOffsetY + linesPerPage){
        scrollOffsetY = caretLine - linesPerPage+1;
    }
    UpdateScrollBars(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    x-=scrollOffsetX;
    int y = (caretLine - scrollOffsetY)*charHeight;
    SetCaretPos(x,y);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);

}
void UpdateScrollBars(HWND hwnd) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    //vertical scroll bars
    SCROLLINFO si_vert; 
    si_vert.cbSize = sizeof(si_vert);
    si_vert.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si_vert.nMin   = 0;
    // Calculate total lines
    LONG totalLines = (LONG)textBuffer.size();
    // Max scroll position is when the last line is just visible at the bottom
    // So, it's (total lines - lines per page)
    si_vert.nMax   = std::max(0L, totalLines - 1);
    si_vert.nPage  = linesPerPage;
    si_vert.nPos   = scrollOffsetY;
    SetScrollInfo(hwnd, SB_VERT, &si_vert, TRUE);

     // Horizontal Scroll Bar
    SCROLLINFO si_horz;
    si_horz.cbSize = sizeof(si_horz);
    si_horz.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si_horz.nMin   = 0;
    
    LONG clientWidth = clientRect.right;
    // The maximum scroll range is the total content width minus the client width.
    // Ensure it's not negative if content fits.
    si_horz.nMax   = std::max(0L, (LONG)maxLineWidthPixels - 1);
    
    si_horz.nPage  = clientWidth; // Page size is the client width
    
    // Ensure scrollOffsetX is within the valid range (0 to nMax)
    scrollOffsetX = std::max(0, std::min(scrollOffsetX, (int)si_horz.nMax));
    si_horz.nPos   = scrollOffsetX;

    SetScrollInfo(hwnd, SB_HORZ, &si_horz, TRUE);
}