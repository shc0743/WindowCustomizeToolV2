#include "OverlayWindow.h"
ns_declare(WindowCustomizeToolV2_app);


void OverlayWindow::onCreated() {
	add_style_ex(WS_EX_LAYERED | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(hwnd, 0, 0x80, LWA_ALPHA); // 半透明
	set_topmost(true); // 置顶
	HMENU menu = sysmenu();
	AppendMenuW(menu, MF_STRING | MF_DISABLED | MF_GRAYED, 0x1001, L"Overlay Window");
	AppendMenuW(menu, MF_SEPARATOR, 0, 0);
	AppendMenuW(menu, MF_STRING | MF_DISABLED | MF_GRAYED, 0x1002, L"Unable to close the window");
	AppendMenuW(menu, MF_STRING, 0x1003, L"Why?");
	for (int i = 0; i < 7; ++i) RemoveMenu(menu, 0, MF_BYPOSITION);
}


void OverlayWindow::onTimer(EventData& ev) {
	switch (ev.wParam) {
	case 2:
		KillTimer(hwnd, 2);
		destroy();
		break;
	default: return;
	}
	ev.preventDefault();
}

class OverlayWindowInfoWindow : public Window {
public:
	OverlayWindowInfoWindow() : Window(L"", 480, 240, 0, 0, WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_TOPMOST) {}
protected:
	void setup_event_handlers() override {
		WINDOW_add_handler(WM_PAINT, [this](EventData& ev) {
			ev.preventDefault();
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			// 设置画笔颜色
			SetDCPenColor(dc, RGB(0xFF, 0xFF, 0xFF));
			// 画背景
			Rectangle(dc, 0, 0, 480, 240);
			HPEN pen = CreatePen(PS_SOLID, 3, 0);
			pen = (HPEN)SelectObject(dc, pen);
			Rectangle(dc, 0, 0, 480, 240);
			pen = (HPEN)SelectObject(dc, pen);
			DeleteObject(pen);
			// 显示文字
			SetDCPenColor(dc, RGB(0xFF, 0xFF, 0xFF));
			TextOutW(dc, 10, 10, L"The overlay window is created by an application on your computer", 64);
			TextOutW(dc, 10, 30, L"that has disabled the close button of the window.", 49);
			TextOutW(dc, 10, 60, L"To close the overlay window, please see the main window for help.", 65);
			TextOutW(dc, 100, 180, L"Press any key to dismiss", 24);
			EndPaint(hwnd, &ps);
		});
		WINDOW_add_handler(WM_KEYUP, [this](EventData& ev) { close(); });
		WINDOW_add_handler(WM_NCHITTEST, [this](EventData& ev) { ev.returnValue(HTCAPTION); });
	}
};

void OverlayWindow::onSysMenu(EventData& ev) {
	if (ev.wParam == 0x1003) {
		OverlayWindowInfoWindow owi;
		owi.create();
		RECT rc{}; GetWindowRect(hwnd, &rc);
		owi.move_to(rc.left, rc.top);
		owi.show();
		owi.run(&owi);
	}
}




ns_end;
