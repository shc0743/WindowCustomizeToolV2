#pragma once
#include <w32use.hpp>
#include <format>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"
#include "OverlayWindow.h"
#include "WindowLocator.h"



class MainWindow : public Window {
public:
	MainWindow() : Window(L"Window Customize Tool V2", 600, 400, 0, 0, WS_OVERLAPPEDWINDOW, WS_EX_LAYERED) {}
	~MainWindow() {
		if (overlay) delete overlay;
	}

	const std::wstring get_class_name() const override {
		return w32oop::util::str::converts::str_wstr(std::format(
			"app::WCT_v2:Window/RTTI={},HashCode={}/#MainWindow", 
			typeid(*this).name(), typeid(*this).hash_code()));
	}

protected:
	void onCreated() override;
	void onDestroy() override;

protected:
	// 控件
	void init_controls();
	void init_config();
	void updateMenuStatus();
	void doLayout(EventData& ev);
	void paint(EventData& ev);
	Static text_targetHwnd, text_clsName;
	Edit edit_targetHwnd, edit_clsName;
	Static finder;
	Static text_winTitle; Edit edit_winTitle; Button btn_applyTitle;
	void startFind(EventData& ev);
	void duringFind(EventData& ev);
	void endFind(EventData& ev);
	void update_target();
	// 查找器
	WCTv2::winlib::WindowLocator locator;
	
protected:
	// 成员变量
	HCURSOR hCurFinding = 0;
	HICON hFinderEmpty = 0, hFinderFilled = 0;
	HWND target = NULL;
	bool isFinding = false;
	WindowCustomizeToolV2_app::OverlayWindow* overlay = nullptr;

	// 事件处理程序
	void onClose(EventData& ev);
	void onMenu(EventData& ev);
	void onSysMenu(EventData& ev);
	void onMinimize(EventData& ev);

protected:
	// 配置
	bool isTopMost = false;
	bool hideWhenMinimized = false;
	bool putBottomWhenUse = false;
	bool useHex = false;

	void toggleTopMostState();

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
	virtual const COLORREF get_window_background_color() const {
		return RGB(0xFA, 0xFA, 0xFA);
	}
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_PAINT, paint);
		WINDOW_add_handler(WM_SIZING, doLayout);
		WINDOW_add_handler(WM_SIZE, doLayout);
		WINDOW_add_handler(WM_LBUTTONDOWN, startFind);
		WINDOW_add_handler(WM_RBUTTONDOWN, startFind);
		WINDOW_add_handler(WM_LBUTTONUP, endFind);
		WINDOW_add_handler(WM_RBUTTONUP, endFind);
		WINDOW_add_handler(WM_MOUSEMOVE, duringFind);
		WINDOW_add_handler(WM_MENU_CHECKED, onMenu);
		WINDOW_add_handler(WM_SYSCOMMAND, onSysMenu);
		WINDOW_add_handler(WM_CLOSE, onClose);
		WINDOW_add_handler(WM_SIZE, onMinimize);
		WINDOW_add_handler(WM_APP + WM_CLOSE, [this](EventData&) { remove_style_ex(WS_EX_LAYERED); destroy(); });
	}
public:
	void hideMainWindow();
};



