#include "isModified.h"
#include "textEditorGlobals.h"

std::vector<std::wstring> savedTextBuffer;

void setOriginal(std::vector<std::wstring> originalTextBuffer, HWND hwnd){
    savedTextBuffer = textBuffer; 
    documentModified = false;
    if (currentFilePath ==L""){
        currentFilePath =L"New Document";
    }
    SetWindowTextW(hwnd, currentFilePath.c_str());
}
void isModifiedTag(std::vector<std::wstring> originalTextBuffer,HWND hwnd){
    if(originalTextBuffer != savedTextBuffer){
        documentModified = true;
        SetWindowTextW(hwnd, (currentFilePath+L" (Modified)").c_str());
    }else{
        documentModified = false;
        SetWindowTextW(hwnd, currentFilePath.c_str());
    }
}