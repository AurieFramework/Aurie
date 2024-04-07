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
			
			// TODO
		}

		bool ObpIsCallbackPartial(
			IN AurieCallback* CallbackObject
		)
		{
			return CallbackObject->Flags.IsPartial;
		}

		bool ObpIsCallbackDeferDeleted(
			IN AurieCallback* CallbackObject
		)
		{
			return CallbackObject->Flags.DeferredDeletion;
		}

		AurieStatus ObpLookupCallbackByName(
			IN const char* Name, 
			IN bool CaseSensitive, 
			OPTIONAL OUT AurieCallback** CallbackObject
		)
		{
			auto callback_position = std::find_if(
				g_ObpCallbackList.begin(),
				g_ObpCallbackList.end(),
				[Name, CaseSensitive](AurieCallback& Object) -> bool
				{
					if (CaseSensitive)
						return !strcmp(Name, Object.CallbackName);

					return !_stricmp(Name, Object.CallbackName);
				}
			);

			if (callback_position == g_ObpCallbackList.cend())
				return AURIE_OBJECT_NOT_FOUND;

			if (CallbackObject)
				*CallbackObject = &(*callback_position);

			return AURIE_SUCCESS;
		}

		AurieStatus ObpAddInterfaceToTable(
			IN AurieModule* Module, 
			IN AurieInterfaceTableEntry& Entry
		)
		{
			Module->InterfaceTable.push_back(Entry);

			return AURIE_SUCCESS;
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

		AurieStatus ObpCreateCallbackObject(
			IN AurieModule* Owner,
			IN const char* CallbackName,
			IN AurieCallbackEntryEx PreCallback, 
			IN AurieCallbackEntry PostCallback, 
			IN bool IsDispatchable,
			IN bool IsPartial, 
			OUT AurieCallback** CallbackObject
		)
		{
			if (AurieSuccess(ObpLookupCallbackByName(CallbackName, false, nullptr)))
				return AURIE_OBJECT_ALREADY_EXISTS;

			AurieCallback new_callback;
			ObpInitializeCallbackObject(
				&new_callback,
				Owner,
				CallbackName,
				PreCallback,
				nullptr,
				nullptr,
				PostCallback,
				IsDispatchable,
				IsPartial
			);

			*CallbackObject = ObpAddCallbackToTable(new_callback);
			return AURIE_SUCCESS;
		}

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
		)
		{
			CallbackObject->CallbackName = CallbackName;
			CallbackObject->OwnerModule = Owner;
			CallbackObject->PreCallback = PreCallback;
			CallbackObject->PreInvokeCallback = PreInvokeCallback;
			CallbackObject->PostInvokeCallback = PostInvokeCallback;
			CallbackObject->PostCallback = PostCallback;

			CallbackObject->Flags.IsPartial = IsPartial;
			CallbackObject->Flags.Dispatchable = IsDispatchable;
			CallbackObject->Flags.Reserved = 0;
		}

		AurieCallback* ObpAddCallbackToTable(
			IN const AurieCallback& Callback
		)
		{
			return &g_ObpCallbackList.emplace_back(Callback);
		}

		bool ObpIsCallbackMethodPresent(
			IN AurieCallback* CallbackObject, 
			IN AurieCallbackEntry CallbackMethod
		)
		{
			return std::find(
				CallbackObject->Callbacks.begin(),
				CallbackObject->Callbacks.end(),
				CallbackMethod
			) != std::end(CallbackObject->Callbacks);
		}

		size_t ObpQueryCallbackMethodCount(
			IN AurieCallback* CallbackObject
		)
		{
			return CallbackObject->Callbacks.size();
		}

		AurieStatus ObpAssignCallback(
			IN AurieCallback* CallbackObject,
			IN uint32_t Position,
			IN AurieCallbackEntry CallbackEntry
		)
		{
			// Make sure we're not doing OOB access
			if (Position > ObpQueryCallbackMethodCount(CallbackObject))
				return AURIE_INVALID_PARAMETER;

			// Make sure the method isn't already present in the callback object
			if (ObpIsCallbackMethodPresent(CallbackObject, CallbackEntry))
				return AURIE_OBJECT_ALREADY_EXISTS;

			// Craft the iterator at Position
			auto target_position = CallbackObject->Callbacks.begin();
			std::advance(target_position, Position);

			// Insert shit there
			CallbackObject->Callbacks.insert(
				target_position,
				CallbackEntry
			);

			return AURIE_SUCCESS;
		}

		AurieStatus ObpNotifyCallback(
			IN AurieCallback* CallbackObject, 
			IN AurieObject* AffectedObject, 
			IN PVOID Argument1, 
			IN PVOID Argument2
		)
		{
			AurieStatus last_status = CallbackObject->PreCallback(
				AffectedObject, 
				Argument1, 
				Argument2
			);

			if (!AurieSuccess(last_status))
				return last_status;

			for (AurieCallbackEntry callback : CallbackObject->Callbacks)
			{
				struct
				{
					PVOID TargetMethod;
					PVOID Argument1;
					PVOID Argument2;
				} pre_invoke_arguments = {
					.TargetMethod = callback,
					.Argument1 = Argument1,
					.Argument2 = Argument2
				};

				// Dispatch the pre-invoke callback first
				last_status = CallbackObject->PreInvokeCallback(
					AffectedObject, 
					&pre_invoke_arguments, 
					nullptr
				);

				// Skip any callbacks for which the preinvoke callback returned 
				if (!AurieSuccess(last_status))
					continue;

				callback(AffectedObject, Argument1, Argument2);

				CallbackObject->PostInvokeCallback(
					AffectedObject,
					Argument1,
					Argument2
				);
			}

			CallbackObject->PostCallback(
				AffectedObject,
				Argument1,
				Argument2
			);

			return AURIE_SUCCESS;
		}

		AurieStatus ObpDeferCallbackDeletion(
			IN AurieCallback* CallbackObject
		)
		{
			if (CallbackObject->Flags.DeferredDeletion)
				return AURIE_ALREADY_COMPLETE;

			CallbackObject->Flags.DeferredDeletion = true;

			return AURIE_SUCCESS;
		}

		void ObpDeleteDeferredCallbacks()
		{
			g_ObpCallbackList.remove_if(
				[](AurieCallback& Callback) -> bool
				{
					return Callback.Flags.DeferredDeletion;
				}
			);
		}

		void ObpSetCallbackFlags(
			IN AurieCallback* CallbackObject, 
			IN bool AllowDispatch,
			IN bool IsPartial,
			IN bool DeferDelete
		)
		{
			CallbackObject->Flags.IsPartial = IsPartial;
			CallbackObject->Flags.Dispatchable = AllowDispatch;
			CallbackObject->Flags.DeferredDeletion = DeferDelete;
			CallbackObject->Flags.Reserved = 0;
		}

		bool ObpIsCallbackDispatchable(
			IN AurieCallback* CallbackObject
		)
		{
			return CallbackObject->Flags.Dispatchable;
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

	AurieStatus ObCreateCallback(
		IN AurieModule* OwnerModule, 
		IN const char* CallbackName, 
		IN AurieCallbackEntry CallbackRoutine, 
		OUT AurieCallback** CallbackObject
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		AurieCallback* callback_object = nullptr;

		// Try to look up an existing callback by the unique name (not case-sensitive)
		last_status = Internal::ObpLookupCallbackByName(
			CallbackName,
			false,
			&callback_object
		);

		// If the callback exists (the lookup succeeds),
		// it can be either already registered, in which case we error,
		// or can be a pseudocallback in which case we re-register it as a "full" callback
		if (AurieSuccess(last_status))
		{
			// If the callback isn't partial, then it means another module already registered it
			if (!Internal::ObpIsCallbackPartial(callback_object))
				return AURIE_OBJECT_ALREADY_EXISTS;

			// If the callback **is** partial, we need to unset the IsPartial flag
			Internal::ObpSetCallbackFlags(
				callback_object,
				Internal::ObpIsCallbackDispatchable(callback_object),
				false,
				Internal::ObpIsCallbackDeferDeleted(callback_object)
			);

			// We also want to assign the owner module to it, as it should currently be nullptr
			callback_object->OwnerModule = OwnerModule;
			
			// Now, a full-fledged callback exists and is linked to the module
			return AURIE_SUCCESS;
		}

		// If no callback exists, we create it and push it to the list
		last_status = Internal::ObpCreateCallbackObject(
			OwnerModule,
			CallbackName,
			nullptr,
			nullptr,
			true,
			false,
			&callback_object
		);

		// If we failed creation, don't touch the contents of the user buffer
		if (!AurieSuccess(last_status))
			return last_status;

		// Creation succeded, we can send the callback on it's merry way
		*CallbackObject = callback_object;
		return AURIE_SUCCESS;
	}

	AurieStatus ObDestroyCallback(
		IN AurieModule* OwnerModule, 
		IN AurieCallback* CallbackObject
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		AurieCallback* callback_object = nullptr;

		// If the owner doesn't match, it's either a partial callback,
		// in which case the owner will be nullptr, or not our callback
		// and we can't delete it.
		if (CallbackObject->OwnerModule != OwnerModule 
			&& !Internal::ObpIsCallbackPartial(CallbackObject))
		{
			return AURIE_ACCESS_DENIED;
		}

		return Internal::ObpDeferCallbackDeletion(CallbackObject);
	}

	AurieStatus ObNotifyCallback(
		IN const char* CallbackName, 
		OPTIONAL IN PVOID Argument1,
		OPTIONAL IN PVOID Argument2
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		AurieCallback* callback_object = nullptr;

		// Try to look up an existing callback by the unique name (not case-sensitive)
		last_status = Internal::ObpLookupCallbackByName(
			CallbackName,
			false,
			&callback_object
		);

		// If the callback doesn't exist, we fail...
		if (!AurieSuccess(last_status))
			return last_status;

		if (!Internal::ObpIsCallbackDispatchable(callback_object))
			return AURIE_ACCESS_DENIED;

		// If the object is deferred to be deleted, or 
		// the callback is without an owner, it cannot be notified.
		if (Internal::ObpIsCallbackDeferDeleted(callback_object) ||
			Internal::ObpIsCallbackPartial(callback_object))
		{
			return AURIE_OBJECT_NOT_FOUND;
		}

		for (const auto callback : callback_object->Callbacks)
		{
			AurieStatus callback_status = AURIE_SUCCESS;

			// User-notified callbacks 
			callback_status = callback_object->PreInvokeCallback(
				callback_object, 
				Argument1, 
				Argument2
			);
		}

	}
}

