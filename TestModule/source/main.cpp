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

	printf("[TestModule] ModulePreload. ElIsProcessSuspended returns %d\n", ElIsProcessSuspended());

	return AURIE_SUCCESS;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module, 
	IN const fs::path& ModulePath
)
{
	AurieStatus last_status = AURIE_SUCCESS;

	printf("Hello from the first Aurie Framework module!\n");
	printf("- AurieModule: %p\n", Module);
	printf("- ModulePath: %S\n", ModulePath.wstring().c_str());
	printf("- g_ArInitialImage: %p\n", g_ArInitialImage);

	PVOID memory = MmAllocateMemory(Module, 16);
	printf("[>] MmAllocateMemory returns %p\n", memory);

	last_status = MmFreeMemory(Module, memory);

	if (AurieSuccess(last_status))
		printf("[>] MmFreeMemory succeeds!\n");
	else
		printf("[!] MmFreeMemory fails!\n");

	return AURIE_SUCCESS;
}