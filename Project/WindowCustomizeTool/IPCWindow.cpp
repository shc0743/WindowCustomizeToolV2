#include "IPCWindow.h"
#include "SettingsDialog.h"
#include "publicdef.h"

ns_declare(WindowCustomizeToolV2_app);



void IPCWindow::onCreated() {
	SetTimer(hwnd, 2, 10000, NULL);

}


void IPCWindow::onDestroy() {
	KillTimer(hwnd, 2);

}


void IPCWindow::openSettingsDialog(EventData& event) {
	if (app::setdlg) {
		if ((*app::setdlg) != NULL) {
			app::setdlg->show(1);
			app::setdlg->focus();
			return;
		}
		delete app::setdlg;
	}
	app::setdlg = new SettingsDialog();
	app::setdlg->create();
	app::setdlg->center();
	app::setdlg->show();
	app::setdlg->focus();
}


void IPCWindow::onTimer(EventData& event) {
	switch (event.wParam) {
	case 2:
		app::save_config(); // 定时保存配置
		break;
	default:
		return;
	}
	event.returnValue(0);
}



ns_end;
