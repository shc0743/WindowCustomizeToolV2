#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "resource.h"


extern HINSTANCE hInst;



class MainWindow : public Window {
public:
	MainWindow() : Window(L"Window Customize Tool V2", 640, 480, 0, 0, WS_OVERLAPPEDWINDOW, WS_EX_LAYERED) {}

protected:
	void onCreated() override;

protected:
	// 覆盖 get_window_icon 方法以返回自定义图标
	virtual const HICON get_window_icon() const {
		return LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SELECTOR));
	}

	virtual void setup_event_handlers() override {

	}
};



