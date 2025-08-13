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

using siapi::NTSTATUS;
using siapi::PVOID;


NTSTATUS siapi::PhDestroyWindowRemote(
	_In_ HANDLE ProcessHandle,
	_In_ HWND WindowHandle
) {
	NTSTATUS status;
	PVOID destroyWindow = NULL;
	PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

	status = PhGetProcessRuntimeLibrary(
		ProcessHandle,
		&runtimeLibrary,
		NULL
	);

	if (!NT_SUCCESS(status))
		return status;

	status = PhGetProcedureAddressRemote(
		ProcessHandle,
		runtimeLibrary->User32FileName.c_str(),
		"DestroyWindow",
		&destroyWindow,
		NULL
	);

	if (!NT_SUCCESS(status))
		goto CleanupExit;

	status = PhInvokeWindowProcedureRemote(
		WindowHandle,
		destroyWindow,
		(PVOID)WindowHandle,
		NULL,
		NULL
	);

CleanupExit:
	return status;
}



// The following are ALL extra dependencies.
// What the xxxk?



NTSTATUS siapi::PhGetProcessRuntimeLibrary(
	_In_ HANDLE ProcessHandle,
	_Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
	_Out_opt_ PBOOLEAN IsWow64Process
)
{
	static PH_PROCESS_RUNTIME_LIBRARY NativeRuntime =
	{
		PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\kernel32.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\user32.dll"),
	};
#ifdef _WIN64
	static PH_PROCESS_RUNTIME_LIBRARY Wow64Runtime =
	{
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\ntdll.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\kernel32.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\user32.dll"),
	};
#ifdef _M_ARM64
	static PH_PROCESS_RUNTIME_LIBRARY Arm32Runtime =
	{
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\ntdll.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\kernel32.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\user32.dll"),
	};
	static PH_PROCESS_RUNTIME_LIBRARY Chpe32Runtime =
	{
		PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\ntdll.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\kernel32.dll"),
		PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\user32.dll"),
	};
#endif
#endif

	* RuntimeLibrary = &NativeRuntime;

	if (IsWow64Process)
		*IsWow64Process = FALSE;

#ifdef _WIN64
	NTSTATUS status;
#ifdef _M_ARM64
	USHORT machine;

	status = PhGetProcessArchitecture(ProcessHandle, &machine);

	if (!NT_SUCCESS(status))
		return status;

	if (machine != IMAGE_FILE_MACHINE_TARGET_HOST)
	{
		switch (machine)
		{
		case IMAGE_FILE_MACHINE_I386:
		case IMAGE_FILE_MACHINE_CHPE_X86:
		{
			*RuntimeLibrary = &Chpe32Runtime;

			if (IsWow64Process)
				*IsWow64Process = TRUE;
		}
		break;
		case IMAGE_FILE_MACHINE_ARMNT:
		{
			*RuntimeLibrary = &Arm32Runtime;

			if (IsWow64Process)
				*IsWow64Process = TRUE;
		}
		break;
		case IMAGE_FILE_MACHINE_AMD64:
		case IMAGE_FILE_MACHINE_ARM64:
			break;
		default:
			return STATUS_INVALID_PARAMETER;
		}
	}
#else
	siapi::BOOLEAN isWow64 = FALSE;

	status = PhGetProcessIsWow64(ProcessHandle, &isWow64);

	if (!NT_SUCCESS(status))
		return status;

	if (isWow64)
	{
		*RuntimeLibrary = &Wow64Runtime;

		if (IsWow64Process)
			*IsWow64Process = TRUE;
	}
#endif
#endif

	return STATUS_SUCCESS;
}

NTSTATUS siapi::PhGetProcessIsWow64(HANDLE ProcessHandle, PBOOLEAN IsWow64Process)
{
	NTSTATUS status;
	ULONG_PTR wow64 = 0;

	status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessWow64Information,
		&wow64,
		sizeof(ULONG_PTR),
		NULL
	);

	if (NT_SUCCESS(status))
	{
		*IsWow64Process = !!wow64;
	}
	else
	{
		*IsWow64Process = FALSE;
	}

	return status;
}

