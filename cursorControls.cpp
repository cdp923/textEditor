#define NOMINMAX
#include "cursorControls.h"
#include "updateCaretAndScroll.h"
#include "searchMode.h"

#include <windows.h>
#include <algorithm>

void mouseDownL(HWND hwnd, LPARAM lParam, WPARAM wParam) {
    int mouseX = LOWORD(lParam);
    int mouseY = HIWORD(lParam);
    
    // First check if click is in search box
    if (isSearchMode) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int searchBoxTop = clientRect.bottom - searchBoxHeight;
        
        if (mouseY > searchBoxTop) {
            // Handle search box clicks
            int buttonRight = clientRect.right - 10;
            int buttonTop = searchBoxTop + 3;
            int buttonSize = 24;
            
            // Up button
            if (mouseX > buttonRight - buttonSize*2 && mouseX < buttonRight - buttonSize &&
                mouseY > buttonTop && mouseY < buttonTop + buttonSize) {
                FindPrevious(hwnd);
                return;
            }
            // Down button
            else if (mouseX > buttonRight - buttonSize && mouseX < buttonRight &&
                     mouseY > buttonTop && mouseY < buttonTop + buttonSize) {
                FindNext(hwnd);
                return;
            }
            // Text area click - set caret position
            else {
                HDC hdc = GetDC(hwnd);
                HFONT hOldFont = (HFONT)SelectObject(hdc, font);
                
                int clickX = mouseX - 10; // Account for margin
                searchCaretPos = (int)searchBoxText.length();
                
                // Find click position in text
                for (int i = 7; i <= searchBoxText.length(); i++) {
                    SIZE textSize;
                    GetTextExtentPoint32W(hdc, searchBoxText.c_str(), i, &textSize);
                    if (textSize.cx >= clickX) {
                        searchCaretPos = i;
                        break;
                    }
                }
                
                SelectObject(hdc, hOldFont);
                ReleaseDC(hwnd, hdc);
                InvalidateRect(hwnd, NULL, TRUE);
                return;
            }
        }
    }
    
    // Normal text area handling (original code)
    int tempCaretLine = (mouseY / charHeight) + scrollOffsetY;
    
    if (tempCaretLine < 0) {
        tempCaretLine = 0;
    }
    
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
    if (tempCaretCol > lineContent.length()) {
        tempCaretCol = lineContent.length();
    }
    caretCol = tempCaretCol;
    SetCapture(hwnd);
    
    // Store selection start
    selection.startLine = selection.endLine = caretLine;
    selection.startCol = selection.endCol = caretCol;
    selection.active = true;
    selectedText = getSelectedText(selection, textBuffer);
    
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
        selectedText = getSelectedText(selection, textBuffer);
        
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
        selectedText = getSelectedText(selection, textBuffer);

        if (selection.startLine == selection.endLine && 
            selection.startCol == selection.endCol) {
            selectedText.clear();
            selection.Clear();
        }

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

}
void DrawSelections(HDC hdc, const RECT& paintRect) {
    // Normalize selection coordinates
    int startLine = std::min(selection.startLine, selection.endLine);
    int endLine = std::max(selection.startLine, selection.endLine);
    int startCol, endCol;
    
    if (selection.startLine < selection.endLine) {
        startCol = selection.startCol;
        endCol = selection.endCol;
    } else if (selection.startLine > selection.endLine) {
        startCol = selection.endCol;
        endCol = selection.startCol;
    } else {
        startCol = std::min(selection.startCol, selection.endCol);
        endCol = std::max(selection.startCol, selection.endCol);
    }
    
    // Setup selection colors
    HBRUSH hbrHighlight = CreateSolidBrush(RGB(180, 215, 255));  // Light blue
    COLORREF oldTextColor = SetTextColor(hdc, RGB(0, 0, 0));
    COLORREF oldBkColor = SetBkColor(hdc, RGB(180, 215, 255));
    
    // Draw visible selections
    for (int line = std::max(startLine, scrollOffsetY); 
         line <= std::min(endLine, scrollOffsetY + (int)paintRect.bottom / charHeight); 
         line++) {
        
        RECT rcLine;
        rcLine.top = (line - scrollOffsetY) * charHeight;
        rcLine.bottom = rcLine.top + charHeight;
        
        // Calculate horizontal bounds
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
        
        // Clip to visible area
        rcLine.left = std::max(rcLine.left, paintRect.left);
        rcLine.right = std::min(rcLine.right, paintRect.right);
        
        if (rcLine.right > rcLine.left) {
            // Draw highlight background
            FillRect(hdc, &rcLine, hbrHighlight);
            
            // Redraw text with selection colors
            int textStart = std::max(0, ((int)rcLine.left + scrollOffsetX) / charWidth);
            int textEnd = std::min((int)textBuffer[line].length(), 
                             ((int)rcLine.right + scrollOffsetX) / charWidth);
            
            if (textEnd > textStart) {
                std::wstring visibleText = textBuffer[line].substr(
                    textStart, textEnd - textStart);
                TextOutW(hdc, 
                        textStart * charWidth - scrollOffsetX, 
                        rcLine.top,
                        visibleText.c_str(), 
                        visibleText.length());
            }
        }
    }
    // Restore DC state
    SetTextColor(hdc, oldTextColor);
    SetBkColor(hdc, oldBkColor);
    DeleteObject(hbrHighlight);
}

std::wstring getSelectedText(const Selection& selection, const std::vector<std::wstring>& textBuffer) {
    std::wstring selectedText;

    // Validate selection
    if (!selection.active || 
        selection.startLine >= textBuffer.size() || 
        selection.endLine >= textBuffer.size()) {
        return selectedText;
    }

    // Normalize selection coordinates
    int startLine = std::min(selection.startLine, selection.endLine);
    int endLine = std::max(selection.startLine, selection.endLine);
    int startCol = (selection.startLine < selection.endLine) ? selection.startCol : selection.endCol;
    int endCol = (selection.startLine < selection.endLine) ? selection.endCol : selection.startCol;

    // Handle single line selection
    if (startLine == endLine) {
        if (startCol == endCol) return selectedText; // Empty selection
        startCol = std::min(startCol, (int)textBuffer[startLine].length());
        endCol = std::min(endCol, (int)textBuffer[startLine].length());
        return textBuffer[startLine].substr(startCol, endCol - startCol);
    }

    // Multi-line selection
    for (int line = startLine; line <= endLine; ++line) {
        int lineStart = (line == startLine) ? startCol : 0;
        int lineEnd = (line == endLine) ? endCol : textBuffer[line].length();

        // Clamp to valid range
        lineStart = std::min(lineStart, (int)textBuffer[line].length());
        lineEnd = std::min(lineEnd, (int)textBuffer[line].length());

        if (lineStart < lineEnd) {
            selectedText += textBuffer[line].substr(lineStart, lineEnd - lineStart);
        }

        // Add newline except after last line
        if (line < endLine) selectedText += L'\n';
    }

    return selectedText;
}