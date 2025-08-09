#pragma once
#include "../lib/configuru.hpp"
#include <w32use.hpp>
#include <ShlObj.h>
#include <Shlwapi.h>

using namespace w32oop::util::str::converts;

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
	extern bool is_portable;
	extern WindowCustomizeToolV2_app::IPCWindow* ipcwin;
	bool checkwin();
	Window& firstAliveWindow();
	MainWindow& firstAliveMainWindow();
	void create_win();
	void quit(bool soft = true);
	int load_config(std::wstring cfg_path);
	void save_config();
};
namespace app {
	extern configuru::Config config;
};

inline bool file_exists(std::wstring file) noexcept {
	return (GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES);
}


#define ns_declare(name) namespace name {
#define ns_end  }

