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
		Internal::MdiCreateModule(
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

	g_ArInitialImage = Internal::MdiAddModuleToList(
		initial_module
	);

	
}

void ArProcessDetach(HINSTANCE Instance)
{
	// Unload
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