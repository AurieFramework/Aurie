// Include NTSTATUS values from this header, because it has all of them
#include <ntstatus.h>

// Don't include NTSTATUS values from Windows.h and winternl.h
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winternl.h>

#include "framework/framework.hpp"

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
		Internal::MdpDispatchEntry(
			&entry,
			entry.ModuleInitialize
		);
	}
}

void ArProcessDetach(HINSTANCE Instance)
{
	UNREFERENCED_PARAMETER(Instance);
	// TODO: Loop all modules, call their unload functions, free memory allocations, free their modules...
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

			HANDLE created_thread = CreateThread(
				nullptr,
				0,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(ArProcessDetach),
				hinstDLL,
				0,
				nullptr
			);

			if (!created_thread)
				return FALSE;

			CloseHandle(created_thread);
			break;
		}
	}

	return TRUE;
}