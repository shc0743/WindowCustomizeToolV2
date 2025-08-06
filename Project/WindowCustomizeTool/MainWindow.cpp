#include "MainWindow.h"


void MainWindow::onCreated() {
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)0xee, LWA_ALPHA);

}
