#include "MainWindow.h"
#include "resource.h"
#include "WindowChangeParent.h"
using namespace std;
using namespace WindowCustomizeToolV2_app;
using namespace WCTv2::winlib;



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