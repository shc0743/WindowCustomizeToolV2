#include "MainWindow.h"
#include "IPCWindow.h"
#include "resource.h"
#include "winlib.hpp"
#include <chrono>
using namespace std;
HICON MainWindow::app_icon;

using namespace WindowCustomizeToolV2_app;
using namespace WCTv2::winlib;


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

std::wstring MainWindow::hwnd_strify(HWND hWnd) {
	if (useHex) {
		return (std::format(L"0x{:X}", reinterpret_cast<ULONGLONG>(hWnd)));
	}
	else {
		return (std::to_wstring(reinterpret_cast<ULONGLONG>(hWnd)));
	}
}


void MainWindow::onCreated() {
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)0xee, LWA_ALPHA);
	sbr.set_parent(this);
	sbr.create(L"", 1, 1);

	register_hot_key(true, false, false, 'W', [this](HotKeyProcData& data) {
		data.preventDefault();
		close();
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, true, 'W', [this](HotKeyProcData& data) {
		data.preventDefault();
		post(WM_APP + WM_CLOSE);
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, false, 'N', [this](HotKeyProcData& data) {
		data.preventDefault();
		app::create_win();
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, false, 'T', [this](HotKeyProcData& data) {
		data.preventDefault();
		toggleTopMostState();
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, false, 'R', [this](HotKeyProcData& data) {
		data.preventDefault();
		dispatchEvent(EventData(hwnd, WM_MENU_CHECKED, ID_MENU_WINDOWMANAGER_RELOAD, 0));
	}, HotKeyOptions::Windowed);
	register_hot_key(false, false, false, VK_F5, [this](HotKeyProcData& data) {
		data.preventDefault();
		dispatchEvent(EventData(hwnd, WM_MENU_CHECKED, ID_MENU_WINDOWMANAGER_RELOAD, 0));
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, false, 'F', [this](HotKeyProcData& data) {
		data.preventDefault();
		dispatchEvent(EventData(hwnd, WM_MENU_CHECKED, ID_MENU_WINDOW_FIND, 0));
	}, HotKeyOptions::Windowed);

	init_controls();
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
	post(WM_TIMER, 3, 0);

	sbr.set_text(0, L"Ready");
	init_success = true;
}

void MainWindow::onDestroy() {
	// 资源清理
	DeleteObject(hFinderEmpty);
	DeleteObject(hFinderFilled);
	DeleteObject(hCurFinding);
	KillTimer(hwnd, 3);

	if (!init_success) return;

	// 保存窗口位置
	RECT rc{}; GetWindowRect(hwnd, &rc);
	app::config["app.main_window.userwindow.x"] = rc.left;
	app::config["app.main_window.userwindow.y"] = rc.top;
	app::config["app.main_window.userwindow.width"] = rc.right - rc.left;
	app::config["app.main_window.userwindow.height"] = rc.bottom - rc.top;

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
	remove_style_ex(WS_EX_LAYERED); // 要使得窗口有正常的关闭动画，窗口不能是Layered窗口
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
	text_targetHwnd = Static(hwnd, L"Target &HWND:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	text_targetHwnd.create();

	edit_targetHwnd = Edit(hwnd, L"0", 120, 10);
	edit_targetHwnd.create();
	edit_targetHwnd.readonly(true);

	finder = Static(hwnd, L"", 32, 32);
	finder.create();
	
	hFinderEmpty = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_ICON_SELECTOR));
	hFinderFilled = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_ICON_SELECTOR_FILLED));
	hCurFinding = LoadCursorW(hInst, MAKEINTRESOURCEW(IDC_CURSOR_WINSELECT));
	if (!hFinderEmpty || !hFinderFilled || !hCurFinding)
		throw std::runtime_error("Failed to load resource");
	finder.add_style(SS_ICON | SS_CENTERIMAGE);
	finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderFilled);

	text_clsName = Static(hwnd, L"| &Class name:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	text_clsName.create();
	edit_clsName = Edit(hwnd, L"", 1, 1);
	edit_clsName.create();
	edit_clsName.readonly(true);

	text_winTitle = Static(hwnd, L"Window &Title:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	edit_winTitle = Edit(hwnd, L"", 1, 1);
	btn_applyTitle = Button(hwnd, L"&Apply", 1, 1);
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

	text_parentWin = Static(hwnd, L"&Parent Window:", 1, 1, 0, 0, Static::STYLE | SS_CENTERIMAGE);
	edit_parentWin = Edit(hwnd, L"", 1, 1);
	btn_selectParent = Button(hwnd, L"&Switch To", 1, 1);
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

	cb_showWin = CheckBox(hwnd, L"&Show", 1, 1);
	cb_enableWin = CheckBox(hwnd, L"&Enabled", 1, 1);
	btn_b2f = Button(hwnd, L"&Bring to Front", 1, 1);
	btn_op_shownormal = Button(hwnd, L"&Restore", 1, 1);
	btn_op_max = Button(hwnd, L"&Maximize", 1, 1);
	btn_op_min = Button(hwnd, L"&Minimize", 1, 1);
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
		wop_report_result(BringWindowToTop(target));
	});
	btn_op_shownormal.onClick([this] (EventData& ev) {
		SetLastError(0);
		ShowWindow(target, SW_RESTORE);
		wop_report_result();
	});
	btn_op_min.onClick([this] (EventData& ev) {
		SetLastError(0);
		ShowWindow(target, SW_MINIMIZE);
		wop_report_result();
	});
	btn_op_max.onClick([this] (EventData& ev) {
		SetLastError(0);
		ShowWindow(target, SW_MAXIMIZE);
		wop_report_result();
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
		thread th([](HWND t, WindowLocator locator) {
			for (int i = 0; i < 5; ++i) {
				HighlightWindow(t, locator.get_highlight_color(), locator.get_highlight_width());
				Sleep(50);
				RestoreScreenContent();
				Sleep(50);
			}
		}, target, locator);
		th.detach();

		wop_report_result(true);
	});
	btn_showpos.onClick([this](EventData&) {
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		thread th([](HWND t) {
			OverlayWindow overlay;
			overlay.create();
			overlay.setTarget(t);
			overlay.show();
			overlay.setClosable(false);
			overlay.closeAfter(2000);
			overlay.run();
		}, target);
		th.detach();
		wop_report_result(true);
	});
	btn_resize.onClick([this](EventData&) {
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		disable();
		thread th([](HWND target, HWND self) {
			OverlayWindow overlay;
			overlay.create();
			overlay.setType(OverlayWindow::TYPE_PROACTIVE);
			overlay.setTarget(target);
			overlay.show();
			overlay.run(&overlay);
			if (IsWindow(self)) EnableWindow(self, TRUE);
		}, target, hwnd);
		th.detach();
		wop_report_result(true);
	});
}

void MainWindow::init_config() {
	isTopMost = app::config.get_or("app.main_window.top_most", false);
	hideWhenMinimized = app::config.get_or("app.main_window.hide_when_minimized", false);
	putBottomWhenUse = app::config.get_or("app.main_window.bottom_when_use", false);
	useHex = app::config.get_or("app.main_window.hwnd_format.hex", true);
	findMode = app::config.get_or("app.main_window.finder.mode", 0);
}

void MainWindow::updateMenuStatus() {
	HMENU hMainMenu = GetMenu(hwnd);
	HMENU menu = GetSubMenu(hMainMenu, 2); // “选项”菜单项
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
}

void MainWindow::doLayout(EventData& ev) {
	// 处理 WM_SIZE / WM_SIZING
	ev.preventDefault();

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
	group_winOperations.resize(10, 110, w - 20, h - 250);
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

	// 调整状态栏大小
	sbr.post((UINT)ev.message, ev.wParam, ev.lParam);
	int parts[] = { w - 80, w };
	sbr.set_parts(2, parts);
}

void MainWindow::paint(EventData& ev) {
	
}

void MainWindow::startFind(EventData& ev) {
	POINT pt; GetCursorPos(&pt);
	RECT rect; GetWindowRect(finder, &rect);

	// 判断鼠标是否在 finder 范围内
	if (!(
		pt.x > rect.left &&
		pt.x < rect.right &&
		pt.y> rect.top &&
		pt.y < rect.bottom
	)) return;
	ev.preventDefault();

	// 开始查找
	isFinding = true;
	// 设置鼠标
	SetCapture(hwnd);
	SetCursor(hCurFinding);
	// 改变图标
	hFinderFilled = (HICON)finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderEmpty);
	// 窗口置底
	if (putBottomWhenUse)
		SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	locator.start();
	locator.mode = findMode;
	locator.setCallback([this](HWND target) {
		if (this->hwnd == target) return;
		this->target = target;
		update_target();
		sbr.set_text(0, L"当前目标窗口: " + hwnd_strify(target));
	});

	sbr.set_text(0, L"正在查找窗口...");
}

void MainWindow::duringFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();

	locator.during();
}

void MainWindow::endFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();
	locator.end();

	// 恢复状态
	isFinding = false;
	ReleaseCapture(); //释放鼠标捕获
	RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
	UpdateWindow(GetDesktopWindow());
	// 恢复图标
	hFinderEmpty = (HICON)finder.send(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFinderFilled);
	// 恢复Z序
	if (putBottomWhenUse)
		SetWindowPos(hwnd, isTopMost ? HWND_TOPMOST : HWND_TOP, 0, 0, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
	// 获取窗口
	sbr.set_text(0, L"Ready");
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	if (this->hwnd == locator.target()) return;
	target = locator.target();
	update_target();
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
	case ID_MENU_WINDOW_FIND:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_WINDOWFINDER1), hwnd, (
			[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)->LRESULT {
				switch (message) {
				case WM_INITDIALOG:
					return TRUE;
				case WM_COMMAND:
					switch (LOWORD(wParam)) {
					case IDOK:
						// 获取窗口……
					case IDCANCEL:
						EndDialog(hDlg, LOWORD(wParam));
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

void MainWindow::onMinimize(EventData& ev) {
	if (ev.wParam != SIZE_MINIMIZED) return;
	if (!hideWhenMinimized) return;
	ev.preventDefault();
	hide();
}

