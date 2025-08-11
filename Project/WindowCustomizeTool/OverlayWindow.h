#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class OverlayWindow : public Window {
public:
	OverlayWindow() : Window(L"Overlay Window", 1, 1, 0, 0, WS_POPUP | WS_SYSMENU) {}

protected:
	void onCreated() override;
	void onTimer(EventData& ev);
	void onSysMenu(EventData& ev);

	bool closable = true;

public:
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0, 0, 0);
	}
protected:
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_TIMER, onTimer);
		WINDOW_add_handler(WM_SYSCOMMAND, onSysMenu);
		WINDOW_add_handler(WM_CLOSE, [this](EventData& ev) {
			if (!closable) ev.preventDefault();
		});
	}

public:
	virtual void closeAfter(DWORD time) {
		SetTimer(hwnd, 2, time, NULL);
	}
	inline void setClosable(bool c) { closable = c; }
};



ns_end;
