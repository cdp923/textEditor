#define NOMINMAX
#include "WindowProc.h"


#include "textEditorGlobals.h"      // For textBuffer, caretLine, caretCol
#include "textMetrics.h"            // For calcTextMetrics, charHeight, font
#include "updateCaretAndScroll.h"   // For UpdateCaretPosition, UpdateScrollBars, and scroll handlers
#include "fileOperations.h"         // For open, save, saveAs
#include "undoStack.h"              // For stack operations

#include <algorithm> 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    //avoid bottleneck by using threads or other multitasking
    switch(uMsg){
        case WM_CREATE:
        {
            textBuffer.push_back(L"");
            font = CreateFont(
                -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE | FIXED_PITCH, L"Consolas" 
            );
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
            FinalizeTypingAction(hwnd);
            UpdateCaretPosition(hwnd); // Ensure caret is at correct position in case window was resized
            ShowCaret(hwnd); // Make the caret visible when the window gains focus
            break;
        }
        case WM_KILLFOCUS:
        {
            FinalizeTypingAction(hwnd);
            HideCaret(hwnd); // Hide the caret when the window loses focus
            break;
        }
        case WM_CLOSE: 
        {
            if(documentModified){
                int result = PromptForSave(hwnd); 
                if (result == IDCANCEL) {
                    //Dont close
                    return 0; 
                }
            }
            
            DestroyWindow(hwnd); 
            return 0; 
        }
        case WM_DESTROY:
        {
            if (font != NULL) {
                DeleteObject(font);
                font = NULL; // Set to NULL after deleting
            }
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
                        FinalizeTypingAction(hwnd);
                        RecordAction(UndoActionType::INSERT_TEXT, caretLine, caretCol, L"    ");
                        InsertTextAt(caretLine, caretCol, L"    "); // Insert 4 spaces
                        caretCol += 4;
                        break;
                    }
                    case L'\b': { // Backspace
                        if (caretCol > 0) {
                            FinalizeTypingAction(hwnd);
                            wchar_t deletedChar = textBuffer[caretLine][caretCol - 1];
                            RecordAction(UndoActionType::DELETE_TEXT, caretLine, caretCol - 1, std::wstring(1, deletedChar));
                            DeleteTextAt(caretLine, caretCol - 1, 1); // Erase character to the left
                            caretCol--;
                        } else if (caretLine > 0) {
                            FinalizeTypingAction(hwnd);
                            // Backspace at beginning of line: merge with previous line
                            int prevLineLength = textBuffer[caretLine - 1].length();
                            std::wstring currentLineContent = textBuffer[caretLine];
                            RecordAction(UndoActionType::LINE_JOIN, caretLine - 1, prevLineLength, currentLineContent);
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
                        FinalizeTypingAction(hwnd);
                        std::wstring remainingText;
                        if (caretCol < textBuffer[caretLine].length()) {
                            remainingText = textBuffer[caretLine].substr(caretCol);
                            textBuffer[caretLine].resize(caretCol); 
                            textBuffer.insert(textBuffer.begin() + caretLine + 1, remainingText); 
                        } else {
                            // If caret is at the end of a line, just add an empty new line
                            textBuffer.insert(textBuffer.begin() + caretLine + 1, L"");
                        }
                        RecordAction(UndoActionType::LINE_SPLIT, caretLine, caretCol, remainingText);
                        caretLine++; 
                        caretCol = 0; 

                        // Adjust scroll offset if new line goes past visible area
                        if (caretLine >= scrollOffsetY + linesPerPage) {
                            scrollOffsetY = caretLine - linesPerPage + 1;
                        }
                        break;
                    }
                    case L' ':{
                        FinalizeTypingAction(hwnd);
                        RecordAction(UndoActionType::INSERT_TEXT, caretLine, caretCol, L" ");
                        InsertTextAt(caretLine, caretCol, std::wstring(1, ch));
                        caretCol++;
                    }
                    default: { // All other printable characters
                        // Determine if we need a new action
                        bool needsNewAction = 
                            (g_currentTypingAction == nullptr) ||
                            (caretLine != g_currentTypingAction->line) ||
                            (caretCol != g_currentTypingAction->col + g_currentTypingAction->text.length());

                        if (needsNewAction) {
                            FinalizeTypingAction(hwnd);
                            g_currentTypingAction = new UndoAction(UndoActionType::INSERT_TEXT, 
                                                                caretLine, caretCol, 
                                                                std::wstring(1, ch));
                        } else {
                            g_currentTypingAction->text += ch;
                        }

                        InsertTextAt(caretLine, caretCol, std::wstring(1, ch));
                        caretCol++;
                        break;
                    }
                }
                trackCaret = true; 
                documentModified = true;
                SetWindowTextW(hwnd, (currentFilePath + (L" (Modified)")).c_str());
                calcTextMetrics(hwnd);
                UpdateScrollBars(hwnd);
                //Update display after any textBuffer or caret position change
                InvalidateRect(hwnd, NULL, TRUE); 
                UpdateCaretPosition(hwnd);       
                break;
            } 
        }
        case WM_KEYDOWN:{
            if (wParam != VK_SHIFT && wParam != VK_CONTROL && wParam != VK_MENU) {
                FinalizeTypingAction(hwnd);
            }
            if (wParam=='Z'&& GetAsyncKeyState(VK_CONTROL) & 0x8000){//Z has to be capital to work, 0x8000 means control key currently held down
                PerformUndo(hwnd);
            }
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
                        documentModified = true;
                        SetWindowTextW(hwnd, (currentFilePath + (documentModified ? L" (Modified)" : L"")).c_str());
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
        case WM_LBUTTONDOWN:
        {
            
            FinalizeTypingAction(hwnd);

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
                return 0; 
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

            SelectObject(hdc, hOldFont);
            ReleaseDC(hwnd, hdc);

            
            trackCaret = true; 
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateCaretPosition(hwnd);
            SetFocus(hwnd); 
            break;
        }
        case WM_MOUSEWHEEL:
        {
            FinalizeTypingAction(hwnd);
            HandleMouseWheelScroll(hwnd, wParam);
            return 0;
        }
        case WM_VSCROLL:
        {
            FinalizeTypingAction(hwnd);
            HandleVerticalScroll(hwnd, wParam);
            return 0;
        }
        case WM_HSCROLL:
        {
            FinalizeTypingAction(hwnd);
            HandleHorizontalScroll(hwnd, wParam);
            return 0;
        }
        case WM_COMMAND:
        {
            trackCaret = true;
            switch (LOWORD(wParam)) {
                case ID_FILE_NEW:
                    NewDocument(hwnd);
                    break;
                case ID_FILE_OPEN:
                    OpenFile(hwnd);
                    break;
                case ID_FILE_SAVE:
                    SaveFile(hwnd);
                    break;
                case ID_FILE_SAVEAS:
                    SaveFileAs(hwnd);
                    break;
                case ID_APP_EXIT: 
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            // Important: don't return 0 too early if other command handlers
            // (e.g., from controls) are needed, but for menus, it's fine.
            return 0; 
        }

        
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}