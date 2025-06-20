#define NOMINMAX
#include "textEditorGlobals.h"
#include "textMetrics.h"
#include "updateCaretAndScroll.h"
#include "infoBar.h"
#include <windows.h>
#include <algorithm>

bool isSearchMode = false;
std::wstring searchQuery;
std::wstring searchBoxText = L"Search: ";
std::vector<std::pair<int, int>> searchMatches;
size_t currentMatchIndex = 0;
int searchBoxHeight = 30;
int searchCaretPos = 8;
int savedCaretCol;
int savedCaretLine;
int savedScrollOffsetX;
int savedScrollOffsetY;

void ActivateSearchMode(HWND hwnd) {
    isSearchMode = true;
    searchBoxText = L"Search: ";
    searchQuery.clear();
    searchMatches.clear();
    currentMatchIndex = 0;
    searchCaretPos = 8;
    
    // Properly save current editor state
    savedCaretCol = caretCol;
    savedCaretLine = caretLine;
    savedScrollOffsetX = scrollOffsetX;
    savedScrollOffsetY = scrollOffsetY;
    
    InvalidateRect(hwnd, NULL, TRUE);
}

void DeactivateSearchMode(HWND hwnd) {
    isSearchMode = false;
    
    // Restore saved editor state instead of zeroing out
    caretCol = savedCaretCol;
    caretLine = savedCaretLine;
    scrollOffsetX = savedScrollOffsetX;
    scrollOffsetY = savedScrollOffsetY;
    
    UpdateCaretPosition(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateScrollBars(hwnd);
}

void DrawSearchBox(HWND hwnd, HDC hdc) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
   
    int searchBoxTop = clientRect.bottom - searchBoxHeight;
    if (showInfoBar) {
        searchBoxTop -= infoBarHeight;
    }
    // Search box rectangle
    RECT searchRect = {
        0,
        searchBoxTop,
        clientRect.right,
        clientRect.bottom
    };
   
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &searchRect, bgBrush);
   
    // Draw border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    MoveToEx(hdc, searchRect.left, searchRect.top, NULL);
    LineTo(hdc, searchRect.right, searchRect.top);
    SelectObject(hdc, oldPen);
   
    // Draw text
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, 10, searchRect.top + 8, searchBoxText.c_str(), (int)searchBoxText.length());
   
    // Draw buttons - Simple approach
    int buttonSize = 24;
    int buttonTop = searchRect.top + 3;
    
    HBRUSH btnBrush = CreateSolidBrush(RGB(220, 220, 220));
    
    // Up button - rightmost
    RECT upBtn = {clientRect.right - 80, buttonTop, clientRect.right - 56, buttonTop + buttonSize};
    FillRect(hdc, &upBtn, btnBrush);
    TextOutW(hdc, upBtn.left + 8, upBtn.top + 4, L"▲", 1);
   
    // Down button - middle
    RECT downBtn = {clientRect.right - 54, buttonTop, clientRect.right - 30, buttonTop + buttonSize};
    FillRect(hdc, &downBtn, btnBrush);
    TextOutW(hdc, downBtn.left + 8, downBtn.top + 4, L"▼", 1);
    
    // Close button - leftmost
    RECT xBtn = {clientRect.right - 28, buttonTop, clientRect.right - 4, buttonTop + buttonSize};
    FillRect(hdc, &xBtn, btnBrush);
    TextOutW(hdc, xBtn.left + 8, xBtn.top + 4, L"X", 1);
   
    // Cleanup
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
    DeleteObject(btnBrush);
   
    // Draw caret
    if (isSearchMode) {
        SIZE textSize;
        GetTextExtentPoint32W(hdc, searchBoxText.c_str(), searchCaretPos, &textSize);
        MoveToEx(hdc, 10 + textSize.cx, searchRect.top + 5, NULL);
        LineTo(hdc, 10 + textSize.cx, searchRect.bottom - 5);
    }
}

void JumpToMatch(HWND hwnd, size_t index) {
    if (index >= searchMatches.size()) return;
    
    auto [line, col] = searchMatches[index];
    caretLine = line;
    caretCol = col;
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Calculate available lines (accounting for search box)
    /*InfoBarHeight
    if(){

    }*/
    int availableHeight = clientRect.bottom - searchBoxHeight;
    if (showInfoBar) {
        availableHeight -= infoBarHeight;
    }
    int availableLines = availableHeight / charHeight;
    
    // Calculate available width
    int availableWidth = clientRect.right;
    int availableChars = availableWidth / charWidth;
    
    // Vertical scrolling - center the match vertically
    int targetScrollY = line - (availableLines / 2);
    scrollOffsetY = std::max(0, std::min(targetScrollY, (int)textBuffer.size() - availableLines));
    
    // Horizontal scrolling - ensure the entire match is visible
    int matchStartCol = col;
    int matchEndCol = col + (int)searchQuery.length();
    
    // If match extends beyond right edge, scroll to show the end
    if (matchEndCol * charWidth > scrollOffsetX + availableWidth) {
        scrollOffsetX = matchEndCol * charWidth - availableWidth + padding;
    }
    
    // If match starts before left edge, scroll to show the beginning
    if (matchStartCol * charWidth < scrollOffsetX) {
        scrollOffsetX = std::max(0, matchStartCol * charWidth - padding); 
    }
    
    // Ensure we don't scroll past the beginning
    scrollOffsetX = std::max(0, scrollOffsetX);
    
    // Force immediate update and refresh
    UpdateCaretPosition(hwnd);
    UpdateScrollBars(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

void DrawSearchMatches(HDC hdc, const RECT& paintRect) {
    if (!isSearchMode || searchQuery.empty()) return;

    // Setup highlight colors (yellow for search matches)
    HBRUSH hbrHighlight = CreateSolidBrush(RGB(255, 255, 150));  // Light yellow
    COLORREF oldTextColor = SetTextColor(hdc, RGB(0, 0, 0));
    COLORREF oldBkColor = SetBkColor(hdc, RGB(255, 255, 150));

    // Highlight current match differently (orange)
    HBRUSH hbrCurrent = CreateSolidBrush(RGB(255, 200, 100));
    
    // Calculate visible area accounting for search box
    int visibleHeight = paintRect.bottom - searchBoxHeight;
    if (showInfoBar) {
        visibleHeight -= infoBarHeight;
    }
    int maxVisibleLine = scrollOffsetY + (visibleHeight / charHeight);
    
    // Draw all visible matches
    for (const auto& [line, col] : searchMatches) {
        // Skip if line isn't visible (accounting for search box)
        if (line < scrollOffsetY || line > maxVisibleLine) {
            continue;
        }

        RECT rcMatch;
        rcMatch.top = (line - scrollOffsetY) * charHeight;
        rcMatch.bottom = rcMatch.top + charHeight;
        
        // Calculate match bounds
        rcMatch.left = col * charWidth - scrollOffsetX;
        rcMatch.right = (col + searchQuery.length()) * charWidth - scrollOffsetX;
        
        // Clip to visible area
        rcMatch.left = std::max(rcMatch.left, paintRect.left);
        rcMatch.right = std::min(rcMatch.right, paintRect.right);
        
        if (rcMatch.right > rcMatch.left) {
            // Use different color for current match
            bool isCurrent = (line == caretLine && col == caretCol);
            FillRect(hdc, &rcMatch, isCurrent ? hbrCurrent : hbrHighlight);
            
            // Redraw text with highlight colors
            int textStart = std::max(0, ((int)rcMatch.left + scrollOffsetX) / charWidth);
            int textEnd = std::min((int)textBuffer[line].length(), 
                             ((int)rcMatch.right + scrollOffsetX) / charWidth);
            
            if (textEnd > textStart) {
                std::wstring visibleText = textBuffer[line].substr(
                    textStart, textEnd - textStart);
                TextOutW(hdc, 
                        textStart * charWidth - scrollOffsetX, 
                        rcMatch.top,
                        visibleText.c_str(), 
                        visibleText.length());
            }
        }
    }

    // Restore DC state
    SetTextColor(hdc, oldTextColor);
    SetBkColor(hdc, oldBkColor);
    DeleteObject(hbrHighlight);
    DeleteObject(hbrCurrent);
}

void FindAllMatches(HWND hwnd) {
    searchMatches.clear();
    searchQuery = searchBoxText.substr(8); // Get text after "Search: "
    
    if (searchQuery.empty()) {
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }
    
    // Find all matches in the document
    for (int line = 0; line < textBuffer.size(); line++) {
        size_t pos = 0;
        while ((pos = textBuffer[line].find(searchQuery, pos)) != std::wstring::npos) {
            searchMatches.emplace_back(line, pos);
            pos += searchQuery.length();
        }
    }
    
    currentMatchIndex = 0;
    if (!searchMatches.empty()) {
        JumpToMatch(hwnd, 0); // Jump to first match
    }
}

void FindNext(HWND hwnd) {
    if (searchMatches.empty()) {
        FindAllMatches(hwnd); // Find matches if none exist
        return;
    }
    
    // Cycle to next match (wrap around if needed)
    currentMatchIndex = (currentMatchIndex + 1) % searchMatches.size();
    JumpToMatch(hwnd, currentMatchIndex);
}

void FindPrevious(HWND hwnd) {
    if (searchMatches.empty()) {
        FindAllMatches(hwnd);
        return;
    }
    
    // Cycle to previous match (wrap around if needed)
    currentMatchIndex = (currentMatchIndex == 0) ? searchMatches.size() - 1 : currentMatchIndex - 1;
    JumpToMatch(hwnd, currentMatchIndex);
}

void HandleSearchKeyDown(HWND hwnd, WPARAM wParam) {
    switch (wParam) {
        case VK_LEFT:
            if (searchCaretPos > 8) searchCaretPos--;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
            
        case VK_RIGHT:
            if (searchCaretPos < searchBoxText.length()) searchCaretPos++;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
            
        case VK_BACK:
            if (searchCaretPos > 8) {
                searchBoxText.erase(searchCaretPos - 1, 1);
                searchCaretPos--;
                FindAllMatches(hwnd); // Update search immediately on deletion
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (searchBoxText == L"Search: ") {
                // If search box is empty, clear matches
                searchQuery.clear();
                searchMatches.clear();
                FindAllMatches(hwnd); // Update search immediately on deletion
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
            
        case VK_RETURN:
            FindNext(hwnd);
            break;
            
        case VK_ESCAPE:
            DeactivateSearchMode(hwnd);
            break;
            
        case VK_F3:
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                FindPrevious(hwnd);
            } else {
                FindNext(hwnd);
            }
            break;
    }
}

void HandleSearchCharacterDown(HWND hwnd, wchar_t ch) {
    // Handle printable characters only
    if (ch >= 32 && ch <= 126) {
        searchBoxText.insert(searchCaretPos, 1, ch);
        searchCaretPos++;
        
        // Update search if we have enough text
        if (searchBoxText.length() > 8) { // "Search: " = 8 chars
            FindAllMatches(hwnd);
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }
}
