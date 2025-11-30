#include "Aurie/shared.hpp"
using namespace Aurie;

EXPORTED AurieStatus ModulePreinitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	bool is_game_suspended = false;
	AurieStatus status = ElIsProcessSuspended(is_game_suspended);

	DbgPrint(
		"[TestModule] ModulePreload. ElIsProcessSuspended returns %s with status %s",
		is_game_suspended ? "true" : "false",
		AurieStatusToString(status)
	);

	return AURIE_SUCCESS;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module, 
	IN const fs::path& ModulePath
)
{
	AurieStatus last_status = AURIE_SUCCESS;

	DbgPrint("Hello from the test Aurie Framework module!");
	DbgPrint("- AurieModule: %p", Module);
	DbgPrint("- ModulePath: %S", ModulePath.wstring().c_str());
	DbgPrint("- g_ArInitialImage: %p", g_ArInitialImage);

	PVOID current_nt_header = nullptr;
	last_status = Internal::PpiGetNtHeader(
		Internal::MdpGetModuleBaseAddress(Module),
		current_nt_header
	);

	if (AurieSuccess(last_status))
		DbgPrint("[>] Internal::PpiGetNtHeader succeeds (current_nt_header %p)!", current_nt_header);
	else
		DbgPrintEx(LOG_SEVERITY_WARNING, "[!] Internal::PpiGetNtHeader fails!");

	return AURIE_SUCCESS;
}