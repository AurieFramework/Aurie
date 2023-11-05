#include <iostream>
#include <filesystem>
#include "KInjector/KInjector.hpp"


void MapFolder(
	IN HANDLE ProcessHandle,
	IN const std::filesystem::path& Folder
)
{
	std::error_code ec;
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
			printf("[>] Library injected: '%S'", entry.path().filename().c_str());
		else
			printf("[>] Library injection failed: '%S'", entry.path().filename().c_str());
	}
}

int wmain(int argc, wchar_t** argv)
{
	// You cannot run the Aurie Loader by itself...
	// It may be wise to pop up a file selection dialog, but that'd overcomplicate the whole thing.
	if (argc < 2)
		return 1;

	// The executable we're actually going to want to launch is 
	// always called the same as our executable, but with a .bak appended to it,
	// i.e. "Will You Snail.exe" will turn into "Will You Snail.exe.bak"
	std::wstring process_command_line = argv[0];
	process_command_line.append(L".bak");

	for (int i = 1; i < argc; i++)
	{
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

	BOOL process_created = CreateProcessW(
		nullptr,
		const_cast<LPWSTR>(process_command_line.data()),
		nullptr,
		nullptr,
		false,
		CREATE_SUSPENDED,
		nullptr,
		nullptr,
		&startup_info,
		&process_info
	);

	if (!process_created)
		return GetLastError();

	printf("[>] Process created with PID %d\n", process_info.dwProcessId);

	MapFolder(process_info.hProcess, std::filesystem::current_path() / "mods" / "native");

	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);

	return 0;
}