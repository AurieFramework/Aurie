#pragma once
#include <Windows.h>

//
// Specific
//

//
// Images
//

typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, * PCLIENT_ID;


//
// Process information structures
//

/**
 * The PEB_LDR_DATA structure contains information about the loaded modules for the process.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb_ldr_data
 */
typedef struct _PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	HANDLE ShutdownThreadId;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	_Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

//
// Strings
//

typedef struct _STRING
{
	USHORT Length;
	USHORT MaximumLength;
	_Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING, * PSTRING, ANSI_STRING, * PANSI_STRING, OEM_STRING, * POEM_STRING;


// symbols
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	union
	{
		UCHAR FlagGroup[4];
		ULONG Flags;
		struct
		{
			ULONG PackagedBinary : 1;
			ULONG MarkedForRemoval : 1;
			ULONG ImageDll : 1;
			ULONG LoadNotificationsSent : 1;
			ULONG TelemetryEntryProcessed : 1;
			ULONG ProcessStaticImport : 1;
			ULONG InLegacyLists : 1;
			ULONG InIndexes : 1;
			ULONG ShimDll : 1;
			ULONG InExceptionTable : 1;
			ULONG ReservedFlags1 : 2;
			ULONG LoadInProgress : 1;
			ULONG LoadConfigProcessed : 1;
			ULONG EntryProcessed : 1;
			ULONG ProtectDelayLoad : 1;
			ULONG ReservedFlags3 : 2;
			ULONG DontCallForThreads : 1;
			ULONG ProcessAttachCalled : 1;
			ULONG ProcessAttachFailed : 1;
			ULONG CorDeferredValidate : 1;
			ULONG CorImage : 1;
			ULONG DontRelocate : 1;
			ULONG CorILOnly : 1;
			ULONG ChpeImage : 1;
			ULONG ChpeEmulatorImage : 1;
			ULONG ReservedFlags5 : 1;
			ULONG Redirected : 1;
			ULONG ReservedFlags6 : 2;
			ULONG CompatDatabaseProcessed : 1;
		};
	};
	USHORT ObsoleteLoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	ULONG TimeDateStamp;
	PVOID EntryPointActivationContext;
	PVOID Lock; // RtlAcquireSRWLockExclusive
	PVOID DdagNode;
	LIST_ENTRY NodeModuleLink;
	PVOID LoadContext;
	PVOID ParentDllBase;
	PVOID SwitchBackContext;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;


/**
 * Process Environment Block (PEB) structure.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb
 */
typedef struct _PEB
{
	//
	// The process was cloned with an inherited address space.
	//
	BOOLEAN InheritedAddressSpace;

	//
	// The process has image file execution options (IFEO).
	//
	BOOLEAN ReadImageFileExecOptions;

	//
	// The process has a debugger attached.
	//
	BOOLEAN BeingDebugged;

	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;            // The process uses large image regions (4 MB).
			BOOLEAN IsProtectedProcess : 1;             // The process is a protected process.
			BOOLEAN IsImageDynamicallyRelocated : 1;    // The process image base address was relocated.
			BOOLEAN SkipPatchingUser32Forwarders : 1;   // The process skipped forwarders for User32.dll functions. 1 for 64-bit, 0 for 32-bit.
			BOOLEAN IsPackagedProcess : 1;              // The process is a packaged store process (APPX/MSIX).
			BOOLEAN IsAppContainer : 1;                 // The process has an AppContainer token.
			BOOLEAN IsProtectedProcessLight : 1;        // The process is a protected process (light).
			BOOLEAN IsLongPathAwareProcess : 1;         // The process is long path aware.
		};
	};

	//
	// Handle to a mutex for synchronization.
	//
	HANDLE Mutant;

	//
	// Pointer to the base address of the process image.
	//
	PVOID ImageBaseAddress;

	//
	// Pointer to the process loader data.
	//
	PPEB_LDR_DATA Ldr;

	//
	// Pointer to the process parameters.
	//
	PVOID ProcessParameters;

	//
	// Reserved.
	//
	PVOID SubSystemData;

	//
	// Pointer to the process default heap.
	//
	PVOID ProcessHeap;

	//
	// Pointer to a critical section used to synchronize access to the PEB.
	//
	PRTL_CRITICAL_SECTION FastPebLock;

	//
	// Pointer to a singly linked list used by ATL.
	//
	PSLIST_HEADER AtlThunkSListPtr;

	//
	// Pointer to the Image File Execution Options key.
	//
	PVOID IFEOKey;

	//
	// Cross process flags.
	//
	union
	{
		ULONG CrossProcessFlags;
		struct
		{
			ULONG ProcessInJob : 1;                 // The process is part of a job.
			ULONG ProcessInitializing : 1;          // The process is initializing.
			ULONG ProcessUsingVEH : 1;              // The process is using VEH.
			ULONG ProcessUsingVCH : 1;              // The process is using VCH.
			ULONG ProcessUsingFTH : 1;              // The process is using FTH.
			ULONG ProcessPreviouslyThrottled : 1;   // The process was previously throttled.
			ULONG ProcessCurrentlyThrottled : 1;    // The process is currently throttled.
			ULONG ProcessImagesHotPatched : 1;      // The process images are hot patched. // RS5
			ULONG ReservedBits0 : 24;
		};
	};

	//
	// User32 KERNEL_CALLBACK_TABLE (ntuser.h)
	//
	union
	{
		PVOID KernelCallbackTable;
		PVOID UserSharedInfoPtr;
	};

	//
	// Reserved.
	//
	ULONG SystemReserved;

	//
	// Pointer to the Active Template Library (ATL) singly linked list (32-bit)
	//
	ULONG AtlThunkSListPtr32;

	//
	// Pointer to the API Set Schema.
	//
	PVOID ApiSetMap;

	//
	// Counter for TLS expansion.
	//
	ULONG TlsExpansionCounter;

	//
	// Pointer to the TLS bitmap.
	//
	PVOID TlsBitmap;

	//
	// Bits for the TLS bitmap.
	//
	ULONG TlsBitmapBits[2];

	//
	// Reserved for CSRSS.
	//
	PVOID ReadOnlySharedMemoryBase;

	//
	// Pointer to the USER_SHARED_DATA for the current SILO.
	//
	PVOID SharedData;

	//
	// Reserved for CSRSS.
	//
	PVOID* ReadOnlyStaticServerData;

	//
	// Pointer to the ANSI code page data. (PCPTABLEINFO)
	//
	PVOID AnsiCodePageData;

	//
	// Pointer to the OEM code page data. (PCPTABLEINFO)
	//
	PVOID OemCodePageData;

	//
	// Pointer to the Unicode case table data. (PNLSTABLEINFO)
	//
	PVOID UnicodeCaseTableData;

	//
	// The total number of system processors.
	//
	ULONG NumberOfProcessors;

	//
	// Global flags for the system.
	//
	union
	{
		ULONG NtGlobalFlag;
		struct
		{
			ULONG StopOnException : 1;          // FLG_STOP_ON_EXCEPTION
			ULONG ShowLoaderSnaps : 1;          // FLG_SHOW_LDR_SNAPS
			ULONG DebugInitialCommand : 1;      // FLG_DEBUG_INITIAL_COMMAND
			ULONG StopOnHungGUI : 1;            // FLG_STOP_ON_HUNG_GUI
			ULONG HeapEnableTailCheck : 1;      // FLG_HEAP_ENABLE_TAIL_CHECK
			ULONG HeapEnableFreeCheck : 1;      // FLG_HEAP_ENABLE_FREE_CHECK
			ULONG HeapValidateParameters : 1;   // FLG_HEAP_VALIDATE_PARAMETERS
			ULONG HeapValidateAll : 1;          // FLG_HEAP_VALIDATE_ALL
			ULONG ApplicationVerifier : 1;      // FLG_APPLICATION_VERIFIER
			ULONG MonitorSilentProcessExit : 1; // FLG_MONITOR_SILENT_PROCESS_EXIT
			ULONG PoolEnableTagging : 1;        // FLG_POOL_ENABLE_TAGGING
			ULONG HeapEnableTagging : 1;        // FLG_HEAP_ENABLE_TAGGING
			ULONG UserStackTraceDb : 1;         // FLG_USER_STACK_TRACE_DB
			ULONG KernelStackTraceDb : 1;       // FLG_KERNEL_STACK_TRACE_DB
			ULONG MaintainObjectTypeList : 1;   // FLG_MAINTAIN_OBJECT_TYPELIST
			ULONG HeapEnableTagByDll : 1;       // FLG_HEAP_ENABLE_TAG_BY_DLL
			ULONG DisableStackExtension : 1;    // FLG_DISABLE_STACK_EXTENSION
			ULONG EnableCsrDebug : 1;           // FLG_ENABLE_CSRDEBUG
			ULONG EnableKDebugSymbolLoad : 1;   // FLG_ENABLE_KDEBUG_SYMBOL_LOAD
			ULONG DisablePageKernelStacks : 1;  // FLG_DISABLE_PAGE_KERNEL_STACKS
			ULONG EnableSystemCritBreaks : 1;   // FLG_ENABLE_SYSTEM_CRIT_BREAKS
			ULONG HeapDisableCoalescing : 1;    // FLG_HEAP_DISABLE_COALESCING
			ULONG EnableCloseExceptions : 1;    // FLG_ENABLE_CLOSE_EXCEPTIONS
			ULONG EnableExceptionLogging : 1;   // FLG_ENABLE_EXCEPTION_LOGGING
			ULONG EnableHandleTypeTagging : 1;  // FLG_ENABLE_HANDLE_TYPE_TAGGING
			ULONG HeapPageAllocs : 1;           // FLG_HEAP_PAGE_ALLOCS
			ULONG DebugInitialCommandEx : 1;    // FLG_DEBUG_INITIAL_COMMAND_EX
			ULONG DisableDbgPrint : 1;          // FLG_DISABLE_DBGPRINT
			ULONG CritSecEventCreation : 1;     // FLG_CRITSEC_EVENT_CREATION
			ULONG LdrTopDown : 1;               // FLG_LDR_TOP_DOWN
			ULONG EnableHandleExceptions : 1;   // FLG_ENABLE_HANDLE_EXCEPTIONS
			ULONG DisableProtDlls : 1;          // FLG_DISABLE_PROTDLLS
		} NtGlobalFlags;
	};

	//
	// Timeout for critical sections.
	//
	LARGE_INTEGER CriticalSectionTimeout;

	//
	// Reserved size for heap segments.
	//
	SIZE_T HeapSegmentReserve;

	//
	// Committed size for heap segments.
	//
	SIZE_T HeapSegmentCommit;

	//
	// Threshold for decommitting total free heap.
	//
	SIZE_T HeapDeCommitTotalFreeThreshold;

	//
	// Threshold for decommitting free heap blocks.
	//
	SIZE_T HeapDeCommitFreeBlockThreshold;

	//
	// Number of process heaps.
	//
	ULONG NumberOfHeaps;

	//
	// Maximum number of process heaps.
	//
	ULONG MaximumNumberOfHeaps;

	//
	// Pointer to an array of process heaps. ProcessHeaps is initialized
	// to point to the first free byte after the PEB and MaximumNumberOfHeaps
	// is computed from the page size used to hold the PEB, less the fixed
	// size of this data structure.
	//
	PVOID* ProcessHeaps;

	//
	// Pointer to the system GDI shared handle table.
	//
	PVOID GdiSharedHandleTable;

	//
	// Pointer to the process starter helper.
	//
	PVOID ProcessStarterHelper;

	//
	// The maximum number of GDI function calls during batch operations (GdiSetBatchLimit)
	//
	ULONG GdiDCAttributeList;

	//
	// Pointer to the loader lock critical section.
	//
	PRTL_CRITICAL_SECTION LoaderLock;

	//
	// Major version of the operating system.
	//
	ULONG OSMajorVersion;

	//
	// Minor version of the operating system.
	//
	ULONG OSMinorVersion;

	//
	// Build number of the operating system.
	//
	USHORT OSBuildNumber;

	//
	// CSD version of the operating system.
	//
	USHORT OSCSDVersion;

	//
	// Platform ID of the operating system.
	//
	ULONG OSPlatformId;

	//
	// Subsystem version of the current process image (PE Headers).
	//
	ULONG ImageSubsystem;

	//
	// Major version of the current process image subsystem (PE Headers).
	//
	ULONG ImageSubsystemMajorVersion;

	//
	// Minor version of the current process image subsystem (PE Headers).
	//
	ULONG ImageSubsystemMinorVersion;

	//
	// Affinity mask for the current process.
	//
	KAFFINITY ActiveProcessAffinityMask;
} PEB, * PPEB;


/**
 * Thread Environment Block (TEB) structure.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-teb
 */
typedef struct _TEB
{
	//
	// Thread Information Block (TIB) contains the thread's stack, base and limit addresses, the current stack pointer, and the exception list.
	//
	NT_TIB NtTib;

	//
	// Reserved.
	//
	PVOID EnvironmentPointer;

	//
	// Client ID for this thread.
	//
	CLIENT_ID ClientId;

	//
	// A handle to an active Remote Procedure Call (RPC) if the thread is currently involved in an RPC operation.
	//
	PVOID ActiveRpcHandle;

	//
	// A pointer to the __declspec(thread) local storage array.
	//
	PVOID ThreadLocalStoragePointer;

	//
	// A pointer to the Process Environment Block (PEB), which contains information about the process.
	//
	PPEB ProcessEnvironmentBlock;

	//
	// The previous Win32 error value for this thread.
	//
	ULONG LastErrorValue;

	//
	// The number of critical sections currently owned by this thread.
	//
	ULONG CountOfOwnedCriticalSections;

	//
	// Reserved.
	//
	PVOID CsrClientThread;

	//
	// Reserved.
	//
	PVOID Win32ThreadInfo;

	//
	// Reserved.
	//
	ULONG User32Reserved[26];

	//
	// Reserved.
	//
	ULONG UserReserved[5];

	//
	// Reserved.
	//
	PVOID WOW32Reserved;

	//
	// The LCID of the current thread. (Kernel32!GetThreadLocale)
	//
	LCID CurrentLocale;

	//
	// Reserved.
	//
	ULONG FpSoftwareStatusRegister;

	//
	// Reserved.
	//
	PVOID ReservedForDebuggerInstrumentation[16];
} TEB, * PTEB;

extern "C++" {
	template<size_t N>
	class _RTL_CONSTANT_STRING_remove_const_template_class;

	template<>
	class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(char)>
	{
	public:
		typedef char T;
	};

	template<>
	class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(WCHAR)>
	{
	public:
		typedef WCHAR T;
	};

#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
	(const_cast<_RTL_CONSTANT_STRING_remove_const_template_class<sizeof((s)[0])>::T*>(s))

#define RTL_CONSTANT_STRING(s)                                      \
	{                                                               \
		sizeof(s) - sizeof((s)[0]),                                 \
			sizeof(s),                                              \
			_RTL_CONSTANT_STRING_remove_const_macro(s)              \
	}
}

extern "C"
{
	FORCEINLINE VOID RtlInitUnicodeString(
		_Out_ PUNICODE_STRING DestinationString,
		_In_opt_z_ PCWSTR SourceString
	)
	{
		if (SourceString)
			DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(UNICODE_NULL);
		else
			DestinationString->MaximumLength = DestinationString->Length = 0;

		DestinationString->Buffer = (PWCH)SourceString;
	}
	

	NTSYSAPI
		NTSTATUS
		NTAPI
		LdrLoadDll(
			_In_opt_ PCWSTR DllPath,
			_In_opt_ PULONG DllCharacteristics,
			_In_ const PUNICODE_STRING DllName,
			_Out_ PVOID* DllHandle
		);

	NTSYSAPI
		ULONG
		STDAPIVCALLTYPE
		DbgPrintEx(
			_In_ ULONG ComponentId,
			_In_ ULONG Level,
			_In_z_ _Printf_format_string_ PCCH Format,
			...
		);

	NTSYSAPI
		VOID
		NTAPI
		DbgBreakPoint(
			VOID
		);

	NTSYSCALLAPI
		NTSTATUS
		NTAPI
		NtSuspendThread(
			_In_ HANDLE ThreadHandle,
			_Out_opt_ PULONG PreviousSuspendCount
		);

}
