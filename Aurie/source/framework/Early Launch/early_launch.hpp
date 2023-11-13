#ifndef AURIE_EARLY_LAUNCH_H_
#define AURIE_EARLY_LAUNCH_H_

#include "../framework.hpp"
#include <TlHelp32.h>
#include <functional>

namespace Aurie
{
	EXPORTED bool ElIsProcessSuspended();

	HWND ElWaitForCurrentProcessWindow();

	bool ElForEachThread(
		IN std::function<bool(const THREADENTRY32& ThreadEntry)> Callback
	);

	bool ElGetEntrypointThread(
		OUT SYSTEM_THREAD_INFORMATION& ThreadInformation
	);

	PVOID ElGetThreadStartAddress(
		IN HANDLE ThreadHandle
	);

	namespace Internal
	{
		// A wrapper around GetModuleHandleW and GetProcAddress
		PVOID ElpGetProcedure(
			IN const wchar_t* ModuleName, 
			IN const char* ProcedureName
		);

		// A simple wrapper around NtQueryInformationThread
		NTSTATUS ElpQueryInformationThread(
			IN HANDLE ThreadHandle,
			IN INT ThreadInformationClass,
			OUT PVOID ThreadInformation,
			IN ULONG ThreadInformationLength,
			OPTIONAL OUT PULONG ReturnLength
		);

		// Wrapper around ElpQueryInformationThread, class ThreadSystemThreadInformation
		NTSTATUS ElpGetSystemThreadInformation(
			IN HANDLE ThreadHandle,
			OUT SYSTEM_THREAD_INFORMATION& ThreadInformation
		);

		NTSTATUS ElpResumeProcess(
			IN HANDLE ProcessHandle
		);
	}
}

#endif // AURIE_EARLY_LAUNCH_H_