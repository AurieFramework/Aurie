// File: framework.hpp
// 
// Defines stuff shared throughout the framework only.
// Structs are actually defined here, and shared all throughout the Aurie Framework project.
// To define a struct that's exposed to loadable modules, see shared.hpp

#ifndef AURIE_FRAMEWORK_H_
#define AURIE_FRAMEWORK_H_

#include "shared.hpp"

// Include NTSTATUS values from this header, because it has all of them
#include <ntstatus.h>

// Don't include NTSTATUS values from Windows.h and winternl.h
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winternl.h>
#include <list>

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

				// If this bit is set, the module is marked for deletion and will be unloaded by the next
				// call to Aurie::Internal::MdpPurgeMarkedModules
				bool MarkedForPurge : 1;
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

		// The initialize routine for the module
		AurieEntry ModuleInitialize = nullptr;

		// The optional preinitialize routine for the module
		AurieEntry ModulePreinitialize = nullptr;

		// An unload routine for the module
		AurieEntry ModuleUnload = nullptr;

		// The __AurieFrameworkInit function
		AurieLoaderEntry FrameworkInitialize = nullptr;

		// Interfaces exposed by the module
		std::list<AurieInterfaceTableEntry> InterfaceTable;

		// Memory allocated by the module
		// 
		// If the allocation is made in the global context (i.e. by MmAllocatePersistentMemory)
		// the allocation is put into g_ArInitialImage of the framework module.
		std::list<AurieMemoryAllocation> MemoryAllocations;

		// Functions hooked by the module by Mm*Hook functions
		std::list<AurieHook> Hooks;

		// If set, notifies the plugin of any module actions
		AurieModuleCallback ModuleOperationCallback = nullptr;

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

	struct AurieHook : AurieObject
	{
		AurieModule* Owner;
		const char* Identifier;

		PVOID SourceFunction;
		PVOID TargetFunction;
		PVOID Trampoline;

		virtual AurieObjectType GetObjectType() override
		{
			return AURIE_OBJECT_HOOK;
		}
	};

	typedef enum _KTHREAD_STATE
	{
		Initialized,
		Ready,
		Running,
		Standby,
		Terminated,
		Waiting,
		Transition,
		DeferredReady,
		GateWaitObsolete,
		WaitingForProcessInSwap,
		MaximumThreadState
	} KTHREAD_STATE, * PKTHREAD_STATE;

	typedef enum _KWAIT_REASON
	{
		Executive,
		FreePage,
		PageIn,
		PoolAllocation,
		DelayExecution,
		Suspended,
		UserRequest,
		WrExecutive,
		WrFreePage,
		WrPageIn,
		WrPoolAllocation,
		WrDelayExecution,
		WrSuspended,
		WrUserRequest,
		WrEventPair,
		WrQueue,
		WrLpcReceive,
		WrLpcReply,
		WrVirtualMemory,
		WrPageOut,
		WrRendezvous,
		WrKeyedEvent,
		WrTerminated,
		WrProcessInSwap,
		WrCpuRateControl,
		WrCalloutStack,
		WrKernel,
		WrResource,
		WrPushLock,
		WrMutex,
		WrQuantumEnd,
		WrDispatchInt,
		WrPreempted,
		WrYieldExecution,
		WrFastMutex,
		WrGuardedMutex,
		WrRundown,
		WrAlertByThreadId,
		WrDeferredPreempt,
		WrPhysicalFault,
		WrIoRing,
		WrMdlCache,
		MaximumWaitReason
	} KWAIT_REASON, * PKWAIT_REASON;

	typedef struct _SYSTEM_THREAD_INFORMATION
	{
		LARGE_INTEGER KernelTime;
		LARGE_INTEGER UserTime;
		LARGE_INTEGER CreateTime;
		ULONG WaitTime;
		ULONG_PTR StartAddress;
		CLIENT_ID ClientId;
		KPRIORITY Priority;
		KPRIORITY BasePriority;
		ULONG ContextSwitches;
		KTHREAD_STATE ThreadState;
		KWAIT_REASON WaitReason;
	} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;
}

#include "Early Launch/early_launch.hpp"
#include "Memory Manager/memory.hpp"
#include "Module Manager/module.hpp"
#include "Object Manager/object.hpp"
#include "PE Parser/pe.hpp"

#endif // AURIE_FRAMEWORK_H_