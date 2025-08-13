#include "WindowChangeParent.h"
ns_declare(WindowCustomizeToolV2_app);



void WindowChangeParent::onCreated() {
	set_topmost(true);
	add_style_ex(WS_EX_LAYERED | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(hwnd, 0, 0xF0, LWA_ALPHA);

	tip = Static(hwnd, L"正在设置父窗口 (Not initialized)", 360, 20, 10, 10, Static::STYLE | SS_CENTER);
	oldParentText = Static(hwnd, L"未初始化", 360, 20, 10, 40);
	finder.set_parent(hwnd);
	finder.onselectstart([this] {
	});
	finder.onselecting([this](HWND h) {
		update_target(h);
	});
	finder.onselectend([this](HWND h) {
		update_target(h);
	});
	finder.create(); finder.move_to(190 - 16, 70);
	newParentText = Static(hwnd, L"请选取新的父窗口，或直接点击“设置”以分离窗口", 360, 20, 10, 130, Static::STYLE | SS_CENTER);
	apply = Button(hwnd, L"设置", 180, 30, 10, 160);
	cancel = Button(hwnd, L"取消", 180, 30, 195, 160);

	tip.create(); oldParentText.create(); newParentText.create();
	apply.create(); cancel.create();

	apply.onClick([this](EventData&) {
		bool ok = SetParent(target, tp);
		if (!ok) {
			MessageBoxW(hwnd, ErrorChecker().message().c_str(), L"操作失败", MB_ICONERROR);
		}
		else close();
	});
	cancel.onClick([this](EventData&) {
		close();
	});

	register_hot_key(false, false, false, VK_ESCAPE, [this](HotKeyProcData& ev) {
		ev.preventDefault();
		close();
	}, HotKeyOptions::Windowed);
}



ns_end;