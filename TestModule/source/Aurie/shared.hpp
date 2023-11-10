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
#include <functional>
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
		
		template <typename TFunction>
		class AurieApiDispatcher
		{
		private:
			using ReturnType = std::function<TFunction>::result_type;
		public:
			template <typename ...TArgs>
			ReturnType operator()(const char* FunctionName, TArgs&... Args)
			{
				auto Func = reinterpret_cast<TFunction*>(g_PpGetFrameworkRoutine(FunctionName));
				return Func(Args...);
			}

			ReturnType operator()(const char* FunctionName)
			{
				auto Func = reinterpret_cast<TFunction*>(g_PpGetFrameworkRoutine(FunctionName));
				return Func();
			}
		};
	}
}

#define AURIE_API_CALL(Function, ...) ::Aurie::Internal::AurieApiDispatcher<decltype(Function)>()(#Function, __VA_ARGS__)

namespace Aurie
{
	bool ElIsProcessSuspended()
	{
		return AURIE_API_CALL(ElIsProcessSuspended);
	}

	PVOID MmAllocatePersistentMemory(
		IN size_t Size
	)
	{
		return AURIE_API_CALL(MmAllocatePersistentMemory, Size);
	}

	PVOID MmAllocateMemory(
		IN AurieModule* Owner,
		IN size_t Size
	)
	{
		return AURIE_API_CALL(MmAllocateMemory, Owner, Size);
	}

	AurieStatus MmFreePersistentMemory(
		IN PVOID AllocationBase
	)
	{
		return AURIE_API_CALL(MmFreePersistentMemory, AllocationBase);
	}

	AurieStatus MmFreeMemory(
		IN AurieModule* Owner,
		IN PVOID AllocationBase
	)
	{
		return AURIE_API_CALL(MmFreeMemory, Owner, AllocationBase);
	}

	size_t MmSigscanModule(
		IN const wchar_t* ModuleName,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	)
	{
		return AURIE_API_CALL(MmSigscanModule, ModuleName, Pattern, PatternMask);
	}

	size_t MmSigscanRegion(
		IN const unsigned char* RegionBase,
		IN const size_t RegionSize,
		IN const unsigned char* Pattern,
		IN const char* PatternMask
	)
	{
		return AURIE_API_CALL(MmSigscanRegion, RegionBase, RegionSize, Pattern, PatternMask);
	}

	namespace Internal
	{
		bool MmpIsAllocatedMemory(
			IN AurieModule* Module,
			IN PVOID AllocationBase
		)
		{
			return AURIE_API_CALL(MmpIsAllocatedMemory, Module, AllocationBase);
		}

		AurieStatus MmpSigscanRegion(
			IN const unsigned char* RegionBase,
			IN const size_t RegionSize,
			IN const unsigned char* Pattern,
			IN const char* PatternMask,
			OUT uintptr_t& PatternBase
		)
		{
			return AURIE_API_CALL(MmpSigscanRegion, RegionBase, RegionSize, Pattern, PatternMask, PatternBase);
		}
	}

	AurieStatus MdMapImage(
		IN const fs::path& ImagePath,
		OUT AurieModule*& Module
	)
	{
		return AURIE_API_CALL(MdMapImage, ImagePath, Module);
	}

	bool MdIsImageInitialized(
		IN AurieModule* Module
	)
	{
		return AURIE_API_CALL(MdIsImageInitialized, Module);
	}

	AurieStatus MdMapFolder(
		IN const fs::path& FolderPath,
		IN bool Recursive
	)
	{
		return AURIE_API_CALL(MdMapFolder, FolderPath, Recursive);
	}

	AurieStatus MdGetImageFilename(
		IN AurieModule* Module,
		OUT std::wstring& Filename
	)
	{
		return AURIE_API_CALL(MdGetImageFilename, Module, Filename);
	}

	AurieStatus MdUnmapImage(
		IN AurieModule* Module
	)
	{
		return AURIE_API_CALL(MdUnmapImage, Module);
	}

	namespace Internal
	{
		AurieStatus MdpQueryModuleInformation(
			IN HMODULE Module,
			OPTIONAL OUT PVOID* ModuleBase,
			OPTIONAL OUT uint32_t* SizeOfModule,
			OPTIONAL OUT PVOID* EntryPoint
		)
		{
			return AURIE_API_CALL(MdpQueryModuleInformation, Module, ModuleBase, SizeOfModule, EntryPoint);
		}

		fs::path& MdpGetImagePath(
			IN AurieModule* Module
		)
		{
			return AURIE_API_CALL(MdpGetImagePath, Module);
		}

		AurieStatus MdpGetImageFolder(
			IN AurieModule* Module,
			OUT fs::path& Path
		)
		{
			return AURIE_API_CALL(MdpGetImageFolder, Module, Path);
		}

		AurieStatus MdpGetNextModule(
			IN AurieModule* Module,
			OUT AurieModule*& NextModule
		)
		{
			return AURIE_API_CALL(MdpGetNextModule, Module, NextModule);
		}

		PVOID MdpGetModuleBaseAddress(
			IN AurieModule* Module
		)
		{
			return AURIE_API_CALL(MdpGetModuleBaseAddress, Module);
		}

		AurieStatus MdpLookupModuleByPath(
			IN const fs::path& ModulePath,
			OUT AurieModule*& Module
		)
		{
			return AURIE_API_CALL(MdpLookupModuleByPath, ModulePath, Module);
		}
	}

	AurieStatus ObCreateInterface(
		IN AurieModule* Module,
		IN AurieInterfaceBase* Interface,
		IN const char* InterfaceName
	)
	{
		return AURIE_API_CALL(ObCreateInterface, Module, Interface, InterfaceName);
	}

	bool ObInterfaceExists(
		IN const char* InterfaceName
	)
	{
		return AURIE_API_CALL(ObInterfaceExists, InterfaceName);
	}

	AurieStatus ObDestroyInterface(
		IN const char* InterfaceName
	)
	{
		return AURIE_API_CALL(ObDestroyInterface, InterfaceName);
	}

	AurieStatus ObGetInterface(
		IN const char* InterfaceName,
		OUT AurieInterfaceBase*& Interface
	)
	{
		return AURIE_API_CALL(ObGetInterface, InterfaceName, Interface);
	}

	namespace Internal
	{
		AurieObjectType ObpGetObjectType(
			IN AurieObject* Object
		)
		{
			return AURIE_API_CALL(ObpGetObjectType, Object);
		}
	}

	AurieStatus PpQueryImageArchitecture(
		IN const fs::path& Path,
		OUT unsigned short& ImageArchitecture
	)
	{
		return AURIE_API_CALL(PpQueryImageArchitecture, Path, ImageArchitecture);
	}

	uintptr_t PpFindFileExportByName(
		IN const fs::path& Path,
		IN const char* ImageExportName
	)
	{
		return AURIE_API_CALL(PpFindFileExportByName, Path, ImageExportName);
	}

	void* PpGetFrameworkRoutine(
		IN const char* ExportName
	)
	{
		return AURIE_API_CALL(PpGetFrameworkRoutine, ExportName);
	}

	AurieStatus PpGetCurrentArchitecture(
		IN unsigned short& ImageArchitecture
	)
	{
		return AURIE_API_CALL(PpGetCurrentArchitecture, ImageArchitecture);
	}

	namespace Internal
	{
		void* PpiFindModuleExportByName(
			IN const AurieModule* Image,
			IN const char* ImageExportName
		)
		{
			return AURIE_API_CALL(PpiFindModuleExportByName, Image, ImageExportName);
		}

		AurieStatus PpiQueryImageArchitecture(
			IN void* Image,
			OUT unsigned short& ImageArchitecture
		)
		{
			return AURIE_API_CALL(PpiQueryImageArchitecture, Image, ImageArchitecture);
		}

		AurieStatus PpiGetNtHeader(
			IN void* Image,
			OUT void*& NtHeader
		)
		{
			return AURIE_API_CALL(PpiGetNtHeader, Image, NtHeader);
		}

		AurieStatus PpiGetModuleSectionBounds(
			IN void* Image,
			IN const char* SectionName,
			OUT uint64_t& SectionBase,
			OUT size_t& SectionSize
		)
		{
			return AURIE_API_CALL(PpiGetModuleSectionBounds, Image, SectionName, SectionBase, SectionSize);
		}

		uint32_t PpiRvaToFileOffset(
			IN PIMAGE_NT_HEADERS ImageHeaders,
			IN uint32_t Rva
		)
		{
			return AURIE_API_CALL(PpiRvaToFileOffset, ImageHeaders, Rva);
		}
	}
}

#endif // AURIE_INCLUDE_PRIVATE
#endif // AURIE_SHARED_H_
