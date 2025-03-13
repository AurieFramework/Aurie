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

		Internal::MmpFreezeCurrentProcess();

		// Creates and enables the actual hook
		AurieInlineHook* created_hook = Internal::MmpCreateInlineHook(
			Module,
			HookIdentifier,
			SourceFunction,
			DestinationFunction
		);

		if (!created_hook)
		{
			Internal::MmpResumeCurrentProcess();
			return AURIE_INSUFFICIENT_MEMORY;
		}

		// If the hook is invalid, we're probably passing invalid parameters to it.
		if (!created_hook->HookInstance)
		{
			Internal::MmpResumeCurrentProcess();
			return AURIE_INVALID_PARAMETER;
		}

		if (Trampoline)
			*Trampoline = created_hook->HookInstance.original<PVOID>();

		Internal::MmpResumeCurrentProcess();
		return AURIE_SUCCESS;
	}

	AurieStatus MmHookExists(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	)
	{
		AurieInlineHook* inline_hook_object = nullptr;
		AurieMidHook* mid_hook_object = nullptr;

		AurieStatus last_status = AURIE_SUCCESS;

		last_status = Internal::MmpLookupInlineHookByName(
			Module,
			HookIdentifier,
			inline_hook_object
		);

		if (!AurieSuccess(last_status))
		{
			last_status = Internal::MmpLookupMidHookByName(
				Module,
				HookIdentifier,
				mid_hook_object
			);
		}

		return last_status;
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
		AurieInlineHook* hook_object = nullptr;

		AurieStatus last_status = Internal::MmpLookupInlineHookByName(
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

	AurieStatus MmCreateMidfunctionHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier,
		IN PVOID SourceAddress,
		IN AurieMidHookFunction TargetHandler
	)
	{
		if (AurieSuccess(MmHookExists(Module, HookIdentifier)))
			return AURIE_OBJECT_ALREADY_EXISTS;

		Internal::MmpFreezeCurrentProcess();

		// Creates and enables the actual hook
		AurieMidHook* created_hook = Internal::MmpCreateMidHook(
			Module,
			HookIdentifier,
			SourceAddress,
			TargetHandler
		);

		Internal::MmpResumeCurrentProcess();

		if (!created_hook)
			return AURIE_INSUFFICIENT_MEMORY;

		// If the hook is invalid, we're probably passing invalid parameters to it.
		if (!created_hook->HookInstance)
			return AURIE_INVALID_PARAMETER;

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
			allocation.AllocationBase = reinterpret_cast<void*>(new char[AllocationSize]);
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

			delete[] AllocationBase;
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

		AurieInlineHook* MmpAddInlineHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieInlineHook&& Hook
		)
		{
			return &OwnerModule->InlineHooks.emplace_back(std::move(Hook));
		}

		AurieMidHook* MmpAddMidHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieMidHook&& Hook
		)
		{
			return &OwnerModule->MidHooks.emplace_back(std::move(Hook));
		}

		AurieStatus MmpRemoveInlineHook(
			IN AurieModule* Module,
			IN AurieInlineHook* Hook,
			IN bool RemoveFromTable
		)
		{
			// Do the unhook atomically
			Internal::MmpFreezeCurrentProcess();

			Hook->HookInstance = {};

			Internal::MmpResumeCurrentProcess();

			if (RemoveFromTable)
			{
				MmpRemoveInlineHookFromTable(
					Module,
					Hook
				);
			}

			return AURIE_SUCCESS;
		}

		AurieStatus MmpRemoveMidHook(
			IN AurieModule* Module,
			IN AurieMidHook* Hook,
			IN bool RemoveFromTable
		)
		{
			// Do the unhook atomically
			Internal::MmpFreezeCurrentProcess();

			Hook->HookInstance = {};

			Internal::MmpResumeCurrentProcess();

			if (RemoveFromTable)
			{
				MmpRemoveMidHookFromTable(
					Module,
					Hook
				);
			}

			return AURIE_SUCCESS;
		}

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN bool RemoveFromTable
		)
		{
			AurieInlineHook* inline_hook_object = nullptr;
			AurieStatus last_status = AURIE_SUCCESS;

			// Try to look it up in the inline hook table
			last_status = MmpLookupInlineHookByName(
				Module,
				HookIdentifier,
				inline_hook_object
			);

			// If we found it, we can remove it
			if (AurieSuccess(last_status))
			{
				return MmpRemoveInlineHook(
					Module,
					inline_hook_object,
					RemoveFromTable
				);
			}

			// We know it's not an inline hook, so try searching for a midhook
			AurieMidHook* mid_hook_object = nullptr;
			last_status = MmpLookupMidHookByName(
				Module,
				HookIdentifier,
				mid_hook_object
			);

			// If we found it, remove it
			if (AurieSuccess(last_status))
			{
				return MmpRemoveMidHook(
					Module,
					mid_hook_object,
					RemoveFromTable
				);
			}

			// Else it's a non-existent hook.
			return AURIE_OBJECT_NOT_FOUND;
		}

		void MmpRemoveInlineHookFromTable(
			IN AurieModule* Module,
			IN AurieInlineHook* Hook
		)
		{
			std::erase_if(
				Module->InlineHooks,
				[Hook](const AurieInlineHook& Entry) -> bool
				{
					return Entry == *Hook;
				}
			);
		}

		void MmpRemoveMidHookFromTable(
			IN AurieModule* Module,
			IN AurieMidHook* Hook
		)
		{
			std::erase_if(
				Module->MidHooks,
				[Hook](const AurieMidHook& Entry) -> bool
				{
					return Entry == *Hook;
				}
			);
		}

		AurieStatus MmpLookupInlineHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieInlineHook*& Hook
		)
		{
			auto iterator = std::find_if(
				Module->InlineHooks.begin(),
				Module->InlineHooks.end(),
				[HookIdentifier](AurieInlineHook& Object) -> bool
				{
					return Object.Identifier == HookIdentifier;
				}
			);

			if (iterator == std::end(Module->InlineHooks))
				return AURIE_OBJECT_NOT_FOUND;

			Hook = &(*iterator);

			return AURIE_SUCCESS;
		}

		AurieStatus MmpLookupMidHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieMidHook*& Hook
		)
		{
			auto iterator = std::find_if(
				Module->MidHooks.begin(),
				Module->MidHooks.end(),
				[HookIdentifier](AurieMidHook& Object) -> bool
				{
					return Object.Identifier == HookIdentifier;
				}
			);

			if (iterator == std::end(Module->MidHooks))
				return AURIE_OBJECT_NOT_FOUND;

			Hook = &(*iterator);

			return AURIE_SUCCESS;
		}

		AurieInlineHook* MmpCreateInlineHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceFunction,
			IN PVOID DestinationFunction
		)
		{
			// Create the hook object
			AurieInlineHook hook = {};
			hook.Owner = Module;
			hook.Identifier = HookIdentifier;
			hook.HookInstance = safetyhook::create_inline(SourceFunction, DestinationFunction);

			// Add the hook to the table
			return MmpAddInlineHookToTable(Module, std::move(hook));
		}

		AurieMidHook* MmpCreateMidHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceInstruction,
			IN AurieMidHookFunction TargetFunction
		)
		{
			AurieMidHook hook = {};
			hook.Owner = Module;
			hook.Identifier = HookIdentifier;
			hook.HookInstance = safetyhook::create_mid(SourceInstruction, reinterpret_cast<safetyhook::MidHookFn>(TargetFunction));

			return MmpAddMidHookToTable(Module, std::move(hook));
		}

		void MmpFreezeCurrentProcess()
		{
			ElForEachThread(
				[](IN const THREADENTRY32& Entry) -> bool
				{
					// Skip my thread
					if (GetCurrentThreadId() == Entry.th32ThreadID)
						return false;

					// Skip everything that's not my process
					if (GetCurrentProcessId() != Entry.th32OwnerProcessID)
						return false;

					HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, Entry.th32ThreadID);

					if (!thread)
						return false;

					SuspendThread(thread);
					CloseHandle(thread);

					return false;
				}
			);
		}

		void MmpResumeCurrentProcess()
		{
			ElForEachThread(
				[](IN const THREADENTRY32& Entry) -> bool
				{
					// Skip my thread
					if (GetCurrentThreadId() == Entry.th32ThreadID)
						return false;

					// Skip everything that's not my process
					if (GetCurrentProcessId() != Entry.th32OwnerProcessID)
						return false;

					HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, Entry.th32ThreadID);

					if (!thread)
						return false;

					ResumeThread(thread);
					CloseHandle(thread);

					return false;
				}
			);
		}
	}
}
