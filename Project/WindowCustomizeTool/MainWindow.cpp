#include "MainWindow.h"
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
}

void MainWindow::onDestroy() {
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
