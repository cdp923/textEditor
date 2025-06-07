#include "fileOperations.h" // Include its own header for function prototypes
#include "TextEditorGlobals.h" // For access to global variables and other utilities
#include "textMetrics.h"       // For calcTextMetrics
#include "updateCaretAndScroll.h" // For UpdateScrollBars, UpdateCaretPosition

#include <fstream>   // For std::wifstream, std::wofstream
#include <commdlg.h> // For GetOpenFileNameW, GetSaveFileNameW
#include <strsafe.h> // For StringCchCopyW, wcsrchr

void LoadTextFromFile(HWND hwnd, const std::wstring& filePath) {
    std::wifstream inputFile(filePath.c_str());
    if (!inputFile.is_open()) {
        MessageBox(hwnd, L"Could not open file for reading.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    textBuffer.clear();
    std::wstring line;

    while (std::getline(inputFile, line)) {
        textBuffer.push_back(line);
    }
    inputFile.close();

    if (textBuffer.empty()) {
        //ensure 1 line if empty to avoid errors
        textBuffer.push_back(L""); 
    }

    currentFilePath = filePath;
    documentModified = false;

    //reset scroll and caret positions
    
    caretLine = 0;
    caretCol = 0;
    scrollOffsetY = 0;
    scrollOffsetX = 0;
    

    // Call functions to update UI
    calcTextMetrics(hwnd);
    UpdateScrollBars(hwnd);
    UpdateCaretPosition(hwnd);
    CreateCaret(hwnd, NULL, 2, charHeight);
    ShowCaret(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);

    SetWindowTextW(hwnd, currentFilePath.c_str());
}

bool SaveTextToFile(HWND hwnd, const std::wstring& filePath) {
    std::wofstream outputFile(filePath.c_str()); 

    if (!outputFile.is_open()) { 
        MessageBox(hwnd, L"Could not open file for writing.", L"Error", MB_ICONERROR | MB_OK);
        return false; 
    }

    for (size_t i = 0; i < textBuffer.size(); ++i) { 
        outputFile << textBuffer[i]; 
        if (i < textBuffer.size() - 1) { //textBuffer.size() - 1 so that the last line doesnt make a new line
            outputFile << L'\n';        
        }
    }
    outputFile.close(); 

    currentFilePath = filePath; 
    documentModified = false;   

    
    SetWindowTextW(hwnd, currentFilePath.c_str());
    return true; // Indicate successful save
}
int PromptForSave(HWND hwnd) {
    if (!documentModified) { 
        return IDYES; 
    }

    
    int result = MessageBox(hwnd,
        L"Do you want to save changes to your document?",
        L"Text Editor",
        MB_YESNOCANCEL | MB_ICONQUESTION // Options: Yes, No, Cancel
    );

    if (result == IDYES) { 
        OPENFILENAMEW ofn; // Structure for file dialog
        wchar_t szFile[MAX_PATH] = L""; // Buffer for file path

        ZeroMemory(&ofn, sizeof(ofn)); // Initialize the file structure
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
        ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0"; // File type filter
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // Dialog flags
        ofn.lpstrDefExt = L"txt"; // Default extension

        if (currentFilePath.empty()) { 
            if (GetSaveFileNameW(&ofn)) { // Open "Save As" dialog
                return SaveTextToFile(hwnd, ofn.lpstrFile) ? IDYES : IDCANCEL;
            } else {
                return IDCANCEL;
            }
        } else {
            StringCchCopyW(ofn.lpstrFile, MAX_PATH, currentFilePath.c_str());
            if (GetSaveFileNameW(&ofn)) { 
                return SaveTextToFile(hwnd, ofn.lpstrFile) ? IDYES : IDCANCEL;
            } else {
                return IDCANCEL; 
            }
        }
    }
    return result; // Return IDNO or IDCANCEL as chosen by the user
}
void NewDocument(HWND hwnd) {
    int result = PromptForSave(hwnd); 
    if (result == IDCANCEL) { 
        return;
    }

    textBuffer.clear(); 
    textBuffer.push_back(L""); 
    currentFilePath.clear();
    documentModified = false; 

    caretLine = 5;   
    caretCol = 5;
    scrollOffsetY = 0;
    scrollOffsetX = 0;

    trackCaret = true;
    calcTextMetrics(hwnd);
    UpdateScrollBars(hwnd);
    UpdateCaretPosition(hwnd);
    CreateCaret(hwnd, NULL, 2, charHeight);
    ShowCaret(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);

    SetWindowTextW(hwnd, L"Text Editor");
}
void OpenFile(HWND hwnd) {
    int result = PromptForSave(hwnd); 
    if (result == IDCANCEL) { 
        return;
    }

    OPENFILENAMEW ofn; 
    wchar_t szFile[MAX_PATH] = L""; // Buffer for file path

    ZeroMemory(&ofn, sizeof(ofn)); // Initialize the structure
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // Flags: path must exist, file must exist

    if (GetOpenFileNameW(&ofn)) { 
        LoadTextFromFile(hwnd, ofn.lpstrFile); 
        ShowCaret(hwnd);
    }
}
void SaveFile(HWND hwnd) {
    if (currentFilePath.empty()) { 
        SaveFileAs(hwnd); 
    } else { 
        SaveTextToFile(hwnd, currentFilePath); 
    }
}
void SaveFileAs(HWND hwnd) {
    OPENFILENAMEW ofn; // Structure for save file dialog
    wchar_t szFile[MAX_PATH] = L""; // Buffer for file path

    if (!currentFilePath.empty()) { 
        const wchar_t* fileName = wcsrchr(currentFilePath.c_str(), L'\\');
        if (fileName) {
            StringCchCopyW(szFile, MAX_PATH, fileName + 1); // Copy filename (after last backslash)
        } else {
            StringCchCopyW(szFile, MAX_PATH, currentFilePath.c_str()); // If no backslash, use full path
        }
    }

    ZeroMemory(&ofn, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // Flags: path must exist, prompt if overwriting
    ofn.lpstrDefExt = L"txt"; // Default extension

    if (GetSaveFileNameW(&ofn)) { 
        SaveTextToFile(hwnd, ofn.lpstrFile); 
    }
}