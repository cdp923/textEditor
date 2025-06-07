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
            //ShowCaret(hwnd); // Make the caret visible when the window gains focus
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
                trackCaret = true; 
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
                    trackCaret = true; 
                    if(caretCol>0){
                        caretCol--;
                    }else if (caretLine>0){
                        caretLine--;
                        caretCol = textBuffer[caretLine].length();
                    }
                    break;
                }
                case VK_RIGHT:{
                    trackCaret = true; 
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
                    trackCaret = true; 
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
                    trackCaret = true; 
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
            HandleMouseWheelScroll(hwnd, wParam);
            return 0;
        }
        case WM_VSCROLL:
        {
            HandleVerticalScroll(hwnd, wParam);
            return 0;
        }
        case WM_HSCROLL:
        {
            HandleHorizontalScroll(hwnd, wParam);
            return 0;
        }

        
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}