// Note to self: Fix project template, change C++ standard to C++17 and the target to DLL
#include "Aurie/shared.hpp"
using namespace Aurie;

EXPORTED AurieStatus ModulePreinitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);


	bool is_game_suspended = false;
	ElIsProcessSuspended(is_game_suspended);
	printf("[TestModule] ModulePreload. ElIsProcessSuspended returns %s\n", is_game_suspended ? "true" : "false");

	return AURIE_SUCCESS;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module, 
	IN const fs::path& ModulePath
)
{
	AurieStatus last_status = AURIE_SUCCESS;

	printf("Hello from the test Aurie Framework module!\n");
	printf("- AurieModule: %p\n", Module);
	printf("- ModulePath: %S\n", ModulePath.wstring().c_str());
	printf("- g_ArInitialImage: %p\n", g_ArInitialImage);

	PVOID current_nt_header = nullptr;
	last_status = Internal::PpiGetNtHeader(
		Internal::MdpGetModuleBaseAddress(Module),
		current_nt_header
	);

	if (AurieSuccess(last_status))
		printf("[>] Internal::PpiGetNtHeader succeeds (current_nt_header %p)!\n", current_nt_header);
	else
		printf("[!] Internal::PpiGetNtHeader fails!\n");

	return AURIE_SUCCESS;
}