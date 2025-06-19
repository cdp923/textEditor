#include "infoBar.h"
#include "textEditorGlobals.h"
#include <windows.h>

bool showInfoBar = true;
int infoBarHeight;
HWND infoBar;
void DrawInfoBar(HWND hwnd, HDC hdc) {
    if (!showInfoBar) return;
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Info bar rectangle at bottom
    RECT infoRect = {
        0,
        clientRect.bottom - infoBarHeight,
        clientRect.right,
        clientRect.bottom
    };
    
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &infoRect, bgBrush);
    
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    MoveToEx(hdc, infoRect.left, infoRect.top, NULL);
    LineTo(hdc, infoRect.right, infoRect.top);
    SelectObject(hdc, oldPen);
    
    // Calculate document statistics
    size_t totalChars = 0;
    size_t totalLines = textBuffer.size();
    
    for (const auto& line : textBuffer) {
        totalChars += line.length();
    }
    
    // Format info text
    wchar_t infoText[256];
    swprintf(infoText, 256,
            L"Ln %d, Col %d  |  Lines: %zu  |  Chars: %zu",
            caretLine + 1,
            caretCol + 1,
            totalLines,
            totalChars);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    RECT textRect = infoRect;
    textRect.left += 10;  // Left padding
    textRect.right -= 10; // Right padding
    
    DrawTextW(hdc, infoText, -1, &textRect, 
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
}

void UpdateInfoBar(HWND hwnd) {
    if (!showInfoBar) return;
    
    // Just invalidate the info bar area for redraw
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    RECT infoRect = {
        0,
        clientRect.bottom - infoBarHeight,
        clientRect.right,
        clientRect.bottom
    };
    
    InvalidateRect(hwnd, &infoRect, FALSE);
}

void InitInfoBar(HWND hwnd) {
    showInfoBar = true;
    
    // Calculate height based on system font
    HDC hdc = GetDC(hwnd);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    infoBarHeight = tm.tmHeight + 12; // Add padding
    
    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);
    
    UpdateInfoBar(hwnd);
}

void ShowHideInfoBar(HWND hwnd) {
    if (showInfoBar == false) {
        InvalidateRect(hwnd, NULL, TRUE);
    }else{
    
    InvalidateRect(hwnd, NULL, TRUE);
    
    // Trigger window resize to adjust editor area
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);\
    rcClient.bottom -= infoBarHeight;
    SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, 
                MAKELPARAM(rcClient.right, rcClient.bottom));
    }
}

RECT GetEditorClientRect(HWND hwnd) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    
    if (showInfoBar) {
        rcClient.bottom -= infoBarHeight;
    }
    
    return rcClient;
}