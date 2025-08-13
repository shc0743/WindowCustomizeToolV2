#include "WindowAlphaEditor.h"

ns_declare(WindowCustomizeToolV2_app);


void WindowAlphaEditor::onCreated() {
	set_topmost(true);
	add_style_ex(WS_EX_LAYERED | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(hwnd, 0, 0xF0, LWA_ALPHA);

	bigFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, L"Consolas");
	text = Static(hwnd, L"255", 1, 1, 0, 0, Static::STYLE | SS_CENTER | SS_CENTERIMAGE);
	text.create();
	text.resize(20, 10, 260, 25);
	text.font(bigFont);
	tb = TrackBar(hwnd, 290, 50);
	tb.create();
	tb.set_range(0, 255);
	tb.pos(255);
	tb.move_to(5, 60);
	tb.onbeforechange([this](EventData& ev) {
		NMTRBTHUMBPOSCHANGING* p = (NMTRBTHUMBPOSCHANGING*)ev.lParam;
		DWORD pos = p->dwPos;
		if ((GetWindowLongPtrW(target, GWL_EXSTYLE) & WS_EX_LAYERED) == 0) {
			SetWindowLongPtrW(target, GWL_EXSTYLE, GetWindowLongPtrW(target, GWL_EXSTYLE) | WS_EX_LAYERED);
		}
		SetLayeredWindowAttributes(target, 0, (BYTE)pos, LWA_ALPHA);
		text.text(std::to_wstring(pos));
	});
}

void WindowAlphaEditor::onDestroy() {
	if (bigFont) DeleteObject(bigFont);
}



ns_end;