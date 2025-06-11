#pragma once

#include <windows.h> // For HWND, MessageBox, OPENFILENAMEW, etc.
#include <string>    // For std::wstring

// Declare the file operation functions
void LoadTextFromFile(HWND hwnd, const std::wstring& filePath);
bool SaveTextToFile(HWND hwnd, const std::wstring& filePath);
int PromptForSave(HWND hwnd);
void NewDocument(HWND hwnd);
void OpenFile(HWND hwnd);
void SaveFile(HWND hwnd);
void SaveFileAs(HWND hwnd);
