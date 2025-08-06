#include <w32use.hpp>
#include "MainWindow.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")


HINSTANCE hInst;


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
	// 创建并显示主窗口

	MainWindow mainWindow;
    mainWindow.create();
    mainWindow.set_main_window();
    mainWindow.center();
    mainWindow.show();
    return mainWindow.run();
}
