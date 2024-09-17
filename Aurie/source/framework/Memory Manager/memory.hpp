#ifndef AURIE_MEMORY_H_
#define AURIE_MEMORY_H_

#include "../framework.hpp"

namespace Aurie
{
	EXPORTED void MmGetFrameworkVersion(
		OUT OPTIONAL short* Major,
		OUT OPTIONAL short* Minor,
		OUT OPTIONAL short* Patch
	);

	// Allocates memory that may be freed only after the Aurie Framework gets unloaded.
	EXPORTED PVOID MmAllocatePersistentMemory(
		IN size_t Size
	);

	EXPORTED PVOID MmAllocateMemory(
		IN AurieModule* Owner,
		IN size_t Size
	);

	EXPORTED AurieStatus MmFreePersistentMemory(
		IN PVOID AllocationBase
	);

	EXPORTED AurieStatus MmFreeMemory(
		IN AurieModule* Owner,
		IN PVOID AllocationBase
	);

	EXPORTED size_t MmSigscanModule(
		IN const wchar_t* ModuleName,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	);

	EXPORTED size_t MmSigscanRegion(
		IN const unsigned char* RegionBase,
		IN const size_t RegionSize,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	);

	EXPORTED AurieStatus MmCreateHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier,
		IN PVOID SourceFunction,
		IN PVOID DestinationFunction,
		OUT OPTIONAL PVOID* Trampoline
	);

	EXPORTED AurieStatus MmHookExists(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	EXPORTED AurieStatus MmRemoveHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	EXPORTED PVOID MmGetHookTrampoline(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	namespace Internal
	{
		AurieMemoryAllocation MmpAllocateMemory(
			IN const size_t AllocationSize,
			IN AurieModule* const OwnerModule
		);

		AurieStatus MmpVerifyCallback(
			IN HMODULE Module,
			IN PVOID CallbackRoutine
		);

		void MmpFreeMemory(
			IN AurieModule* OwnerModule,
			IN PVOID AllocationBase,
			IN bool RemoveTableEntry
		);
		
		AurieMemoryAllocation* MmpAddAllocationToTable(
			IN const AurieMemoryAllocation& Allocation
		);

		EXPORTED bool MmpIsAllocatedMemory(
			IN AurieModule* Module,
			IN PVOID AllocationBase
		);

		EXPORTED AurieStatus MmpSigscanRegion(
			IN const unsigned char* RegionBase,
			IN const size_t RegionSize,
			IN const unsigned char* Pattern,
			IN const char* PatternMask,
			OUT uintptr_t& PatternBase
		);

		void MmpRemoveAllocationsFromTable(
			IN AurieModule* OwnerModule,
			IN const PVOID AllocationBase
		);

		AurieHook* MmpAddHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieHook&& Hook
		);

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN bool RemoveFromTable
		);

		void MmpRemoveHookFromTable(
			IN AurieModule* Module,
			IN AurieHook* Hook
		);

		AurieStatus MmpLookupHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieHook*& Hook
		);

		AurieHook* MmpCreateHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceFunction,
			IN PVOID DestinationFunction
		);

		AurieBreakpoint MmpInitializeBreakpointObject(
			IN PVOID Rip,
			IN AurieBreakpointCallback Callback
		);

		EXPORTED AurieStatus MmpSetBreakpoint(
			IN PVOID Rip,
			IN AurieBreakpointCallback BreakpointCallback
		);

		EXPORTED AurieStatus MmpUnsetBreakpoint(
			IN PVOID Rip
		);

		bool MmpInsertBreakpointOpcode(
			IN PVOID Address,
			IN AurieBreakpoint& BreakpointObject
		);

		bool MmpRemoveBreakpointOpcode(
			IN PVOID Address,
			IN AurieBreakpoint BreakpointObject
		);

		void MmpFreezeCurrentProcess();

		void MmpResumeCurrentProcess();

		LONG MmpExceptionHandler(
			IN PEXCEPTION_POINTERS ExceptionContext
		);

		inline std::map<ULONG64, AurieBreakpoint> g_BreakpointList;
	}
}

#endif // AURIE_MEMORY_H_