#define NOMINMAX
#include "WindowProc.h"


#include "textEditorGlobals.h"      // For textBuffer, caretLine, caretCol
#include "textMetrics.h"            // For calcTextMetrics, charHeight, font
#include "updateCaretAndScroll.h"   // For UpdateCaretPosition, UpdateScrollBars, and scroll handlers
#include "fileOperations.h"         // For open, save, saveAs
#include "undoStack.h"              // For stack operations
#include "characterCase.h"          //For characterCase
#include "isModified.h"

#include <algorithm> 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    //avoid bottleneck by using threads or other multitasking
    switch(uMsg){
        case WM_CREATE:
        {
            textBuffer.push_back(L"");
            setOriginal(textBuffer, hwnd);
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
            FinalizeAction(hwnd);
            UpdateCaretPosition(hwnd); // Ensure caret is at correct position in case window was resized
            ShowCaret(hwnd); // Make the caret visible when the window gains focus
            break;
        }
        case WM_KILLFOCUS:
        {
            FinalizeAction(hwnd);
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
            characterCase(ch, hwnd);
            break;
        }
        case WM_KEYDOWN:{
            if (wParam != VK_SHIFT && wParam != VK_CONTROL && wParam != VK_MENU) {
                FinalizeAction(hwnd);
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
            
            FinalizeAction(hwnd);

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
            FinalizeAction(hwnd);
            HandleMouseWheelScroll(hwnd, wParam);
            return 0;
        }
        case WM_VSCROLL:
        {
            FinalizeAction(hwnd);
            HandleVerticalScroll(hwnd, wParam);
            return 0;
        }
        case WM_HSCROLL:
        {
            FinalizeAction(hwnd);
            HandleHorizontalScroll(hwnd, wParam);
            return 0;
        }
        case WM_COMMAND:
        {
            trackCaret = true;
            FinalizeAction(hwnd);
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