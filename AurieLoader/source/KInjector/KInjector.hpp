#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <functional>
#include <string>

namespace KInjector
{
	enum class Architecture : char
	{
		Unknown = 0,
		x86,
		x64
	};

	// Main injection function.
	// Automatically opens a HANDLE to the target process.
	bool Inject(
		IN ULONG ProcessId,
		IN PCWSTR DllName
	);

	// Already have a HANDLE?
	// Use this overload.
	bool Inject(
		IN HANDLE ProcessHandle,
		IN PCWSTR DllName
	);

	namespace Internal
	{
		// Gets the processes architecture by calling IsWow64Process
		bool GetProcessArchitecture(
			IN HANDLE ProcessHandle,
			OUT Architecture& ProcessArchitecture
		);

		// Loops through all processes in the system, and calls the Callback function for each one.
		bool ForEachProcess(
			IN std::function<bool(const PROCESSENTRY32W&)> Callback
		);

		// Loops through all modules in a target process, and calls the Callback function for each one.
		bool ForEachModule(
			IN HANDLE ProcessHandle,
			IN std::function<bool(const MODULEENTRY32W&)> Callback
		);

		// Finds the correct module path for each process architecture.
		std::wstring GetModulePath(
			IN HANDLE ProcessHandle,
			IN PCWSTR ModuleName
		);

		// Finds the base address of a given module in a target process.
		PVOID GetModuleBaseAddress(
			IN HANDLE ProcessHandle,
			IN PCWSTR ModuleName
		);

		// The internal injection routine. You may replace this if you want.
		bool InjectInternal(
			IN HANDLE ProcessHandle,
			IN LPTHREAD_START_ROUTINE StartRoutine,
			IN PCWSTR DllPath
		);

		namespace PE
		{
			// GetProcAddress, but operates on an on-disk file.
			uintptr_t GetExportOffset(
				IN PCWSTR ImagePath,
				IN PCSTR FunctionName
			);

			// Helper function for converting RVAs to an offset into the file on-disk.
			DWORD RvaToFileOffset(
				IN PIMAGE_NT_HEADERS Headers,
				IN DWORD Rva
			);
		}
	}
}