#include "MainWindow.h"
#include "IPCWindow.h"
#include "resource.h"
HICON MainWindow::app_icon;



void MainWindow::onCreated() {
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)0xee, LWA_ALPHA);

	register_hot_key(true, false, false, 'W', [this](HotKeyProcData& data) {
		data.preventDefault();
		close();
	}, HotKeyOptions::Windowed);
	register_hot_key(true, false, false, 'N', [this](HotKeyProcData& data) {
		data.preventDefault();
		app::create_win();
	}, HotKeyOptions::Windowed);

	init_controls();
	SetMenu(hwnd, LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU_MAINWND)));
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
	remove_style_ex(WS_EX_LAYERED); // 要使得窗口有正常的关闭动画，窗口不能是Layered窗口
}

void MainWindow::hideMainWindow() {
	auto* animation = new std::function<void(EventData&)>;
	*animation = [this, animation](EventData& ev) {
		if (ev.wParam != SIZE_MINIMIZED) return;
		hide();
		removeEventListener(WM_SIZE, *animation);
		delete animation;
		app::icon.showNotification(L"Window Customize Tool V2", L"主窗口已隐藏，可在托盘图标中显示");
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

}

void MainWindow::doLayout(EventData& ev) {
	// 处理 WM_SIZE / WM_SIZING
	ev.preventDefault();

	text_targetHwnd.resize(10, 10, 100, 24);
	edit_targetHwnd.resize(120, 10, 110, 24);
	finder.resize(240-4, 10-4, 32, 32);

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

	// 找到目标窗口
	// TODO
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
	default:
		return;
	}
	ev.preventDefault();
}

