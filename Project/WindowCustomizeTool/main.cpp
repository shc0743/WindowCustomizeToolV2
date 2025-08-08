#include <w32use.hpp>
#include "publicdef.h"
#include "MainWindow.h"
#include "SettingsDialog.h"
#include "IPCWindow.h"
using namespace std;
using namespace WindowCustomizeToolV2_app;

#pragma comment(linker, "\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")


HINSTANCE hInst;
namespace app {
	vector<Window*> win;
	SettingsDialog* setdlg;
	TrayIcon icon;
	Menu menu;
	WindowCustomizeToolV2_app::IPCWindow* ipcwin;
	bool no_main_window;
	bool checkwin() { return win.size() != 0; };
	Window& firstAliveWindow() {
		if (win.size() < 1) throw runtime_error("No windows available");
		for (auto& w : win) {
			if (w && IsWindow(*(w))) return *w;
		}
		throw runtime_error("No alive windows found");
	}
	MainWindow& firstAliveMainWindow() {
		if (win.size() < 1) throw runtime_error("No windows available");
		for (auto& w : win) {
			if (w && IsWindow(*(w))) {
				auto ptr = dynamic_cast<MainWindow*>(w);
				if (ptr) return *ptr;
			}
		}
		throw runtime_error("No alive windows found");
	}
	void create_win() {
		MainWindow* new_window = new MainWindow();
		new_window->create();
		new_window->center();
		new_window->show(1);
		new_window->text(new_window->text() +
			L" (" + to_wstring(win.size() + 1) + L")");
		new_window->focus();
		win.push_back((new_window));
	}
	void quit(bool soft) {
		bool ok = false;
		for (auto& w : win) {
			if (IsWindow(*w)) {
				w->post(WM_APP + WM_CLOSE);
				ok = true;
			}
		}
		if (!soft || !ok) PostQuitMessage(0);
	}
}


using namespace app;


int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
) {
	::hInst = hInstance;

	// 初始化 COM 库
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		MessageBox(NULL, L"COM 初始化失败", L"错误", MB_ICONERROR);
		return -1;
	}
	// 初始化公共控件库
	INITCOMMONCONTROLSEX icc{};
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_ALL_CLASSES;
	InitCommonControlsEx(&icc);
	// 设置全局选项，关闭所有窗口时退出应用程序
	Window::set_global_option(Window::Option_QuitWhenWindowAllClosed, true);
	Window::set_global_option(Window::Option_DebugMode, true);
	// 创建并显示主窗口
	try {
		// 如果已经有窗口，那么不单独创建进程
		if (HWND hWnd = FindWindowW(IPCWindow().get_class_name().c_str(), NULL)) {
			if (SendMessageTimeoutW(hWnd, WM_APP + WM_SHOWWINDOW,
				GetCurrentProcessId(), 0, SMTO_BLOCK, 2000, NULL)) {
				return 0;
			}
		}

		icon.setTooltip(L"Window Customize Tool V2");
		icon.onClick([](EventData& ev) {
			try {
				firstAliveMainWindow().show(1);
				firstAliveMainWindow().focus();
			}
			catch (runtime_error) {
				// 创建主窗口
				create_win();
			}
		});
		icon.onBalloonClick(icon.onClick());
		// 设置托盘图标菜单
		menu = Menu({
			MenuItem(L"打开主窗口 (&O)", 1, [] {
				firstAliveMainWindow().show(1);
				firstAliveMainWindow().focus();
			}),
			MenuItem(L"隐藏主窗口 (&H)", 3, [] {
				firstAliveMainWindow().hideMainWindow();
			}),
			MenuItem::separator(),
			MenuItem(L"新建窗口 (&N)", 4, [] {
				create_win();
			}),
			MenuItem(L"窗口列表 (&L)", 5, [] {
				// 获取仍然存在的窗口列表
				map<size_t, MainWindow*> alive_windows;
				size_t n = 0;
				for (auto& w : win) {
					++n;
					if (w && (*w != NULL) && (dynamic_cast<MainWindow*>(w))) {
						alive_windows.emplace(make_pair(
							n, dynamic_cast<MainWindow*>(w)));
					}
				}
				if (alive_windows.empty()) {
					MessageBoxTimeoutW(NULL, L"没有可用的窗口", L"错误",
						MB_ICONERROR, 0, 2000);
					return;
				}
				// 构造菜单
				Menu menu;
				for (auto& i : alive_windows) {
					menu.get_children().push_back(MenuItem(
						format(L"窗口 #{}", i.first), UINT(i.first), [i] {
						if (i.second && IsWindow(*(i.second))) {
							i.second->show(1);
							i.second->focus();
						}
					}));
				}
				menu.get_children().push_back(MenuItem::separator());
				menu.get_children().push_back(MenuItem(
					L"全部关闭", 999999, [&alive_windows] {
						for (auto& i : alive_windows) if (i.second) i.second->close();
				}));
				menu.pop();
			}),
			MenuItem::separator(),
			MenuItem(L"设置 (&S)", 6, [] {
				if (app::ipcwin) app::ipcwin->post(WM_APP + WM_SETTINGCHANGE);
			}),
			MenuItem::separator(),
			MenuItem(L"退出 (&X)", 2, [] {
				quit(false);
			}),
		});
		icon.setMenu(&menu);
		menu.set_exception_handler([](const exception& exc) {
			if (exc.what() == string("No windows available") ||
				exc.what() == string("No alive windows found")) {
				MessageBoxTimeoutW(NULL, L"没有可用的窗口", L"错误",
					MB_ICONERROR, 0, 2000);
			}
			else MessageBoxTimeoutW(NULL, (w32oop::util::str::converts::str_wstr
				(exc.what()) + L"\n\n" + ErrorChecker().message())
				.c_str(), L"Fatal Error", MB_ICONERROR, 0, 2000);
			return true; // 表示异常已被处理
		});

		if (wstring(lpCmdLine).find(L"--no-main-window") == wstring::npos) {
			MainWindow* main_window = new MainWindow();
			win.push_back(main_window);
			icon.setIcon(main_window->get_window_icon());
			main_window->create();
			main_window->center();
			main_window->show();
			main_window->text(main_window->text() + L" (1)");
		}
		else {
			no_main_window = true;
			icon.setIcon(MainWindow().get_window_icon());
		}

		IPCWindow ipcWin;
		ipcWin.create();
		::app::ipcwin = &ipcWin;

		Window::set_global_option(Window::Option_EnableHotkey, true);
		int ret = Window::run();
		while (ret == 0x121234) ret = Window::run(); // 再次运行消息循环
		// 释放内存
		for (auto& w : win) {
			if (w) delete w;
		}
		win.clear();

		// 返回
		return ret;
	}
	catch (exception& exc) {
		MessageBoxW(NULL, (w32oop::util::str::converts::str_wstr(exc.what())
			+ L"\n\n" + ErrorChecker().message()).c_str(), L"Fatal Error", MB_ICONERROR);
		return -1;
	}
}
