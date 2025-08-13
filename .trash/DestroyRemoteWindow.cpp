#include "DestroyRemoteWindow.h"
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
	BOOLEAN isWow64 = FALSE;

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


NTSTATUS siapi::PhGetProcedureAddressRemote(
	_In_ HANDLE ProcessHandle,
	_In_ PCWSTR FileName,
	_In_ PCSTR ProcedureName,
	_Out_ PVOID* ProcedureAddress,
	_Out_opt_ PVOID* DllBase
)
{
	NTSTATUS status;
	PPH_STRING fileName = NULL;
	PH_MAPPED_IMAGE mappedImage;
	PH_MAPPED_IMAGE_EXPORTS exports;
	PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT context;
	PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;
#ifdef _M_ARM64
	USHORT processArchitecture;
	ULONG exportsFlags;
#endif

	status = PhLoadMappedImageEx(
		FileName,
		NULL,
		&mappedImage
	);

	if (!NT_SUCCESS(status))
		return status;

	fileName = PhDosPathNameToNtPathName(FileName);

	if (PhIsNullOrEmptyString(fileName))
	{
		status = PhGetProcessMappedFileName(
			NtCurrentProcess(),
			mappedImage.ViewBase,
			&fileName
		);

		if (!NT_SUCCESS(status))
			goto CleanupExit;
	}

	memset(&context, 0, sizeof(PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT));
	context.FileName = fileName;

	status = PhEnumProcessModulesLimited(
		ProcessHandle,
		PhpGetProcedureAddressRemoteLimitedCallback,
		&context
	);

	if (!context.DllBase)
	{
		memset(&parameters, 0, sizeof(PH_ENUM_PROCESS_MODULES_PARAMETERS));
		parameters.Callback = PhpGetProcedureAddressRemoteCallback;
		parameters.Context = &context;
		parameters.Flags = PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME;

		switch (mappedImage.Magic)
		{
		case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			status = PhEnumProcessModules32Ex(ProcessHandle, &parameters);
			break;
		case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			status = PhEnumProcessModulesEx(ProcessHandle, &parameters);
			break;
		}

		if (!NT_SUCCESS(status))
			goto CleanupExit;
	}

	if (!context.DllBase)
	{
		status = STATUS_DLL_NOT_FOUND;
		goto CleanupExit;
	}

#ifdef _M_ARM64
	exportsFlags = 0;
	if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
		mappedImage.NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64)
	{
		status = PhGetProcessArchitecture(ProcessHandle, &processArchitecture);

		if (!NT_SUCCESS(status))
			goto CleanupExit;

		if (processArchitecture == IMAGE_FILE_MACHINE_AMD64)
			exportsFlags |= PH_GET_IMAGE_EXPORTS_ARM64X;
	}

	status = PhGetMappedImageExportsEx(&exports, &mappedImage, exportsFlags);
#else
	status = PhGetMappedImageExports(&exports, &mappedImage);
#endif

	if (!NT_SUCCESS(status))
		goto CleanupExit;

	if (IS_INTRESOURCE(ProcedureName))
	{
		status = PhGetMappedImageExportFunctionRemote(
			&exports,
			NULL,
			PtrToUshort(ProcedureName),
			context.DllBase,
			ProcedureAddress
		);
	}
	else
	{
		status = PhGetMappedImageExportFunctionRemote(
			&exports,
			ProcedureName,
			0,
			context.DllBase,
			ProcedureAddress
		);
	}

	if (NT_SUCCESS(status))
	{
		if (DllBase)
			*DllBase = context.DllBase;
	}

CleanupExit:
	PhUnloadMappedImage(&mappedImage);
	PhClearReference(&fileName);

	return status;
}

NTSTATUS siapi::PhLoadMappedImageEx(
	_In_opt_ PCWSTR FileName,
	_In_opt_ HANDLE FileHandle,
	_Out_ PPH_MAPPED_IMAGE MappedImage
)
{
	NTSTATUS status;
	PVOID viewBase;
	SIZE_T viewSize;

	status = PhMapViewOfEntireFileEx(
		FileName,
		FileHandle,
		&viewBase,
		&viewSize
	);

	if (NT_SUCCESS(status))
	{
		MappedImage->Signature = *(PUSHORT)viewBase;
		MappedImage->ViewBase = viewBase;
		MappedImage->ViewSize = viewSize;

		switch (MappedImage->Signature)
		{
		case IMAGE_DOS_SIGNATURE:
		{
			status = PhInitializeMappedImage(
				MappedImage,
				viewBase,
				viewSize
			);
		}
		break;
		case IMAGE_ELF_SIGNATURE:
		{
			status = PhInitializeMappedWslImage(
				MappedImage,
				viewBase,
				viewSize
			);
		}
		break;
		default:
			status = STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;
			break;
		}

		if (!NT_SUCCESS(status))
		{
			PhUnloadMappedImage(MappedImage);
		}
	}

	return status;
}


NTSTATUS siapi::PhMapViewOfEntireFileEx(
	_In_opt_ PCWSTR FileName,
	_In_opt_ HANDLE FileHandle,
	_Out_ PVOID* ViewBase,
	_Out_ PSIZE_T ViewSize
)
{
	NTSTATUS status;
	BOOLEAN openedFile = FALSE;
	LARGE_INTEGER sectionSize;
	HANDLE sectionHandle;
	SIZE_T viewSize;
	PVOID viewBase;

	if (!FileName && !FileHandle)
		return STATUS_INVALID_PARAMETER_MIX;

	// Open the file if we weren't supplied a file handle.
	if (!FileHandle)
	{
		status = PhCreateFile(
			&FileHandle,
			FileName,
			FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_DELETE,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
		);

		if (!NT_SUCCESS(status))
			return status;

		openedFile = TRUE;
	}

	// Get the file size and create the section.

	status = PhGetFileSize(FileHandle, &sectionSize);

	if (!NT_SUCCESS(status))
		goto CleanupExit;

	status = PhCreateSection(
		&sectionHandle,
		SECTION_QUERY | SECTION_MAP_READ,
		&sectionSize,
		PAGE_READONLY,
		SEC_COMMIT,
		FileHandle
	);

	if (!NT_SUCCESS(status))
		goto CleanupExit;

	viewSize = (SIZE_T)sectionSize.QuadPart;
	viewBase = NULL;

	// Map the section.

	status = PhMapViewOfSection(
		sectionHandle,
		NtCurrentProcess(),
		&viewBase,
		0,
		NULL,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_READONLY
	);

	NtClose(sectionHandle);

	if (!NT_SUCCESS(status))
		goto CleanupExit;

	*ViewBase = viewBase;
	*ViewSize = (SIZE_T)sectionSize.QuadPart;

CleanupExit:
	if (openedFile)
		NtClose(FileHandle);

	return status;
}



