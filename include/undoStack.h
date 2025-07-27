#pragma once
#include "textEditorGlobals.h"
#include "updateCaretAndScroll.h"
#include <windows.h>
#include <stack>
#include <string>

enum class UndoActionType {
    INSERT_TEXT, // User typed something (needs to be deleted on undo)
    DELETE_TEXT, // User deleted something (needs to be inserted on undo)
    LINE_SPLIT,  // User pressed Enter (line was split)
    LINE_JOIN    // User pressed Backspace to join lines
    // Add other types as needed (PASTE, CUT, REPLACE)
};

struct UndoAction {
    UndoActionType type; // What kind of action was it?
    int line;            
    int col;            
    std::wstring text;  
    
    UndoAction(UndoActionType type, int line, int col, const std::wstring& text = L"")
        : type(type), line(line), col(col), text(text) {}
};

extern std::stack<UndoAction> undoStack;

// Helper functions for character classification and grouping
bool IsWordChar(wchar_t ch);
bool ShouldGroupChars(wchar_t char1, wchar_t char2);

// Recording functions for undo actions
void RecordTyping(int line, int col, wchar_t ch);
void RecordDeletion(int line, int col, wchar_t ch);
void RecordAction(UndoActionType type, int line, int col, const std::wstring& text = L"");

// Undo execution
void PerformUndo(HWND hwnd);

// Text manipulation functions
void InsertTextAt(int line, int col, const std::wstring& text);
void DeleteTextAt(int line, int col, size_t length);
void MergeLines(int targetLine);
void SplitLine(int line, int col, const std::wstring& newRemainingText);

// Utility functions
void clearStack(std::stack<UndoAction>& undoStack);