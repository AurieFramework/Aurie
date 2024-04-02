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

	// Creates a named callback object, which the OwnerModule can notify to call all callbacks registered under it
	EXPORTED AurieStatus ObCreateCallback(
		IN AurieModule* OwnerModule,
		IN const char* CallbackName,
		IN AurieCallbackEntry CallbackRoutine,
		OUT AurieCallback** CallbackObject
	);

	// Destroys a named callback object
	EXPORTED AurieStatus ObDestroyCallback(
		IN AurieModule* OwnerModule,
		IN AurieCallback* CallbackObject
	);

	// Adds a new function to act as a callback to an existing callback object
	EXPORTED AurieStatus ObRegisterCallback(
		IN const char* CallbackName,
		IN AurieCallbackEntry Routine
	);

	// Removes a function from an existing callback object
	EXPORTED AurieStatus ObUnregisterCallback(
		IN const char* CallbackName,
		IN AurieCallbackEntry Routine
	);

	namespace Internal
	{
		EXPORTED AurieStatus ObpCreateCallbackObject(
			IN AurieModule* Owner,
			IN const char* CallbackName,
			IN AurieCallbackEntry PreCallback,
			IN AurieCallbackEntry PostCallback,
			IN bool IsDispatchable,
			IN bool IsPartial,
			OUT AurieCallback** CallbackObject
		);

		void ObpInitializeCallbackObject(
			IN AurieCallback* CallbackObject,
			IN AurieModule* Owner,
			IN const char* CallbackName,
			IN AurieCallbackEntry PreCallback,
			IN AurieCallbackEntryEx PreInvokeCallback,
			IN AurieCallbackEntry PostInvokeCallback,
			IN AurieCallbackEntry PostCallback,
			IN bool IsDispatchable,
			IN bool IsPartial
		);

		AurieCallback* ObpAddCallbackToTable(
			IN const AurieCallback& Callback
		);

		EXPORTED AurieStatus ObpAssignCallback(
			IN AurieCallback* CallbackObject,
			IN uint32_t Position,
			IN AurieCallbackEntry CallbackEntry
		);

		void ObpSetCallbackFlags(
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
			IN bool CaseSensitive,
			OPTIONAL OUT AurieCallback** CallbackObject
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

		inline std::list<AurieCallback> g_ObpCallbackList;
	}
}

#endif // AURIE_OBJECT_H_