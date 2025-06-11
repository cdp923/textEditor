#pragma once

#include "characterCase.h"
#include "textEditorGlobals.h"
#include "updateCaretAndScroll.h"
#include "undoStack.h"
#include "textMetrics.h"

#include <windows.h>


void characterCase(wchar_t ch, HWND hwnd);
void returnCase(wchar_t ch, HWND hwnd);
void backspaceCase(wchar_t ch, HWND hwnd);
void tabCase(wchar_t ch, HWND hwnd);
void spaceCase(wchar_t ch, HWND hwnd);
void defaultCase(wchar_t, HWND hwnd);
