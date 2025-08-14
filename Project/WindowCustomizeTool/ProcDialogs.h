#pragma once
#include <w32use.hpp>
#include "publicdef.h"
#include "resource.h"
ns_declare(WindowCustomizeToolV2_app);



LRESULT CALLBACK SwpDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SendMsgDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


ns_end;
