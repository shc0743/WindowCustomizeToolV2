#include "ProcDialogs.h"
#include "winlib.hpp"
#include <fstream>
using namespace std;
ns_declare(WindowCustomizeToolV2_app);




LRESULT SwpDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
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

LRESULT SendMsgDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static map<wstring, UINT> msg_map;
	switch (message) {
	case WM_INITDIALOG:
		if (!lParam) return FALSE;
		SetPropW(hDlg, L"target", (HWND)lParam);
		SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_TARGET, format(L"当前窗口: 0x{:X}", (ULONG_PTR)lParam).c_str());
		SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_TOT, L"5000");
		SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_MSG, L"0");
		SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_WPARAM, L"0");
		SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_LPARAM, L"0");
		do {
			// 尝试获取消息列表关联。
			WCHAR appName[1024]{}; GetModuleFileNameW(hInst, appName, 1024);
			auto attr = GetFileAttributesW((appName + L".window-message-data.ini"s).c_str());
			if (attr != FILE_ATTRIBUTE_DIRECTORY && attr != INVALID_FILE_ATTRIBUTES) {
				PostMessage(hDlg, WM_USER + 3, 0, 0);
			}
			else try {
				int pref = app::config.get_or("app.send_message_dialog.wm_data.pref", (int)0);
				if (pref == 1) break;
				int user = 0;
				TaskDialog(hDlg, NULL, L"资源下载", L"需要下载额外的资源以获得更好的用户体验。",
					L"资源名称: window-message-data.ini\n资源用途: 在“发送消息”对话框中提供更好的用户体验。\n\n"
					L"是否立即下载？\n[是] -- 下载\n[否] -- 暂不下载\n[取消] -- 不再提醒",
					TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, &user);
				if (user == IDCANCEL) app::config["app.send_message_dialog.wm_data.pref"] = 1;
				if (user != IDYES) break;
				// 下载资源
				EnableWindow(hDlg, FALSE);
				HDC dc = GetDC(NULL);
				RECT rc{}; GetWindowRect(hDlg, &rc);
				TextOutW(dc, rc.left, rc.top, L"正在下载资源，请稍候...", 13);
				ReleaseDC(NULL, dc);
				thread([hDlg](wstring appName) {
					auto exec = [hDlg](wstring c) {
						DWORD err = exec_app(c, SW_HIDE);
						if (err == 0) return 0;
						TaskDialog(hDlg, NULL, L"无法完成下载", L"无法完成资源下载。",
							format(L"对外部程序的调用返回了错误:\n命令行 = {}\n错误代码 = {}", c, err).c_str(),
							TDCBF_CANCEL_BUTTON, TD_ERROR_ICON, NULL);
						return 1;
						};
					auto quickread = [](wstring file) -> wstring {
						HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE) return L"";
						CHAR buffer[2048]{}; DWORD r{};
						if (!ReadFile(hFile, buffer, 2048, &r, NULL)) {
							CloseHandle(hFile); return L"";
						}
						CloseHandle(hFile);
						return str_wstr(buffer);
						};
					WCHAR dir[1024]{}; wstring cd;
					GetCurrentDirectoryW(1024, dir); cd = dir;
					GetTempPathW(1024, dir); SetCurrentDirectoryW(dir);
					w32oop::util::RAIIHelper r([cd, hDlg] {
						SetCurrentDirectoryW(cd.c_str());
						EnableWindow(hDlg, TRUE);
						});
					if (exec(L"curl https://index.updates.clspd.top/download/WindowCustomizeTool_v2/10 -o wctlib.wm.data")) return;
					wstring dl_url = quickread(L"wctlib.wm.data");
					if (exec(L"curl " + dl_url + L" -o \"" + appName + L".window-message-data.ini")) return;
					DeleteFileW(L"wctlib.wm.data");
					MessageBoxW(hDlg, L"下载完成。", L"资源下载", MB_ICONINFORMATION);
					PostMessage(hDlg, WM_USER + 3, 0, 0);
					}, appName).detach();
			}
			catch (...) { break; }
		} while (0);
		break;
	case WM_USER + 3: {
		HWND cb = GetDlgItem(hDlg, IDC_SENDMESSAGE_MSG);
		if (msg_map.size() == 0) {
			WCHAR appName[1024]{}; GetModuleFileNameW(hInst, appName, 1024);
			char buf[256]{};
			if (fstream fp = fstream(wstr_str(appName) + ".window-message-data.ini", ios::in)) {
				while (fp.getline(buf, 156)) {
					if (buf[0] == 0 || buf[0] == ' ' || buf[0] == '\r' || buf[0] == '\t') continue;
					vector<string> container;
					w32oop::util::str::operations::split(buf, "\t", container);
					msg_map.emplace(str_wstr(container[0]), (UINT)atol(container[1].c_str()));
				}
			}
			else {
				SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)L"(资源文件加载失败。)");
			}
		}
		for (const auto& i : msg_map) {
			SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)i.first.c_str());
		}
	}
					break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDABORT:
			ShellExecuteW(NULL, L"open", L"https://learn.microsoft.com/en-us/windows/"
				"win32/api/winuser/nf-winuser-sendmessagetimeoutw", NULL, NULL, SW_NORMAL);
			break;
		case IDOK:
		{
			DWORD timeout = GetDlgItemInt(hDlg, IDC_SENDMESSAGE_TOT, NULL, FALSE);
			using UserInput = WCHAR[256];
			UserInput szMsg{}, szWp{}, szLp{};
			GetDlgItemTextW(hDlg, IDC_SENDMESSAGE_MSG, szMsg, 256);
			GetDlgItemTextW(hDlg, IDC_SENDMESSAGE_WPARAM, szWp, 256);
			GetDlgItemTextW(hDlg, IDC_SENDMESSAGE_LPARAM, szLp, 256);
			// 转换MSG
			UINT message = 0;
			if ((szMsg[0] == L'0' && (szMsg[1] == L'x' || szMsg[1] == L'X'))) {
				message = wcstoul(szMsg + 2, nullptr, 16);
				if (message == 0 && (!(szMsg[2] == L'0' && szMsg[3] == 0))) {
					// 用户输入的并不是 0 ，可却转换得到了 0
					// 说明用户输入有误
					MessageBoxW(hDlg, L"输入不是有效的十六进制数。", NULL, MB_ICONERROR);
					break;
				}
			}
			// 检查是否是纯数字
			else if (std::all_of(szMsg, szMsg + wcslen(szMsg), [](wchar_t c) { return iswdigit(c); })) {
				message = wcstoul(szMsg, nullptr, 10);
			}
			// 检查是否在消息映射表中
			else {
				if (msg_map.contains(szMsg)) {
					message = msg_map.at(szMsg);
				}
				else {
					MessageBoxW(hDlg, L"无效的消息值！请输入有效的消息名称、十进制或十六进制值。", L"错误", MB_ICONERROR);
					break;
				}
			}
			// 转换 WPARAM
			WPARAM wp = 0; LPARAM lp = 0;
			if (szWp[0] == L'0' && (szWp[1] == L'x' || szWp[1] == L'X'))
				wp = wcstoul(szWp + 2, nullptr, 16);
			else wp = wcstoul(szWp, nullptr, 10);
			// 转换 LPARAM
			if (SendMessage(GetDlgItem(hDlg, IDC_SENDMESSAGE_LPSTR), BM_GETCHECK, 0, 0) & BST_CHECKED) {
				lp = reinterpret_cast<LPARAM>(szLp); // 作为字符串
			}
			else if (szLp[0] == L'0' && (szLp[1] == L'x' || szLp[1] == L'X')) {
				lp = wcstoull(szLp + 2, nullptr, 16);
			}
			else lp = wcstoull(szLp, nullptr, 10);
			// 发送消息
			HWND hWnd = (HWND)GetPropW(hDlg, L"target");
			DWORD_PTR result = 0;
			auto success = SendMessageTimeoutW(hWnd, message, wp, lp,
				SMTO_ABORTIFHUNG, (std::min)(DWORD(5000), timeout), &result);
			if (success) SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_RESULT, format(L"成功: 返回值 0x{:X}", result).c_str());
			else SetDlgItemTextW(hDlg, IDC_SENDMESSAGE_RESULT, format(L"失败: {} 返回值 0x{:X}", ErrorChecker().message(), result).c_str());
			MessageBeep(success ? MB_ICONINFORMATION : MB_ICONERROR);
			break;
		}
		case IDCANCEL:
			DestroyWindow(hDlg);
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




ns_end;