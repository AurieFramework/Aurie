#ifndef AURIE_OBJECT_H_
#define AURIE_OBJECT_H_

#include "../framework.hpp"

namespace Aurie
{
	// Creates a named interface for inter-module communication
	EXPORTED AurieStatus ObCreateInterface(
		IN AurieModule* Module,
		IN AurieInterfaceBase* Interface,
		IN const char* InterfaceName
	);

	// Checks whether a named interface exists
	EXPORTED bool ObInterfaceExists(
		IN const char* InterfaceName
	);

	// Destroys a given named interface
	EXPORTED AurieStatus ObDestroyInterface(
		IN AurieModule* Module,
		IN const char* InterfaceName
	);

	// Gets the AurieInterfaceBase pointer to a given named interface
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

	EXPORTED AurieStatus ObNotifyCallback(
		IN const char* CallbackName,
		OPTIONAL IN PVOID Argument1,
		OPTIONAL IN PVOID Argument2
	);

	namespace Internal
	{
		// Creates a callback object and adds it to the global table of callbacks.
		// If you really need to, use it to create a partial callback and then finish
		// the initialization with ObCreateCallback
		EXPORTED AurieStatus ObpCreateCallbackObject(
			IN AurieModule* Owner,
			IN const char* CallbackName,
			IN AurieCallbackEntryEx PreCallback,
			IN AurieCallbackEntry PostCallback,
			IN bool IsDispatchable,
			IN bool IsPartial,
			OUT AurieCallback** CallbackObject
		);

		// Initializes a callback object. Use ObCreateCallback.
		void ObpInitializeCallbackObject(
			IN AurieCallback* CallbackObject,
			IN AurieModule* Owner,
			IN const char* CallbackName,
			IN AurieCallbackEntryEx PreCallback,
			IN AurieCallbackEntryEx PreInvokeCallback,
			IN AurieCallbackEntry PostInvokeCallback,
			IN AurieCallbackEntry PostCallback,
			IN bool IsDispatchable,
			IN bool IsPartial
		);

		// Adds a callback object to the global table
		AurieCallback* ObpAddCallbackToTable(
			IN const AurieCallback& Callback
		);
		
		EXPORTED bool ObpIsCallbackMethodPresent(
			IN AurieCallback* CallbackObject,
			IN AurieCallbackEntry CallbackMethod
		);

		EXPORTED size_t ObpQueryCallbackMethodCount(
			IN AurieCallback* CallbackObject
		);

		// Assigns a callback routine at a given priority
		EXPORTED AurieStatus ObpAssignCallback(
			IN AurieCallback* CallbackObject,
			IN uint32_t Position,
			IN AurieCallbackEntry CallbackEntry
		);

		AurieStatus ObpNotifyCallback(
			IN AurieCallback* CallbackObject,
			IN AurieObject* AffectedObject,
			IN PVOID Argument1,
			IN PVOID Argument2
		);

		// Forcefully destroys a callback object
		AurieStatus ObpDeferCallbackDeletion(
			IN AurieCallback* CallbackObject
		);

		// Deletes and frees all callbacks that have been marked as deferred-deleted.
		// While it should theoretically be safe to delete the callbacks
		// even if they're looped through, it's safer to do it this way
		// because someone might try to delete a callback in that callback
		void ObpDeleteDeferredCallbacks();

		// Sets a callback object's flags
		void ObpSetCallbackFlags(
			IN AurieCallback* CallbackObject,
			IN bool AllowDispatch,
			IN bool IsPartial,
			IN bool DeferDelete
		);

		// Checks whether a callback is dispatchable
		bool ObpIsCallbackDispatchable(
			IN AurieCallback* CallbackObject
		);

		// Checks whether a callback is a pseudocallback or a full one
		bool ObpIsCallbackPartial(
			IN AurieCallback* CallbackObject
		);

		// Checks whether the callback will be purged soon
		bool ObpIsCallbackDeferDeleted(
			IN AurieCallback* CallbackObject
		);

		// Looks up a callback object by its name
		AurieStatus ObpLookupCallbackByName(
			IN const char* Name,
			IN bool CaseSensitive,
			OPTIONAL OUT AurieCallback** CallbackObject
		);

		// Adds an interface to a module's table
		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module,
			IN AurieInterfaceTableEntry& Entry
		);

		// Internal routine for destroying interfaces
		AurieStatus ObpDestroyInterface(
			IN AurieModule* Module,
			IN AurieInterfaceBase* Interface,
			IN bool Notify,
			IN bool RemoveFromList
		);

		// Looks up the owner and interface table entry given an interface name
		AurieStatus ObpLookupInterfaceOwner(
			IN const char* InterfaceName,
			IN bool CaseInsensitive,
			OUT AurieModule*& Module,
			OUT AurieInterfaceTableEntry*& TableEntry
		);

		// Obsolete function that destroys an interface given just a name.
		AurieStatus ObpDestroyInterfaceByName(
			IN const char* InterfaceName
		);

		// Queries an AurieObject's type
		EXPORTED AurieObjectType ObpGetObjectType(
			IN AurieObject* Object
		);

		// Queries an exported routine's address from a module that owns a given interface
		EXPORTED AurieStatus ObpLookupInterfaceOwnerExport(
			IN const char* InterfaceName,
			IN const char* ExportName,
			OUT PVOID& ExportAddress
		);

		inline std::list<AurieCallback> g_ObpCallbackList;
	}
}

#endif // AURIE_OBJECT_H_