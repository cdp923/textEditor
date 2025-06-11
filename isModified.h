#pragma once

#include <windows.h> 
#include <vector>
#include <string>    // For std::wstring

extern std::vector<std::wstring> savedTextBuffer;

void isModifiedTag(std::vector<std::wstring> originalTextBuffer, HWND hwnd);
void setOriginal(std::vector<std::wstring> originalTextBuffer, HWND hwnd);