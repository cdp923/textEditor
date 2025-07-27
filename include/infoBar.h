#pragma once

#include <windows.h>
extern bool showInfoBar;
extern HWND infoBar;
extern int infoBarHeight;

// Function declarations
void InitInfoBar(HWND hwnd);
void DrawInfoBar(HWND hwnd, HDC hdc);
void UpdateInfoBar(HWND hwnd);
void ShowHideInfoBar(HWND hwnd);
RECT GetEditorClientRect(HWND hwnd);