#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class WindowAlphaEditor : public Window {
public:
	WindowAlphaEditor() : Window(L"Window Alpha Editor",
		320, 200, 0, 0, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU) {
	}

	inline void setTarget(HWND h) {
		target = h;
		if ((GetWindowLongPtrW(target, GWL_EXSTYLE) & WS_EX_LAYERED)) {
			COLORREF cr{}; BYTE alpha{}; DWORD flags{};
			GetLayeredWindowAttributes(target, &cr, &alpha, &flags);
			if (flags & LWA_ALPHA) tb.pos(alpha);
			text.text(std::to_wstring(alpha));
		}
	}

protected:
	void onCreated() override;
	void onDestroy() override;

	HWND target = NULL;
	HFONT bigFont = NULL;
	Static text;
	TrackBar tb;

public:
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0xF0, 0xF0, 0xF0);
	}
protected:
	virtual void setup_event_handlers() override {

	}
};



ns_end;
