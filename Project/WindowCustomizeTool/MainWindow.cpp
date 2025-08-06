#include "MainWindow.h"


void MainWindow::onCreated() {
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)0xee, LWA_ALPHA);

}

void MainWindow::onClose(EventData& ev) {
	remove_style_ex(WS_EX_LAYERED); // 要使得窗口有正常的关闭动画，窗口不能是Layered窗口
}
