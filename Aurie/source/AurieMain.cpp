// Include NTSTATUS values from this header, because it has all of them
#include <ntstatus.h>

// Don't include NTSTATUS values from Windows.h and winternl.h
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winternl.h>

#include "framework/framework.hpp"


// Unload routine, frees everything properly
void ArProcessDetach(HINSTANCE Instance)
{
	using namespace Aurie;

	// Unload all modules except the initial image
	// First calls the ModuleUnload functions (if they're set up)
	for (auto& entry : Internal::g_LdrModuleList)
	{
		// Skip the initial image
		if (&entry == g_ArInitialImage)
			continue;
		
		Internal::MdpDispatchEntry(
			&entry,
			entry.ModuleUnload
		);

		// Free all the memory allocations for the module
		for (auto& allocation : entry.MemoryAllocations)
		{
			Internal::MmpFreeMemory(
				&entry,
				allocation.AllocationBase
			);
		}

		entry.MemoryAllocations.clear();

		FreeLibrary(entry.ImageBase.Module);
	}

	// Free persistent memory
	for (auto& allocation : g_ArInitialImage->MemoryAllocations)
	{
		Internal::MmpFreeMemory(
			g_ArInitialImage,
			allocation.AllocationBase
		);
	}

	g_ArInitialImage = nullptr;
	Internal::g_LdrModuleList.clear();
}

// Called upon framework initialization (DLL_PROCESS_ATTACH) event.
// This is the first function that runs.
void ArProcessAttach(HINSTANCE Instance)
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
			nullptr,
			nullptr,
			nullptr,
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
		initial_module
	);

	// Get the current folder (where the main executable is)
	fs::path folder_path;
	if (!AurieSuccess(
		Internal::MdpGetImageFolder(
			g_ArInitialImage, 
			folder_path
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

	// Craft the path from which the mods will be loaded
	folder_path = folder_path / "mods" / "aurie";

	// Load everything from %APPDIR%\\mods\\aurie
	Internal::MdpRecursiveMapFolder(
		folder_path,
		nullptr
	);

	// Call ModulePreload on all loaded plugins
	for (auto& entry : Internal::g_LdrModuleList)
	{
		Internal::MdpDispatchEntry(
			&entry,
			entry.ModulePreinitialize
		);
	}

	// Resume our process
	if (ElIsProcessSuspended())
	{
		Internal::ElpResumeProcess(GetCurrentProcess());
	}
		
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

		if (!AurieSuccess(last_status))
		{

		}
	}

	while (!GetAsyncKeyState(VK_END))
	{
		Sleep(1);
	}

	ArProcessDetach(Instance);
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