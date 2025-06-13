#pragma once

#include "textEditorGlobals.h"
#include "textMetrics.h"

#include <windows.h>

void mouseDownL(HWND hwnd, LPARAM lParam, WPARAM wParam);
void mouseDragL(HWND hwnd, LPARAM lParam, WPARAM wParam);
void mouseUpL(HWND hwnd);
void DrawSelectionHighlight(HDC hdc, const RECT& rcLine);
void cursorChecks(PAINTSTRUCT ps, HWND hwnd, HDC hdc);