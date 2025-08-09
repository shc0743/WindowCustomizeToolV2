#include "OverlayWindow.h"
ns_declare(WindowCustomizeToolV2_app);


void OverlayWindow::onCreated() {
	add_style_ex(WS_EX_LAYERED | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(hwnd, 0, 0x80, LWA_ALPHA); // 半透明
	set_topmost(true); // 置顶
}



ns_end;
