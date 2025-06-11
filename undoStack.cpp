#define NOMINMAX

#include "undoStack.h"
#include "isModified.h"


std::stack<UndoAction> undoStack;

//Undoing single words was giving me difficulty
UndoAction* g_currentTypingAction = nullptr;
UndoAction* g_currentDeletionAction = nullptr;



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

void FinalizeTypingAction(HWND hwnd) {
    if (g_currentTypingAction != nullptr) {
        // Only push if there's actual text recorded
        if (!g_currentTypingAction->text.empty()) {
            // Check if we can merge with the previous undo action
            if (!undoStack.empty()) {
                UndoAction& prevAction = undoStack.top();
                if (prevAction.type == UndoActionType::INSERT_TEXT &&
                    prevAction.line == g_currentTypingAction->line &&
                    (prevAction.col + prevAction.text.length()) == g_currentTypingAction->col) {
                    // Merge with previous action
                    prevAction.text += g_currentTypingAction->text;
                    delete g_currentTypingAction;
                    g_currentTypingAction = nullptr;
                    return;
                }
            }
            
            // Otherwise push as new action
            undoStack.push(*g_currentTypingAction);
        }
        delete g_currentTypingAction;
        g_currentTypingAction = nullptr;
    }
}
void FinalizeDeletionAction(HWND hwnd) {
    if (g_currentDeletionAction != nullptr) {
        if (!g_currentDeletionAction->text.empty()) {
            // Check if we can merge with previous undo action
            if (!undoStack.empty()) {
                UndoAction& prevAction = undoStack.top();
                if (prevAction.type == UndoActionType::DELETE_TEXT &&
                    prevAction.line == g_currentDeletionAction->line &&
                    prevAction.col == (g_currentDeletionAction->col + g_currentDeletionAction->text.length())) {
                    // Merge with previous deletion
                    prevAction.text = g_currentDeletionAction->text + prevAction.text;
                    prevAction.col = g_currentDeletionAction->col;
                    delete g_currentDeletionAction;
                    g_currentDeletionAction = nullptr;
                    return;
                }
            }
            
            undoStack.push(*g_currentDeletionAction);
        }
        delete g_currentDeletionAction;
        g_currentDeletionAction = nullptr;
    }
}
void FinalizeAction(HWND hwnd){
    FinalizeDeletionAction(hwnd);
    FinalizeTypingAction(hwnd);
}

void RecordAction(UndoActionType type, int line, int col, const std::wstring& text) {
    undoStack.push(UndoAction(type, line, col, text));
}


void PerformUndo(HWND hwnd) {
    FinalizeAction(hwnd);

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