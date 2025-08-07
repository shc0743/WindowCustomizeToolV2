#pragma once
#include <w32use.hpp>
#include <format>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"



class MainWindow : public Window {
public:
	MainWindow() : Window(L"Window Customize Tool V2", 600, 400, 0, 0, WS_OVERLAPPEDWINDOW, WS_EX_LAYERED) {}

	const std::wstring get_class_name() const override {
		return w32oop::util::str::converts::str_wstr(std::format(
			"app::WCT_v2:Window/RTTI={},HashCode={}/#MainWindow", 
			typeid(*this).name(), typeid(*this).hash_code()));
	}

protected:
	void onCreated() override;
	void onDestroy() override;

	// 成员变量

	// 事件处理程序
	void onClose(EventData& ev);

protected:
	static HICON app_icon;
public:
	// 覆盖 get_window_icon 方法以返回自定义图标
	virtual const HICON get_window_icon() const {
		if (!app_icon) {
			app_icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SELECTOR));
		}
		return app_icon;
	}
protected:
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_CLOSE, onClose);
		WINDOW_add_handler(WM_APP + WM_CLOSE, [this](EventData&) { remove_style_ex(WS_EX_LAYERED); destroy(); });
	}
public:
	void hideMainWindow();
};



