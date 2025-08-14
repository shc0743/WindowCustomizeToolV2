#include "winlib.hpp"
#include <memory>

HWND GetWindowUnderPoint(POINT pt, HWND exclude) {
	HWND window = WindowFromPoint(pt);
	if (!exclude || window != exclude) return window;

	// 如果需要排除指定窗口，尝试查找下一个窗口
	HWND nextHwnd = GetAncestor(window, GA_ROOT);
	do {
		nextHwnd = GetWindow(nextHwnd, GW_HWNDNEXT);
		if (!nextHwnd) {
			return NULL;
		}

		// 检查窗口是否包含该点
		RECT rect;
		GetWindowRect(nextHwnd, &rect);
		if (pt.x >= rect.left && pt.x <= rect.right &&
			pt.y >= rect.top && pt.y <= rect.bottom) {
			if (window != exclude) {
				return window;
			}
		}
	} while (true);
	return NULL;
}

// 增加记录颜色和宽度的全局变量
static RECT g_lastRect = { 0 };
static HPEN g_lastPen = nullptr;
static COLORREF g_lastColor = CLR_INVALID;
static UINT g_lastWidth = 0;

void HighlightWindow(HWND target, COLORREF color, UINT width) {
	// 检查是否有必要更新
	RECT rcCurrent = { 0 };
	if (target) GetWindowRect(target, &rcCurrent);

	// 检查状态是否变化：目标区域、颜色或线宽
	const bool sameRect = target && !IsRectEmpty(&g_lastRect) && EqualRect(&rcCurrent, &g_lastRect);
	const bool sameStyle = (color == g_lastColor) && (width == g_lastWidth);

	// 如果状态未变化且目标有效，直接返回
	if (sameRect && sameStyle && target) {
		return;
	}

	HDC hDc = GetDC(NULL);

	// 清除旧边框（如果存在）
	if (!IsRectEmpty(&g_lastRect)) {
		HPEN hOldPen = (HPEN)SelectObject(hDc, g_lastPen);
		SetROP2(hDc, R2_NOTXORPEN);
		Rectangle(hDc, g_lastRect.left, g_lastRect.top, g_lastRect.right, g_lastRect.bottom);
		SelectObject(hDc, hOldPen);

		DeleteObject(g_lastPen);
		g_lastPen = nullptr;
		ZeroMemory(&g_lastRect, sizeof(RECT));
		g_lastColor = CLR_INVALID;
		g_lastWidth = 0;
	}
	else if (!target) {
		// 重置全局变量
		g_lastPen = nullptr;
		ZeroMemory(&g_lastRect, sizeof(RECT));
		g_lastColor = CLR_INVALID;
		g_lastWidth = 0;
	}

	// 绘制新边框（如果目标有效）
	if (target && !IsRectEmpty(&rcCurrent)) {
		g_lastPen = CreatePen(PS_SOLID, width, color);
		HPEN hOldPen = (HPEN)SelectObject(hDc, g_lastPen);
		SetROP2(hDc, R2_NOTXORPEN);
		Rectangle(hDc, rcCurrent.left, rcCurrent.top, rcCurrent.right, rcCurrent.bottom);
		SelectObject(hDc, hOldPen);

		g_lastRect = rcCurrent;
		g_lastColor = color;
		g_lastWidth = width;
	}

	ReleaseDC(NULL, hDc);
}

DWORD exec_app(std::wstring command, int show) {
	STARTUPINFOW si{}; PROCESS_INFORMATION pi{};

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = show;

	auto cl = std::make_unique<WCHAR[]>(command.size() + 1);
	memcpy(cl.get(), command.c_str(), (command.size() + 1) * sizeof(WCHAR));

	if (!CreateProcessW(NULL, cl.get(), NULL, NULL, FALSE,
		CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return -1;
	}

	DWORD code = 0;
	ResumeThread(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return code;
}


