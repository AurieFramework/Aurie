#ifndef AURIE_OBJECT_H_
#define AURIE_OBJECT_H_

#include "../framework.hpp"

namespace Aurie
{
	EXPORTED AurieStatus ObCreateInterface(
		IN AurieModule* Module,
		IN AurieInterfaceBase* Interface,
		IN const char* InterfaceName
	);

	EXPORTED bool ObInterfaceExists(
		IN const char* InterfaceName
	);

	EXPORTED AurieStatus ObDestroyInterface(
		IN AurieModule* Module,
		IN const char* InterfaceName
	);

	EXPORTED AurieStatus ObGetInterface(
		IN const char* InterfaceName,
		OUT AurieInterfaceBase*& Interface
	);

	namespace Internal
	{
		EXPORTED void ObpSetModuleOperationCallback(
			IN AurieModule* Module,
			IN AurieModuleCallback CallbackRoutine
		);

		void ObpDispatchModuleOperationCallbacks(
			IN AurieModule* AffectedModule,
			IN AurieEntry Routine,
			IN bool IsFutureCall
		);

		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module,
			IN AurieInterfaceTableEntry& Entry
		);

		AurieOperationInfo ObpCreateOperationInfo(
			IN AurieModule* Module,
			IN bool IsFutureCall
		);

		AurieStatus ObpDestroyInterface(
			IN AurieModule* Module,
			IN AurieInterfaceBase* Interface,
			IN bool Notify,
			IN bool RemoveFromList
		);

		AurieStatus ObpLookupInterfaceOwner(
			IN const char* InterfaceName,
			IN bool CaseInsensitive,
			OUT AurieModule*& Module,
			OUT AurieInterfaceTableEntry*& TableEntry
		);

		AurieStatus ObpDestroyInterfaceByName(
			IN const char* InterfaceName
		);

		EXPORTED AurieObjectType ObpGetObjectType(
			IN AurieObject* Object
		);

		EXPORTED AurieStatus ObpLookupInterfaceOwnerExport(
			IN const char* InterfaceName,
			IN const char* ExportName,
			OUT PVOID& ExportAddress
		);
	}
}

#endif // AURIE_OBJECT_H_