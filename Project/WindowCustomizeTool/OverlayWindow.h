#pragma once
#include <w32use.hpp>
#include <commctrl.h>
#include "publicdef.h"
#include "resource.h"

ns_declare(WindowCustomizeToolV2_app);


class OverlayWindow : public Window {
public:
	OverlayWindow() : Window(L"Overlay Window", 1, 1, 0, 0, WS_POPUP) {}

protected:
	void onCreated() override;

public:
	virtual const COLORREF get_window_background_color() const override {
		return RGB(0, 0, 0);
	}
protected:
	virtual void setup_event_handlers() override {

	}
};



ns_end;
