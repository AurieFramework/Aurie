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
		// The requested interface was not found.
		AURIE_INTERFACE_NOT_FOUND,
		// The requested access to the object was denied.
		AURIE_ACCESS_DENIED,
		// A callback with the same priority is already registered.
		AURIE_PRIORITY_COLLISION,
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
		AURIE_OBJECT_LIST = 4,
		// An AurieHook object
		AURIE_OBJECT_HOOK = 5
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
		virtual AurieStatus Destroy() = 0;
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
		const fs::path ModulePath
	);

	using AurieLoaderEntry = AurieStatus(*)(
		IN AurieModule* InitialImage,
		IN void* (*PpGetFrameworkRoutine)(IN const char* ImageExportName),
		IN OPTIONAL AurieEntry Routine,
		IN OPTIONAL const fs::path Path,
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

		EXPORTED AurieStatus __aurie_fwk_init(
			IN AurieModule* InitialImage,
			IN void* (*PpGetFrameworkRoutine)(IN const char* ImageExportName),
			IN OPTIONAL AurieEntry Routine,
			IN OPTIONAL const fs::path Path,
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
				return Routine(Module, Path);
			
			return AURIE_SUCCESS;
		}
	}
}

#endif // AURIE_INCLUDE_PRIVATE
#endif // AURIE_SHARED_H_
