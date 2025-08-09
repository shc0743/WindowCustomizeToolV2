#pragma once
#include <Windows.h>
#include <dwmapi.h>

// a bad implement.
HWND GetWindowUnderPoint(POINT pt, HWND exclude = NULL);

// 高亮窗口
void HighlightWindow(HWND target, COLORREF color, UINT width);


inline void RestoreScreenContent() {
	HighlightWindow(NULL, 0, 0); // 传入 NULL 擦除旧边框
}

