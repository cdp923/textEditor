#define NOMINMAX
#include "cursorControls.h"
#include "updateCaretAndScroll.h"

#include <windows.h>
#include <algorithm>

void mouseDownL(HWND hwnd, LPARAM lParam, WPARAM wParam){
    int mouseX = LOWORD(lParam);
    int mouseY = HIWORD(lParam);

    
    int tempCaretLine = (mouseY / charHeight) + scrollOffsetY;

    
    if (tempCaretLine < 0) {
        tempCaretLine = 0;
    }
    // If clicking below the last line of text, place caret at the end of the last line
    if (tempCaretLine >= textBuffer.size()) {
        caretLine = textBuffer.size() - 1; 
        if (caretLine < 0) caretLine = 0; 
        caretCol = textBuffer[caretLine].length(); 
        
        
        trackCaret = true; 
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateCaretPosition(hwnd);
        SetFocus(hwnd);
        return; 
    }

    // Now tempCaretLine is guaranteed to be a valid index within textBuffer
    caretLine = tempCaretLine;

    
    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, font); 

    const std::wstring& lineContent = textBuffer[caretLine];
    int effectiveMouseX = mouseX + scrollOffsetX;
    int tempCaretCol = 0;

    if (!lineContent.empty()) {
        
        int low = 0;
        int high = lineContent.length();
        int mid;

        while (low <= high) {
            mid = low + (high - low) / 2;
            SIZE size;
            GetTextExtentPoint32W(hdc, lineContent.c_str(), mid, &size);

            if (size.cx <= effectiveMouseX) {
                tempCaretCol = mid; // This many characters fit to the left of click
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        // Adjust for midpoint: If click is past the midpoint of the character at tempCaretCol,
        // move to the beginning of the next character.
        if (tempCaretCol < lineContent.length()) { 
            SIZE charWidthSize;
            GetTextExtentPoint32W(hdc, lineContent.c_str() + tempCaretCol, 1, &charWidthSize);
            
            SIZE currentTextWidth; 
            GetTextExtentPoint32W(hdc, lineContent.c_str(), tempCaretCol, &currentTextWidth);

            if (effectiveMouseX > (currentTextWidth.cx + charWidthSize.cx / 2)) {
                tempCaretCol++;
            }
        }
    } 
    if (tempCaretCol > lineContent.length()) {
        tempCaretCol = lineContent.length();
    }
    caretCol = tempCaretCol;
    SetCapture(hwnd);  // Continue tracking mouse outside window
    
    // Store selection start
    selection.startLine = selection.endLine = caretLine;
    selection.startCol = selection.endCol = caretCol;
    selection.active = true;
    
    

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);

    
    trackCaret = true; 
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateCaretPosition(hwnd);
    SetFocus(hwnd); 
}
void mouseDragL(HWND hwnd, LPARAM lParam, WPARAM wParam){
    if (GetCapture() == hwnd && (wParam & MK_LBUTTON)) {
        // Get mouse position
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);
        
        // Calculate line (same as LBUTTONDOWN)
        int tempCaretLine = (mouseY / charHeight) + scrollOffsetY;
        tempCaretLine = std::clamp(tempCaretLine, 0, (int)textBuffer.size() - 1);
        caretLine = tempCaretLine;
        
        // Calculate column (same as LBUTTONDOWN)
        HDC hdc = GetDC(hwnd);
        HFONT hOldFont = (HFONT)SelectObject(hdc, font);
        
        const std::wstring& lineContent = textBuffer[caretLine];
        int effectiveMouseX = mouseX + scrollOffsetX;
        int tempCaretCol = 0;
        
        if (!lineContent.empty()) {
            int low = 0;
            int high = lineContent.length();
            
            while (low <= high) {
                int mid = low + (high - low) / 2;
                SIZE size;
                GetTextExtentPoint32W(hdc, lineContent.c_str(), mid, &size);
                
                if (size.cx <= effectiveMouseX) {
                    tempCaretCol = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            
            if (tempCaretCol < lineContent.length()) {
                SIZE charWidthSize;
                GetTextExtentPoint32W(hdc, lineContent.c_str() + tempCaretCol, 1, &charWidthSize);
                SIZE currentTextWidth;
                GetTextExtentPoint32W(hdc, lineContent.c_str(), tempCaretCol, &currentTextWidth);
                
                if (effectiveMouseX > (currentTextWidth.cx + charWidthSize.cx / 2)) {
                    tempCaretCol++;
                }
            }
        }
        tempCaretCol = std::min(tempCaretCol, (int)lineContent.length());
        caretCol = tempCaretCol;
        
        SelectObject(hdc, hOldFont);
        ReleaseDC(hwnd, hdc);
        
        selection.endLine = caretLine;
        selection.endCol = caretCol;
        selection.active = true;
        
        // Visual update
        trackCaret = true;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateCaretPosition(hwnd);
    }
}
void mouseUpL(HWND hwnd){
    // Only process if we were tracking a selection
    if (GetCapture() == hwnd) {
        ReleaseCapture();  // Stop tracking mouse outside window, edit to autoscroll

        // Finalize the selection (optional - you may have already updated during WM_MOUSEMOVE)
        selection.endLine = caretLine;
        selection.endCol = caretCol;

        if (selection.startLine == selection.endLine && 
            selection.startCol == selection.endCol) {
            selection.Clear();
        }

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

}
void DrawSelectionHighlight(HDC hdc, const RECT& rcLine) {
    // Use system selection colors (adapts to user's theme)
    HBRUSH hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);
    SetBkMode (hdc, TRANSPARENT);
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    FillRect(hdc, &rcLine, hbrHighlight);
}
void cursorChecks(PAINTSTRUCT ps, HWND hwnd, HDC hdc){
    if (selection.active) {
        // Normalize selection
        int startLine = selection.startLine;
        int endLine = selection.endLine;
        int startCol = selection.startCol;
        int endCol = selection.endCol;
        
        if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
            std::swap(startLine, endLine);
            std::swap(startCol, endCol);
        }
        
        // Draw highlights
        for (int line = std::max(startLine, scrollOffsetY); 
            line <= std::min(endLine, std::min(scrollOffsetY + (int)ps.rcPaint.bottom / charHeight, (int)textBuffer.size() - 1)); 
            line++) {
            
            RECT rcLine;
            rcLine.top = (line - scrollOffsetY) * charHeight;
            rcLine.bottom = rcLine.top + charHeight;
            
            if (line == startLine && line == endLine) {
                rcLine.left = startCol * charWidth - scrollOffsetX;
                rcLine.right = endCol * charWidth - scrollOffsetX;
            } 
            else if (line == startLine) {
                rcLine.left = startCol * charWidth - scrollOffsetX;
                rcLine.right = textBuffer[line].length() * charWidth - scrollOffsetX;
            }
            else if (line == endLine) {
                rcLine.left = 0 - scrollOffsetX;
                rcLine.right = endCol * charWidth - scrollOffsetX;
            }
            else {
                rcLine.left = 0 - scrollOffsetX;
                rcLine.right = textBuffer[line].length() * charWidth - scrollOffsetX;
            }
            
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            rcLine.left = std::max(rcLine.left, 0L);
            rcLine.right = std::min(rcLine.right, rcClient.right);
            
            if (rcLine.right > rcLine.left) {
                // Choose one of these methods:
                DrawSelectionHighlight(hdc, rcLine);    // Recommended: follows user theme
                // DrawSelectionHighlight_Simple(hdc, rcLine);  // Alternative: custom color
                // DrawSelectionHighlight_Pattern(hdc, rcLine); // Alternative: pattern effect
            }
        }
    }
}