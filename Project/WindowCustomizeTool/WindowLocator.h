#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WCTv2::winlib);
using std::function;


class WindowLocatorCore {
public:
	WindowLocatorCore() {};
	virtual ~WindowLocatorCore() = default;

	int mode = 0;
protected:
	COLORREF highlight_color = RGB(0, 0, 0);
	UINT highlight_width = 5;
	HWND m_target = NULL;

public:
	COLORREF get_highlight_color() const { return highlight_color; }
	UINT get_highlight_width() const { return highlight_width; }
	void set_highlight_color(COLORREF color) { highlight_color = color; }
	void set_highlight_width(UINT width) { highlight_width = width; }

public:
	virtual void start();
	virtual void during();
	virtual void end();
	virtual HWND target() const;

private:
	void getwin();

protected:
	function<void(HWND)> callback;
public:
	inline void setCallback(function<void(HWND)> callback) {
		this->callback = callback;
	}
};

class WindowLocatorControl : public Window {
public:
	WindowLocatorControl() : Window(L"", 32, 32, 0, 0, WS_CHILD | WS_VISIBLE), m_parent(NULL) {}
	WindowLocatorControl(HWND parent) : Window(L"", 32, 32, 0, 0, WS_CHILD | WS_VISIBLE), m_parent(parent) {}

protected:
	HWND m_parent;
	HWND target = NULL;
	Static finder; bool isFinding = false;
	WindowLocatorCore locator;
	HCURSOR hCurFinding = 0;
	HICON hFinderEmpty = 0, hFinderFilled = 0;
	void onCreated() override;
	void onDestroy() override;

	void startFind(EventData& ev);
	void duringFind(EventData& ev);
	void endFind(EventData& ev);

	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_LBUTTONDOWN, startFind);
		WINDOW_add_handler(WM_RBUTTONDOWN, startFind);
		WINDOW_add_handler(WM_LBUTTONUP, endFind);
		WINDOW_add_handler(WM_RBUTTONUP, endFind);
		WINDOW_add_handler(WM_MOUSEMOVE, duringFind);
	}

	HWND new_window() override {
		auto cls = get_class_name();
		return CreateWindowExW(
			setup_info->styleEx,
			cls.c_str(),
			setup_info->title.c_str(),
			setup_info->style,
			setup_info->x, setup_info->y,
			setup_info->width, setup_info->height,
			m_parent, 0, GetModuleHandle(NULL), this
		);
	}

	std::function<void()> selectstart;
	std::function<void(HWND)> selecting, selected;
public:
	int findMode = 0;
	void set_parent(HWND parent) {
		m_parent = parent;
		if (!hwnd) return;
		SetParent(hwnd, parent);
		override_style(WS_CHILD);
	}
	inline void onselectstart(std::function<void()> callback) {
		this->selectstart = callback;
	}
	inline void onselecting(std::function<void(HWND)> callback) {
		this->selecting = callback;
	}
	inline void onselectend(std::function<void(HWND)> callback) {
		this->selected = callback;
	}
	WindowLocatorCore& get_locator() { return locator; }
	const WindowLocatorCore& get_locator() const { return locator; }
};

ns_end;
