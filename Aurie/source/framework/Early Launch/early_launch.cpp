#include "early_launch.hpp"

namespace Aurie
{
	bool ElIsProcessSuspended()
	{
		SYSTEM_THREAD_INFORMATION entrypoint_thread_information = {};

		// Make sure we got the entrypoint thread
		if (!ElGetEntrypointThread(entrypoint_thread_information))
			return false;

		// We're checking if the entrypoint thread is waiting because it's suspended
		if (entrypoint_thread_information.ThreadState != Waiting)
			return false;

		return entrypoint_thread_information.WaitReason == Suspended;
	}

	HWND ElWaitForCurrentProcessWindow()
	{
		struct ElProcessWindowInformation
		{
			HANDLE ProcessHandle;
			HWND ProcessWindow;
		} window_information = { GetCurrentProcess(), nullptr};

		auto enum_windows_callback = [](IN HWND Window, IN LPARAM Parameter) -> BOOL
			{
				auto window_information = reinterpret_cast<ElProcessWindowInformation*>(Parameter);
				DWORD window_process_id = 0;

				// Make sure we know who the window belongs to
				if (!GetWindowThreadProcessId(Window, &window_process_id))
					return true;

				// Make sure the window belongs to our process
				if (window_process_id != GetProcessId(window_information->ProcessHandle))
					return true;

				// Skip console windows, in case we have one.
				// This function is only meant to be called if the subsystem  
				// of the current process is IMAGE_SUBSYSTEM_WINDOWS_GUI.
				if (Window == GetConsoleWindow())
					return true;

				// Skip any invisible windows
				if (!IsWindowVisible(Window))
					return true;

				window_information->ProcessWindow = Window;
				return false;
			};

		while (!window_information.ProcessWindow)
		{
			EnumWindows(enum_windows_callback, reinterpret_cast<LPARAM>(&window_information));
		}

		return window_information.ProcessWindow;
	}

	bool ElForEachThread(
		IN std::function<bool(const THREADENTRY32& ThreadEntry)> Callback
	)
	{
		// Capture all threads in the system
		HANDLE thread_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0x1337);

		// If we can't capture the threads for whatever reason
		if (thread_snapshot == INVALID_HANDLE_VALUE)
			return false;

		THREADENTRY32 thread_entry = {}; thread_entry.dwSize = sizeof(thread_entry);

		// Make sure we can start enumerating
		if (!Thread32First(
			thread_snapshot,
			&thread_entry
		))
		{
			CloseHandle(thread_snapshot);
			return false;
		}

		// Loop all the threads
		do
		{
			if (Callback(thread_entry))
			{
				CloseHandle(thread_snapshot);
				return true;
			}

		} while (Thread32Next(thread_snapshot, &thread_entry));

		CloseHandle(thread_snapshot);
		return false;
	}

	bool ElGetEntrypointThread(
		OUT SYSTEM_THREAD_INFORMATION& ThreadInformation
	)
	{
		// Query the entry point of the main executable
		PVOID entrypoint_address = nullptr;
		if (!AurieSuccess(
			Internal::MdpQueryModuleInformation(
				GetModuleHandleA(nullptr),
				nullptr,
				nullptr,
				&entrypoint_address
			)
		))
		{
			return false;
		}

		return ElForEachThread(
			[entrypoint_address, &ThreadInformation](const THREADENTRY32& ThreadEntry) -> bool
			{
				// Skip any threads that aren't in our own process
				if (ThreadEntry.th32OwnerProcessID != GetCurrentProcessId())
					return false;

				HANDLE thread_handle = OpenThread(
					THREAD_ALL_ACCESS,
					false,
					ThreadEntry.th32ThreadID
				);

				// We can't open this thread for some reason
				if (!thread_handle)
					return false;

				// Query the threads information
				SYSTEM_THREAD_INFORMATION thread_information;
				NTSTATUS last_status = Internal::ElpGetSystemThreadInformation(
					thread_handle,
					thread_information
				);

				// Get the thread start address while we still have the 
				PVOID thread_start_address = ElGetThreadStartAddress(thread_handle);

				// Throw away the thread handle, as we don't need it anymore
				CloseHandle(thread_handle);

				// If we failed to query the thread information
				if (!NT_SUCCESS(last_status))
					return false;

				// Check the start address of our thread
				// We can't use thread_information.StartAddress, since that points to RtlUserThreadStart.
				if (thread_start_address != entrypoint_address)
					return false;

				// Save the thread information
				ThreadInformation = thread_information;

				// We found our entry point thread.
				return true;
			}
		);
	}

	PVOID ElGetThreadStartAddress(
		IN HANDLE ThreadHandle
	)
	{
		PVOID thread_start_address = nullptr;

		Internal::ElpQueryInformationThread(
			ThreadHandle,
			9, // ThreadQuerySetWin32StartAddress
			&thread_start_address,
			sizeof(thread_start_address),
			nullptr
		);

		// If ElpQueryInformationThread fails, we return nullptr
		return thread_start_address;
	}

	namespace Internal
	{
		PVOID ElpGetProcedure(
			IN const wchar_t* ModuleName,
			IN const char* ProcedureName
		)
		{
			HMODULE target_module = GetModuleHandleW(ModuleName);

			if (!target_module)
				return nullptr;

			return GetProcAddress(target_module, ProcedureName);
		}

		NTSTATUS ElpQueryInformationThread(
			IN HANDLE ThreadHandle,
			IN INT ThreadInformationClass, 
			OUT PVOID ThreadInformation, 
			IN ULONG ThreadInformationLength, 
			OPTIONAL OUT PULONG ReturnLength
		)
		{
			using FN_NtQueryInformationThread = NTSTATUS(NTAPI*)(
				IN HANDLE ThreadHandle,
				IN INT ThreadInformationClass,
				OUT PVOID ThreadInformation,
				IN ULONG ThreadInformationLength,
				OPTIONAL OUT PULONG ReturnLength
			);

			auto NtQueryInformationThread = reinterpret_cast<FN_NtQueryInformationThread>(ElpGetProcedure(
				L"ntdll.dll",
				"NtQueryInformationThread"
			));

			if (!NtQueryInformationThread)
				return STATUS_UNSUCCESSFUL;

			return NtQueryInformationThread(
				ThreadHandle,
				ThreadInformationClass,
				ThreadInformation,
				ThreadInformationLength,
				ReturnLength
			);
		}

		NTSTATUS ElpGetSystemThreadInformation(
			IN HANDLE ThreadHandle, 
			OUT SYSTEM_THREAD_INFORMATION& ThreadInformation
		)
		{
			return ElpQueryInformationThread(
				ThreadHandle,
				40, // ThreadSystemThreadInformation
				&ThreadInformation,
				sizeof(ThreadInformation),
				nullptr
			);
		}

		NTSTATUS ElpResumeProcess(
			IN HANDLE ProcessHandle
		)
		{
			using FN_NtResumeProcess = NTSTATUS(NTAPI*)(
				IN HANDLE ProcessHandle
			);

			auto NtResumeProcess = reinterpret_cast<FN_NtResumeProcess>(ElpGetProcedure(
				L"ntdll.dll",
				"NtResumeProcess"
			));

			if (!NtResumeProcess)
				return STATUS_UNSUCCESSFUL;

			return NtResumeProcess(
				ProcessHandle
			);
		}
	}
}