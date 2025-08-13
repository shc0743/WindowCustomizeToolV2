#include "../lib/DestroyRemoteWindow.h"
#include "MainWindow.h"
#include "IPCWindow.h"
#include "resource.h"
#include "winlib.hpp"
#include "WindowAlphaEditor.h"
#include "WindowChangeParent.h"
#include <chrono>
using namespace std;
HICON MainWindow::app_icon;

using namespace WindowCustomizeToolV2_app;
using namespace WCTv2::winlib;


#pragma comment(lib, "winmm.lib")


wstring MainWindow::current_time() {
	// 获取当前系统时间点
	auto now = std::chrono::system_clock::now();

	// 转换为time_t类型（秒级精度）
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	// 转换为本地时间结构
	std::tm tm;
	localtime_s(&tm, &t);  // Windows安全的本地时间转换

	// 使用std::format进行格式化（C++20特性）
	return std::format(L"{:02}:{:02}:{:02}",
		tm.tm_hour,   // 小时 (0-23)
		tm.tm_min,    // 分钟 (0-59)
		tm.tm_sec);   // 秒 (0-59)
}

std::wstring MainWindow::hwnd_strify(HWND hWnd) const {
	if (useHex) {
		return (std::format(L"0x{:X}", reinterpret_cast<ULONGLONG>(hWnd)));
	}
	else {
		return (std::to_wstring(reinterpret_cast<ULONGLONG>(hWnd)));
	}
}


void MainWindow::onCreated() {
	add_style_ex(WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)app::config.get_or("app.main_window.visual.alpha", 0xee), LWA_ALPHA);
	sbr.set_parent(this);
	sbr.create(L"", 1, 1);

	init_controls();
	init_hotkey();
	init_config();
	if (isTopMost) set_topmost(true);
	SetMenu(hwnd, LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_MENU_MAINWND)));
	updateMenuStatus();

	HMENU sys = sysmenu();
	AppendMenuW(sys, MF_SEPARATOR, 0, NULL);
	AppendMenuW(sys, MF_STRING, 1001, L"关闭此窗口 (&L)");

	success_text = ErrorChecker(0).raw_message();

	// 只在第一个主窗口恢复用户的窗口位置
	bool shouldRestorePos = true;
	try {
		app::firstAliveMainWindow();
		shouldRestorePos = false;
	}
	catch (...) {}
	try {
		int x = app::config.get_or("app.main_window.userwindow.x", -1);
		int y = app::config.get_or("app.main_window.userwindow.y", -1);
		int w = app::config.get_or("app.main_window.userwindow.width", -1);
		int h = app::config.get_or("app.main_window.userwindow.height", -1);
		if (x != -1 && y != -1 && w != -1 && h != -1) {
			if (shouldRestorePos) resize(x, y, w, h);
			else {
				resize(w, h);
				center();
			}
		}
		else { throw 1; }
	}
	catch (...) { center(); }
	sbr.simple(false);

	SetTimer(hwnd, 3, 1000, 0);
	SetTimer(hwnd, 4, (UINT)app::config.get_or("app.main_window.target.auto_refresh.time", 2000), 0);
	post(WM_TIMER, 3, 0);

	update_target();
	sbr.set_text(0, L"Ready");
	init_success = true;
}

void MainWindow::onDestroy() {
	// 资源清理
	KillTimer(hwnd, 3);
	KillTimer(hwnd, 4);

	if (!init_success) return;

	// 保存窗口位置
	RECT rc{}; GetWindowRect(hwnd, &rc);
	app::config["app.main_window.userwindow.x"] = rc.left;
	app::config["app.main_window.userwindow.y"] = rc.top;
	app::config["app.main_window.userwindow.width"] = rc.right - rc.left;
	app::config["app.main_window.userwindow.height"] = rc.bottom - rc.top;
	// 保存透明度
	app::config["app.main_window.visual.alpha"] = last_alpha;

	if (app::no_main_window) return;
	// 如果所有 MainWindow 都关闭了，则退出应用程序
	for (auto& w : app::win) {
		if (w && w != this && IsWindow(*(w))) return;
	}
	// 只剩下我们自己了
	PostQuitMessage(0);
}

void MainWindow::onTimer(EventData& ev) {
	switch (ev.wParam) {
	case 3:
		// 更新时间
		sbr.set_text(1, current_time());
		break;
	case 4:
		if (autoRefresh) {
			if (GetForegroundWindow() == hwnd) {
				HWND focus = GetFocus();
				if (focus) {
					WCHAR cls[256]{}; GetClassNameW(focus, cls, 256);
					if (_wcsicmp(cls, L"Edit") == 0) {
						// 用户正在输入，不更新
						break;
					}
				}
			}
			update_target();
		}
		break;
	default:
		return;
	}
	ev.preventDefault();
}

void MainWindow::onClose(EventData& ev) {
	if (hideWhenMinimized) {
		// 改为隐藏而不是关闭
		ev.preventDefault();
		hideMainWindow();
		return;
	}
	ev.preventDefault();
	send(WM_APP + WM_CLOSE);
}

void MainWindow::hideMainWindow() {
	auto* animation = new std::function<void(EventData&)>;
	*animation = [this, animation](EventData& ev) {
		if (ev.wParam != SIZE_MINIMIZED) return;
		hide();
		removeEventListener(WM_SIZE, *animation);
		delete animation;
		if (app::config.get_or("app.main_window.notifications.winhide.shown", false) == false) {
			app::icon.showNotification(L"Window Customize Tool V2", L"主窗口已隐藏，可在托盘图标中显示");
			app::config["app.main_window.notifications.winhide.shown"] = true;
			app::save_config();
		}
	};
	addEventListener(WM_SIZE, *animation); show(SW_MINIMIZE);
}

void MainWindow::init_controls() {
	text_targetHwnd = Static(hwnd, L"Target HWND:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	text_targetHwnd.create();

	edit_targetHwnd = Edit(hwnd, L"0", 120, 10);
	edit_targetHwnd.create();
	edit_targetHwnd.readonly(true);

	finder.set_parent(*this);
	finder.create();
	finder.onselectstart([this] {
		finder.findMode = findMode;
		if (putBottomWhenUse) {
			SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		target_old = target;
	});
	finder.onselecting([this](HWND h) {
		this->target = h;
		update_target();
		sbr.set_text(0, L"当前目标窗口: " + hwnd_strify(h));
	});
	finder.onselectend([this](HWND h) {
		// 恢复Z序
		if (putBottomWhenUse)
			SetWindowPos(hwnd, isTopMost ? HWND_TOPMOST : HWND_TOP, 0, 0, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
		// 获取窗口
		sbr.set_text(0, L"Ready");
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		if (this->hwnd == h) {
			target = target_old;
		}
		else {
			target = target_old = h;
		}
		update_target();
	});

	text_clsName = Static(hwnd, L"| Class name:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	text_clsName.create();
	edit_clsName = Edit(hwnd, L"", 1, 1);
	edit_clsName.create();
	edit_clsName.readonly(true);

	text_winTitle = Static(hwnd, L"Window Title:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	edit_winTitle = Edit(hwnd, L"", 1, 1);
	btn_applyTitle = Button(hwnd, L"Apply", 1, 1);
	text_winTitle.create();
	edit_winTitle.create();
	btn_applyTitle.create();

	btn_applyTitle.onClick([this](EventData& ev) {
		if (target) {
			auto txt = edit_winTitle.text();
			SetLastError(0);
			bool ok = SetWindowTextW(target, txt.c_str());
			DWORD ec = GetLastError();
			if (ec != 0) ok = false;
			wop_report_result(ok, ec);
		}
		else MessageBoxW(hwnd, L"No window selected", L"Error", MB_OK | MB_ICONERROR);
	});

	text_parentWin = Static(hwnd, L"Parent Window:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	edit_parentWin = Edit(hwnd, L"", 1, 1);
	btn_selectParent = Button(hwnd, L"Switch To", 1, 1);
	text_parentWin.create();
	edit_parentWin.create();
	btn_selectParent.create();
	edit_parentWin.readonly(true);
	btn_selectParent.onClick([this](EventData& ev) {
		if (target) {
			HWND parent = GetParent(target);
			if (!parent) {
				sbr.set_text(0, current_time() + 
					format(L" 失败: 父窗口无效。 {}", hwnd_strify(parent)));
			}
			else {
				target = parent;
			}
			update_target();
		}
		else MessageBoxW(hwnd, L"No window selected", L"Error", MB_OK | MB_ICONERROR);
	});

	group_winOperations = Button(hwnd, L"Window Operations", 1, 1, 0, 0, 0,
		Button::STYLE & ~BS_CENTER | BS_GROUPBOX);
	group_winOperations.create();
	group_winOperations.remove_style(WS_TABSTOP);

	cb_showWin = CheckBox(hwnd, L"Show", 1, 1);
	cb_enableWin = CheckBox(hwnd, L"Enabled", 1, 1);
	btn_b2f = Button(hwnd, L"Bring to Front", 1, 1);
	btn_op_shownormal = Button(hwnd, L"Restore", 1, 1);
	btn_op_max = Button(hwnd, L"Maximize", 1, 1);
	btn_op_min = Button(hwnd, L"Minimize", 1, 1);
	cb_showWin.create(); cb_enableWin.create(); btn_b2f.create();
	btn_op_shownormal.create(); btn_op_min.create(); btn_op_max.create();
	cb_showWin.onChanged([this] (EventData& ev) {
		if (!target) { cb_showWin.uncheck(); return; }
		bool show = cb_showWin.checked();
		SetLastError(0);
		ShowWindow(target, show ? SW_SHOW : SW_HIDE);
		wop_report_result();
	});
	cb_enableWin.onChanged([this] (EventData& ev) {
		if (!target) { cb_enableWin.uncheck(); return; }
		bool enabled = cb_enableWin.checked();
		SetLastError(0);
		EnableWindow(target, enabled);
		wop_report_result();
	});
	btn_b2f.onClick([this] (EventData& ev) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_BTF);
	});
	btn_op_shownormal.onClick([this] (EventData& ev) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_SHOW);
	});
	btn_op_min.onClick([this] (EventData& ev) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_MINIMIZE);
	});
	btn_op_max.onClick([this] (EventData& ev) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_MAXIMIZE);
	});

	btn_highlight = Button(hwnd, L"Highlight", 1, 1);
	btn_showpos = Button(hwnd, L"Locate", 1, 1);
	text_winpos = Static(hwnd, L"Pos: --", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	btn_swp = Button(hwnd, L"SetWindowPos", 1, 1);
	btn_resize = Button(hwnd, L"Resize", 1, 1);
	btn_highlight.create(); btn_showpos.create();
	text_winpos.create(); btn_swp.create(); btn_resize.create();

	btn_highlight.onClick([this](EventData&) {
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		thread([](HWND t, WindowLocatorCore locator) {
			for (int i = 0; i < 5; ++i) {
				HighlightWindow(t, locator.get_highlight_color(), locator.get_highlight_width());
				Sleep(50);
				RestoreScreenContent();
				Sleep(50);
			}
		}, target, finder.get_locator()).detach();
		wop_report_result(true);
	});
	btn_showpos.onClick([this](EventData&) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_HIGHLIGHT);
	});
	btn_resize.onClick([this](EventData&) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_RESIZE);
	});
	btn_swp.onClick([this](EventData&) {
		postMenuEvent(ID_MENU_WINDOWMANAGER_SWP);
	});

	cb_topmost = CheckBox(hwnd, L"Always on top", 1, 1); cb_topmost.create();
	btn_zorder = Button(hwnd, L"Z-order", 1, 1); btn_zorder.create();
	btn_border = Button(hwnd, L"Border...", 1, 1); btn_border.create();
	btn_corner = Button(hwnd, L"Corner...", 1, 1); btn_corner.create();
	btn_winstyle = Button(hwnd, L"Style...", 1, 1, 0, 0, 0, Button::STYLE | BS_SPLITBUTTON); btn_winstyle.create();
	btn_adjust = Button(hwnd, L"Adjust...", 1, 1); btn_adjust.create();

	cb_topmost.onChanged([this](EventData&) {
		if (!target) { cb_topmost.uncheck(); return; }
		bool current_state = IsWindowTopMost(target);
		if (cb_topmost.checked() == current_state) return;
		bool ok = SetWindowPos(target, cb_topmost.checked() ? HWND_TOPMOST : hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		wop_report_result(ok);
	});
	btn_zorder.onClick([this](EventData&) {
		context_anyorder_internal(1, btn_zorder);
	});
	btn_border.onClick([this](EventData&) {
		context_anyorder_internal(2, btn_border);
	});
	btn_corner.onClick([this](EventData&) {
		context_anyorder_internal(3, btn_corner);
	});
	btn_winstyle.onClick([this](EventData&) {
		context_anyorder_internal(4, btn_winstyle);
	});
	btn_winstyle.on(BCN_DROPDOWN, [this](EventData&) {
		RECT rc{}; GetWindowRect(btn_winstyle, &rc);
		int ret = Menu({
			MenuItem(L"Style", 1),
			MenuItem(L"StyleEx", 2)
		}).pop(rc.left, rc.bottom);
		if (!ret) return;
		// 1 + 3 == 4
		// 2 + 3 == 5
		context_anyorder_internal(ret + 3, btn_winstyle);
	});
	btn_adjust.onClick([this](EventData&) {
		context_anyorder_internal(6, btn_adjust);
	});

	btn_close = Button(hwnd, L"Close", 1, 1); btn_close.create();
	btn_destroy = Button(hwnd, L"Destroy", 1, 1); btn_destroy.create();
	btn_endtask = Button(hwnd, L"EndTask", 1, 1); btn_endtask.create();
	btn_properties = Button(hwnd, L"Properties", 1, 1); btn_properties.create();
	btn_close.onClick([this](EventData&) { postMenuEvent(ID_MENU_WINDOWMANAGER_CLOSE); });
	btn_destroy.onClick([this](EventData&) { postMenuEvent(ID_MENU_WINDOWMANAGER_DESTROY); });
	btn_endtask.onClick([this](EventData&) { postMenuEvent(ID_MENU_WINDOWMANAGER_ENDTASK); });
	btn_properties.onClick([this](EventData&) { postMenuEvent(ID_MENU_WINDOWMANAGER_PROPERIES); });
}

void MainWindow::init_hotkey() {
	// Ctrl+W
	register_hot_key(true, false, false, 'W', [this](HotKeyProcData& data) {
		data.preventDefault();
		close();
	}, HotKeyOptions::Windowed);
	// Ctrl+Shift+W
	register_hot_key(true, false, true, 'W', [this](HotKeyProcData& data) {
		data.preventDefault();
		post(WM_APP + WM_CLOSE);
	}, HotKeyOptions::Windowed);
	// Ctrl+N
	register_hot_key(true, false, false, 'N', [this](HotKeyProcData& data) {
		data.preventDefault();
		app::create_win();
	}, HotKeyOptions::Windowed);
	// Ctrl+T
	register_hot_key(true, false, false, 'T', [this](HotKeyProcData& data) {
		data.preventDefault();
		toggleTopMostState();
	}, HotKeyOptions::Windowed);
	// Ctrl+R
	register_hot_key(true, false, false, 'R', [this](HotKeyProcData& data) {
		data.preventDefault(); postMenuEvent(ID_MENU_WINDOWMANAGER_RELOAD);
	}, HotKeyOptions::Windowed);
	// F5
	register_hot_key(false, false, false, VK_F5, [this](HotKeyProcData& data) {
		data.preventDefault(); postMenuEvent(ID_MENU_WINDOWMANAGER_RELOAD);
	}, HotKeyOptions::Windowed);
	// Ctrl+F
	register_hot_key(true, false, false, 'F', [this](HotKeyProcData& data) {
		data.preventDefault(); postMenuEvent(ID_MENU_WINDOW_FIND);
	}, HotKeyOptions::Windowed);
	// Alt+Enter
	register_hot_key(false, true, false, VK_RETURN, [this](HotKeyProcData& data) {
		data.preventDefault(); postMenuEvent(ID_MENU_WINDOWMANAGER_PROPERIES);
	}, HotKeyOptions::Windowed);
}

void MainWindow::init_config() {
	isTopMost = app::config.get_or("app.main_window.top_most", false);
	hideWhenMinimized = app::config.get_or("app.main_window.hide_when_minimized", false);
	putBottomWhenUse = app::config.get_or("app.main_window.bottom_when_use", false);
	useHex = app::config.get_or("app.main_window.hwnd_format.hex", true);
	findMode = app::config.get_or("app.main_window.finder.mode", 0);
	autoRefresh = app::config.get_or("app.main_window.target.auto_refresh.enabled", true);

	if (auto corner = (DWM_WINDOW_CORNER_PREFERENCE)app::config.get_or("app.main_window.visual.corner", 0)) {
		DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
		corner_config = corner;
	}
}

void MainWindow::updateMenuStatus() {
	HMENU hMainMenu = GetMenu(hwnd);

	// “选项”菜单项
	HMENU menu = GetSubMenu(hMainMenu, 2);
	CheckMenuItem(menu, ID_MENU_OPTIONS_ALWAYSONTOP, isTopMost ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_MENU_OPTIONS_HIDEWHENMINIMID, hideWhenMinimized ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_MENU_OPTIONS_PUT_BUTTOM_WHEN_USE, putBottomWhenUse ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuRadioItem(menu,
		ID_MENU_OPTIONS_SHOWFORMAT_DEC,
		ID_MENU_OPTIONS_SHOWFORMAT_HEX,
		useHex ? ID_MENU_OPTIONS_SHOWFORMAT_HEX : ID_MENU_OPTIONS_SHOWFORMAT_DEC,
		MF_BYCOMMAND);
	CheckMenuRadioItem(menu,
		ID_MENU_OPTIONS_WLPREF_CONTROLS,
		ID_MENU_OPTIONS_WLPREF_ROOTOWNER,
		((findMode == GA_ROOTOWNER) ? ID_MENU_OPTIONS_WLPREF_ROOTOWNER : (
			(findMode == GA_ROOT) ? ID_MENU_OPTIONS_WLPREF_TOPLEVEL : ID_MENU_OPTIONS_WLPREF_CONTROLS)
		),
		MF_BYCOMMAND);
	CheckMenuItem(menu, ID_MENU_OPTIONS_AUTOREFRESH, autoRefresh ? MF_CHECKED : MF_UNCHECKED);

	// “视图”菜单项
	menu = GetSubMenu(hMainMenu, 3);
	int idCorner = ID_MENU_VIEW_MAINWINDOW_CORNER_0 + corner_config;
	if (idCorner < ID_MENU_VIEW_MAINWINDOW_CORNER_0 || idCorner > ID_MENU_VIEW_MAINWINDOW_CORNER_3)
		idCorner = ID_MENU_VIEW_MAINWINDOW_CORNER_0;
	CheckMenuRadioItem(menu,
		ID_MENU_VIEW_MAINWINDOW_CORNER_0,
		ID_MENU_VIEW_MAINWINDOW_CORNER_3,
		idCorner,
		MF_BYCOMMAND);

	// “窗口”菜单项
	menu = GetSubMenu(hMainMenu, 4);
	//EnableMenuItem(menu, 4, ((target && IsWindow(target)) ? (MF_ENABLED) : (MF_DISABLED | MF_GRAYED)) | MF_BYPOSITION);
}

void MainWindow::doLayout(EventData& ev) {
	// 处理 WM_SIZE / WM_SIZING
	ev.preventDefault();
	ev.returnValue(TRUE);

	RECT rc = { 0 }; GetClientRect(hwnd, &rc);
	LONG w = rc.right - rc.left, h = rc.bottom - rc.top;

	// 调整控件位置
	text_targetHwnd.resize(10, 10, 100, 24);
	edit_targetHwnd.resize(120, 10, 110, 24);
	finder.resize(240-4, 10-4, 32, 32);
	text_clsName.resize(275, 10, 100, 24);
	edit_clsName.resize(380, 10, w - 390, 24);
	text_winTitle.resize(10, 44, 100, 24);
	edit_winTitle.resize(120, 44, w - 200, 24);
	btn_applyTitle.resize(w - 70, 44, 60, 24);
	text_parentWin.resize(10, 78, 100, 24);
	edit_parentWin.resize(120, 78, w - 230, 24);
	btn_selectParent.resize(w - 100, 78, 90, 24);
	group_winOperations.resize(10, 110, w - 20, 170);
	cb_showWin.resize(20, 135, 60, 25);
	cb_enableWin.resize(90, 135, 80, 25);
	btn_b2f.resize(180, 135, w - 470, 25);
	btn_op_shownormal.resize(w - 280, 135, 80, 25);
	btn_op_min.resize(w - 190, 135, 80, 25);
	btn_op_max.resize(w - 100, 135, 80, 25);
	btn_highlight.resize(20, 170, 100, 25);
	btn_showpos.resize(130, 170, 80, 25);
	text_winpos.resize(220, 170, w - 460, 25);
	btn_swp.resize(w - 230, 170, 120, 25);
	btn_resize.resize(w - 100, 170, 80, 25);
	cb_topmost.resize(20, 205, 120, 25);
	btn_zorder.resize(150, 205, 80, 25);
	btn_border.resize(240, 205, 80, 25);
	btn_corner.resize(330, 205, 80, 25);
	btn_winstyle.resize(420, 205, 100, 25);
	btn_adjust.resize(530, 205, w - 550, 25);
	btn_close.resize(20, 240, 80, 25);
	btn_destroy.resize(110, 240, 80, 25);
	btn_endtask.resize(200, 240, 80, 25);
	btn_properties.resize(290, 240, w - 310, 25);

	// 调整状态栏大小
	sbr.post((UINT)ev.message, ev.wParam, ev.lParam);
	int parts[] = { w - 80, w };
	sbr.set_parts(2, parts);
}

void MainWindow::paint(EventData& ev) {
	
}

void MainWindow::update_target() {
	// 意图：更新目标窗口的相关信息。
	if (!target) {
		edit_targetHwnd.text(L"0"); // NULL
		edit_clsName.text(L"");
		edit_winTitle.text(L"");
		edit_parentWin.text(L"NULL");
		cb_showWin.check(false);
		cb_enableWin.check(false);
		text_winpos.text(L"Pos: --");
		return;
	}

	if (IsHungAppWindow(target))
		edit_targetHwnd.text(hwnd_strify(target) + L" (hung window)");
	else edit_targetHwnd.text(hwnd_strify(target));

	auto buffer = make_unique<wchar_t[]>(32768);
	WCHAR wc[260]{};
	GetClassNameW(target, buffer.get(), 260); // 窗口类最大 256 字符
	edit_clsName.text(buffer.get());
	GetWindowTextW(target, buffer.get(), 32768);
	edit_winTitle.text(buffer.get());
	HWND p = GetParent(target);
	if (IsWindow(p)) {
		GetClassNameW(p, wc, 260);
		GetWindowTextW(p, buffer.get(), 32768);
		edit_parentWin.text(format(L"({}) [{}] Class:[{}]", 
			hwnd_strify(p), buffer.get(), wc));
	}
	else {
		edit_parentWin.text(L"NULL");
	}

	cb_showWin.check(IsWindowVisible(target));
	cb_enableWin.check(IsWindowEnabled(target));

	RECT rc{}; GetWindowRect(target, &rc);
	text_winpos.text(format(L"Pos: ({},{});{}x{}",
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top
	));

	cb_topmost.check(IsWindowTopMost(target));
}
void MainWindow::wop_report_result(bool ok, DWORD code) {
	wstring text = ok ? success_text : ErrorChecker(code).message();
	sbr.set_text(0, current_time() + L" " + text);
	update_target();
}

void MainWindow::onMenu(EventData& ev) {
	// 处理菜单…
	// w32oop 内部将菜单标识符作为 wParam
	switch (ev.wParam) {
	case ID_MENU_ABOUT:
		ShellAboutW(hwnd, L"窗口自定义工具 v2", L"GNU General Public License 3.0\n"
			"https://github.com/shc0743/WindowCustomizeToolV2", get_window_icon());
		break;
	case ID_MENU_FILE_CLOSE:
		close();
		break;
	case ID_MENU_FILE_EXIT:
		app::quit();
		break;
	case ID_MENU_NEW_FRAME_MAINWND:
		app::create_win();
		break;
	case ID_MENU_FILE_INSTANCES:
		app::menu.get_children()[4].click();
		break;
	case ID_MENU_OPTIONS_SETTINGS:
		if (app::ipcwin) app::ipcwin->post(WM_APP + WM_SETTINGCHANGE);
		break;
	case ID_MENU_OPTIONS_PUT_BUTTOM_WHEN_USE:
		putBottomWhenUse = !putBottomWhenUse;
		app::config["app.main_window.bottom_when_use"] = putBottomWhenUse;
		// 更新菜单状态
		updateMenuStatus();
		break;
	case ID_MENU_OPTIONS_HIDEWHENMINIMID:
		hideWhenMinimized = !hideWhenMinimized;
		app::config["app.main_window.hide_when_minimized"] = hideWhenMinimized;
		updateMenuStatus();
		break;
	case ID_MENU_OPTIONS_ALWAYSONTOP:
		toggleTopMostState();
		break;
	case ID_MENU_OPTIONS_SHOWFORMAT_DEC:
	case ID_MENU_OPTIONS_SHOWFORMAT_HEX:
		useHex = (ev.wParam == ID_MENU_OPTIONS_SHOWFORMAT_HEX);
		app::config["app.main_window.hwnd_format.hex"] = useHex;
		updateMenuStatus();
		update_target();
		break;
	case ID_MENU_FILE_CLOSE_TOTALLY:
		post(WM_APP + WM_CLOSE);
		break;
	case ID_MENU_HELP_ABOUT:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hwnd, (
			[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)->LRESULT {
				switch (message) {
					case WM_INITDIALOG:
						return TRUE;
					case WM_COMMAND:
						switch (LOWORD(wParam)) {
							case IDOK:
							case IDCANCEL:
								EndDialog(hDlg, LOWORD(wParam));
								break;
							case IDRETRY:
								ShellExecuteW(hDlg, L"open", L"https://github.com/shc0743/WindowCustomizeToolV2", NULL, NULL, SW_SHOW);
								break;
							default:
								return FALSE;
						}
						return TRUE;
					default:
						return FALSE;
				}
			}
		));
		break;
	case ID_MENU_WINDOW_FIND: {
		INT_PTR r = DialogBoxParamW(hInst, MAKEINTRESOURCE(IDD_DIALOG_WINDOWFINDER1), hwnd, (
			[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)->LRESULT {
				switch (message) {
				case WM_INITDIALOG:
					return TRUE;
				case WM_COMMAND:
					switch (LOWORD(wParam)) {
					case IDABORT:
						ShellExecuteW(hDlg, L"open", L"https://learn.microsoft.com/zh-cn/"
							"windows/win32/api/winuser/nf-winuser-findwindoww",
							NULL, NULL, SW_SHOW);
						break;
					case IDOK: {
						// 获取窗口…… 
						HWND hw = 0;
						WCHAR caption[2048] = { 0 }, classn[256] = { 0 }, handle[32] = { 0 };
						GetDlgItemTextW(hDlg, IDC_WINDOWFINDER_CAPTION, caption, 2048);
						GetDlgItemTextW(hDlg, IDC_WINDOWFINDER_CLASS, classn, 256);
						GetDlgItemTextW(hDlg, IDC_WINDOWFINDER_HANDLE, handle, 32);
						if (handle[0] != 0) {
							// 解析用户输入的字符串。如果用户输入以 0x 开头，则尝试解析为十六进制；
							// 否则尝试解析为十进制。
							hw = (HWND)(ULONG_PTR)wcstoul(handle, NULL, handle[0] == L'0' && handle[1] == L'x' ? 16 : 10);
						}
						if (!IsWindow(hw)) {
							if (caption[0] || classn[0])
								hw = FindWindowW(classn[0] ? classn : NULL, caption[0] ? caption : NULL);
							else hw = NULL;
						}
						EndDialog(hDlg, (INT_PTR)hw);
					}
						break;
					case IDCANCEL:
						EndDialog(hDlg, NULL);
						break;
					default:
						return FALSE;
					}
					return TRUE;
				default:
					return FALSE;
				}
			}
		), 0);
		if (r) {
			target = (HWND)(LONG_PTR)r;
			update_target();
		}
		else {
			sbr.set_text(0, current_time() + L" 未找到符合条件的窗口。");
		}
	}
		break;
	case ID_MENU_OPTIONS_WLPREF_HELP:
		TaskDialog(hwnd, NULL, L"帮助", L"窗口查找偏好",
			L"控件		- 默认模式。查找窗口内的控件。\n"
			L"顶级窗口	- GA_ROOT。查找控件所属的根窗口。\n"
			L"根窗口		- GA_ROOTOWNER。查找窗口的所属者。",
			TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, NULL
		);
		break;
	case ID_MENU_OPTIONS_WLPREF_CONTROLS:
	case ID_MENU_OPTIONS_WLPREF_TOPLEVEL:
	case ID_MENU_OPTIONS_WLPREF_ROOTOWNER:
		findMode = (ev.wParam == ID_MENU_OPTIONS_WLPREF_CONTROLS) ? 0 :
			((ev.wParam == ID_MENU_OPTIONS_WLPREF_TOPLEVEL) ?
				GA_ROOT : GA_ROOTOWNER);
		app::config["app.main_window.finder.mode"] = findMode;
		updateMenuStatus();
		update_target();
		break;
	case ID_MENU_WINDOWMANAGER_RELOAD:
		update_target();
		sbr.set_text(0, current_time() + L" 窗口信息更新成功。");
		break;
	case ID_MENU_OPTIONS_AUTOREFRESH:
		autoRefresh = !autoRefresh;
		app::config["app.main_window.target.auto_refresh.enabled"] = autoRefresh;
		updateMenuStatus();
		break;
	case ID_MENU_VIEW_MAINWINDOW_ALPHA:
		thread(createAlphaEditor, hwnd).detach();
		break;
	case ID_MENU_VIEW_MAINWINDOW_CORNER_0:
	case ID_MENU_VIEW_MAINWINDOW_CORNER_1:
	case ID_MENU_VIEW_MAINWINDOW_CORNER_2:
	case ID_MENU_VIEW_MAINWINDOW_CORNER_3:
	{
		// 偷懒: 菜单ID是连续的，所以可以直接减去起始ID
		corner_config = (DWM_WINDOW_CORNER_PREFERENCE)(ev.wParam - ID_MENU_VIEW_MAINWINDOW_CORNER_0);
		app::config["app.main_window.visual.corner"] = corner_config;
		updateMenuStatus();
		DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_config, sizeof(corner_config));
	}
		break;
#pragma region 窗口管理器相关处理
	case ID_MENU_WINDOWMANAGER_BTF:
		wop_report_result(BringWindowToTop(target));
		break;
	case ID_MENU_WINDOWMANAGER_SHOW:
	case ID_MENU_WINDOWMANAGER_HIDE:
	case ID_MENU_WINDOWMANAGER_MINIMIZE:
	case ID_MENU_WINDOWMANAGER_MAXIMIZE:
	{
		const map<WPARAM, int> MyMap = {
			{ ID_MENU_WINDOWMANAGER_SHOW, SW_RESTORE },
			{ ID_MENU_WINDOWMANAGER_HIDE, SW_HIDE },
			{ ID_MENU_WINDOWMANAGER_MINIMIZE, SW_MINIMIZE },
			{ ID_MENU_WINDOWMANAGER_MAXIMIZE, SW_MAXIMIZE }
		};
		SetLastError(0);
		ShowWindow(target, MyMap.at(ev.wParam));
		wop_report_result();
	}
		break;
	case ID_MENU_WINDOWMANAGER_HIGHLIGHT:
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		thread([](HWND t) {
			OverlayWindow overlay;
			overlay.create();
			overlay.setTarget(t);
			overlay.show();
			overlay.setClosable(false);
			overlay.closeAfter(2000);
			overlay.run(&overlay);
		}, target).detach();
		wop_report_result(true);
		break;
	case ID_MENU_WINDOWMANAGER_RESIZE:
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		disable();
		thread([](HWND target, HWND self) {
			OverlayWindow overlay;
			overlay.create();
			overlay.setType(OverlayWindow::TYPE_PROACTIVE);
			overlay.setTarget(target);
			overlay.show();
			overlay.run(&overlay);
			if (IsWindow(self)) {
				EnableWindow(self, TRUE);
				SetForegroundWindow(self);
			}
		}, target, hwnd).detach();
		wop_report_result(true);
		break;
	case ID_MENU_WINDOWMANAGER_SWP:
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_DIALOG_SWP_ARGS), hwnd, SwpDlgHandler, (LPARAM)target);
		break;
	case ID_MENU_WINDOWMANAGER_CLOSE:
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		disable();
		SendMessageTimeoutW(target, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, NULL);
		enable();
		wop_report_result(false == IsWindow(target));
		break;
	case ID_MENU_WINDOWMANAGER_DESTROY:
		// 使用 APC 销毁目标窗口。
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
	{
		DWORD pid = 0; GetWindowThreadProcessId(target, &pid);
		wop_report_result(siapi::DestroyRemoteWindow(target, pid));
	}
		break;
	case ID_MENU_WINDOWMANAGER_ENDTASK:
	{
		HMODULE user32 = GetModuleHandleW(L"user32.dll");
		if (!user32) { wop_report_result(false); break; }
		typedef BOOL(WINAPI* EndTask_t)(HWND hWnd, BOOL fShutDown, BOOL fForce);
		EndTask_t EndTask = (EndTask_t)GetProcAddress(user32, "EndTask");
		if (!EndTask) { wop_report_result(false); break; }
		disable();
		if (!EndTask(target, FALSE, FALSE)) {
			wop_report_result(false);
			thread([] {
				PlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMHAND, 0, SND_ALIAS_ID);
			}).detach();
		}
		else wop_report_result(false == IsWindow(target));
		enable();
	}
		break;
#pragma endregion

	default:
		return;
	}
	ev.preventDefault();
}

void MainWindow::onSysMenu(EventData& ev) {
	switch (ev.wParam) {
	case 1001:
		post(WM_APP + WM_CLOSE);
		break;
	default:
		return;
	}
	ev.preventDefault();
}

void MainWindow::toggleTopMostState() {
	isTopMost = !isTopMost;
	app::config["app.main_window.top_most"] = isTopMost;
	set_topmost(isTopMost);
	updateMenuStatus();
}

void MainWindow::createAlphaEditor(HWND hWnd) {
	WindowAlphaEditor wa;
	wa.create();
	wa.setTarget(hWnd);
	wa.center(hWnd);
	wa.show();
	wa.run(&wa);
}

void MainWindow::onMinimize(EventData& ev) {
	if (ev.wParam != SIZE_MINIMIZED) return;
	if (!hideWhenMinimized) return;
	ev.preventDefault();
	hide();
}

LRESULT MainWindow::SwpDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		if (!lParam) return FALSE;
		SetPropW(hDlg, L"target", (HWND)lParam);
		// 获取目标窗口的位置并填充输入框
		{
			RECT rc{}; HWND target = (HWND)lParam;
			GetWindowRect(target, &rc);

			SetDlgItemInt(hDlg, IDC_EDIT_SWP_X, rc.left, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_SWP_Y, rc.top, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_SWP_cx, rc.right - rc.left, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_SWP_cy, rc.bottom - rc.top, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_SWP_uFlags, 0, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_SWP_hWndInsertAfter, (INT)LONG_PTR(
				IsWindowTopMost(target) ? HWND_TOPMOST : 0
			), TRUE);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_SWP_HELP:
			ShellExecuteW(NULL, L"open", L"https://docs.microsoft.com/en-us/windows/"
				"win32/api/winuser/nf-winuser-setwindowpos", NULL, NULL, SW_NORMAL);
			break;
		case IDOK:
		case IDC_BUTTON_SWP_APPLY:
			// 应用更改
		{
			int
				x = GetDlgItemInt(hDlg, IDC_EDIT_SWP_X, NULL, TRUE),
				y = GetDlgItemInt(hDlg, IDC_EDIT_SWP_Y, NULL, TRUE),
				cx = GetDlgItemInt(hDlg, IDC_EDIT_SWP_cx, NULL, TRUE),
				cy = GetDlgItemInt(hDlg, IDC_EDIT_SWP_cy, NULL, TRUE),
				uFlags = GetDlgItemInt(hDlg, IDC_EDIT_SWP_uFlags, NULL, TRUE),
				hWndInsertAfter = GetDlgItemInt(hDlg, IDC_EDIT_SWP_hWndInsertAfter, NULL, TRUE);
			HWND hWnd = (HWND)GetPropW(hDlg, L"target");
			bool ok = SetWindowPos(hWnd, (HWND)(INT_PTR)hWndInsertAfter, x, y, cx, cy, uFlags);
			if (!ok) MessageBoxW(hDlg, ErrorChecker().message().c_str(), NULL, MB_ICONERROR);
		}
			if (LOWORD(wParam) == IDOK) EndDialog(hDlg, LOWORD(wParam));
			break;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		default:
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void MainWindow::context_anyorder_internal(int type, Window& btn_element) {
	if (!IsWindow(this->target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
	// 1 - Zorder
	// 2 - Border
	// 3 - Corner
	// 4 - Style
	// 5 - StyleEx
	// 6 - Adjust
	Menu menu; HWND target = this->target;
	auto zset = [target](HWND insertAfter) {
		SetWindowPos(target, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	};
	if (type == 1) menu = Menu({
		MenuItem(L"HWND_TOP", 1, std::bind(zset, HWND_TOP)),
		MenuItem(L"HWND_BOTTOM", 2, std::bind(zset, HWND_BOTTOM)),
		MenuItem(L"HWND_TOPMOST", 3, std::bind(zset, HWND_TOPMOST)),
		MenuItem(L"HWND_NOTOPMOST", 4, std::bind(zset, HWND_NOTOPMOST)),
	});
	auto bset = [target](bool borderful) {
		constexpr auto Frame = WS_CAPTION | WS_THICKFRAME;
		LONG_PTR style = GetWindowLongPtrW(target, GWL_STYLE);
		if (borderful) style |= Frame;
		else style &= ~(Frame);
		SetWindowLongPtrW(target, GWL_STYLE, style);
		SetWindowPos(target, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	};
	if (type == 2) menu = Menu({
		MenuItem(L"Border", 1, std::bind(bset, true)),
		MenuItem(L"Borderless", 2, std::bind(bset, false)),
	});
	auto cornerset = [target](DWM_WINDOW_CORNER_PREFERENCE pref) {
		DwmSetWindowAttribute(target, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
	};
	if (type == 3) menu = Menu({
		MenuItem(L"Default", 1, std::bind(cornerset, DWMWCP_DEFAULT)),
		MenuItem(L"Do not round", 2, std::bind(cornerset, DWMWCP_DONOTROUND)),
		MenuItem(L"Round", 3, std::bind(cornerset, DWMWCP_ROUND)),
		MenuItem(L"Round small", 4, std::bind(cornerset, DWMWCP_ROUNDSMALL)),
	});
	if (type == 4 || type == 5) {
		// Style adjustor
		RECT rc{}; GetWindowRect(btn_element, &rc);
		while (1) {
			int index = (type == 4) ? GWL_STYLE : GWL_EXSTYLE;
			LONG_PTR style = GetWindowLongPtrW(target, index);
			auto check_style = [&style](LONG flag) {
				return (style & flag) == flag;
			};
			auto styletoggle = [&style](LONG flag) {
				if ((style & flag) == flag) {
					style &= ~flag;
				}
				else {
					style |= flag;
				}
			};
			if (type == 4) {
				auto basic_style_set = [&style](LONG flag) {
					style &= ~(WS_POPUP | WS_CHILD);
					style |= flag;
				};
				menu = Menu({
					MenuItem(L"WS_OVERLAPPED", 2, std::bind(basic_style_set, WS_OVERLAPPED)),
					MenuItem(L"WS_POPUP", 3, std::bind(basic_style_set, WS_POPUP)),
					MenuItem(L"WS_CHILD", 4, std::bind(basic_style_set, WS_CHILD)),
					MenuItem::separator(),
					MenuItem(L"WS_BORDER", 5, std::bind(styletoggle, WS_BORDER)),
					MenuItem(L"WS_CAPTION", 6, std::bind(styletoggle, WS_CAPTION)),
					MenuItem(L"WS_DLGFRAME", 7, std::bind(styletoggle, WS_DLGFRAME)),
					MenuItem(L"WS_THICKFRAME (WS_SIZEBOX)", 8, std::bind(styletoggle, WS_THICKFRAME)),
					MenuItem::separator(),
					MenuItem(L"WS_SYSMENU", 9, std::bind(styletoggle, WS_SYSMENU)),
					MenuItem(L"WS_MINIMIZEBOX", 10, std::bind(styletoggle, WS_MINIMIZEBOX)),
					MenuItem(L"WS_MAXIMIZEBOX", 11, std::bind(styletoggle, WS_MAXIMIZEBOX)),
					MenuItem::separator(),
					MenuItem(L"WS_HSCROLL", 12, std::bind(styletoggle, WS_HSCROLL)),
					MenuItem(L"WS_VSCROLL", 13, std::bind(styletoggle, WS_VSCROLL)),
					MenuItem(L"WS_CLIPCHILDREN", 14, std::bind(styletoggle, WS_CLIPCHILDREN)),
					MenuItem(L"WS_CLIPSIBLINGS", 15, std::bind(styletoggle, WS_CLIPSIBLINGS)),
					MenuItem(L"WS_GROUP", 16, std::bind(styletoggle, WS_GROUP)),
					MenuItem(L"WS_TABSTOP", 17, std::bind(styletoggle, WS_TABSTOP)),
					MenuItem::separator(),
					MenuItem(L"完成更改 (&0)", 1),
				});
				if (check_style(WS_POPUP)) menu.get_children()[1].check();
				else if (check_style(WS_CHILD)) menu.get_children()[2].check();
				else menu.get_children()[0].check(); // Overlapped
				menu.get_children()[4].check(check_style(WS_BORDER));
				menu.get_children()[5].check(check_style(WS_CAPTION));
				menu.get_children()[6].check(check_style(WS_DLGFRAME));
				menu.get_children()[7].check(check_style(WS_THICKFRAME));
				menu.get_children()[9].check(check_style(WS_SYSMENU));
				menu.get_children()[10].check(check_style(WS_MINIMIZEBOX));
				menu.get_children()[11].check(check_style(WS_MAXIMIZEBOX));
				menu.get_children()[13].check(check_style(WS_HSCROLL));
				menu.get_children()[14].check(check_style(WS_VSCROLL));
				menu.get_children()[15].check(check_style(WS_CLIPCHILDREN));
				menu.get_children()[16].check(check_style(WS_CLIPSIBLINGS));
				menu.get_children()[17].check(check_style(WS_GROUP));
				menu.get_children()[18].check(check_style(WS_TABSTOP));
			}
			else if (type == 5) {
				auto basic_style_set = [&style](LONG flag) {
					style &= ~(WS_EX_APPWINDOW | WS_EX_TOOLWINDOW);
					style |= flag;
				};
				menu = Menu({
					MenuItem(L"WS_EX_APPWINDOW", 2, std::bind(basic_style_set, WS_EX_APPWINDOW)),
					MenuItem(L"WS_EX_TOOLWINDOW", 3, std::bind(basic_style_set, WS_EX_TOOLWINDOW)),
					MenuItem::separator(),
					MenuItem(L"WS_EX_DLGMODALFRAME", 4, std::bind(styletoggle, WS_EX_DLGMODALFRAME)),
					MenuItem(L"WS_EX_NOACTIVATE", 5, std::bind(styletoggle, WS_EX_NOACTIVATE)),
					MenuItem(L"WS_EX_TOPMOST", 6, std::bind(styletoggle, WS_EX_TOPMOST)),
					MenuItem(L"WS_EX_ACCEPTFILES", 7, std::bind(styletoggle, WS_EX_ACCEPTFILES)),
					MenuItem(L"WS_EX_MDICHILD", 8, std::bind(styletoggle, WS_EX_MDICHILD)),
					MenuItem(L"WS_EX_WINDOWEDGE", 9, std::bind(styletoggle, WS_EX_WINDOWEDGE)),
					MenuItem(L"WS_EX_CLIENTEDGE", 10, std::bind(styletoggle, WS_EX_CLIENTEDGE)),
					MenuItem(L"WS_EX_STATICEDGE", 11, std::bind(styletoggle, WS_EX_STATICEDGE)),
					MenuItem(L"WS_EX_CONTROLPARENT", 12, std::bind(styletoggle, WS_EX_CONTROLPARENT)),
					MenuItem::separator(),
					MenuItem(L"WS_EX_LAYERED", 13, std::bind(styletoggle, WS_EX_LAYERED)),
					MenuItem(L"WS_EX_TRANSPARENT", 14, std::bind(styletoggle, WS_EX_TRANSPARENT)),
					MenuItem::separator(),
					MenuItem(L"完成更改 (&0)", 1),
				});
				if (check_style(WS_EX_TOOLWINDOW)) menu.get_children()[1].check();
				else if (check_style(WS_EX_APPWINDOW)) menu.get_children()[0].check();
				menu.get_children()[3].check(check_style(WS_EX_DLGMODALFRAME));
				menu.get_children()[4].check(check_style(WS_EX_NOACTIVATE));
				menu.get_children()[5].check(check_style(WS_EX_TOPMOST));
				menu.get_children()[6].check(check_style(WS_EX_ACCEPTFILES));
				menu.get_children()[7].check(check_style(WS_EX_MDICHILD));
				menu.get_children()[8].check(check_style(WS_EX_WINDOWEDGE));
				menu.get_children()[9].check(check_style(WS_EX_CLIENTEDGE));
				menu.get_children()[10].check(check_style(WS_EX_STATICEDGE));
				menu.get_children()[11].check(check_style(WS_EX_CONTROLPARENT));
				menu.get_children()[13].check(check_style(WS_EX_LAYERED));
				menu.get_children()[14].check(check_style(WS_EX_TRANSPARENT));
			}
			int i = menu.pop(rc.left, rc.bottom);
			if (i == 1 || i == 0) break;
			SetWindowLongPtrW(target, index, style);
			SetWindowPos(target, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
		return;
	}
	if (type == 6) menu = Menu({
		MenuItem(L"调整透明度 (&A)", 1, [this] { thread(createAlphaEditor, this->target).detach(); }),
		MenuItem(L"调整父窗口 (&P)", 2, [this] {
			thread([](HWND target, HWND centerSource) {
				WindowChangeParent wcp;
				wcp.create();
				wcp.setTarget(target);
				wcp.center(centerSource);
				wcp.show();
				wcp.run(&wcp);
			}, this->target, hwnd).detach();
		}),
	});
	RECT rc{}; GetWindowRect(btn_element, &rc);
	menu.pop(rc.left, rc.bottom);
}


