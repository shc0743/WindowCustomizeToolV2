#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "WindowLocator.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class WindowChangeParent : public Window {
public:
	WindowChangeParent() : Window(L"Window Change Parent",
		400, 240, 0, 0, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU) {
	}

	inline void setTarget(HWND h) {
		if (!hwnd) throw std::runtime_error("please create() first");
		target = h;
		tip.text(std::format(L"正在为 0x{:X} 设置新的父窗口", (ULONG_PTR)target));
		HWND curParent = GetParent(target);
		WCHAR title[256]{};
		if (curParent) GetWindowTextW(curParent, title, 256);
		oldParentText.text(std::format(L"当前父窗口: 0x{:X} {}", (ULONG_PTR)curParent, title));
	}

protected:
	void onCreated() override;
	void update_target(HWND h) {
		tp = h;
		WCHAR title[256]{};
		if (h) GetWindowTextW(h, title, 256);
		newParentText.text(std::format(L"新的父窗口: 0x{:X} {}", (ULONG_PTR)h, title));
	}

	HWND target = NULL, tp = NULL;
	Static tip, newParentText, oldParentText;
	WCTv2::winlib::WindowLocatorControl finder;
	Button apply, cancel;

public:
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0xF0, 0xF0, 0xF0);
	}
protected:
	virtual void setup_event_handlers() override {

	}
};



ns_end;
