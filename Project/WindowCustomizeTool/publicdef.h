#pragma once

extern HINSTANCE hInst;
class MainWindow;
namespace app {
	extern std::vector<Window*> win;
	extern TrayIcon icon;
	extern Menu menu;
	extern bool no_main_window;
	bool checkwin();
	Window& firstAliveWindow();
	MainWindow& firstAliveMainWindow();
	void create_win();
	void quit(bool soft = true);
};


#define ns_declare(name) namespace name {
#define ns_end  }

