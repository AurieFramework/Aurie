#ifndef AURIE_MEMORY_H_
#define AURIE_MEMORY_H_

#include "../framework.hpp"

namespace Aurie
{
	// Queries the version of the running framework
	EXPORTED void MmGetFrameworkVersion(
		OUT OPTIONAL short* Major,
		OUT OPTIONAL short* Minor,
		OUT OPTIONAL short* Patch
	);

	// Allocates memory that is freed after the Aurie Framework and all modules unload
	EXPORTED PVOID MmAllocatePersistentMemory(
		IN size_t Size
	);

	// Allocates memory that is automatically freed upon a module's unloading
	EXPORTED PVOID MmAllocateMemory(
		IN AurieModule* Owner,
		IN size_t Size
	);

	// Frees persistent memory allocated via MmAllocatePersistentMemory
	EXPORTED AurieStatus MmFreePersistentMemory(
		IN PVOID AllocationBase
	);

	// Frees memory allocated via MmAllocateMemory
	EXPORTED AurieStatus MmFreeMemory(
		IN AurieModule* Owner,
		IN PVOID AllocationBase
	);

	// Scans a module's memory for an array of bytes
	EXPORTED size_t MmSigscanModule(
		IN const wchar_t* ModuleName,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	);

	// Scans a memory region for an array of bytes
	EXPORTED size_t MmSigscanRegion(
		IN const unsigned char* RegionBase,
		IN const size_t RegionSize,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	);

	// Creates a trampoline hook
	EXPORTED AurieStatus MmCreateHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier,
		IN PVOID SourceFunction,
		IN PVOID DestinationFunction,
		OUT OPTIONAL PVOID* Trampoline
	);

	// Checks if a given trampoline hook exists
	EXPORTED AurieStatus MmHookExists(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	// Removes an already existing trampoline hook
	EXPORTED AurieStatus MmRemoveHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	// Retrieves the address of the trampoline for a given hook
	EXPORTED PVOID MmGetHookTrampoline(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	namespace Internal
	{
		// Internal function for allocating memory
		AurieMemoryAllocation MmpAllocateMemory(
			IN const size_t AllocationSize,
			IN AurieModule* const OwnerModule
		);

		// Verifies that a callback resides in an expected module region
		AurieStatus MmpVerifyCallback(
			IN HMODULE Module,
			IN PVOID CallbackRoutine
		);

		// Internal routine for freeing memory
		void MmpFreeMemory(
			IN AurieModule* OwnerModule,
			IN PVOID AllocationBase,
			IN bool RemoveTableEntry
		);
		
		// Adds an AurieMemoryAllocation to the internal table
		AurieMemoryAllocation* MmpAddAllocationToTable(
			IN const AurieMemoryAllocation& Allocation
		);

		// Checks whether a memory is allocated by a certain module
		EXPORTED bool MmpIsAllocatedMemory(
			IN AurieModule* Module,
			IN PVOID AllocationBase
		);

		// Internal function for signature scanning a memory region
		EXPORTED AurieStatus MmpSigscanRegion(
			IN const unsigned char* RegionBase,
			IN const size_t RegionSize,
			IN const unsigned char* Pattern,
			IN const char* PatternMask,
			OUT uintptr_t& PatternBase
		);

		// Removes a memory allocation from the module's table without freeing the memory
		void MmpRemoveAllocationsFromTable(
			IN AurieModule* OwnerModule,
			IN const PVOID AllocationBase
		);

		// Adds an AurieHook object to the module's hook table
		AurieHook* MmpAddHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieHook&& Hook
		);

		// Internal routine for removing trampoline hooks
		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN bool RemoveFromTable
		);

		// Removes an AurieHook object from the module's hook table
		void MmpRemoveHookFromTable(
			IN AurieModule* Module,
			IN AurieHook* Hook
		);

		// Internal method for looking up hook objects
		AurieStatus MmpLookupHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieHook*& Hook
		);

		// Internal method for creating trampoline hooks
		AurieHook* MmpCreateHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceFunction,
			IN PVOID DestinationFunction
		);
	}
}

#endif // AURIE_MEMORY_H_