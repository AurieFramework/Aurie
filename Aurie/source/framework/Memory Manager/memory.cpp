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

		void MmpAddAllocationToTable(
			IN const AurieMemoryAllocation& Allocation
		)
		{
			AurieModule& owner_module = *Allocation.OwnerModule;
			owner_module.MemoryAllocations.push_back(Allocation);
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
	}
}


