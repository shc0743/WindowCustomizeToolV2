#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "MainWindow.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class SettingsDialog : public Window {
public:
	SettingsDialog() : Window(L"设置 - Window Customize Tool V2",
		600, 400, 0, 0,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) {}

protected:
	void onCreated() override;

public:
	virtual const HICON get_window_icon() const override {
		return MainWindow().get_window_icon();
	}
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0xF0, 0xF0, 0xF0);
	}
protected:
	virtual void setup_event_handlers() override {

	}
};



ns_end;
