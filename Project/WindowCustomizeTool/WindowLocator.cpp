#include "WindowLocator.h"
#include "winlib.hpp"
ns_declare(WCTv2::winlib);



void WindowLocator::start() {
	m_target = NULL;
	callback = nullptr;
}


void WindowLocator::during() {
	// 找到窗口
	POINT pt{};
	GetCursorPos(&pt);
	m_target = GetWindowUnderPoint(pt);
	if (callback) callback(m_target);
	if (!m_target) {
		RestoreScreenContent();
		return;
	}

	HighlightWindow(m_target, highlight_color, highlight_width);
}


void WindowLocator::end() {
	// 恢复屏幕
	RestoreScreenContent();
	// 找到目标窗口
	POINT pt{};
	GetCursorPos(&pt);
	m_target = GetWindowUnderPoint(pt);
}


HWND WindowLocator::target() const {
	return m_target;
}


ns_end;
