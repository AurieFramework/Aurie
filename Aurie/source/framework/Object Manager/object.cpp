#include "object.hpp"

namespace Aurie
{
	AurieStatus ObCreateInterface(
		IN AurieModule* Module, 
		IN AurieInterfaceBase* Interface, 
		IN const char* InterfaceName
	)
	{
		if (ObInterfaceExists(InterfaceName))
			return AURIE_OBJECT_ALREADY_EXISTS;

		AurieInterfaceTableEntry table_entry = {};
		table_entry.Interface = Interface;
		table_entry.InterfaceName = InterfaceName;
		table_entry.OwnerModule = Module;

		// Make sure the interface knows it's being set up,
		// and that it succeeds at doing so. We don't want an
		// uninitialized, half-broken interface exposed!

		AurieStatus last_status = Interface->Create();
		if (!AurieSuccess(last_status))
			return last_status;

		return Internal::ObpAddInterfaceToTable(
			Module,
			table_entry
		);
	}

	bool ObInterfaceExists(
		IN const char* InterfaceName
	)
	{
		AurieModule* containing_module = nullptr;
		AurieInterfaceTableEntry* table_entry = nullptr;
		
		// If we find a module containing the interface, that means the interface exists!
		// ObpLookupInterfaceOwner will return AURIE_INTERFACE_NOT_FOUND if it doesn't exist.
		return AurieSuccess(
			Internal::ObpLookupInterfaceOwner(
				InterfaceName,
				true,
				containing_module,
				table_entry
			)
		);
	}

	namespace Internal
	{
		AurieStatus ObpDestroyInterfaceByName(
			IN const char* InterfaceName
		)
		{
			AurieModule* owner_module = nullptr;
			AurieInterfaceTableEntry* table_entry = nullptr;
			AurieStatus last_status = AURIE_SUCCESS;

			last_status = ObpLookupInterfaceOwner(
				InterfaceName,
				true,
				owner_module,
				table_entry
			);

			if (!AurieSuccess(last_status))
				return last_status;

			return ObpDestroyInterface(
				owner_module,
				table_entry->Interface,
				true,
				true
			);
		}

		AurieStatus ObpLookupInterfaceOwnerExport(
			IN const char* InterfaceName, 
			IN const char* ExportName,
			OUT PVOID& ExportAddress
		)
		{
			AurieStatus last_status = AURIE_SUCCESS;

			AurieModule* interface_owner = nullptr;
			AurieInterfaceTableEntry* table_entry = nullptr;

			// First, look up the interface owner AurieModule
			last_status = ObpLookupInterfaceOwner(
				InterfaceName,
				true,
				interface_owner,
				table_entry
			);

			if (!AurieSuccess(last_status))
				return last_status;

			// Now, get the module base address
			PVOID module_base_address = MdpGetModuleBaseAddress(
				interface_owner
			);

			// Module has no base address?
			if (!module_base_address)
				return AURIE_FILE_PART_NOT_FOUND;

			// Get the thing
			PVOID procedure_address = GetProcAddress(
				reinterpret_cast<HMODULE>(module_base_address),
				ExportName
			);

			// No export with that name...
			if (!procedure_address)
				return AURIE_OBJECT_NOT_FOUND;

			ExportAddress = procedure_address;
			return AURIE_SUCCESS;
		}

		AurieObjectType ObpGetObjectType(
			IN AurieObject* Object
		)
		{
			return Object->GetObjectType();
		}

		void ObpSetModuleOperationCallback(
			IN AurieModule* Module, 
			IN AurieModuleCallback CallbackRoutine
		)
		{
			Module->ModuleOperationCallback = CallbackRoutine;
		}

		void ObpDispatchModuleOperationCallbacks(
			IN AurieModule* AffectedModule, 
			IN AurieEntry Routine, 
			IN bool IsFutureCall
		)
		{
			// Determine the operation type
			// Yes I know, this is ugly, if you know a better solution
			// feel free to PR / tell me.
			AurieModuleOperationType current_operation_type = AURIE_OPERATION_UNKNOWN;

			if (Routine == AffectedModule->ModulePreinitialize)
				current_operation_type = AURIE_OPERATION_PREINITIALIZE;
			else if (Routine == AffectedModule->ModuleInitialize)
				current_operation_type = AURIE_OPERATION_INITIALIZE;
			else if (Routine == AffectedModule->ModuleUnload)
				current_operation_type = AURIE_OPERATION_UNLOAD;
			
			AurieOperationInfo operation_information = ObpCreateOperationInfo(
				AffectedModule,
				IsFutureCall
			);

			for (auto& loaded_module : g_LdrModuleList)
			{
				if (!loaded_module.ModuleOperationCallback)
					continue;

				loaded_module.ModuleOperationCallback(
					AffectedModule,
					current_operation_type,
					&operation_information
				);
			}
		}

		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module, 
			IN AurieInterfaceTableEntry& Entry
		)
		{
			Module->InterfaceTable.push_back(Entry);

			return AURIE_SUCCESS;
		}

		AurieOperationInfo ObpCreateOperationInfo(
			IN AurieModule* Module,
			IN bool IsFutureCall
		)
		{
			AurieOperationInfo operation_information = {};

			operation_information.IsFutureCall = IsFutureCall;
			operation_information.ModuleBaseAddress = MdpGetModuleBaseAddress(Module);

			return operation_information;
		}

		AurieStatus ObpDestroyInterface(
			IN AurieModule* Module, 
			IN AurieInterfaceBase* Interface,
			IN bool Notify,
			IN bool RemoveFromList
		)
		{
			if (Notify)
			{
				Interface->Destroy();
			}

			if (RemoveFromList)
			{
				Module->InterfaceTable.remove_if(
					[Interface](const AurieInterfaceTableEntry& Entry) -> bool
					{
						// Remove all interface entries with this one interface
						return Entry.Interface == Interface;
					}
				);
			}

			return AURIE_SUCCESS;
		}

		AurieStatus ObpLookupInterfaceOwner(
			IN const char* InterfaceName,
			IN bool CaseInsensitive,
			OUT AurieModule*& Module,
			OUT AurieInterfaceTableEntry*& TableEntry
		)
		{
			// Loop every single module
			for (auto& loaded_module : Internal::g_LdrModuleList)
			{
				// Check if we found it in this module
				auto iterator = std::find_if(
					loaded_module.InterfaceTable.begin(),
					loaded_module.InterfaceTable.end(),
					[CaseInsensitive, InterfaceName](const AurieInterfaceTableEntry& entry) -> bool
					{
						// Do a case insensitive comparison if needed
						if (CaseInsensitive)
						{
							return !_stricmp(entry.InterfaceName, InterfaceName);
						}

						return !strcmp(entry.InterfaceName, InterfaceName);
					}
				);

				// We found the interface in the current module!
				if (iterator != std::end(loaded_module.InterfaceTable))
				{
					Module = &loaded_module;
					TableEntry = &(*iterator);
					return AURIE_SUCCESS;
				}
			}

			// We didn't find any interface with that name.
			return AURIE_OBJECT_NOT_FOUND;
		}
	}

	AurieStatus ObGetInterface(
		IN const char* InterfaceName,
		OUT AurieInterfaceBase*& Interface
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		AurieModule* owner_module = nullptr;
		AurieInterfaceTableEntry* interface_entry = nullptr;

		last_status = Internal::ObpLookupInterfaceOwner(
			InterfaceName,
			true,
			owner_module,
			interface_entry
		);

		if (!AurieSuccess(last_status))
			return last_status;

		Interface = interface_entry->Interface;
		return AURIE_SUCCESS;
	}

	AurieStatus ObDestroyInterface(
		IN AurieModule* Module,
		IN const char* InterfaceName
	)
	{
		AurieModule* owner_module = nullptr;
		AurieInterfaceTableEntry* table_entry = nullptr;

		AurieStatus last_status = Internal::ObpLookupInterfaceOwner(
			InterfaceName,
			true,
			owner_module,
			table_entry
		);

		if (!AurieSuccess(last_status))
			return last_status;

		if (owner_module != Module)
			return AURIE_ACCESS_DENIED;

		last_status = Internal::ObpDestroyInterface(
			Module,
			table_entry->Interface,
			true,
			true
		);

		return AURIE_SUCCESS;
	}
}

