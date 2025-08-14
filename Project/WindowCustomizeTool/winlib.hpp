#pragma once
#include <Windows.h>
#include <dwmapi.h>
#include <string>

// a bad implement.
HWND GetWindowUnderPoint(POINT pt, HWND exclude = NULL);

// 高亮窗口
void HighlightWindow(HWND target, COLORREF color, UINT width);


inline void RestoreScreenContent() {
	HighlightWindow(NULL, 0, 0); // 传入 NULL 擦除旧边框
}


// 获取窗口是否为置顶
inline bool IsWindowTopMost(HWND hWnd) {
	return (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}


DWORD exec_app(std::wstring command, int show = SW_SHOW);
