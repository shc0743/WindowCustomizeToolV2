#include "DestroyRemoteWindow.h"
#include "ph.h"
#include "phlib/include/apiimport.h"
#include "phlib/include/mapldr.h"
#include <VersionHelpers.h>
/*
Important: This file comes from System Informer project.
System Informer: https://github.com/winsiderss/systeminformer
License: GPL-3.0

For perm links please see the .h file.
*/
using namespace siapi;


// edited, raw func: WepDestroyRemoteWindow
BOOL __stdcall siapi::DestroyRemoteWindow(
	_In_ HWND TargetWindow,
	_In_ DWORD ProcessId
) {
	NTSTATUS status = -1;
	HANDLE processHandle = NULL;

	// Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
	//(PhWindowsVersion >= WINDOWS_8 && PhWindowsVersion <= WINDOWS_8_1)
	if (IsWindows8OrGreater() == true && IsWindowsThresholdOrGreater() == false)
	{
		processHandle = OpenProcess(
			PROCESS_ALL_ACCESS,
			0,
			ProcessId
		);
	}

	// Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
	if (!processHandle)
	{
		processHandle = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
			PROCESS_VM_READ | SYNCHRONIZE,
			0,
			ProcessId
		);
	}

	if (processHandle)
	{
		status = PhDestroyWindowRemote(
			processHandle,
			TargetWindow
		);

		CloseHandle(processHandle);
	}

	return NT_SUCCESS(status);
}



