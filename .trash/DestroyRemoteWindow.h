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

#include <Windows.h>
#include <ntstatus.h>
#include <string>

namespace siapi {
using namespace std;

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


/**
 * Gets the address of a procedure in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param FileName The file name of the DLL containing the procedure.
 * \param ProcedureName The name or ordinal of the procedure.
 * \param ProcedureAddress A variable which receives the address of the procedure in the address
 * space of the process.
 * \param DllBase A variable which receives the base address of the DLL containing the procedure.
 */
NTSTATUS PhGetProcedureAddressRemote(
	_In_ HANDLE ProcessHandle,
	_In_ PCWSTR FileName,
	_In_ PCSTR ProcedureName,
	_Out_ PVOID* ProcedureAddress,
	_Out_opt_ PVOID* DllBase
);


constexpr inline NTSTATUS NT_SUCCESS(NTSTATUS Status) {
	return ((NTSTATUS)(Status)) >= 0;
}
inline wstring PH_STRINGREF_INIT(PCWSTR str) { return str; }


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


typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PLDR_INIT_ROUTINE EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	union
	{
		UCHAR FlagGroup[4];
		ULONG Flags;
		struct
		{
			ULONG PackagedBinary : 1;
			ULONG MarkedForRemoval : 1;
			ULONG ImageDll : 1;
			ULONG LoadNotificationsSent : 1;
			ULONG TelemetryEntryProcessed : 1;
			ULONG ProcessStaticImport : 1;
			ULONG InLegacyLists : 1;
			ULONG InIndexes : 1;
			ULONG ShimDll : 1;
			ULONG InExceptionTable : 1;
			ULONG ReservedFlags1 : 2;
			ULONG LoadInProgress : 1;
			ULONG LoadConfigProcessed : 1;
			ULONG EntryProcessed : 1;
			ULONG ProtectDelayLoad : 1;
			ULONG ReservedFlags3 : 2;
			ULONG DontCallForThreads : 1;
			ULONG ProcessAttachCalled : 1;
			ULONG ProcessAttachFailed : 1;
			ULONG CorDeferredValidate : 1;
			ULONG CorImage : 1;
			ULONG DontRelocate : 1;
			ULONG CorILOnly : 1;
			ULONG ChpeImage : 1;
			ULONG ChpeEmulatorImage : 1;
			ULONG ReservedFlags5 : 1;
			ULONG Redirected : 1;
			ULONG ReservedFlags6 : 2;
			ULONG CompatDatabaseProcessed : 1;
		};
	};
	USHORT ObsoleteLoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	ULONG TimeDateStamp;
	PACTIVATION_CONTEXT EntryPointActivationContext;
	PVOID Lock; // RtlAcquireSRWLockExclusive
	PLDR_DDAG_NODE DdagNode;
	LIST_ENTRY NodeModuleLink;
	PLDRP_LOAD_CONTEXT LoadContext;
	PVOID ParentDllBase;
	PVOID SwitchBackContext;
	RTL_BALANCED_NODE BaseAddressIndexNode;
	RTL_BALANCED_NODE MappingInfoIndexNode;
	PVOID OriginalBase;
	LARGE_INTEGER LoadTime;
	ULONG BaseNameHashValue;
	LDR_DLL_LOAD_REASON LoadReason; // since WIN8
	ULONG ImplicitPathOptions;
	ULONG ReferenceCount; // since WIN10
	ULONG DependentLoadFlags;
	UCHAR SigningLevel; // since REDSTONE2
	ULONG CheckSum; // since 22H1
	PVOID ActivePatchImageBase;
	LDR_HOT_PATCH_STATE HotPatchState;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
/**
 * A callback function passed to PhEnumProcessModules() and called for each process module.
 *
 * \param Module A structure providing information about the module.
 * \param Context A user-defined value passed to PhEnumProcessModules().
 *
 * \return TRUE to continue the enumeration, FALSE to stop.
 */
typedef _Function_class_(PH_ENUM_PROCESS_MODULES_CALLBACK)
BOOLEAN NTAPI PH_ENUM_PROCESS_MODULES_CALLBACK(
	_In_ PLDR_DATA_TABLE_ENTRY Module,
	_In_opt_ PVOID Context
);
typedef PH_ENUM_PROCESS_MODULES_CALLBACK* PPH_ENUM_PROCESS_MODULES_CALLBACK;



using PH_STRING = wstring; using PPH_STRING = PH_STRING*;

typedef struct _PH_MAPPED_IMAGE
{
	USHORT Signature;
	PVOID ViewBase;
	SIZE_T ViewSize;

	union
	{
		struct // PE image
		{
			union
			{
				PIMAGE_NT_HEADERS32 NtHeaders32;
				PIMAGE_NT_HEADERS64 NtHeaders64;
				PIMAGE_NT_HEADERS NtHeaders;
			};

			USHORT Magic;
			USHORT NumberOfSections;
			PIMAGE_SECTION_HEADER Sections;
		};

		struct // ELF image
		{
			struct _ELF_IMAGE_HEADER* Header;
			union
			{
				struct _ELF_IMAGE_HEADER32* Headers32;
				struct _ELF_IMAGE_HEADER64* Headers64;
			};
		};
	};
} PH_MAPPED_IMAGE, * PPH_MAPPED_IMAGE;

typedef struct _PH_MAPPED_IMAGE_EXPORTS
{
	PPH_MAPPED_IMAGE MappedImage;
	ULONG NumberOfEntries;

	PIMAGE_DATA_DIRECTORY DataDirectory;
	IMAGE_DATA_DIRECTORY DataDirectoryARM64X;
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	PULONG AddressTable;
	PULONG NamePointerTable;
	PUSHORT OrdinalTable;
} PH_MAPPED_IMAGE_EXPORTS, * PPH_MAPPED_IMAGE_EXPORTS;

typedef class _PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT
{
public:
	PVOID DllBase;
	PPH_STRING FileName;
} PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT, * PPH_PROCEDURE_ADDRESS_REMOTE_CONTEXT;

typedef struct _PH_ENUM_PROCESS_MODULES_PARAMETERS
{
	PPH_ENUM_PROCESS_MODULES_CALLBACK Callback;
	PVOID Context;
	ULONG Flags;
} PH_ENUM_PROCESS_MODULES_PARAMETERS, * PPH_ENUM_PROCESS_MODULES_PARAMETERS;

NTSTATUS PhLoadMappedImageEx(
	_In_opt_ PCWSTR FileName,
	_In_opt_ HANDLE FileHandle,
	_Out_ PPH_MAPPED_IMAGE MappedImage
);
NTSTATUS PhMapViewOfEntireFileEx(
	_In_opt_ PCWSTR FileName,
	_In_opt_ HANDLE FileHandle,
	_Out_ PVOID* ViewBase,
	_Out_ PSIZE_T ViewSize
);


inline NTSTATUS NtClose(HANDLE Handle) {
	HMODULE _ntdll = GetModuleHandleW(L"ntdll.dll");
	if (!_ntdll) return -1;
	typedef NTSTATUS(__stdcall* _NtClose_t)(HANDLE);
	_NtClose_t NtClose = (_NtClose_t)GetProcAddress(_ntdll, "NtClose");
	if (!NtClose) return -1;
	return NtClose(Handle);
}


}; // namespace siapi

