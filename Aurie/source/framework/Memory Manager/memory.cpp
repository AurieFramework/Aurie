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

		AurieBreakpoint MmpInitializeBreakpointObject(
			IN PVOID Rip, 
			IN AurieBreakpointCallback Callback
		)
		{
			AurieBreakpoint object;
			object.BreakpointAddress = Rip;
			object.Callback = Callback;

			return object;
		}

		EXPORTED AurieStatus MmpSetBreakpoint(
			IN PVOID Rip, 
			IN AurieBreakpointCallback BreakpointCallback
		)
		{
			// Only one breakpoint can be at a given address
			if (g_BreakpointList.contains(reinterpret_cast<ULONG64>(Rip)))
				return AURIE_OBJECT_ALREADY_EXISTS;

			AurieBreakpoint breakpoint_object = MmpInitializeBreakpointObject(Rip, BreakpointCallback);

			// Suspend all threads except ours, such that the 
			// process of inserting a breakpoint can be done atomically.
			MmpSuspendAllThreads();

			// Try to insert the breakpoint opcode
			if (!MmpInsertBreakpointOpcode(
				Rip,
				breakpoint_object
			))
			{
				MmpResumeAllThreads();
				return AURIE_EXTERNAL_ERROR;
			}

			// Insert the breakpoint object
			g_BreakpointList.insert(
				{
					reinterpret_cast<ULONG64>(Rip),
					breakpoint_object
				}
			);

			MmpResumeAllThreads();
			return AURIE_SUCCESS;
		}

		EXPORTED AurieStatus MmpUnsetBreakpoint(
			IN PVOID Rip
		)
		{
			const ULONG64 rip = reinterpret_cast<ULONG64>(Rip);

			if (!g_BreakpointList.contains(rip))
				return AURIE_OBJECT_NOT_FOUND;

			// Suspend all threads except ours such that the process 
			// can be done atomically.
			MmpSuspendAllThreads();
			
			// Remove the breakpoint opcode
			MmpRemoveBreakpointOpcode(
				Rip,
				g_BreakpointList[rip]
			);

			// Resume all threads
			MmpResumeAllThreads();

			// Remove our entry
			g_BreakpointList.erase(rip);

			return AURIE_SUCCESS;
		}

		bool MmpInsertBreakpointOpcode(
			IN PVOID Address,
			IN AurieBreakpoint& BreakpointObject
		)
		{
			constexpr unsigned char nop = { 0x90 };

			// TODO: Check that the address is on an instruction boundary.
			// Save the replaced byte
			BreakpointObject.ReplacedByte = *static_cast<PUCHAR>(Address);

			// Use WPM because it tries really hard to get the memory written:
			// https://devblogs.microsoft.com/oldnewthing/20181206-00/?p=100415
			return WriteProcessMemory(
				GetCurrentProcess(),
				Address,
				&nop,
				sizeof(nop),
				nullptr
			);
		}

		bool MmpRemoveBreakpointOpcode(
			IN PVOID Address, 
			IN AurieBreakpoint BreakpointObject
		)
		{
			// Use WPM because it tries really hard to get the memory written:
			// https://devblogs.microsoft.com/oldnewthing/20181206-00/?p=100415
			return WriteProcessMemory(
				GetCurrentProcess(),
				Address,
				&BreakpointObject.ReplacedByte,
				sizeof(BreakpointObject.ReplacedByte),
				nullptr
			);
		}

		void MmpSuspendAllThreads()
		{
			ElForEachThread(
				[](IN const THREADENTRY32& Entry) -> bool
				{
					HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, Entry.th32ThreadID);

					if (!thread)
						return false;

					SuspendThread(thread);
					CloseHandle(thread);

					return false;
				}
			);
		}

		void MmpResumeAllThreads()
		{
			ElForEachThread(
				[](IN const THREADENTRY32& Entry) -> bool
				{
					HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, Entry.th32ThreadID);

					if (!thread)
						return false;

					ResumeThread(thread);
					CloseHandle(thread);

					return false;
				}
			);
		}

		LONG MmpExceptionHandler(
			IN PEXCEPTION_POINTERS ExceptionContext
		)
		{
			PCONTEXT processor_context = ExceptionContext->ContextRecord;
			const ULONG32 exception_reason = ExceptionContext->ExceptionRecord->ExceptionCode;

			// We don't care about any exceptions other than #BP. 
			if (exception_reason != EXCEPTION_BREAKPOINT)
				return EXCEPTION_CONTINUE_SEARCH;

			// Continue searching for an exception handler if we're not breakpointed here.
			if (!g_BreakpointList.contains(processor_context->Rip))
				return EXCEPTION_CONTINUE_SEARCH;

			// Invoke the callback, let it freely adjust the CPU context.
			bool should_continue_execution = g_BreakpointList[processor_context->Rip].Callback(
				processor_context,
				exception_reason
			);

			return should_continue_execution ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
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
