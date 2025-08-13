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

#include <ntstatus.h>
#include <Windows.h>
#include <string>

namespace siapi {
using namespace std;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;


BOOL WINAPI DestroyRemoteWindow(
	_In_ HWND TargetWindow,
	_In_ DWORD ProcessId
);

/**
 * Destroys the specified window in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_LIMITED_INFORMATION access.
 * \param WindowHandle A handle to the window to be destroyed.
 *
 * \return Successful or errant status.
 *
 * \remarks A thread cannot call DestroyWindow for a window created by a different thread,
 * unless we queue a special APC to the owner thread.
 */
NTSTATUS PhDestroyWindowRemote(
	_In_ HANDLE ProcessHandle,
	_In_ HWND WindowHandle
);


typedef class _PH_PROCESS_RUNTIME_LIBRARY
{
public:
	wstring NtdllFileName;
	wstring Kernel32FileName;
	wstring User32FileName;
} PH_PROCESS_RUNTIME_LIBRARY, * PPH_PROCESS_RUNTIME_LIBRARY;

NTSTATUS PhGetProcessRuntimeLibrary(
	_In_ HANDLE ProcessHandle,
	_Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
	_Out_opt_ PBOOLEAN IsWow64Process
);

constexpr inline NTSTATUS NT_SUCCESS(NTSTATUS Status) {
	return ((NTSTATUS)(Status)) >= 0;
}
//inline wstring PH_STRINGREF_INIT(PCWSTR str) { return str; }


using PROCESSINFOCLASS = PROCESS_INFORMATION_CLASS;
static constexpr PROCESSINFOCLASS ProcessWow64Information = (PROCESSINFOCLASS)26;
using NtQueryInformationProcess_t = NTSTATUS(NTAPI*)(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength);

inline NTSTATUS NTAPI NtQueryInformationProcess(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
) {
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (!ntdll) return -1;
	auto _Func = (NtQueryInformationProcess_t)GetProcAddress(ntdll, "NtQueryInformationProcess");
	if (!_Func) return -1;
	return _Func(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}



/**
 * Gets whether a process is running under 32-bit emulation.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IsWow64Process A variable which receives a boolean indicating whether the process is 32-bit.
 */
 //FORCEINLINE
NTSTATUS
PhGetProcessIsWow64(
	_In_ HANDLE ProcessHandle,
	_Out_ PBOOLEAN IsWow64Process
);


}; // namespace siapi

