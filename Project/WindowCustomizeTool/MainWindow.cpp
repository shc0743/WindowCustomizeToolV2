#include "MainWindow.h"
#include "IPCWindow.h"
#include "resource.h"
#include "winlib.hpp"
using namespace std;
HICON MainWindow::app_icon;

using namespace WindowCustomizeToolV2_app;



void MainWindow::onCreated() {
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)0xee, LWA_ALPHA);

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

	init_controls();
	init_config();
	if (isTopMost) set_topmost(true);
	SetMenu(hwnd, LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_MENU_MAINWND)));
	updateMenuStatus();

	HMENU sys = sysmenu();
	AppendMenuW(sys, MF_SEPARATOR, 0, NULL);
	AppendMenuW(sys, MF_STRING, 1001, L"关闭此窗口 (&L)");
}

void MainWindow::onDestroy() {
	// 资源清理
	DeleteObject(hFinderEmpty);
	DeleteObject(hFinderFilled);
	DeleteObject(hCurFinding);

	if (app::no_main_window) return;
	// 如果所有 MainWindow 都关闭了，则退出应用程序
	for (auto& w : app::win) {
		if (w && w != this && IsWindow(*(w))) return;
	}
	// 只剩下我们自己了
	PostQuitMessage(0);
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
	text_targetHwnd = Static(hwnd, L"Target &HWND:", 100, 24);
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

	text_clsName = Static(hwnd, L"| &Class name:", 1, 1);
	text_clsName.create();
	edit_clsName = Edit(hwnd, L"", 1, 1);
	edit_clsName.create();
	edit_clsName.readonly(true);

	text_winTitle = Static(hwnd, L"Window &Title:", 1, 1);
	edit_winTitle = Edit(hwnd, L"", 1, 1);
	btn_applyTitle = Button(hwnd, L"&Apply", 1, 1);
	text_winTitle.create();
	edit_winTitle.create();
	btn_applyTitle.create();

	btn_applyTitle.onClick([this](EventData& ev) {
		if (target) SetWindowTextW(target, edit_winTitle.text().c_str());
		else MessageBoxW(hwnd, L"No window selected", L"Error", MB_OK | MB_ICONERROR);
	});
}

void MainWindow::init_config() {
	isTopMost = app::config.get_or("app.main_window.top_most", false);
	hideWhenMinimized = app::config.get_or("app.main_window.hide_when_minimized", false);
	putBottomWhenUse = app::config.get_or("app.main_window.bottom_when_use", false);
	useHex = app::config.get_or("app.main_window.hwnd_format.hex", true);
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
}

void MainWindow::doLayout(EventData& ev) {
	// 处理 WM_SIZE / WM_SIZING
	ev.preventDefault();

	RECT rc = { 0 }; GetClientRect(hwnd, &rc);
	LONG w = rc.right - rc.left, h = rc.bottom - rc.top;

	text_targetHwnd.resize(10, 10, 100, 24);
	edit_targetHwnd.resize(120, 10, 110, 24);
	finder.resize(240-4, 10-4, 32, 32);
	text_clsName.resize(275, 10, 100, 24);
	edit_clsName.resize(380, 10, w - 390, 24);
	text_winTitle.resize(10, 44, 100, 24);
	edit_winTitle.resize(120, 44, w - 200, 24);
	btn_applyTitle.resize(w - 70, 44, 60, 24);
}

void MainWindow::paint(EventData& ev) {
	ev.returnValue(0);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	// TODO: 在此处添加使用 hdc 的任何绘图代码...


	EndPaint(hwnd, &ps);
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
	locator.setCallback([this](HWND target) {
		if (this->hwnd == target) return;
		this->target = target;
		update_target();
	});
}

void MainWindow::duringFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();

	locator.during();
}

void MainWindow::endFind(EventData& ev) {
	if (!isFinding) return;
	ev.preventDefault();

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
	locator.end();
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
		return;
	}

	if (useHex) {
		edit_targetHwnd.text(std::format(L"0x{:X}", reinterpret_cast<ULONGLONG>(target)));
	}
	else {
		edit_targetHwnd.text(std::to_wstring(reinterpret_cast<ULONGLONG>(target)));
	}

	auto buffer = make_unique<wchar_t[]>(32768);
	GetClassNameW(target, buffer.get(), 256); // 窗口类最大 256 字符
	edit_clsName.text(buffer.get());
	GetWindowTextW(target, buffer.get(), 32768);
    edit_winTitle.text(buffer.get());
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

