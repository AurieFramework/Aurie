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
		IN const char* InterfaceName
	);

	EXPORTED AurieStatus ObGetInterface(
		IN const char* InterfaceName,
		OUT AurieInterfaceBase*& Interface
	);

	namespace Internal
	{
		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module,
			IN AurieInterfaceTableEntry& Entry
		);

		AurieStatus ObpDestroyInterface(
			IN AurieModule* Module,
			IN AurieInterfaceBase* Interface,
			IN bool Notify
		);

		AurieStatus ObpLookupInterfaceOwner(
			IN const char* InterfaceName,
			IN bool CaseInsensitive,
			OUT AurieModule*& Module,
			OUT AurieInterfaceTableEntry*& TableEntry
		);

		EXPORTED AurieObjectType ObpGetObjectType(
			IN AurieObject* Object
		);
	}
}

#endif // AURIE_OBJECT_H_