#define NOMINMAX
#include "WindowProc.h"


#include "textEditorGlobals.h"      // For textBuffer, caretLine, caretCol
#include "textMetrics.h"            // For calcTextMetrics, charHeight, font
#include "updateCaretAndScroll.h"   // For UpdateCaretPosition, UpdateScrollBars, and scroll handlers
#include "fileOperations.h"         // For open, save, saveAs
#include "undoStack.h"              // For stack operations
#include "characterCase.h"          //For characterCase
#include "isModified.h"
#include "cursorControls.h"

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
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HFONT hOldFont = (HFONT)SelectObject(hdc, font);
            
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            
            // Draw text first
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            SetBkMode(hdc, TRANSPARENT);
            
            for (int i = scrollOffsetY; i < textBuffer.size(); ++i) {
                int screenLineY = (i - scrollOffsetY) * charHeight;
                if (screenLineY >= 0 && screenLineY < ps.rcPaint.bottom) {
                    TextOutW(hdc, -scrollOffsetX, screenLineY, 
                            textBuffer[i].c_str(), textBuffer[i].length());
                }
            }
            
            cursorChecks(ps, hwnd, hdc);
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
            if (wParam=='Z'&& GetAsyncKeyState(VK_CONTROL) & 0x8000){//Z has to be capital to work, 0x8000 means control key currently held down
                PerformUndo(hwnd);
                break;
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
                        caretLine++;
                        caretCol = 0;
                    }
                    break;
                }
                case VK_DOWN:{
                    trackCaret = true; 
                    if (caretLine < textBuffer.size()-1){
                        caretLine++;
                        if(caretCol>textBuffer[caretLine].length()){
                            caretCol = textBuffer[caretLine].length();
                        }
                    }else {
                        textBuffer.push_back(L"");
                        caretLine++;
                        isModifiedTag(textBuffer, hwnd);
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
        case WM_MOUSEMOVE: {
            mouseDragL(hwnd, lParam, wParam);
        break;
        }
        case WM_LBUTTONDOWN:
        {
            mouseDownL(hwnd, lParam, wParam);
            break;
        }
        case WM_LBUTTONUP:{
            mouseUpL(hwnd);
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