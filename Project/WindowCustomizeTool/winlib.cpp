#include "winlib.hpp"

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

#if 0
void HighlightWindow(HWND target, COLORREF color, UINT width) {
	HDC hDc = GetDC(NULL);
	RECT rc{}; GetWindowRect(target, &rc);
	// 绘制一个矩形来表示目标窗口
	// 保存原有画笔和画刷
	HPEN hOldPen = (HPEN)SelectObject(hDc, GetStockObject(DC_PEN));
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, GetStockObject(NULL_BRUSH));

	// 设置画笔颜色和宽度
	SetDCPenColor(hDc, color); // 红色
	// 若要更粗的边框，需创建自定义画笔（例如宽度为2）
	HPEN hPen = CreatePen(PS_SOLID, width, color);
	SelectObject(hDc, hPen);

	// 绘制矩形边框
	Rectangle(hDc, rc.left, rc.top, rc.right, rc.bottom);

	// 恢复原有设置并释放资源
	SelectObject(hDc, hOldPen);
	SelectObject(hDc, hOldBrush);
	DeleteObject(hPen); // 如果是自定义画笔

	ReleaseDC(NULL, hDc);
}
#elif 0
// 全局变量记录上一次的矩形和画笔
static RECT g_lastRect = { 0 };
static HPEN g_lastPen = nullptr;

void HighlightWindow(HWND target, COLORREF color, UINT width) {
	HDC hDc = GetDC(NULL);
	RECT rc{};
	GetWindowRect(target, &rc);

	// 1. 擦除旧边框（如果存在）
	if (!IsRectEmpty(&g_lastRect)) {
		// 使用相同的画笔和 XOR 模式再次绘制旧矩形（自动擦除）
		HPEN hOldPen = (HPEN)SelectObject(hDc, g_lastPen);
		SetROP2(hDc, R2_NOTXORPEN); // XOR 模式
		Rectangle(hDc, g_lastRect.left, g_lastRect.top, g_lastRect.right, g_lastRect.bottom);
		SelectObject(hDc, hOldPen);

		// 清理旧画笔
		if (g_lastPen) {
			DeleteObject(g_lastPen);
			g_lastPen = nullptr;
		}
		ZeroMemory(&g_lastRect, sizeof(RECT));
	}

	// 2. 绘制新边框（如果目标有效）
	if (target != NULL) {
		// 创建新画笔
		g_lastPen = CreatePen(PS_SOLID, width, color);
		HPEN hOldPen = (HPEN)SelectObject(hDc, g_lastPen);
		SetROP2(hDc, R2_NOTXORPEN); // XOR 模式
		Rectangle(hDc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hDc, hOldPen);

		// 保存当前矩形
		g_lastRect = rc;
	}

	ReleaseDC(NULL, hDc);
}
#elif 1
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
#endif
