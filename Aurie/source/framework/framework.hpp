// File: framework.hpp
// 
// Defines stuff shared throughout the framework only.
// Structs are actually defined here, and shared all throughout the Aurie Framework project.
// To define a struct that's exposed to loadable modules, see shared.hpp

#ifndef AURIE_FRAMEWORK_H_
#define AURIE_FRAMEWORK_H_

#include "shared.hpp"
#include <Windows.h>


namespace Aurie
{
	// Base class for every Aurie object
	// Objects that inherit from this struct must implement the following functions
	// so that the object manager can properly work with them.
	struct AurieObject
	{
		virtual AurieObjectType GetObjectType() = 0;
	};

	// Describes the interface internally
	struct AurieInterfaceTableEntry : AurieObject
	{
		AurieModule* OwnerModule = nullptr;
		const char* InterfaceName = nullptr;
		AurieInterfaceBase* Interface = nullptr;

		virtual AurieObjectType GetObjectType() override
		{
			return AURIE_OBJECT_INTERFACE;
		}
	};

	struct AurieMemoryAllocation : AurieObject
	{
		PVOID AllocationBase = nullptr;
		size_t AllocationSize = 0;
		AurieModule* OwnerModule = nullptr;

		virtual AurieObjectType GetObjectType() override
		{
			return AURIE_OBJECT_ALLOCATION;
		}
	};

	struct AurieHandle
	{
		AurieModule* Owner;
		AurieObject* Object;
	};

	// A direct representation of a loaded object.
	// Contains internal resources such as the interface table.
	// This structure should be opaque to modules as the contents may change at any time.
	struct AurieModule : AurieObject
	{
		union
		{
			uint8_t Bitfield = 0;
			struct
			{
				// If this bit is set, the module's Initialize function has been called.
				bool IsInitialized : 1;

				// If this bit is set, the module's Preload function has been called.
				// This call to Preload happens before the call to Initialize.
				// 
				// If the Aurie Framework is injected into a running process, this function still runs first,
				// although it obviously runs after the process entrypoint already ran.
				bool IsPreloaded : 1;
			};
		} Flags = {};

		// Describes the image base (and by extent the module).
		union
		{
			HMODULE Module;
			PVOID Pointer;
			uintptr_t Address;
		} ImageBase = {};

		// Specifies the image size in memory.
		uint32_t ImageSize = 0;

		// The path of the loaded image.
		fs::path ImagePath;

		// The address of the Windows entrypoint of the image.
		union
		{
			PVOID Pointer;
			uintptr_t Address;
		} ImageEntrypoint = {};

		// AurieInitialize
		AurieEntry ModuleInitialize = nullptr;

		// AuriePreInitialize (optional)
		AurieEntry ModulePreinitialize = nullptr;

		// __aurie_fwk_init address
		AurieLoaderEntry FrameworkInitialize = nullptr;

		// Interfaces exposed by the module
		std::list<AurieInterfaceTableEntry> InterfaceTable;

		// Memory allocated by the module
		// 
		// If the allocation is made in the global context (i.e. by MmAllocatePersistentMemory)
		// the allocation is put into g_ArInitialImage of the framework module.
		std::list<AurieMemoryAllocation> MemoryAllocations;

		virtual AurieObjectType GetObjectType() override
		{
			return AURIE_OBJECT_MODULE;
		}

		bool operator==(const AurieModule& Other) const
		{
			return (this->ImageBase.Address == Other.ImageBase.Address) &&
				(this->ImageSize == Other.ImageSize) &&
				(this->ImagePath == Other.ImagePath);
		}
	};
}

#include "Memory Manager/memory.hpp"
#include "Module Manager/module.hpp"
#include "Object Manager/object.hpp"
#include "PE Parser/pe.hpp"

#endif // AURIE_FRAMEWORK_H_