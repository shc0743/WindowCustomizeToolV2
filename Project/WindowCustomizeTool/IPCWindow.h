#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class IPCWindow : public Window {
public:
	IPCWindow() : Window(L"IPC Window", 1, 1) {}

protected:
	void onCreated() override;
	void onDestroy() override;

private:
	void openSettingsDialog(EventData& event);
	void onTimer(EventData& event);

protected:
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_CLOSE, [this](EventData& ev) { ev.preventDefault(); });
		WINDOW_add_handler(WM_APP + WM_SHOWWINDOW, [this](EventData&) { app::create_win(); });
		WINDOW_add_handler(WM_APP + WM_SETTINGCHANGE, openSettingsDialog);
		WINDOW_add_handler(WM_TIMER, onTimer);
		UINT WM_TaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));
		WINDOW_add_handler(WM_TaskbarCreated, [this](EventData& ev) {
			ev.preventDefault();
			app::icon.add();
		});
	}
};



ns_end;
