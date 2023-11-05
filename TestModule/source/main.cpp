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
	printf("Hello from the first Aurie Framework module!\n");
	printf("- AurieModule: %p\n", Module);
	printf("- ModulePath: %S\n", ModulePath.wstring().c_str());
	printf("- g_ArInitialImage: %p\n", g_ArInitialImage);

	return AURIE_SUCCESS;
}