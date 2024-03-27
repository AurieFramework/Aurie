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

	EXPORTED AurieStatus ObCreateCallback(
		IN AurieModule* OwnerModule,
		IN const char* CallbackName,
		IN AurieCallbackEntry CallbackRoutine,
		OUT AurieCallback** CallbackObject
	);

	EXPORTED AurieStatus ObDestroyCallback(
		IN AurieModule* OwnerModule,
		IN AurieCallback* CallbackObject
	);

	EXPORTED AurieStatus ObRegisterCallback(
		IN const char* CallbackName,
		IN AurieCallbackEntry Routine
	);

	EXPORTED AurieStatus ObUnregisterCallback(
		IN const char* CallbackName,
		IN AurieCallbackEntry Routine
	);

	namespace Internal
	{
		EXPORTED AurieStatus ObpCreateCallbackObject(
			IN const char* CallbackName,
			IN AurieCallbackEntry PreCallback,
			IN AurieCallbackEntry PostCallback,
			IN bool IsDispatchable,
			IN bool IsPartial,
			OUT AurieCallback** CallbackObject
		);

		EXPORTED AurieStatus ObpAssignCallback(
			IN AurieCallback* CallbackObject,
			IN uint32_t Position,
			IN AurieCallbackEntry CallbackEntry
		);

		AurieStatus ObpSetCallbackFlags(
			IN AurieCallback* CallbackObject,
			IN bool AllowDispatch,
			IN bool IsPartial
		);

		bool ObpIsCallbackDispatchable(
			IN AurieCallback* CallbackObject
		);

		bool ObpIsCallbackPartial(
			IN AurieCallback* CallbackObject
		);

		AurieStatus ObpLookupCallbackByName(
			IN const char* Name,
			OUT AurieCallback*& CallbackObject
		);

		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module,
			IN AurieInterfaceTableEntry& Entry
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