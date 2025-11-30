// Include NTSTATUS values from this header, because it has all of them
#include <ntstatus.h>

// Don't include NTSTATUS values from Windows.h and winternl.h
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winternl.h>

#include "framework/framework.hpp"

// Unload routine, frees everything properly
static void ArProcessDetach(HINSTANCE)
{
	using namespace Aurie;

	Beep(1200, 200);

	// Unload all modules except the initial image
	// First calls the ModuleUnload functions (if they're set up)
	for (auto& entry : Internal::g_LdrModuleList)
	{
		// Skip the initial image
		if (&entry == g_ArInitialImage)
			continue;
		
		// Unmap the image (but don't remove it from the list)
		Internal::MdpUnmapImage(
			&entry,
			false,
			true
		);
	}

	// Free persistent memory
	for (auto& allocation : g_ArInitialImage->MemoryAllocations)
	{
		Internal::MmpFreeMemory(
			g_ArInitialImage,
			allocation.AllocationBase,
			false
		);
	}

	// Remove all the allocations, they're now invalid
	g_ArInitialImage->MemoryAllocations.clear();

	// Null the initial image, and clear the module list
	g_ArInitialImage = nullptr;
	Internal::g_LdrModuleList.clear();

	// Destroy the console
	Internal::DbgpDestroyConsole();
}

// Called upon framework initialization (DLL_PROCESS_ATTACH) event.
// This is the first function that runs.
static void ArProcessAttach(HINSTANCE Instance)
{
	using namespace Aurie;

	// Query the image path
	DWORD process_name_size = MAX_PATH;
	wchar_t process_name[MAX_PATH] = { 0 };
	if (!QueryFullProcessImageNameW(
		GetCurrentProcess(),
		0,
		process_name,
		&process_name_size
	))
	{
		return (void)MessageBoxA(
			nullptr,
			"Failed to query process path!",
			"Aurie Framework",
			MB_OK | MB_TOPMOST | MB_ICONERROR | MB_SETFOREGROUND
		);
	}

	// Create the initial image for the Aurie Framework.
	AurieModule initial_module;
	if (!AurieSuccess(
		Internal::MdpCreateModule(
			process_name,
			Instance,
			false,
			0,
			initial_module
		)
	))
	{
		return (void)MessageBoxA(
			nullptr,
			"Failed to create initial module!",
			"Aurie Framework",
			MB_OK | MB_TOPMOST | MB_ICONERROR | MB_SETFOREGROUND
		);
	}

	g_ArInitialImage = Internal::MdpAddModuleToList(
		std::move(initial_module)
	);

	// Get the current folder (where the main executable is)
	fs::path game_folder;
	if (!AurieSuccess(
		Internal::MdpGetImageFolder(
			g_ArInitialImage,
			game_folder
		)
	))
	{
		return (void)MessageBoxA(
			nullptr,
			"Failed to get initial folder!",
			"Aurie Framework",
			MB_OK | MB_TOPMOST | MB_ICONERROR | MB_SETFOREGROUND
		);
	}

	SetCurrentDirectoryW(game_folder.native().c_str());

	// Create a logger
	Internal::DbgpCreateConsole("Aurie Framework Log | Press Ctrl+C to close");
	Internal::DbgpInitLogger();

	LPWSTR command_line = GetCommandLineW();
	if (wcsstr(command_line, L"-aurie_wait_for_debug"))
	{
		DbgPrintEx(LOG_SEVERITY_INFO, "[ArProcessAttach] Waiting for debugger...", game_folder.wstring().c_str());
		while (!IsDebuggerPresent()) {};

		Sleep(500); // Give time for the debugger to initialize
	}

	DbgPrintEx(LOG_SEVERITY_TRACE, "[ArProcessAttach] Current folder is %S", game_folder.wstring().c_str());

	// Load all native mods
	// AurieLoader used to do this in the past, but 
	fs::path native_path = game_folder / "mods" / "native";
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(native_path, ec))
	{
		// If the file isn't a regular file (but instead a directory, symlink, etc.), skip it
		if (!entry.is_regular_file())
			continue;

		// If the file has no name (wtf?), skip it
		if (!entry.path().has_filename())
			continue;
		
		// If the file has no extension, skip it
		if (!entry.path().filename().has_extension())
			continue;

		// If the file doesn't end with a .dll extension, skip it.
		if (entry.path().filename().extension().compare(L".dll"))
			continue;

		// If the module is already loaded, skip it.
		if (GetModuleHandleW(entry.path().c_str()))
			continue;

		HMODULE loaded_library = LoadLibraryW(entry.path().c_str());
		DbgPrintEx(LOG_SEVERITY_TRACE, "[ArProcessAttach] Loaded native module \"%S\" at %p", entry.path().c_str(), loaded_library);
	}


	// Craft the path from which the mods will be loaded
	game_folder = game_folder / "mods" / "aurie";

	// Build a list of "priority mods" - these are mods that have a ModuleEntrypoint 
	// We can't use MdpMapFolder as that doesn't allow us to specify custom conditions
	std::vector<fs::path> priority_modules;
	Internal::MdpBuildModuleList(
		game_folder,
		true,
		[](const fs::directory_entry& Entry)
		{
			if (!Internal::MdpIsValidModulePredicate(Entry))
				return false;

			if (!PpFindFileExportByName(Entry, "ModuleEntrypoint"))
				return false;

			return true;
		},
		priority_modules
	);

	// Sort the priority modules list from A-Z
	std::sort(
		priority_modules.begin(),
		priority_modules.end()
	);

	Internal::MdpMapModulesFromList(
		priority_modules
	);

	// Call ModuleEntrypoint on all loaded plugins
	for (auto& entry : Internal::g_LdrModuleList)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		last_status = Internal::MdpDispatchEntry(
			&entry,
			entry.ModuleEntrypoint
		);

		// Mark mods failed for loading for the purge
		if (!AurieSuccess(last_status))
		{
			std::wstring module_name = L"<unknown>";

			// We don't care about the result - if the function fails, we fall back to the "<unknown>" string.
			MdGetImageFilename(&entry, module_name);

			DbgPrintEx(
				LOG_SEVERITY_WARNING,
				"[ArProcessAttach] Module \"%S\" failed ModuleEntrypoint with status %s and will be purged.",
				module_name.c_str(),
				AurieStatusToString(last_status)
			);

			Internal::MdpMarkModuleForPurge(&entry);
		}
		else
		{
			entry.Flags.EntrypointRan = true;
		}
	}

	// Load everything from %APPDIR%\\mods\\aurie
	Internal::MdpMapFolder(
		game_folder,
		true,
		false,
		nullptr
	);

	// Purge all the modules that failed loading
	// We purge after loading everything else, because doing it the other way around would just re-load the module again.
	Internal::MdpPurgeMarkedModules();

	// Call ModulePreinitialize on all loaded plugins
	for (auto& entry : Internal::g_LdrModuleList)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		// Skip modules that have already been preloaded - this can happen in one specific scenario:
		// Mod A gets loaded, in ModulePreload calls MdMapImage on Mod B.
		// 
		// Mod B is loaded while the game process is suspended, which prompts
		// the module manager to treat it as a non-runtime loaded mod.
		// 
		// The module manager will not call the ModuleInitialize routine
		// if the process is suspended at the time of the call.
		if (MdIsImagePreinitialized(&entry))
			continue;

		last_status = Internal::MdpDispatchEntry(
			&entry,
			entry.ModulePreinitialize
		);

		// Mark mods failed for loading for the purge
		if (!AurieSuccess(last_status))
		{
			std::wstring module_name = L"<unknown>";

			// We don't care about the result - if the function fails, we fall back to the "<unknown>" string.
			MdGetImageFilename(&entry, module_name);

			DbgPrintEx(
				LOG_SEVERITY_WARNING,
				"[ArProcessAttach] Module \"%S\" failed ModulePreinitialize with status %s and will be purged.",
				module_name.c_str(),
				AurieStatusToString(last_status)
			);

			Internal::MdpMarkModuleForPurge(&entry);
		}
		else
		{
			entry.Flags.IsPreloaded = true;
		}
	}

	// Purge all the modules that failed loading
	// We can't do this in the for loop because of iterators...
	Internal::MdpPurgeMarkedModules();

	// Resume our process if needed
	bool is_process_suspended = false;
	if (!AurieSuccess(ElIsProcessSuspended(is_process_suspended)) || is_process_suspended)
	{
		Internal::ElpResumeProcess(GetCurrentProcess());
	}

	// Now we have to wait until the current process has finished initializating
	
	// Query the process subsystem
	unsigned short current_process_subsystem = 0;
	PpGetImageSubsystem(
		GetModuleHandleA(nullptr),
		current_process_subsystem
	);

	// If the current process is a GUI process, wait for its window
	if (current_process_subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
		ElWaitForCurrentProcessWindow();

	WaitForInputIdle(GetCurrentProcess(), INFINITE);

	// Call ModuleEntry on all loaded plugins
	for (auto& entry : Internal::g_LdrModuleList)
	{
		// Ignore modules that are already initialized?
		if (MdIsImageInitialized(&entry))
			continue;

		AurieStatus last_status = Internal::MdpDispatchEntry(
			&entry,
			entry.ModuleInitialize
		);

		// Mark mods failed for loading for the purge
		if (!AurieSuccess(last_status))
		{
			std::wstring module_name = L"<unknown>";

			// We don't care about the result - if the function fails, we fall back to the "<unknown>" string.
			MdGetImageFilename(&entry, module_name);

			DbgPrintEx(
				LOG_SEVERITY_WARNING, 
				"[ArProcessAttach] Module \"%S\" failed ModuleInitialize with status %s and will be purged.",
				module_name.c_str(),
				AurieStatusToString(last_status)
			);

			Internal::MdpMarkModuleForPurge(&entry);
		}
		else
		{
			entry.Flags.IsInitialized = true;
		}
	}

	// Purge all the modules that failed loading
	// We can't do this in the for loop because of iterators...
	Internal::MdpPurgeMarkedModules();

	DbgPrintEx(LOG_SEVERITY_TRACE, "[ArProcessAttach] Init done.");

	DWORD state = 0;
	while (!state)
	{
		state = GetAsyncKeyState(VK_END);
		Sleep(1);
	}

	DbgPrintEx(LOG_SEVERITY_TRACE, "[ArProcessAttach] GetAsyncKeyState State: %x", state);

	DbgPrintEx(LOG_SEVERITY_TRACE, "[ArProcessAttach] Unloading now.");

	// Flush all loggers
	spdlog::apply_all([](std::shared_ptr<spdlog::logger> Logger) { Logger->flush(); });

	// Stop logger thread
	spdlog::shutdown();

	// Calls DllMain with DLL_PROCESS_DETACH, which calls ArProcessDetach
	FreeLibraryAndExitThread(Instance, 0);
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)  
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hinstDLL);

			HANDLE created_thread = CreateThread(
				nullptr,
				0,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(ArProcessAttach),
				hinstDLL,
				0,
				nullptr
			);

			if (!created_thread)
				return FALSE;

			CloseHandle(created_thread);
			break;
		}
	case DLL_PROCESS_DETACH:
		{
			// Process termination, the kernel will free stuff for us.
			if (lpvReserved)
				return TRUE;

			ArProcessDetach(hinstDLL);

			break;
		}
	}

	return TRUE;
}