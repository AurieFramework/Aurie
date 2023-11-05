// File: shared.hpp
// 
// Defines stuff shared between the Framework and its modules.
// Structs are meant to be opaque here, unless AURIE_INCLUDE_PRIVATE is defined.
// AURIE_INCLUDE_PRIVATE is set to 1 in the main Aurie Framework project through Visual Studio's Project Properties.

#ifndef AURIE_SHARED_H_
#define AURIE_SHARED_H_

// Includes
#include <cstdint>
#include <filesystem>

// Defines
#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif // FORCEINLINE

// Nameless structure / union
#pragma warning(disable : 4201)

#ifndef EXPORTED
#define EXPORTED extern "C" __declspec(dllexport)
#endif // EXPORTED

#ifndef IMPORTED
#define IMPORTED extern "C" __declspec(dllimport)
#endif // IMPORTED

#ifndef IN
#define IN
#endif // IN

#ifndef OUT
#define OUT	
#endif // OUT

#ifndef OPTIONAL
#define OPTIONAL
#endif

namespace Aurie
{
	namespace fs = ::std::filesystem;

	// Opaque
	struct AurieModule;
	struct AurieList;
	struct AurieObject;
	struct AurieMemoryAllocation;

	// Forward declarations (not opaque)
	struct AurieInterfaceBase;
	using PVOID = void*; // Allow usage of PVOID even without including PVOID

	enum AurieStatus : uint32_t
	{
		// The operation completed successfully.
		AURIE_SUCCESS = 0,
		// An invalid architecture was specified.
		AURIE_INVALID_ARCH,
		// An error occured in an external function call.
		AURIE_EXTERNAL_ERROR,
		// The requested file was not found.
		AURIE_FILE_NOT_FOUND,
		// The requested access to the object was denied.
		AURIE_ACCESS_DENIED,
		// An object with the same identifier / priority is already registered.
		AURIE_OBJECT_ALREADY_EXISTS,
		// One or more parameters were invalid.
		AURIE_INVALID_PARAMETER,
		// Insufficient memory is available.
		AURIE_INSUFFICIENT_MEMORY,
		// An invalid signature was detected.
		AURIE_INVALID_SIGNATURE,
		// The requested operation is not implemented.
		AURIE_NOT_IMPLEMENTED,
		// An internal error occured in the module.
		AURIE_MODULE_INTERNAL_ERROR,
		// The module failed to resolve dependencies.
		AURIE_MODULE_DEPENDENCY_NOT_RESOLVED,
		// The module failed to initialize.
		AURIE_MODULE_INITIALIZATION_FAILED,
		// The target file header, directory, or RVA could not be found or is invalid.
		AURIE_FILE_PART_NOT_FOUND,
		// The object was not found.
		AURIE_OBJECT_NOT_FOUND
	};

	enum AurieObjectType : uint32_t
	{
		// An AurieModule object
		AURIE_OBJECT_MODULE = 1,
		// An AurieInterfaceBase object
		AURIE_OBJECT_INTERFACE = 2,
		// An AurieMemoryAllocation object
		AURIE_OBJECT_ALLOCATION = 3,
		// An AurieHook object
		AURIE_OBJECT_HOOK = 4
	};

	constexpr bool AurieSuccess(const AurieStatus Status) noexcept
	{
		return Status == AURIE_SUCCESS;
	}

	// All interfaces must inherit from the following class
	// You can add your own functions (make sure to open-source the interface class declaration)
	struct AurieInterfaceBase
	{
		// Interface "constructor"
		virtual AurieStatus Create() = 0;
		// Interface "destructor"
		virtual void Destroy() = 0;
		// Query interface version
		virtual void QueryVersion(
			OUT short& Major,
			OUT short& Minor,
			OUT short& Patch
		) = 0;
	};

	// Always points to the initial Aurie image
	// Initialized in either ArProcessAttach or __aurie_fwk_init
	inline AurieModule* g_ArInitialImage = nullptr;

	using AurieEntry = AurieStatus(*)(
		AurieModule* Module,
		const fs::path& ModulePath
		);

	using AurieLoaderEntry = AurieStatus(*)(
		IN AurieModule* InitialImage,
		IN void* (*PpGetFrameworkRoutine)(IN const char* ImageExportName),
		IN OPTIONAL AurieEntry Routine,
		IN OPTIONAL const fs::path& Path,
		IN OPTIONAL AurieModule* SelfModule
		);
}

#ifndef AURIE_INCLUDE_PRIVATE
#include <Windows.h>

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)
{
	return TRUE;
}

namespace Aurie
{
	inline AurieModule* g_ArSelfModule = nullptr;

	namespace Internal
	{
		// Points to aurie!PpGetFrameworkRoutine
		// Initialized in __aurie_fwk_init
		// Only present in loadable modules
		inline void* (*g_PpGetFrameworkRoutine)(
			IN const char* ImageExportName
			);

		EXPORTED AurieStatus __AurieFrameworkInit(
			IN AurieModule* InitialImage,
			IN void* (*PpGetFrameworkRoutine)(IN const char* ImageExportName),
			IN OPTIONAL AurieEntry Routine,
			IN OPTIONAL const fs::path& Path,
			IN OPTIONAL AurieModule* SelfModule
		)
		{
			if (!g_ArInitialImage)
				g_ArInitialImage = InitialImage;

			if (!g_ArSelfModule)
				g_ArSelfModule = SelfModule;

			if (!g_PpGetFrameworkRoutine)
				g_PpGetFrameworkRoutine = PpGetFrameworkRoutine;

			if (Routine)
				return Routine(SelfModule, Path);

			return AURIE_SUCCESS;
		}

		// Universal API dispatchers made from broken YYTK updates
		// This one is an adaptation of FunctionWrapper in YYTK's beta2 branch.
		template <typename TRet, typename ...TArgs>
		FORCEINLINE auto __AurieApiDispatch(const char* FunctionName, TArgs&... Arguments)
		{
			using FN_DispatchedRoutine = TRet(*)(TArgs...);

			return reinterpret_cast<FN_DispatchedRoutine>(g_PpGetFrameworkRoutine(FunctionName))(Arguments...);
		}

		// And this one is just a continuation of the first one, since some functions don't have parameters
		template <typename TRet>
		FORCEINLINE auto __AurieApiDispatch(const char* FunctionName)
		{
			using FN_DispatchedRoutine = TRet(*)();

			return reinterpret_cast<FN_DispatchedRoutine>(g_PpGetFrameworkRoutine(FunctionName))();
		}
	}
}

#include <functional>
// API definitions here
namespace Aurie
{
	bool ElIsProcessSuspended()
	{
		using ResultType = std::function<decltype(ElIsProcessSuspended)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__);
	}

	PVOID MmAllocatePersistentMemory(
		IN size_t Size
	)
	{
		using ResultType = std::function<decltype(MmAllocatePersistentMemory)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Size);
	}

	PVOID MmAllocateMemory(
		IN AurieModule* Owner,
		IN size_t Size
	)
	{
		using ResultType = std::function<decltype(MmAllocateMemory)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Owner, Size);
	}

	AurieStatus MmFreePersistentMemory(
		IN PVOID AllocationBase
	)
	{
		using ResultType = std::function<decltype(MmFreePersistentMemory)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, AllocationBase);
	}

	AurieStatus MmFreeMemory(
		IN AurieModule* Owner,
		IN PVOID AllocationBase
	)
	{
		using ResultType = std::function<decltype(MmFreeMemory)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Owner, AllocationBase);
	}

	AurieStatus MdMapImage(
		IN const fs::path& ImagePath,
		OUT AurieModule*& Module
	)
	{
		using ResultType = std::function<decltype(MdMapImage)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, ImagePath, Module);
	}

	bool MdIsImageInitialized(
		IN AurieModule* Module
	)
	{
		using ResultType = std::function<decltype(MdIsImageInitialized)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Module);
	}

	AurieStatus MdMapFolder(
		IN const fs::path& FolderPath,
		IN bool Recursive
	)
	{
		using ResultType = std::function<decltype(MdMapFolder)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, FolderPath, Recursive);
	}

	AurieStatus MdGetImageFilename(
		IN AurieModule* Module,
		OUT std::wstring& Filename
	)
	{
		using ResultType = std::function<decltype(MdGetImageFilename)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Module, Filename);
	}

	AurieStatus ObCreateInterface(
		IN AurieModule* Module,
		IN AurieInterfaceBase* Interface,
		IN const char* InterfaceName
	)
	{
		using ResultType = std::function<decltype(ObCreateInterface)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Module, Interface, InterfaceName);
	}

	bool ObInterfaceExists(
		IN const char* InterfaceName
	)
	{
		using ResultType = std::function<decltype(ObInterfaceExists)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, InterfaceName);
	}

	AurieStatus ObDestroyInterface(
		IN const char* InterfaceName
	)
	{
		using ResultType = std::function<decltype(ObDestroyInterface)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, InterfaceName);
	}

	AurieStatus PpQueryImageArchitecture(
		IN const fs::path& Path,
		OUT unsigned short& ImageArchitecture
	)
	{
		using ResultType = std::function<decltype(PpQueryImageArchitecture)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Path, ImageArchitecture);
	}

	uintptr_t PpFindFileExportByName(
		IN const fs::path& Path,
		IN const char* ImageExportName
	)
	{
		using ResultType = std::function<decltype(PpFindFileExportByName)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, Path, ImageExportName);
	}

	void* PpGetFrameworkRoutine(
		IN const char* ExportName
	)
	{
		using ResultType = std::function<decltype(PpGetFrameworkRoutine)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, ExportName);
	}

	AurieStatus PpGetCurrentArchitecture(
		IN unsigned short& ImageArchitecture
	)
	{
		using ResultType = std::function<decltype(PpGetCurrentArchitecture)>::result_type;
		return Internal::__AurieApiDispatch<ResultType>(__func__, ImageArchitecture);
	}
}

#endif // AURIE_INCLUDE_PRIVATE
#endif // AURIE_SHARED_H_
