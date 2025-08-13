#include "WindowLocator.h"
#include "winlib.hpp"
#include "resource.h"
ns_declare(WCTv2::winlib);



void WindowLocatorCore::start() {
	m_target = NULL;
	callback = nullptr;
}


void WindowLocatorCore::during() {
	// 找到窗口
	getwin();
	if (callback) callback(m_target);
	if (!m_target) {
		RestoreScreenContent();
		return;
	}

	HighlightWindow(m_target, highlight_color, highlight_width);
}


void WindowLocatorCore::end() {
	// 恢复屏幕
	RestoreScreenContent();
	// 找到目标窗口
	getwin();
}


HWND WindowLocatorCore::target() const {
	return m_target;
}

void WindowLocatorCore::getwin() {
	POINT pt{};
	GetCursorPos(&pt);
	m_target = GetWindowUnderPoint(pt);
	if (mode != 0 && m_target) {
		if (mode == GA_ROOT) m_target = GetAncestor(m_target, GA_ROOT);
		if (mode == GA_ROOTOWNER) m_target = GetAncestor(m_target, GA_ROOTOWNER);
	}
}


void WindowLocatorControl::onCreated() {
	finder = Static(hwnd, L"", 32, 32, 0, 0);
	finder.create();

	hFinderEmpty = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_ICON_SELECTOR));
	hFinderFilled = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_ICON_SELECTOR_FILLED));
	hCurFinding = LoadCursorW(hInst, MAKEINTRESOURCEW(IDC_CURSOR_WINSELECT));
	if (!hFinderEmpty || !hFinderFilled || !hCurFinding)
		throw std::runtime_error("Failed to load resource");
	finder.add_style(SS_ICON | SS_CENTERIMAGE);
	finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderFilled);
}

void WindowLocatorControl::onDestroy() {
	DeleteObject(hFinderEmpty);
	DeleteObject(hFinderFilled);
	DeleteObject(hCurFinding);
}

void WindowLocatorControl::startFind(EventData& ev) {
	//POINT pt; GetCursorPos(&pt);
	//RECT rect; GetWindowRect(finder, &rect);
	//// 判断鼠标是否在 finder 范围内
	//if (!(
	//	pt.x > rect.left &&
	//	pt.x < rect.right &&
	//	pt.y> rect.top &&
	//	pt.y < rect.bottom
	//	)) return;
	ev.preventDefault();

	if (selectstart) selectstart();

	// 开始查找
	isFinding = true;
	// 设置鼠标
	SetCapture(hwnd);
	SetCursor(hCurFinding);
	// 改变图标
	hFinderFilled = (HICON)finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderEmpty);

	locator.start();
	locator.mode = findMode;
}
void WindowLocatorControl::duringFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();

	locator.during();
	if (selecting) selecting(locator.target());
}
void WindowLocatorControl::endFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();
	locator.end();

	// 恢复状态
	isFinding = false;
	ReleaseCapture(); //释放鼠标捕获
	RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
	UpdateWindow(GetDesktopWindow());
	// 恢复图标
	hFinderEmpty = (HICON)finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderFilled);
	// 获取窗口
	if (selected) selected(locator.target());
}




ns_end;