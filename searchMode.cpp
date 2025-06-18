#define NOMINMAX
#include "textEditorGlobals.h"
#include "textMetrics.h"
#include "updateCaretAndScroll.h"
#include <windows.h>
#include <algorithm>

bool isSearchMode = false;
std::wstring searchQuery;
std::wstring searchBoxText = L"Search: ";
std::vector<std::pair<int, int>> searchMatches;
size_t currentMatchIndex = 0;
int searchBoxHeight = 30;
int searchCaretPos = 8;

void ActivateSearchMode(HWND hwnd) {
    isSearchMode = true;
    searchBoxText = L"Search: ";
    searchQuery.clear();
    searchMatches.clear();
    currentMatchIndex = 0;
    searchCaretPos = 8;
    InvalidateRect(hwnd, NULL, TRUE);
}

void DeactivateSearchMode(HWND hwnd) {
    isSearchMode = false;
    InvalidateRect(hwnd, NULL, TRUE);
}

void DrawSearchBox(HWND hwnd, HDC hdc) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Search box rectangle
    RECT searchRect = {
        0, 
        clientRect.bottom - searchBoxHeight,
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
    
    // Draw buttons
    int buttonSize = 24;
    int buttonRight = clientRect.right - 10;
    int buttonTop = searchRect.top + 3;
    
    // Up button
    RECT upBtn = {buttonRight - buttonSize*2, buttonTop, buttonRight - buttonSize, buttonTop + buttonSize};
    HBRUSH btnBrush = CreateSolidBrush(RGB(220, 220, 220));
    FillRect(hdc, &upBtn, btnBrush);
    DrawTextW(hdc, L"▲", -1, &upBtn, DT_CENTER | DT_VCENTER);
    
    // Down button
    RECT downBtn = {buttonRight - buttonSize, buttonTop, buttonRight, buttonTop + buttonSize};
    FillRect(hdc, &downBtn, btnBrush);
    DrawTextW(hdc, L"▼", -1, &downBtn, DT_CENTER | DT_VCENTER);
    
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
    
    // Center the match in view
    scrollOffsetY = std::max(0, line - linesPerPage / 2);
    scrollOffsetX = std::max(0, col - 40);
    
    UpdateCaretPosition(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}
void DrawSearchMatches(HDC hdc, const RECT& paintRect) {
    if (!isSearchMode || searchQuery.empty()) return;

    // Setup highlight colors (yellow for search matches)
    HBRUSH hbrHighlight = CreateSolidBrush(RGB(255, 255, 150));  // Light yellow
    COLORREF oldTextColor = SetTextColor(hdc, RGB(0, 0, 0));
    COLORREF oldBkColor = SetBkColor(hdc, RGB(255, 255, 150));

    // Highlight current match differently (orange)
    HBRUSH hbrCurrent = CreateSolidBrush(RGB(255, 200, 100));
    
    // Draw all visible matches
    for (const auto& [line, col] : searchMatches) {
        // Skip if line isn't visible
        if (line < scrollOffsetY || line > scrollOffsetY + paintRect.bottom / charHeight) {
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
void HandleSearchInput(HWND hwnd, WPARAM wParam) {
    switch (wParam) {
        case VK_RETURN:
            searchQuery = searchBoxText.substr(8);
            break;
            
        case VK_ESCAPE:
            DeactivateSearchMode(hwnd);
            break;
            
        case VK_LEFT:
            if (searchCaretPos > 8) searchCaretPos--;
            break;
            
        case VK_RIGHT:
            if (searchCaretPos < searchBoxText.length()) searchCaretPos++;
            break;
            
        case VK_BACK:
            if (searchCaretPos > 8) {
                searchBoxText.erase(searchCaretPos - 1, 1);
                searchCaretPos--;
            }
            break;
            
        default:
            if (wParam >= 32 && wParam <= 126) {
                searchBoxText.insert(searchCaretPos, 1, (wchar_t)wParam);
                searchCaretPos++;
            }
            break;
    }
    if (searchBoxText.length() > 8) {
        FindAllMatches(hwnd);
    }
    InvalidateRect(hwnd, NULL, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
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
