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
    bool isContinuation;


    UndoAction(UndoActionType type, int line, int col, const std::wstring& text = L"", bool cont = false)
        : type(type), line(line), col(col), text(text), isContinuation(cont) {}
};

extern std::stack<UndoAction> undoStack;
extern UndoAction* g_currentTypingAction;

void RecordAction(UndoActionType type, int line, int col, const std::wstring& text = L"");
void PerformUndo(HWND hwnd);
void FinalizeTypingAction(HWND hwnd);

void InsertTextAt(int line, int col, const std::wstring& text);
void DeleteTextAt(int line, int col, size_t length);
void MergeLines(int targetLine);
void SplitLine(int line, int col, const std::wstring& newRemainingText);