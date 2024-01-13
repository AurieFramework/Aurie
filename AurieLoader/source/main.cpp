#include <iostream>
#include <filesystem>
#include "KInjector/KInjector.hpp"

size_t MapFolder(
	IN HANDLE ProcessHandle,
	IN const std::filesystem::path& Folder
)
{
	std::error_code ec;
	size_t module_count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(Folder, ec))
	{
		if (!entry.is_regular_file())
			continue;

		if (!entry.path().has_filename())
			continue;

		if (!entry.path().filename().has_extension())
			continue;

		if (entry.path().filename().extension().compare(L".dll"))
			continue;

		if (KInjector::Inject(ProcessHandle, entry.path().wstring().c_str()))
		{
			printf("[>] Library injected: '%S'\n", entry.path().filename().c_str());
			module_count++;
		}
		else
		{
			printf("[>] Library injection failed: '%S'\n", entry.path().filename().c_str());
		}
	}

	return module_count;
}

int wmain(int argc, wchar_t** argv)
{
	// Represents if the -aurie_nofreeze argument is present
	bool no_process_freeze = false;

	// Represents if the -aurie_wait_for_input argument is present
	bool wait_for_input = false;

	// argv[1] is always the executable name (or path) if we're using IFEO, which is the only
	// method explicitely supported by AurieLoader.
	std::wstring process_command_line;
	for (int i = 1; i < argc; i++)
	{
		// Check for our argument (might add more later if needed)
		if (!_wcsicmp(argv[i], L"-aurie_nofreeze"))
		{
			// Set the flag, and skip forwarding the argument to avoid potential detection by the target process
			no_process_freeze = true;
			continue;
		}

		if (!_wcsicmp(argv[i], L"-aurie_wait_for_input"))
		{
			wait_for_input = true;
			continue;
		}

		// Append the full argument (might have spaces, so surround it with double-quotes)
		process_command_line.append(L"\"");
		process_command_line.append(argv[i]);
		process_command_line.append(L"\"");

		// Append a space between the arguments if we're not on the last argument
		if (i < (argc - 1))
			process_command_line.append(L" ");
	}

	STARTUPINFOW startup_info = {};
	PROCESS_INFORMATION process_info = {};

	// Craft the creation flags for our process
	DWORD creation_flags = DEBUG_ONLY_THIS_PROCESS;
	if (!no_process_freeze)
		creation_flags |= CREATE_SUSPENDED;

	BOOL process_created = CreateProcessW(
		nullptr,
		const_cast<LPWSTR>(process_command_line.data()),
		nullptr,
		nullptr,
		false,
		creation_flags,
		nullptr,
		nullptr,
		&startup_info,
		&process_info
	);

	if (!process_created)
		return GetLastError();

	printf("[>] Process created with PID %d\n", process_info.dwProcessId);

	DebugActiveProcessStop(process_info.dwProcessId);

	// If the flag is provided, we pause so that the debugger can be attached to the game
	// I use this for debugging to attach the debugger before AurieLoader injects, probably not useful otherwise
	if (wait_for_input)
	{
		printf("[!] -aurie_wait_for_input flag provided, waiting for input...\n");
		std::cin.ignore();
	}
	
	// Get the image filename of the process
	WCHAR process_path_buffer[512] = { 0 };
	DWORD process_path_buffer_size = sizeof(process_path_buffer) / sizeof(process_path_buffer[0]);

	size_t module_count = 0;
	if (!QueryFullProcessImageNameW(
		process_info.hProcess,
		0, // PROCESS_NAME_WIN32
		process_path_buffer,
		&process_path_buffer_size
	))
	{
		printf("[!] Failed to determine process image path, current directory will be used!\n");

		module_count = MapFolder(
			process_info.hProcess,
			std::filesystem::current_path() / "mods" / "native"
		);
	}
	else
	{
		module_count = MapFolder(
			process_info.hProcess,
			std::filesystem::path(process_path_buffer).parent_path() / "mods" / "native"
		);
	}

	printf("[>] MapFolder mapped %lld modules\n", module_count);
	
	// If no modules were mapped, we can just resume the process.
	if (module_count == 0)
		ResumeThread(process_info.hThread);

	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);

	printf("[>] Execution complete\n");
	Sleep(1000);

	return 0;
}