#ifndef AURIE_MEMORY_H_
#define AURIE_MEMORY_H_

#include "../framework.hpp"

namespace Aurie
{
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
		IN const char* HookIdentifier,
		IN PVOID SourceFunction,
		IN PVOID TargetFunction
	);

	EXPORTED AurieStatus MmHookExists(
		IN AurieModule* Module,
		IN const char* HookIdentifier
	);

	EXPORTED AurieStatus MmRemoveHook(
		IN AurieModule* Module,
		IN const char* HookIdentifier
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

		AurieHook* MmpAddHookToTable(
			IN const AurieHook& Hook
		);

		AurieStatus MmpCreateHook(
			IN AurieModule* Module,
			IN const char* HookIdentifier,
			IN PVOID SourceFunction,
			IN PVOID TargetFunction,
			OUT AurieHook*& HookObject
		);

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN const AurieHook& Hook
		);

		void MmpInitializeHookObject(
			OUT AurieHook& HookObject,
			IN AurieModule* OwnerModule,
			IN PVOID SourceFunction,
			IN PVOID TargetFunction,
			IN PVOID Trampoline,
			IN const char* HookIdentifier
		);

		// MinHook Multihook (m417z/minhook) uses a unique identifier
		// to allow multiple modules hooking the same function.
		// It's important these don't overlap.
		ULONG_PTR MmpGetModuleHookId(
			IN AurieModule* Module
		);

		AurieStatus MmpLookupHookByName(
			IN AurieModule* Module,
			IN const char* HookIdentifier,
			OUT AurieHook*& HookObject
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

		void MmpRemoveHookFromTable(
			IN AurieModule* OwnerModule,
			IN const AurieHook& Hook
		);
	}
}

#endif // AURIE_MEMORY_H_