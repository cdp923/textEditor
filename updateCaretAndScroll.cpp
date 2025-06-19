#define NOMINMAX 

#include "updateCaretAndScroll.h" 
#include "textEditorGlobals.h" // For textBuffer, caretLine, caretCol, scrollOffsetX, scrollOffsetY
#include "textMetrics.h"    // For charHeight, linesPerPage, font, maxLineWidthPixels
#include "infoBar.h"

#include <windows.h>
#include <algorithm> // For std::max, std::min

void UpdateCaretPosition(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    
    int x = 0;
    if (caretLine < textBuffer.size()) {
        SIZE size;
        GetTextExtentPoint32W(hdc, textBuffer[caretLine].c_str(), caretCol, &size);
        x = size.cx;
    }
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    if (trackCaret){
    // Horizontal auto-scroll
    bufferZoneX = (clientRect.right - clientRect.left)/2;
        if (x <= scrollOffsetX + bufferZoneX) {
            scrollOffsetX = std::max(0, x - bufferZoneX);
        }
        else if (x > scrollOffsetX + clientRect.right - padding) {
            scrollOffsetX = x - clientRect.right + padding;
        }
        
        // Vertical auto-scroll
        if (caretLine < scrollOffsetY + bufferZoneY) {
            if ((caretLine <= 1)) {
                scrollOffsetY = 0;
            }
            else {
                scrollOffsetY = caretLine - bufferZoneY;
            }
        }
        else if (caretLine >= scrollOffsetY + linesPerPage) {
            scrollOffsetY = caretLine - linesPerPage + 1;
        }
        x -= scrollOffsetX;
        int y = (caretLine - scrollOffsetY) * charHeight;

        caretHiddenCount = 0;
        // Determine if caret should be visible
        // Set caret position and visibility
        SetCaretPos(x, y);
        ShowCaret(hwnd);
        
    }else{
        x = x-scrollOffsetX;
        int y = (caretLine - scrollOffsetY) * charHeight;
        bool caretVisible = (y >= 0 && y < clientRect.bottom) && 
                    (x >= 0 && x< clientRect.right);
        //hiding is cumulative, so caretHiddenCount prevents it from triggering more than once
        if (!caretVisible&&caretHiddenCount==0){
            HideCaret(hwnd);
            caretHiddenCount ++;
        }else{
            SetCaretPos(x, y);
            ShowCaret(hwnd);
        }
    }
    /*
    // Calculate screen position
    x -= scrollOffsetX;
    int y = (caretLine - scrollOffsetY) * charHeight;
    
    // Update scroll bars
    UpdateScrollBars(hwnd);
    
    // Determine if caret should be visible
    
    // Set caret position and visibility
    SetCaretPos(x, y);
    */
    UpdateScrollBars(hwnd);
    // Force redraw if needed
    UpdateInfoBar(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    
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
    si_horz.nMax   = std::max(0L, (LONG)maxLineWidthPixels + padding);
    
    si_horz.nPage  = clientWidth; // Page size is the client width
    
    // Ensure scrollOffsetX is within the valid range (0 to nMax)
    scrollOffsetX = std::max(0, std::min(scrollOffsetX, (int)si_horz.nMax));
    si_horz.nPos   = scrollOffsetX;

    SetScrollInfo(hwnd, SB_HORZ, &si_horz, TRUE);
}

void HandleMouseWheelScroll(HWND hwnd, WPARAM wParam){
    short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    DWORD fwKeys = GET_KEYSTATE_WPARAM(wParam); // Get state of Ctrl, Shift, etc.

    if (fwKeys & MK_SHIFT) { // Check if Shift key is pressed for horizontal scroll
        int oldScrollOffsetX = scrollOffsetX;
        int pixelsToScroll = zDelta; // Or zDelta / WHEEL_DELTA * some_pixel_amount, e.g., charWidth * 3

        scrollOffsetX -= pixelsToScroll; // Adjust based on wheel direction

        // Clamp scrollOffsetX to valid range
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int maxPossibleScrollX = std::max(0, (int)(maxLineWidthPixels - clientRect.right));
        scrollOffsetX = std::max(0, std::min(scrollOffsetX, maxPossibleScrollX));

        if (scrollOffsetX != oldScrollOffsetX) {
            trackCaret = false; 
            UpdateScrollBars(hwnd);
            ScrollWindowEx(hwnd, (oldScrollOffsetX - scrollOffsetX), 0,
                        NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
            UpdateCaretPosition(hwnd);
        }
    } else { // Normal vertical scrolling
        int linesToScroll = zDelta / WHEEL_DELTA * 3;
        int oldScrollOffsetY = scrollOffsetY;

        scrollOffsetY -= linesToScroll;
        scrollOffsetY = std::max(0, scrollOffsetY);
        scrollOffsetY =std:: min(scrollOffsetY, std::max(0, (int)textBuffer.size() - linesPerPage));

        if (scrollOffsetY != oldScrollOffsetY) {
            trackCaret = false; 
            UpdateScrollBars(hwnd);
            ScrollWindowEx(hwnd, 0, (oldScrollOffsetY - scrollOffsetY) * charHeight,
                        NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
            UpdateCaretPosition(hwnd);
        }
    }
}

void HandleVerticalScroll(HWND hwnd, WPARAM wParam){
    int nScrollCode = LOWORD(wParam); // SB_LINEUP, SB_LINEDOWN, SB_THUMBPOSITION, etc.
    int nPos = HIWORD(wParam);        // Thumb position for SB_THUMBPOSITION
    int oldScrollOffsetY = scrollOffsetY;

    switch (nScrollCode) {
        case SB_LINEUP: // Up arrow
            scrollOffsetY--;
            break;
        case SB_LINEDOWN: // Down arrow
            scrollOffsetY++;
            break;
        case SB_THUMBPOSITION: // Dragging the thumb
        case SB_THUMBTRACK: // Real-time dragging
            scrollOffsetY = nPos;
            break;
        case SB_PAGEUP: // Page Up key or click in scroll area above thumb
            scrollOffsetY -= linesPerPage;
            break;
        case SB_PAGEDOWN: // Page Down key or click in scroll area below thumb
            scrollOffsetY += linesPerPage;
            break;
    }

    // Clamp scrollOffsetY
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    GetScrollInfo(hwnd, SB_VERT, &si);
    scrollOffsetY = std::max(0, std::min(scrollOffsetY, (int)si.nMax - (int)si.nPage + 1));

    if (scrollOffsetY != oldScrollOffsetY) {
        // Update scroll bar position using SetScrollInfo
        trackCaret = false; 
        SCROLLINFO si_update; 
        si_update.cbSize = sizeof(si_update);
        si_update.fMask = SIF_POS;
        si_update.nPos = scrollOffsetY;
        SetScrollInfo(hwnd, SB_VERT, &si_update, TRUE);

        ScrollWindowEx(hwnd, (oldScrollOffsetY - scrollOffsetY), 0,
                    NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
        UpdateCaretPosition(hwnd);
    }
}
void HandleHorizontalScroll(HWND hwnd, WPARAM wParam){
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int nScrollCode = LOWORD(wParam);
    int nPos = HIWORD(wParam);
    int oldScrollOffsetX = scrollOffsetX;

    switch (nScrollCode) {
        case SB_LINELEFT:
            scrollOffsetX -= charWidth;
            break;
        case SB_LINERIGHT:
            scrollOffsetX += charWidth;
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            scrollOffsetX = nPos;
            break;
        case SB_PAGELEFT:
            // Scroll by a "page" (client width or a fixed amount, i'll decide later)
            scrollOffsetX -= clientRect.right;
            break;
        case SB_PAGERIGHT:
            // Scroll by a "page" (client width or a fixed amount, i'll decide later)
            scrollOffsetX += clientRect.right;
            break;
    }

    // Clamp scrollOffsetX
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    GetScrollInfo(hwnd, SB_HORZ, &si);
    scrollOffsetX = std::max(0, std::min(scrollOffsetX, (int)si.nMax - (int)si.nPage + 1));

    if (scrollOffsetX != oldScrollOffsetX) {
        // Update scroll bar position using SetScrollInfo
        trackCaret = false; 
        SCROLLINFO si_update;
        si_update.cbSize = sizeof(si_update);
        si_update.fMask = SIF_POS;
        si_update.nPos = scrollOffsetX;
        SetScrollInfo(hwnd, SB_HORZ, &si_update, TRUE);

        ScrollWindowEx(hwnd, (oldScrollOffsetX - scrollOffsetX), 0,
                    NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
        UpdateCaretPosition(hwnd);
    }
}