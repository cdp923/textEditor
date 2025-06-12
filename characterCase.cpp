#include "characterCase.h"

#include "characterCase.h"
#include "textEditorGlobals.h"
#include "updateCaretAndScroll.h"
#include "undoStack.h"
#include "textMetrics.h"
#include "isModified.h"

void characterCase(wchar_t ch, HWND hwnd){
    // Ensure we are within valid line bounds AND process valid input characters
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
            //Was having trouble with undoing single words over a space
            case L' ':{
                spaceCase(ch, hwnd);
                break;
            }
            default: { // All other printable characters
                defaultCase(ch, hwnd);
                break;
            }
        }
        trackCaret = true; 
        isModifiedTag(textBuffer, hwnd);
        calcTextMetrics(hwnd);
        UpdateScrollBars(hwnd);
        //Update display after any textBuffer or caret position change
        InvalidateRect(hwnd, NULL, TRUE); 
        UpdateCaretPosition(hwnd);       
    } 
}
void returnCase(wchar_t ch, HWND hwnd){
    // If caret is in the middle of a line, split it
    FinalizeAction(hwnd);
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
}

void backspaceCase(wchar_t ch, HWND hwnd){
    if (caretCol > 0) {
        FinalizeTypingAction(hwnd);
        wchar_t deletedChar = textBuffer[caretLine][caretCol - 1];
        
        //Undo logic start
        bool needsNewDelAction =
            (g_currentDeletionAction == nullptr) ||
            (caretLine != g_currentDeletionAction->line) ||
            (caretCol != g_currentDeletionAction->col + g_currentDeletionAction->text.length());
        
        // Handle space characters - always finalize and create new action
        if(deletedChar == L' ') {
            FinalizeDeletionAction(hwnd);
            g_currentDeletionAction = new UndoAction(UndoActionType::DELETE_TEXT,
                    caretLine, caretCol-1,
                    std::wstring(1, deletedChar));
        }
        // Handle other characters - only create new action if needed
        if (needsNewDelAction&&deletedChar != L' ') {
            FinalizeDeletionAction(hwnd);
            g_currentDeletionAction = new UndoAction(UndoActionType::DELETE_TEXT,
                                                caretLine, caretCol-1,
                                                std::wstring(1, deletedChar));
        } else {
            // Prepend the character since we're deleting backwards
            g_currentDeletionAction->text = deletedChar + g_currentDeletionAction->text;
            // Update the starting position
            g_currentDeletionAction->col = caretCol - 1;
        }
        //Undo logic end
        
        DeleteTextAt(caretLine, caretCol - 1, 1); // Erase character to the left
        caretCol--;
    } else if (caretLine > 0) {
        FinalizeAction(hwnd);
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
}

void tabCase(wchar_t ch, HWND hwnd){
    FinalizeAction(hwnd);
    RecordAction(UndoActionType::INSERT_TEXT, caretLine, caretCol, L"    ");
    InsertTextAt(caretLine, caretCol, L"    "); // Insert 4 spaces
    caretCol += 4;
}
void spaceCase(wchar_t ch, HWND hwnd){
    FinalizeAction(hwnd);
    RecordAction(UndoActionType::INSERT_TEXT, caretLine, caretCol, L" ");
    InsertTextAt(caretLine, caretCol, std::wstring(1, ch));
    caretCol++;
}
void defaultCase(wchar_t ch, HWND hwnd){
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
}