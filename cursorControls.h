#pragma once

#include "textEditorGlobals.h"
#include "textMetrics.h"

#include <windows.h>

void mouseDownL(HWND hwnd, LPARAM lParam, WPARAM wParam);
void mouseDragL(HWND hwnd, LPARAM lParam, WPARAM wParam);
void mouseUpL(HWND hwnd);
void DrawSelections(HDC hdc, const RECT& paintRect);
std::wstring getSelectedText(const Selection& selection, const std::vector<std::wstring>& textBuffer);