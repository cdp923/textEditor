#include "isModified.h"
#include "textEditorGlobals.h"
#include <filesystem>

std::vector<std::wstring> savedTextBuffer;

void setOriginal(std::vector<std::wstring> originalTextBuffer, HWND hwnd){
    savedTextBuffer = textBuffer; 
    documentModified = false;
    if (currentFilePath ==L""){
        SetWindowTextW(hwnd, (L"New Document"));
    }else{
        std::filesystem::path path(currentFilePath);
        SetWindowTextW(hwnd, path.filename().c_str());
    }
}
void isModifiedTag(std::vector<std::wstring> originalTextBuffer,HWND hwnd){
    if(originalTextBuffer != savedTextBuffer){
        documentModified = true;
        if (currentFilePath ==L""){
            SetWindowTextW(hwnd, (L"New Document (Modified)"));
        }else{
            std::filesystem::path path(currentFilePath);
            SetWindowTextW(hwnd, ((path.filename().wstring())+L" (Modified)").c_str());
        }
    }else{
        documentModified = false;
        if (currentFilePath ==L""){
            SetWindowTextW(hwnd, (L"New Document"));
        }else{
            std::filesystem::path path(currentFilePath);
            SetWindowTextW(hwnd, path.filename().c_str());
        }
    }
}