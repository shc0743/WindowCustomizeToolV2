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
		640, 480, 0, 0,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) {}

protected:
	void onCreated() override;

public:
	virtual const HICON get_window_icon() const {
		return MainWindow().get_window_icon();
	}
protected:
	virtual void setup_event_handlers() override {

	}
};



ns_end;
