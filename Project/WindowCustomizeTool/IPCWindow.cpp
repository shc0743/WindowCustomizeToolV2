#include "IPCWindow.h"
#include "SettingsDialog.h"
#include "publicdef.h"

void WindowCustomizeToolV2_app::IPCWindow::openSettingsDialog(EventData& event) {
	if (app::setdlg) {
		if ((*app::setdlg) != NULL) {
			app::setdlg->show(1);
			app::setdlg->focus();
			return;
		}
		delete app::setdlg;
	}
	app::setdlg = new WindowCustomizeToolV2_app::SettingsDialog();
	app::setdlg->create();
	app::setdlg->center();
	app::setdlg->show();
	app::setdlg->focus();
}
