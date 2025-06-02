#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <cstdio>     // For printf
#include <vector>     // For std::vector
#include <string>     // For std::wstring


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::vector<std::wstring> textBuffer;

//caret and scroll variables 
int caretLine = 0;
int caretCol = 0;
int scrollOffset = 0; // Topmost visible line index

//text variables
HFONT font = NULL; 
TEXTMETRICW textMetrics; // Store font metrics (character width, line height, etc.)
int charWidth = 0;       // Average width of a character
int charHeight = 0;      // Height of a character
int linesPerPage = 0;    // Number of lines that fit vertically in the client area
int maxCharWidth = 0;    // Maximum character width for horizontal scrolling (optional, but good for monospace fonts)

void calcTextMetrics(HWND hwnd){
    HDC hdc = GetDC(hwnd);

    // Create or get a font. Planning on adding custom fonts
    font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

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
    if (linesPerPage == 0) linesPerPage = 1;
    // Select the old font back into the device context
    SelectObject(hdc, hOldFont);

    ReleaseDC(hwnd, hdc); // Release the device context
}

void UpdateCaretPosition(HWND hwnd){
    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);
    int x=0;
    if(caretLine<textBuffer.size()){
        if (caretCol<=textBuffer[caretLine].length())
        {
        SIZE size;
        // Get the width of the text up to the cursor position to ensure accurate X axis cursor movement
        GetTextExtentPoint32W(hdc, textBuffer[caretLine].c_str(), caretCol, &size);
        x = size.cx;
        }else if (caretCol>textBuffer[caretLine].length()){
        SIZE size;
        // Get the width of the text up to the cursor position to ensure accurate X axis cursor movement
        GetTextExtentPoint32W(hdc, textBuffer[caretLine].c_str(), caretCol, &size);
        x = size.cx;
        }
    }
    int y = (caretLine - scrollOffset)*charHeight;
    SetCaretPos(x,y);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
    //register window class
    const wchar_t CLASS_NAME[] = L"Text Editor";

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              //class style
        CLASS_NAME,                     //window class
        L"Text Editor",    //Window text
        WS_OVERLAPPEDWINDOW,            //Window style

        //Size and Postiton
        CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, 

        NULL,       //Parent Window
        NULL,       //Menu
        hInstance,  //Instance handle
        NULL        //Aditional app data
    );

    if (hwnd == NULL){
        return 0;
    }
    ShowWindow(hwnd, nCmdShow);
    
    //Run MSG loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while(GetMessage(&msg, NULL, 0, 0)>0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
        
    return 0;
    
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    //avoid bottleneck by using threads or other multitasking
    switch(uMsg){
        case WM_CREATE:
        {
            textBuffer.push_back(L"");
            calcTextMetrics(hwnd);
            CreateCaret(hwnd, NULL, 2, charHeight);
            //Gain focus immediately, so show immediately
            ShowCaret(hwnd);
            UpdateCaretPosition(hwnd);
            break;
        }
        case WM_SIZE:
        {
            calcTextMetrics(hwnd); 
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
            for (int i = scrollOffset; i < textBuffer.size(); ++i) {
                int screenLineY = (i - scrollOffset) * charHeight;

                // Only draw lines that are actually visible within the client area
                if (screenLineY >= 0 && screenLineY < ps.rcPaint.bottom) { // Check if line is within paint rect vertically
                    TextOutW(hdc, 0, screenLineY, textBuffer[i].c_str(), textBuffer[i].length());
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
                            if (caretLine < scrollOffset) {
                                scrollOffset = caretLine;
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
                        if (caretLine >= scrollOffset + linesPerPage) {
                            scrollOffset = caretLine - linesPerPage + 1;
                        }
                        break;
                    }
                    default: { // All other printable characters
                        textBuffer[caretLine].insert(caretCol, 1, ch); 
                        caretCol++; 
                        break;
                    }
                } 

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
                    }else if (caretLine<textBuffer.size()){
                        caretLine++;
                        caretCol = 0;
                    }else if (caretLine == textBuffer.size()){
                        textBuffer.resize(caretLine + 1);
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
                        if(caretLine >=scrollOffset + linesPerPage){
                            scrollOffset = caretLine - linesPerPage+1;
                            InvalidateRect(hwnd, NULL, TRUE); // Redraw
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
                        if(caretLine <scrollOffset){
                            scrollOffset = caretLine;
                            InvalidateRect(hwnd, NULL, TRUE); // Redraw
                        }
                    }
                    break;
                }
            }
            //Update display after any textBuffer or caret position change
            InvalidateRect(hwnd, NULL, TRUE); 
            UpdateCaretPosition(hwnd); 
            break;
        }

        
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
