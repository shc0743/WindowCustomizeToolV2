#pragma once

extern HINSTANCE hInst;
class MainWindow;
namespace WindowCustomizeToolV2_app { 
	class IPCWindow;
	class SettingsDialog;
};
namespace app {
	extern std::vector<Window*> win;
	extern WindowCustomizeToolV2_app::SettingsDialog* setdlg;
	extern TrayIcon icon;
	extern Menu menu;
	extern bool no_main_window;
	extern WindowCustomizeToolV2_app::IPCWindow* ipcwin;
	bool checkwin();
	Window& firstAliveWindow();
	MainWindow& firstAliveMainWindow();
	void create_win();
	void quit(bool soft = true);
};


#define ns_declare(name) namespace name {
#define ns_end  }

