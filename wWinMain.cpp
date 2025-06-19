// main.cpp
#ifndef UNICODE
#define UNICODE
#endif

#define NOMINMAX 

#include "WindowProc.h" 
#include "textEditorGlobals.h"         // For textBuffer and caret variables
#include "textMetrics.h"            // For calcTextMetrics and font variables
#include "updateCaretAndScroll.h"   // For UpdateCaretPosition and UpdateScrollBars

#include <windows.h>

/*
Terminal commands
cd ..
cd ..
cd projects/textEditor
windres textEditor.rc -O coff -o textEditor.res
g++ wWinMain.cpp WindowProc.cpp textEditorGlobals.cpp textMetrics.cpp updateCaretAndScroll.cpp fileOperations.cpp undoStack.cpp characterCase.cpp isModified.cpp cursorControls.cpp searchMode.cpp infoBar.cpp textEditor.res -o textEditor.exe -mwindows -municode -lcomdlg32
textEditor.exe
*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
    //register window class
    const wchar_t CLASS_NAME[] = L"Text Editor";

    WNDCLASS wc={};
    //ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WindowProc;    //This is how it communicates with WindowProc
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              //class style
        CLASS_NAME,                     //window class
        L"Text Editor",    //Window text
        WS_OVERLAPPEDWINDOW| WS_VSCROLL | WS_HSCROLL,            //Window style

        //Size and Postiton
        CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, 

        NULL,       //Parent Window
        LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU)),       //Menu
        hInstance,  //Instance handle
        NULL        //Aditional app data
    );

    if (hwnd == NULL){
        return 0;
    }
    SetFocus(hwnd);
    ShowWindow(hwnd, nCmdShow);
    
    //Run MSG loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while(GetMessage(&msg, NULL, 0, 0)>0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
        
    return 0;
    
}