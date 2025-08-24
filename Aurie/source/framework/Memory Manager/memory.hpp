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

	EXPORTED AurieStatus MmEnableHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	EXPORTED AurieStatus MmDisableHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier
	);

	EXPORTED AurieStatus MmCreateUnsafeHook(
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

	EXPORTED AurieStatus MmCreateMidfunctionHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier,
		IN PVOID SourceAddress,
		IN AurieMidHookFunction TargetHandler
	);

	EXPORTED AurieStatus MmGetRegistersForHook(
		IN AurieModule* Module,
		IN std::string_view HookIdentifier,
		OUT ProcessorContext& Context
	);

	namespace Internal
	{
		AurieMemoryAllocation MmpAllocateMemory(
			IN const size_t AllocationSize,
			IN AurieModule* const OwnerModule
		);

		AurieStatus MmpVerifyCallback(
			IN AurieModule* Module,
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

		AurieInlineHook* MmpAddInlineHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieInlineHook&& Hook
		);

		AurieRpHook* MmpAddRPInlineHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieRpHook&& Hook
		);

		AurieMidHook* MmpAddMidHookToTable(
			IN AurieModule* OwnerModule,
			IN AurieMidHook&& Hook
		);

		AurieStatus MmpRemoveInlineHook(
			IN AurieModule* Module,
			IN AurieInlineHook* Hook,
			IN bool RemoveFromTable
		);

		AurieStatus MmpRemoveMidHook(
			IN AurieModule* Module,
			IN AurieMidHook* Hook,
			IN bool RemoveFromTable
		);

		AurieStatus MmpRemoveRPHook(
			IN AurieModule* Module,
			IN AurieRpHook* Hook,
			IN bool RemoveFromTable
		);

		AurieStatus MmpRemoveHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN bool RemoveFromTable
		);

		AurieStatus MmpEnableHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier
		);

		AurieStatus MmpDisableHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier
		);

		void MmpRemoveInlineHookFromTable(
			IN AurieModule* Module,
			IN AurieInlineHook* Hook
		);

		void MmpRemoveMidHookFromTable(
			IN AurieModule* Module,
			IN AurieMidHook* Hook
		);

		void MmpRemoveRPHookFromTable(
			IN AurieModule* Module,
			IN AurieRpHook* Hook
		);

		AurieStatus MmpLookupInlineHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieInlineHook*& Hook
		);

		AurieStatus MmpLookupMidHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieMidHook*& Hook
		);

		AurieStatus MmpLookupRPHookByName(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			OUT AurieRpHook*& Hook
		);

		AurieInlineHook* MmpCreateInlineHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceFunction,
			IN PVOID DestinationFunction
		);

		AurieMidHook* MmpCreateMidHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceInstruction,
			IN AurieMidHookFunction TargetFunction
		);

		AurieRpHook* MmpCreateRpHook(
			IN AurieModule* Module,
			IN std::string_view HookIdentifier,
			IN PVOID SourceInstruction,
			IN PVOID DestinationFunction
		);

		void MmpFreezeCurrentProcess();

		void MmpResumeCurrentProcess();

		AurieStatus MmpGetRegistersForRPHook(
			IN AurieRpHook* HookObject,
			OUT ProcessorContext& Context
		);
	}
}

#endif // AURIE_MEMORY_H_