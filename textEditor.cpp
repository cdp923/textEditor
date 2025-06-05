#ifndef UNICODE
#define UNICODE
#endif


#define NOMINMAX 

#include <windows.h>
#include <algorithm> // For std::max and std::min
#include <cstdio>     // For printf
#include <vector>     // For std::vector
#include <string>     // For std::wstring
#include <strsafe.h>



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateScrollBars(HWND hwnd);

std::vector<std::wstring> textBuffer;

//caret and scroll variables 
int caretLine = 0;
int caretCol = 0;
int scrollOffsetY = 0; 
int scrollOffsetX = 0; 
int maxLineWidthPixels = 0;

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
    if (linesPerPage == 0) {linesPerPage = 1;}
    maxLineWidthPixels = 0;
    SIZE size;
    for (const auto& line : textBuffer) {
        GetTextExtentPoint32W(hdc, line.c_str(), line.length(), &size);
        if (size.cx > maxLineWidthPixels) {
            maxLineWidthPixels = size.cx+3;
        }
    }
    maxLineWidthPixels = std::max(maxLineWidthPixels, (int)clientRect.right); // Ensure at least client width
    // Select the old font back into the device context
    SelectObject(hdc, hOldFont);

    ReleaseDC(hwnd, hdc); // Release the device context
}

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
    if(x<=scrollOffsetX+3){
        scrollOffsetX = std::max(0, x - 3);
    }else if (x > scrollOffsetX + clientRect.right - 3) {
            scrollOffsetX = x - clientRect.right + 3;
        }
    //Vertical autoscroll
    if(caretLine<scrollOffsetY){
        if((caretLine<=1)){
            scrollOffsetY =0;
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
        WS_OVERLAPPEDWINDOW| WS_VSCROLL | WS_HSCROLL,            //Window style

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
