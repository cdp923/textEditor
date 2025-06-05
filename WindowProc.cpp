#define NOMINMAX
#include "WindowProc.h"


#include "textEditorGlobals.h"         // For textBuffer, caretLine, caretCol
#include "textMetrics.h"            // For calcTextMetrics, charHeight, font
#include "updateCaretAndScroll.h"   // For UpdateCaretPosition, UpdateScrollBars, and scroll handlers

#include <algorithm> 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    //avoid bottleneck by using threads or other multitasking
    switch(uMsg){
        case WM_CREATE:
        {
            textBuffer.push_back(L"");
            calcTextMetrics(hwnd);
            UpdateScrollBars(hwnd);
            CreateCaret(hwnd, NULL, 2, charHeight);
            //Gain focus immediately, so show immediately
            ShowCaret(hwnd);
            UpdateCaretPosition(hwnd);
            break;
        }
        case WM_SIZE:
        {
            calcTextMetrics(hwnd); 
            UpdateScrollBars(hwnd);
            UpdateCaretPosition(hwnd);
            InvalidateRect(hwnd, NULL, TRUE); 
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
             // All painting occurs here, between BeginPaint and EndPaint.
            HFONT hOldFont = (HFONT)SelectObject(hdc, font);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            // Loop through and draw visible lines (using scrollOffset and linesPerPage)
            for (int i = scrollOffsetY; i < textBuffer.size(); ++i) {
                int screenLineY = (i - scrollOffsetY) * charHeight;

                // Only draw lines that are actually visible within the client area
                if (screenLineY >= 0 && screenLineY < ps.rcPaint.bottom) { // Check if line is within paint rect vertically
                    TextOutW(hdc, -scrollOffsetX, screenLineY, textBuffer[i].c_str(), textBuffer[i].length());
                }
            }
            // Always select the old font back!
            SelectObject(hdc, hOldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SETFOCUS:
        {
            UpdateCaretPosition(hwnd); // Ensure caret is at correct position in case window was resized
            ShowCaret(hwnd); // Make the caret visible when the window gains focus
            break;
        }
        case WM_KILLFOCUS:
        {
            HideCaret(hwnd); // Hide the caret when the window loses focus
            break;
        }
        case WM_DESTROY:
        {
            DestroyCaret();
            PostQuitMessage(0);
            return 0;
        }
        case WM_CHAR:
        {
            wchar_t ch = (wchar_t)wParam;

            // Ensure we are within valid line bounds AND process valid input characters
            if (ch >= 32 || ch == L'\t' || ch == L'\r' || ch == L'\b') {
                if (caretLine >= textBuffer.size()) {
                    textBuffer.resize(caretLine + 1);
                }
                switch(ch) {
                    case L'\t': {
                        textBuffer[caretLine].insert(caretCol, L"    "); // Insert 4 spaces
                        caretCol += 4;
                        break;
                    }
                    case L'\b': { // Backspace
                        if (caretCol > 0) {
                            textBuffer[caretLine].erase(caretCol - 1, 1); // Erase character to the left
                            caretCol--;
                        } else if (caretLine > 0) {
                            // Backspace at beginning of line: merge with previous line
                            std::wstring currentLineContent = textBuffer[caretLine];
                            textBuffer.erase(textBuffer.begin() + caretLine); // Remove current line
                            caretLine--; // Move caret to previous line
                            caretCol = textBuffer[caretLine].length(); // Move caret to end of previous line
                            textBuffer[caretLine] += currentLineContent; // Append content to previous line
                            // Check if scrollOffset needs to decrease if we merge up and current line was at top
                            if (caretLine < scrollOffsetY) {
                                scrollOffsetY = caretLine;
                            }
                        }
                        break;
                    }
                    case L'\r': { // Enter key
                        // If caret is in the middle of a line, split it
                        if (caretCol < textBuffer[caretLine].length()) {
                            std::wstring remainingText = textBuffer[caretLine].substr(caretCol);
                            textBuffer[caretLine].resize(caretCol); 
                            textBuffer.insert(textBuffer.begin() + caretLine + 1, remainingText); 
                        } else {
                            // If caret is at the end of a line, just add an empty new line
                            textBuffer.insert(textBuffer.begin() + caretLine + 1, L"");
                        }
                        caretLine++; 
                        caretCol = 0; 

                        // Adjust scroll offset if new line goes past visible area
                        if (caretLine >= scrollOffsetY + linesPerPage) {
                            scrollOffsetY = caretLine - linesPerPage + 1;
                        }
                        break;
                    }
                    default: { // All other printable characters
                        textBuffer[caretLine].insert(caretCol, 1, ch); 
                        caretCol++; 
                        break;
                    }
                } 
                calcTextMetrics(hwnd);
                UpdateScrollBars(hwnd);
                //Update display after any textBuffer or caret position change
                InvalidateRect(hwnd, NULL, TRUE); 
                UpdateCaretPosition(hwnd);       
                break;
            } 
        }
        case WM_KEYDOWN:{
            switch (wParam){
                case VK_LEFT:{
                    if(caretCol>0){
                        caretCol--;
                    }else if (caretLine>0){
                        caretLine--;
                        caretCol = textBuffer[caretLine].length();
                    }
                    break;
                }
                case VK_RIGHT:{
                    if(caretCol<textBuffer[caretLine].length()){
                        caretCol++;
                    }else if (caretLine<textBuffer.size()-1){
                        caretCol++; 
                        caretLine++;
                        caretCol = 0;
                    }
                    break;
                }
                case VK_DOWN:{
                    if (caretLine <= textBuffer.size()){
                        caretLine++;
                        if(caretCol>textBuffer[caretLine].length()){
                            caretCol = textBuffer[caretLine].length();
                        }
                    }else if (caretLine >= textBuffer.size()) {
                        textBuffer.resize(caretLine + 1);
                        caretLine++;
                    }
                    break;
                }
                case VK_UP:{
                    if (caretLine >0){
                        caretLine--;
                        if (caretCol>textBuffer[caretLine].length()){
                            caretCol = textBuffer[caretLine].length();
                        }
                    }
                    break;
                }
            }
            //Update display after any textBuffer or caret position change
            UpdateScrollBars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE); 
            UpdateCaretPosition(hwnd); 
            break;
        }
        case WM_MOUSEWHEEL:
        {
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
                    UpdateScrollBars(hwnd);
                    ScrollWindowEx(hwnd, 0, (oldScrollOffsetY - scrollOffsetY) * charHeight,
                                NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
                    UpdateCaretPosition(hwnd);
                }
            }
            return 0;
        }
        case WM_VSCROLL:
        {
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
            GetScrollInfo(hwnd, SB_HORZ, &si);
            scrollOffsetY = std::max(0, std::min(scrollOffsetY, (int)si.nMax - (int)si.nPage + 1));

            if (scrollOffsetY != oldScrollOffsetY) {
                // Update scroll bar position using SetScrollInfo
                SCROLLINFO si_update;
                si_update.cbSize = sizeof(si_update);
                si_update.fMask = SIF_POS;
                si_update.nPos = scrollOffsetY;
                SetScrollInfo(hwnd, SB_HORZ, &si_update, TRUE);

                ScrollWindowEx(hwnd, (oldScrollOffsetY - scrollOffsetY), 0,
                            NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
                UpdateCaretPosition(hwnd);
            }
            return 0;
        }
        case WM_HSCROLL:
        {
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
                SCROLLINFO si_update;
                si_update.cbSize = sizeof(si_update);
                si_update.fMask = SIF_POS;
                si_update.nPos = scrollOffsetX;
                SetScrollInfo(hwnd, SB_HORZ, &si_update, TRUE);

                ScrollWindowEx(hwnd, (oldScrollOffsetX - scrollOffsetX), 0,
                            NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
                UpdateCaretPosition(hwnd);
            }
            return 0;
        }

        
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}