#define NOMINMAX

#include "undoStack.h"
#include "isModified.h"

std::stack<UndoAction> undoStack;

// Helper function to determine if a character is a word character
bool IsWordChar(wchar_t ch) {
    return iswalnum(ch) || ch == L'_';
}

// Helper function to determine if we should group characters together
bool ShouldGroupChars(wchar_t char1, wchar_t char2) {
    bool char1IsWord = IsWordChar(char1);
    bool char2IsWord = IsWordChar(char2);
    
    // Group word characters together
    if (char1IsWord && char2IsWord) return true;
    
    // Group whitespace together (but not with other chars)
    if (iswspace(char1) && iswspace(char2)) return true;
    
    // Don't group different types
    return false;
}

void InsertTextAt(int line, int col, const std::wstring& text) {
    if (line < 0 || line >= textBuffer.size()) return;
    if (col < 0) col = 0;
    if (col > textBuffer[line].length()) col = textBuffer[line].length();
    textBuffer[line].insert(col, text);
}

void DeleteTextAt(int line, int col, size_t length) {
    if (line < 0 || line >= textBuffer.size()) return;
    if (col < 0) col = 0;
    if (col >= textBuffer[line].length()) return;

    size_t actualLength = std::min(length, textBuffer[line].length() - col);
    if (actualLength > 0) {
        textBuffer[line].erase(col, actualLength);
    }
}

void MergeLines(int targetLine) {
    if (targetLine < 0 || targetLine >= textBuffer.size() - 1) return;
    textBuffer[targetLine] += textBuffer[targetLine + 1];
    textBuffer.erase(textBuffer.begin() + targetLine + 1);
}

void SplitLine(int line, int col, const std::wstring& newRemainingText) {
    if (line < 0 || line >= textBuffer.size()) return;
    if (col < 0) col = 0;
    if (col > textBuffer[line].length()) col = textBuffer[line].length();

    textBuffer[line].resize(col); 
    textBuffer.insert(textBuffer.begin() + line + 1, newRemainingText); 
}

// Record a single character insertion for grouping
void RecordTyping(int line, int col, wchar_t ch) {
    if (undoStack.empty()) {
        // First action - just push it
        undoStack.push(UndoAction(UndoActionType::INSERT_TEXT, line, col, std::wstring(1, ch)));
        return;
    }
    
    UndoAction& lastAction = undoStack.top();
    
    // Check if we can merge with the last action
    bool canMerge = (lastAction.type == UndoActionType::INSERT_TEXT) &&
                    (lastAction.line == line) &&
                    (lastAction.col + lastAction.text.length() == col) &&
                    ShouldGroupChars(lastAction.text.back(), ch);
    
    if (canMerge) {
        // Merge with existing action
        lastAction.text += ch;
    } else {
        // Create new action
        undoStack.push(UndoAction(UndoActionType::INSERT_TEXT, line, col, std::wstring(1, ch)));
    }
}

// Record a single character deletion for grouping
void RecordDeletion(int line, int col, wchar_t ch) {
    if (undoStack.empty()) {
        // First action - just push it
        undoStack.push(UndoAction(UndoActionType::DELETE_TEXT, line, col, std::wstring(1, ch)));
        return;
    }
    
    UndoAction& lastAction = undoStack.top();
    
    // Check if we can merge with the last action (for backspace - deleting backwards)
    bool canMerge = (lastAction.type == UndoActionType::DELETE_TEXT) &&
                    (lastAction.line == line) &&
                    (lastAction.col == col + 1) && // Previous deletion was one position to the right
                    ShouldGroupChars(lastAction.text[0], ch); // Compare with first char of deletion
    
    if (canMerge) {
        // Merge with existing action - prepend since we're going backwards
        lastAction.text = ch + lastAction.text;
        lastAction.col = col; // Update starting position
    } else {
        // Create new action
        undoStack.push(UndoAction(UndoActionType::DELETE_TEXT, line, col, std::wstring(1, ch)));
    }
}

// Record other actions (line operations, etc.)
void RecordAction(UndoActionType type, int line, int col, const std::wstring& text) {
    undoStack.push(UndoAction(type, line, col, text));
}

void PerformUndo(HWND hwnd) {
    if (undoStack.empty()) {
        return;
    }

    UndoAction action = undoStack.top();
    undoStack.pop();

    switch (action.type) {
        case UndoActionType::INSERT_TEXT:
            DeleteTextAt(action.line, action.col, action.text.length());
            caretLine = action.line;
            caretCol = action.col;
            break;

        case UndoActionType::DELETE_TEXT:
            InsertTextAt(action.line, action.col, action.text);
            caretLine = action.line;
            caretCol = action.col + action.text.length();
            break;

        case UndoActionType::LINE_SPLIT:
            // To undo a line split, merge the line back.
            MergeLines(action.line);
            caretLine = action.line;
            caretCol = action.col;
            break;

        case UndoActionType::LINE_JOIN:
            // To undo a line join, split the line back.
            SplitLine(action.line, action.col, action.text);
            caretLine = action.line;
            caretCol = action.col;
            break;
    }

    UpdateCaretPosition(hwnd);
    isModifiedTag(textBuffer, hwnd);
    UpdateScrollBars(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

void clearStack(std::stack<UndoAction>& undoStack) {
    while(!undoStack.empty()) {
        undoStack.pop();
    }
}