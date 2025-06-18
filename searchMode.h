#include <windows.h>
#include <string>
#include <vector>
extern bool isSearchMode;
extern std::wstring searchQuery;
extern std::wstring searchBoxText;
extern std::vector<std::pair<int, int>> searchMatches;
extern size_t currentMatchIndex;
extern int searchBoxHeight;
extern int searchCaretPos;

void ActivateSearchMode(HWND hwnd);
void DeactivateSearchMode(HWND hwnd);
void DrawSearchBox(HWND hwnd, HDC hdc);
void HandleSearchInput(HWND hwnd, WPARAM wParam);
void FindAllMatches(HWND hwnd);
void JumpToMatch(HWND hwnd, size_t index);
void FindNext(HWND hwnd);
void FindPrevious(HWND hwnd);
void DrawSearchMatches(HDC hdc, const RECT& paintRect);