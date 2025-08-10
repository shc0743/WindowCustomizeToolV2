#include "WindowLocator.h"
#include "winlib.hpp"
ns_declare(WCTv2::winlib);



void WindowLocator::start() {
	m_target = NULL;
	callback = nullptr;
}


void WindowLocator::during() {
	// 找到窗口
	getwin();
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
	getwin();
}


HWND WindowLocator::target() const {
	return m_target;
}

void WindowLocator::getwin() {
	POINT pt{};
	GetCursorPos(&pt);
	m_target = GetWindowUnderPoint(pt);
	if (mode != 0 && m_target) {
		if (mode == GA_ROOT) m_target = GetAncestor(m_target, GA_ROOT);
		if (mode == GA_ROOTOWNER) m_target = GetAncestor(m_target, GA_ROOTOWNER);
	}
}


ns_end;
