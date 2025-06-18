#include "characterCase.h"
#include "textEditorGlobals.h"
#include "updateCaretAndScroll.h"
#include "undoStack.h"
#include "textMetrics.h"
#include "isModified.h"
#include "searchMode.h"

void characterCase(wchar_t ch, HWND hwnd, WPARAM wParam) {
    // Ensure we are within valid line bounds AND process valid input characters
    if (isSearchMode){
        HandleSearchInput(hwnd, wParam);
    }else{
        if (ch >= 32 || ch == L'\t' || ch == L'\r' || ch == L'\b') {
            if (caretLine >= textBuffer.size()) {
                textBuffer.resize(caretLine + 1);
            }
            
            switch(ch) {
                case L'\t': {
                    tabCase(ch, hwnd);
                    break;
                }
                case L'\b': { // Backspace
                    backspaceCase(ch, hwnd);
                    break;
                }
                case L'\r': { // Enter key
                    returnCase(ch, hwnd);
                    break;
                }
                case L' ': {
                    spaceCase(ch, hwnd);
                    break;
                }
                default: { // All other printable characters
                    defaultCase(ch, hwnd);
                    break;
                }
            }
        }
        
        trackCaret = true; 
        isModifiedTag(textBuffer, hwnd);
        calcTextMetrics(hwnd);
        UpdateScrollBars(hwnd);
        // Update display after any textBuffer or caret position change
        InvalidateRect(hwnd, NULL, TRUE); 
        UpdateCaretPosition(hwnd);       
    } 
}

void returnCase(wchar_t ch, HWND hwnd) {
    // If caret is in the middle of a line, split it
    std::wstring remainingText;
    if (caretCol < textBuffer[caretLine].length()) {
        remainingText = textBuffer[caretLine].substr(caretCol);
        textBuffer[caretLine].resize(caretCol); 
        textBuffer.insert(textBuffer.begin() + caretLine + 1, remainingText); 
    } else {
        // If caret is at the end of a line, just add an empty new line
        textBuffer.insert(textBuffer.begin() + caretLine + 1, L"");
    }
    
    // Record the line split for undo
    RecordAction(UndoActionType::LINE_SPLIT, caretLine, caretCol, remainingText);
    
    caretLine++; 
    caretCol = 0; 

    // Adjust scroll offset if new line goes past visible area
    if (caretLine >= scrollOffsetY + linesPerPage) {
        scrollOffsetY = caretLine - linesPerPage + 1;
    }
}

void backspaceCase(wchar_t ch, HWND hwnd) {
    if (caretCol > 0) {
        // Get the character we're about to delete
        wchar_t deletedChar = textBuffer[caretLine][caretCol - 1];
        
        // Record the deletion for undo (this handles grouping automatically)
        RecordDeletion(caretLine, caretCol - 1, deletedChar);
        
        // Perform the actual deletion
        DeleteTextAt(caretLine, caretCol - 1, 1);
        caretCol--;
        
    } else if (caretLine > 0) {
        // Backspace at beginning of line: merge with previous line
        int prevLineLength = textBuffer[caretLine - 1].length();
        std::wstring currentLineContent = textBuffer[caretLine];
        
        // Record the line join for undo
        RecordAction(UndoActionType::LINE_JOIN, caretLine - 1, prevLineLength, currentLineContent);
        
        // Perform the line merge
        textBuffer.erase(textBuffer.begin() + caretLine);
        caretLine--;
        caretCol = textBuffer[caretLine].length();
        textBuffer[caretLine] += currentLineContent;
        
        // Check if scrollOffset needs to decrease if we merge up and current line was at top
        if (caretLine < scrollOffsetY) {
            scrollOffsetY = caretLine;
        }
    }
}

void tabCase(wchar_t ch, HWND hwnd) {
    // Record the tab insertion as a single action (don't group tabs with other typing)
    RecordAction(UndoActionType::INSERT_TEXT, caretLine, caretCol, L"    ");
    
    // Insert 4 spaces
    InsertTextAt(caretLine, caretCol, L"    ");
    caretCol += 4;
}

void spaceCase(wchar_t ch, HWND hwnd) {
    // Record the space (this will group with other spaces automatically)
    RecordTyping(caretLine, caretCol, ch);
    
    // Insert the space
    InsertTextAt(caretLine, caretCol, std::wstring(1, ch));
    caretCol++;
}

void defaultCase(wchar_t ch, HWND hwnd) {
    // Record the character (this will group with similar characters automatically)
    RecordTyping(caretLine, caretCol, ch);
    
    // Insert the character
    InsertTextAt(caretLine, caretCol, std::wstring(1, ch));
    caretCol++;
}