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
	virtual void setup_event_handlers() override {
		WINDOW_add_handler(WM_CLOSE, [this](EventData& ev) { ev.preventDefault(); });
		WINDOW_add_handler(WM_APP + WM_SHOWWINDOW, [this](EventData&) { app::create_win(); });
		UINT WM_TaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));
		WINDOW_add_handler(WM_TaskbarCreated, [this](EventData& ev) {
			ev.preventDefault();
			app::icon.add();
		});
	}
};



ns_end;
