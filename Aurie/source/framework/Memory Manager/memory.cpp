#include "memory.hpp"
#include <MinHook.h>

namespace Aurie
{
	PVOID MmAllocatePersistentMemory(
		IN size_t Size
	)
	{
		return MmAllocateMemory(
			g_ArInitialImage,
			Size
		);
	}

	PVOID MmAllocateMemory(
		IN AurieModule* Owner,
		IN size_t Size
	)
	{
		AurieMemoryAllocation allocation = Internal::MmpAllocateMemory(
			Size,
			Owner
		);

		Internal::MmpAddAllocationToTable(allocation);

		return allocation.AllocationBase;
	}

	AurieStatus MmFreePersistentMemory(
		IN PVOID AllocationBase
	)
	{
		return MmFreeMemory(
			g_ArInitialImage, 
			AllocationBase
		);
	}

	AurieStatus MmFreeMemory(
		IN AurieModule* Owner, 
		IN PVOID AllocationBase
	)
	{
		if (!Internal::MmpIsAllocatedMemory(
			Owner,
			AllocationBase
		))
		{
			return AURIE_INVALID_PARAMETER;
		}

		Internal::MmpFreeMemory(
			Owner,
			AllocationBase,
			true
		);

		return AURIE_SUCCESS;
	}

	namespace Internal
	{
		AurieMemoryAllocation MmpAllocateMemory(
			IN const size_t AllocationSize, 
			IN AurieModule* const OwnerModule
		)
		{
			AurieMemoryAllocation allocation;
			allocation.AllocationBase = malloc(AllocationSize);
			allocation.AllocationSize = AllocationSize;
			allocation.OwnerModule = OwnerModule;

			return allocation;
		}

		AurieStatus MmpVerifyCallback(
			IN HMODULE Module,
			IN PVOID CallbackRoutine
		)
		{
			if (CallbackRoutine && Module)
				return AURIE_SUCCESS;

			return AURIE_ACCESS_DENIED;
		}

		void MmpFreeMemory(
			IN AurieModule* OwnerModule,
			IN PVOID AllocationBase,
			IN bool RemoveTableEntry
		)
		{
			if (RemoveTableEntry)
			{
				MmpRemoveAllocationsFromTable(
					OwnerModule,
					AllocationBase
				);
			}
			
			free(AllocationBase);
		}

		AurieMemoryAllocation* MmpAddAllocationToTable(
			IN const AurieMemoryAllocation& Allocation
		)
		{
			AurieModule& owner_module = *Allocation.OwnerModule;
			owner_module.MemoryAllocations.push_back(Allocation);

			return &(owner_module.MemoryAllocations.back());
		}

		AurieHook* MmpAddHookToTable(
			IN const AurieHook& Hook
		)
		{
			AurieModule& owner_module = *Hook.Owner;
			owner_module.Hooks.push_back(Hook);

			return &(owner_module.Hooks.back());
		}

		AurieStatus MmpCreateHook(
			IN AurieModule* Module,
			IN const char* HookIdentifier, 
			IN PVOID SourceFunction, 
			IN PVOID TargetFunction, 
			OUT AurieHook*& HookObject
		)
		{
			PVOID trampoline = nullptr;
			MH_STATUS minhook_status = MH_OK;

			// Create the hook with our module's ID
			minhook_status = MH_CreateHookEx(
				Internal::MmpGetModuleHookId(Module),
				SourceFunction,
				TargetFunction,
				&trampoline
			);

			// Handle MinHook errors
			if (minhook_status == MH_ERROR_ALREADY_CREATED)
				return AURIE_OBJECT_ALREADY_EXISTS;

			// Don't like else if here
			if (minhook_status != MH_OK)
				return AURIE_EXTERNAL_ERROR;

			// Create the hook object and add it to the table
			AurieHook hook_object;
			MmpInitializeHookObject(
				hook_object,
				Module,
				SourceFunction,
				TargetFunction,
				trampoline,
				HookIdentifier
			);

			HookObject = MmpAddHookToTable(*HookObject);

			return AURIE_SUCCESS;
		}

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN const AurieHook& Hook
		)
		{
			MH_STATUS minhook_status = MH_RemoveHook(
				MmpGetModuleHookId(Module),
				Hook.SourceFunction
			);

			if (minhook_status != MH_OK)
				return AURIE_EXTERNAL_ERROR;
			
			MmpRemoveHookFromTable(
				Module,
				Hook
			);

			return AURIE_SUCCESS;
		}

		void MmpInitializeHookObject(
			OUT AurieHook& HookObject, 
			IN AurieModule* OwnerModule, 
			IN PVOID SourceFunction, 
			IN PVOID TargetFunction, 
			IN PVOID Trampoline,
			IN const char* HookIdentifier
		)
		{
			HookObject.Owner = OwnerModule;
			HookObject.SourceFunction = SourceFunction;
			HookObject.TargetFunction = TargetFunction;
			HookObject.Trampoline = Trampoline;
			HookObject.Identifier = HookIdentifier;
		}

		ULONG_PTR MmpGetModuleHookId(
			IN AurieModule* Module
		)
		{
			return Module->ImageBase.Address;
		}

		bool MmpIsAllocatedMemory(
			IN AurieModule* Module, 
			IN PVOID AllocationBase
		)
		{
			return std::find_if(
				Module->MemoryAllocations.begin(),
				Module->MemoryAllocations.end(),
				[AllocationBase](const AurieMemoryAllocation& Allocation) -> bool
				{
					return Allocation.AllocationBase == AllocationBase;
				}
			) != Module->MemoryAllocations.end();
		}

		AurieStatus MmpSigscanRegion(
			IN const unsigned char* RegionBase, 
			IN const size_t RegionSize, 
			IN const unsigned char* Pattern, 
			IN const char* PatternMask, 
			OUT uintptr_t& PatternBase
		)
		{
			size_t pattern_size = strlen(PatternMask);

			// Loop all bytes in the region
			for (size_t region_byte = 0; region_byte < RegionSize - pattern_size; region_byte++)
			{
				// Loop all bytes in the pattern and compare them to the bytes in the region
				bool pattern_matches = true;
				for (size_t in_pattern_byte = 0; in_pattern_byte < pattern_size; in_pattern_byte++)
				{
					pattern_matches &= 
						(PatternMask[in_pattern_byte] == '?') || 
						(RegionBase[region_byte + in_pattern_byte] == Pattern[in_pattern_byte]);

					// If it already doesn't match, we don't have to iterate anymore
					if (!pattern_matches)
						break;
				}

				if (pattern_matches)
				{
					PatternBase = reinterpret_cast<uintptr_t>(RegionBase + region_byte);
					return AURIE_SUCCESS;
				}
			}

			return AURIE_OBJECT_NOT_FOUND;
		}

		void MmpRemoveAllocationsFromTable(
			IN AurieModule* OwnerModule,
			IN const PVOID AllocationBase
		)
		{
			OwnerModule->MemoryAllocations.remove_if(
				[AllocationBase](const AurieMemoryAllocation& Allocation)
				{
					return Allocation.AllocationBase == AllocationBase;
				}
			);
		}

		void MmpRemoveHookFromTable(
			IN AurieModule* OwnerModule, 
			IN const AurieHook& Hook
		)
		{
			OwnerModule->Hooks.remove_if(
				[Hook](const AurieHook& ModuleHook)
				{
					return 
						ModuleHook.SourceFunction == Hook.SourceFunction && 
						ModuleHook.TargetFunction == Hook.TargetFunction;
				}
			);
		}
	}

	size_t MmSigscanModule(
		IN const wchar_t* ModuleName, 
		IN const unsigned char* Pattern, 
		IN const char* PatternMask
	)
	{
		// Capture the module we're searching for
		HMODULE module_handle = GetModuleHandleW(ModuleName);
		if (!module_handle)
			return 0;

		AurieStatus last_status = AURIE_SUCCESS;

		// Query the text section address in the module
		uint64_t text_section_base = 0;
		size_t text_section_size = 0;
		last_status = Internal::PpiGetModuleSectionBounds(
			module_handle,
			".text",
			text_section_base,
			text_section_size
		);

		if (!AurieSuccess(last_status))
			return 0;

		return MmSigscanRegion(
			reinterpret_cast<const unsigned char*>(module_handle) + text_section_base,
			text_section_size,
			Pattern,
			PatternMask
		);
	}

	size_t MmSigscanRegion(
		IN const unsigned char* RegionBase,
		IN const size_t RegionSize, 
		IN const unsigned char* Pattern, 
		IN const char* PatternMask
	)
	{
		if (!PatternMask || !strlen(PatternMask))
			return 0;

		uintptr_t pattern_base = 0;
		AurieStatus last_status = AURIE_SUCCESS;

		last_status = Internal::MmpSigscanRegion(
			RegionBase,
			RegionSize,
			Pattern,
			PatternMask,
			pattern_base
		);

		if (!AurieSuccess(last_status))
			return 0;

		return pattern_base;
	}

	AurieStatus MmCreateHook(
		IN AurieModule* Module, 
		IN const char* HookIdentifier, 
		IN PVOID SourceFunction, 
		IN PVOID TargetFunction
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		// Make sure the hook does NOT exist
		last_status = MmHookExists(Module, HookIdentifier);
		if (AurieSuccess(last_status))
			return AURIE_OBJECT_ALREADY_EXISTS;

		AurieHook* hook_object = nullptr;

		return Internal::MmpCreateHook(
			Module,
			HookIdentifier,
			SourceFunction,
			TargetFunction,
			hook_object
		);
	}

	AurieStatus MmHookExists(
		IN AurieModule* Module, 
		IN const char* HookIdentifier
	)
	{
		AurieHook* hook_object = nullptr;

		return Internal::MmpLookupHookByName(
			Module,
			HookIdentifier,
			hook_object
		);
	}

	AurieStatus MmRemoveHook(
		IN AurieModule* Module, 
		IN const char* HookIdentifier
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		// Make sure the hook exists
		AurieHook* hook_object = nullptr;
		last_status = Internal::MmpLookupHookByName(
			Module,
			HookIdentifier,
			hook_object
		);

		if (!AurieSuccess(last_status))
			return last_status;

		return Internal::MmpRemoveHook(
			Module,
			*hook_object
		);
	}

	PVOID MmGetHookTrampoline(
		IN AurieModule* Module, 
		IN const char* HookIdentifier
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		// Make sure the hook exists
		AurieHook* hook_object = nullptr;
		last_status = Internal::MmpLookupHookByName(
			Module,
			HookIdentifier,
			hook_object
		);

		if (!AurieSuccess(last_status))
			return nullptr;

		return hook_object->Trampoline;
	}
}
