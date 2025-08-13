#pragma once
#include <w32use.hpp>
#include <format>
#include <commctrl.h>
#include <dwmapi.h>
#include "publicdef.h"
#include "resource.h"
#include "OverlayWindow.h"
#include "WindowLocator.h"



class MainWindow : public Window {
public:
	MainWindow() : Window(L"Window Customize Tool V2", 640, 480, 0, 0, WS_OVERLAPPEDWINDOW) {}
	~MainWindow() {
		
	}

	const std::wstring get_class_name() const override {
		return w32oop::util::str::converts::str_wstr(std::format(
			"app::WCT_v2:Window/RTTI={},HashCode={}/#MainWindow", 
			typeid(*this).name(), typeid(*this).hash_code()));
	}

	static std::wstring current_time();
	std::wstring hwnd_strify(HWND hWnd) const;

protected:
	void onCreated() override;
	void onDestroy() override;

protected:
	// 控件
	void init_controls();
	void init_hotkey();
	void init_config();
	bool init_success = false;
	void updateMenuStatus();
	void doLayout(EventData& ev);
	void paint(EventData& ev);
	StatusBar sbr;
	Static text_targetHwnd, text_clsName;
	Edit edit_targetHwnd, edit_clsName;
	Static text_winTitle; Edit edit_winTitle; Button btn_applyTitle;
	Static text_parentWin; Edit edit_parentWin; Button btn_selectParent;
	Button group_winOperations;
	CheckBox cb_enableWin, cb_showWin;
	Button btn_b2f, btn_op_shownormal, btn_op_min, btn_op_max;
	Button btn_highlight, btn_showpos; Static text_winpos;
	Button btn_swp, btn_resize;
	CheckBox cb_topmost; Button btn_zorder, btn_border, btn_corner, btn_winstyle, btn_adjust;
	Button btn_close, btn_destroy, btn_endtask, btn_properties;
	void update_target();
	void context_anyorder_internal(int type, Window& btn_element);
	void wop_report_result(bool ok = (GetLastError() == 0), DWORD code = GetLastError());
	// 查找器
	WCTv2::winlib::WindowLocatorControl finder;
	
protected:
	// 成员变量
	std::wstring success_text;
	HWND target = NULL, target_old = NULL;
	bool isFinding = false;

	// 事件处理程序
	void onTimer(EventData& ev);
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
	int findMode = 0;
	bool autoRefresh = false;
	DWM_WINDOW_CORNER_PREFERENCE corner_config = DWMWCP_DEFAULT;
	BYTE last_alpha = 255;

	void toggleTopMostState();

	static void createAlphaEditor(HWND hWnd);

	static LRESULT CALLBACK SwpDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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
		WINDOW_add_handler(WM_MENU_CHECKED, onMenu);
		WINDOW_add_handler(WM_SYSCOMMAND, onSysMenu);
		WINDOW_add_handler(WM_TIMER, onTimer);
		WINDOW_add_handler(WM_CLOSE, onClose);
		WINDOW_add_handler(WM_SIZE, onMinimize);
		WINDOW_add_handler(WM_APP + WM_CLOSE, [this](EventData&) {
			COLORREF cr{}; BYTE alpha{}; DWORD flags{};
			GetLayeredWindowAttributes(hwnd, &cr, &alpha, &flags);
			last_alpha = alpha;
			remove_style_ex(WS_EX_LAYERED); // 要使得窗口有正常的关闭动画，窗口不能是Layered窗口
			destroy();
		});
	}
	void postMenuEvent(WPARAM menu_id) {
		dispatchEvent(EventData(hwnd, WM_MENU_CHECKED, menu_id, 0));
	}
public:
	void hideMainWindow();
};



