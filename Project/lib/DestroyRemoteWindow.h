#pragma once
/*
Important: This file comes from System Informer project.
System Informer: https://github.com/winsiderss/systeminformer
License: GPL-3.0
Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.

Perm links:
https://github.com/winsiderss/systeminformer/blob/c030ded1762546b5a36ffcc471f2a69b08915365/plugins/WindowExplorer/wnddlg.c#L690
https://github.com/winsiderss/systeminformer/blob/c030ded1762546b5a36ffcc471f2a69b08915365/phlib/nativethread.c#L3029
*/

#define WIN32_NO_STATUS 1
#include <Windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <string>

namespace siapi {
using namespace std;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;


BOOL WINAPI DestroyRemoteWindow(
	_In_ HWND TargetWindow,
	_In_ DWORD ProcessId
);



}; // namespace siapi

