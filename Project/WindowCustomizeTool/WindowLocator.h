#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WCTv2::winlib);


class WindowLocator {
public:
	WindowLocator() {};
	virtual ~WindowLocator() = default;

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
	std::function<void(HWND)> callback;
public:
	inline void setCallback(std::function<void(HWND)> callback) {
		this->callback = callback;
	}
};



ns_end;
