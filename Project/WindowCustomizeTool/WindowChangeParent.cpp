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
	apply = Button(hwnd, L"设置", 180, 30, 10, 160, 0, Button::STYLE | BS_SPLITBUTTON);
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
	apply.on(BCN_DROPDOWN, [this](EventData& ev) {
		RECT rc{};
		GetWindowRect(apply, &rc);
		Menu({
			MenuItem(L"窗口向上一级", 1, [this] { HWND hParent = GetParent(tp); if (hParent) update_target(hParent); }),
			MenuItem(L"手动输入 HWND", 2, [this] {
				InputDialog idd(L"输入 HWND"); idd.create(); idd.center(hwnd);
				idd.setButtonsText(L"确定", L"取消");
				auto value = idd.getInput<ULONG_PTR>(L"请输入 HWND 的值。", (ULONG_PTR)(tp ? tp : GetParent(target)));
				if (value.has_value()) update_target((HWND)(ULONG_PTR)value.value());
			}),
		}).pop(rc.left, rc.bottom);
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