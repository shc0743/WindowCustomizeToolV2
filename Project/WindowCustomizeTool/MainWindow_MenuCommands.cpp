#include "../lib/DestroyRemoteWindow.h"
#include "MainWindow.h"
#include "IPCWindow.h"
#include "resource.h"
#include "ProcDialogs.h"
#include "../version_number.h"
using namespace std;
using namespace WindowCustomizeToolV2_app;
using namespace WCTv2::winlib;


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
		DialogBoxParamW(hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hwnd, (
			[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)->LRESULT {
				switch (message) {
					case WM_INITDIALOG:
						SetDlgItemTextW(hDlg, IDIGNORE, format(L"内部版本号: {}", app::version).c_str());
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
		), 0);
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
	case ID_MENU_WINDOWMANAGER_SENDMESSAGE:
	{
		if (!IsWindow(target)) return wop_report_result(false, ERROR_INVALID_WINDOW_HANDLE);
		HWND hDlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_DIALOG_SENDMESSAGE), hwnd, SendMsgDlgHandler, (LPARAM)target);
		if (!hDlg) return wop_report_result(false);
		sbr.set_text(0, current_time() + L" 消息发送器已打开。");
		SetForegroundWindow(hDlg);
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

