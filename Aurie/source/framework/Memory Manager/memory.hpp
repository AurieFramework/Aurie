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
		
		void MmpAddAllocationToTable(
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
	}
}

#endif // AURIE_MEMORY_H_