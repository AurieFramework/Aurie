#include "memory.hpp"

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
			// TODO: Should probably validate if a module backed by AurieModule is calling.
			// Not checking this might be opening up some detection vectors where unauthorized
			// callers could register and potentially manipulate callbacks.
			if (!Module || !CallbackRoutine)
				return AURIE_INVALID_PARAMETER;

			uint64_t section_offset = 0;
			size_t section_size = 0;
			
			// Query the .text section of the module
			// Unless registering a callback from dynamic code,
			// this check should always pass.
			AurieStatus last_status = AURIE_SUCCESS;
			last_status = PpiGetModuleSectionBounds(
				Module,
				".text",
				section_offset,
				section_size
			);

			if (!AurieSuccess(last_status))
				return last_status;

			uint64_t module_callback = reinterpret_cast<uint64_t>(CallbackRoutine);
			uint64_t module_text_start = reinterpret_cast<uint64_t>(Module) + section_offset;
			uint64_t module_text_end = module_text_start + section_size;
			
			// Verify that the callback is coming from the module's .text section.
			// If it's not, we may be registering a malicious callback.
			if (module_callback < module_text_start || module_callback > module_text_end)
				return AURIE_ACCESS_DENIED;

			return AURIE_SUCCESS;
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
			return &Allocation.OwnerModule->MemoryAllocations.emplace_back(Allocation);
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

		AurieHook* MmpAddHookToTable(
			IN AurieModule* OwnerModule, 
			IN AurieHook&& Hook
		)
		{
			return &OwnerModule->Hooks.emplace_back(std::move(Hook));
		}

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module, 
			IN std::string_view HookIdentifier,
			IN bool RemoveFromTable
		)
		{
			AurieHook* hook_object = nullptr;
			AurieStatus last_status = AURIE_SUCCESS;

			last_status = MmpLookupHookByName(
				Module,
				HookIdentifier,
				hook_object
			);

			if (!AurieSuccess(last_status))
				return last_status;

			hook_object->HookInstance = {};

			if (RemoveFromTable)
			{
				MmpRemoveHookFromTable(
					Module,
					hook_object
				);
			}

			return AURIE_SUCCESS;
		}

		void MmpRemoveHookFromTable(
			IN AurieModule* Module, 
			IN AurieHook* Hook
		)
		{
			std::erase_if(
				Module->Hooks,
				[Hook](const AurieHook& Entry) -> bool
				{
					return Entry == *Hook;
				}
			);
		}

		AurieStatus MmpLookupHookByName(
			IN AurieModule* Module, 
			IN std::string_view HookIdentifier, 
			OUT AurieHook*& Hook
		)
		{
			auto iterator = std::find_if(
				Module->Hooks.begin(),
				Module->Hooks.end(),
				[HookIdentifier](AurieHook& Object) -> bool
				{
					return Object.Identifier == HookIdentifier;
				}
			);

			if (iterator == std::end(Module->Hooks))
				return AURIE_OBJECT_NOT_FOUND;

			Hook = &(*iterator);

			return AURIE_SUCCESS;
		}

		AurieHook* MmpCreateHook(
			IN AurieModule* Module, 
			IN std::string_view HookIdentifier, 
			IN PVOID SourceFunction, 
			IN PVOID DestinationFunction
		)
		{
			AurieHook hook = {};
			hook.Owner = Module;
			hook.Identifier = HookIdentifier;
			hook.HookInstance = safetyhook::create_inline(SourceFunction, DestinationFunction);

			return MmpAddHookToTable(Module, std::move(hook));
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
		IN std::string_view HookIdentifier, 
		IN PVOID SourceFunction, 
		IN PVOID DestinationFunction, 
		OUT OPTIONAL PVOID* Trampoline
	)
	{
		if (AurieSuccess(MmHookExists(Module, HookIdentifier)))
			return AURIE_OBJECT_ALREADY_EXISTS;

		// Creates and enables the actual hook
		AurieHook* created_hook = Internal::MmpCreateHook(
			Module,
			HookIdentifier,
			SourceFunction,
			DestinationFunction
		);

		if (!created_hook)
			return AURIE_INSUFFICIENT_MEMORY;

		// If the hook is invalid, we're probably passing invalid parameters to it.
		if (!created_hook->HookInstance)
			return AURIE_INVALID_PARAMETER;

		if (Trampoline)
			*Trampoline = created_hook->HookInstance.original<PVOID>();
		
		return AURIE_SUCCESS;
	}

	AurieStatus MmHookExists(
		IN AurieModule* Module, 
		IN std::string_view HookIdentifier
	)
	{
		AurieHook* object = nullptr;

		return Internal::MmpLookupHookByName(
			Module,
			HookIdentifier,
			object
		);
	}

	AurieStatus MmRemoveHook(
		IN AurieModule* Module, 
		IN std::string_view HookIdentifier
	)
	{
		// The internal routine checks for the hook's existance, 
		// so we can just call it without performing any other checks.
		return Internal::MmpRemoveHook(
			Module,
			HookIdentifier,
			true
		);
	}

	void* MmGetHookTrampoline(
		IN AurieModule* Module, 
		IN std::string_view HookIdentifier
	)
	{
		AurieHook* hook_object = nullptr;

		AurieStatus last_status = Internal::MmpLookupHookByName(
			Module,
			HookIdentifier,
			hook_object
		);

		if (!AurieSuccess(last_status))
			return nullptr;

		return hook_object->HookInstance.original<PVOID>();
	}

	EXPORTED void MmGetFrameworkVersion(
		OUT OPTIONAL short* Major, 
		OUT OPTIONAL short* Minor, 
		OUT OPTIONAL short* Patch
	)
	{
		if (Major)
			*Major = AURIE_FWK_MAJOR;

		if (Minor)
			*Minor = AURIE_FWK_MINOR;

		if (Patch)
			*Patch = AURIE_FWK_PATCH;
	}

}
