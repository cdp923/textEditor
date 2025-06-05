#pragma once

#include <windows.h> 

void UpdateCaretPosition(HWND hwnd);
void UpdateScrollBars(HWND hwnd);

// Below handlers are now part of WindowProc.cpp, but can be moved here
//void HandleMouseWheelScroll(HWND hwnd, WPARAM wParam);
//void HandleVerticalScroll(HWND hwnd, WPARAM wParam);
//void HandleHorizontalScroll(HWND hwnd, WPARAM wParam);