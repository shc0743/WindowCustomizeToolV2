#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class OverlayWindow : public Window {
public:
	OverlayWindow() : Window(L"Overlay Window", 1, 1, 0, 0, WS_POPUP | WS_SYSMENU) {}

	enum Types {
		TYPE_PASSIVE = 0,
		TYPE_PROACTIVE = 1,
	};

protected:
	void onCreated() override;
	void initMenu();
	void initSize();
	void onTimer(EventData& ev);
	void onSysMenu(EventData& ev);
	void doLayout(EventData& ev);

	bool closable = true;
	HWND target = NULL;
	Types type = TYPE_PASSIVE;
	bool isUserSize = true;

public:
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0, 0, 0);
	}
protected:
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_TIMER, onTimer);
		WINDOW_add_handler(WM_SYSCOMMAND, onSysMenu);
		auto dl = [this](EventData& data) { if (data.hwnd != this->hwnd) return; doLayout(data); };
		addEventListener(WM_MOVING, dl);
		addEventListener(WM_MOVE, dl);
		addEventListener(WM_SIZING, dl);
		addEventListener(WM_SIZE, dl);
		WINDOW_add_handler(WM_KEYUP, [this](EventData& ev) {
			if (!closable) return;
			if (ev.wParam == VK_ESCAPE || ev.wParam == VK_RETURN) close();
		});
		WINDOW_add_handler(WM_CLOSE, [this](EventData& ev) {
			if (!closable) ev.preventDefault();
		});
		WINDOW_add_handler(WM_NCHITTEST, [this](EventData& ev) {
			if (type == TYPE_PROACTIVE) {
				auto ret = DefWindowProc(hwnd, (UINT)ev.message, ev.wParam, ev.lParam);
				if (ret == HTCLIENT) ret = HTCAPTION;
				ev.returnValue(ret);
			}
		});
	}

public:
	virtual void closeAfter(DWORD time) {
		SetTimer(hwnd, 2, time, NULL);
	}
	inline void setClosable(bool c) { closable = c; initMenu(); }
	inline void setTarget(HWND h) { target = h; initSize(); post(WM_SIZE); }
	void setType(Types t) {
		type = t; 
		if (t == TYPE_PROACTIVE) add_style(WS_SIZEBOX);
		if (t == TYPE_PASSIVE) remove_style(WS_SIZEBOX);
		initSize();
		post(WM_SIZE);
	}
};



ns_end;
