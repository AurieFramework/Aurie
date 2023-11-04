#ifndef AURIE_MODULE_H_
#define AURIE_MODULE_H_

#include "../framework.hpp"
#include <Windows.h>
#include <map>

namespace Aurie
{
	// Maps an image into memory and returns the pointer to its AurieModule struct
	EXPORTED AurieStatus MdMapImage(
		IN const fs::path& ImagePath, 
		OUT AurieModule*& Module
	);

	// Checks whether an image is initialized or not
	EXPORTED bool MdIsImageInitialized(
		IN AurieModule* Module
	);

	// Gets the filename from an image
	EXPORTED AurieStatus MdGetImageFilename(
		IN AurieModule* Module,
		OUT std::wstring& Filename
	);

	namespace Internal
	{
		// Creates an AurieModule object - for internal use only
		// Keep in mind struct AurieModule should be opaque
		AurieStatus MdiCreateModule(
			IN const fs::path& ImagePath,
			IN HMODULE ImageModule,
			IN AurieEntry ModuleInitialize,
			IN AurieEntry ModulePreinitialize,
			IN AurieLoaderEntry FrameworkInitialize,
			IN uint8_t BitFlags,
			OUT AurieModule& Module
		);

		// Internal routine responsible for ensuring module compatibility
		AurieStatus MdiMapImage(
			IN const fs::path& ImagePath,
			OUT HMODULE ImageBase
		);

		// Adds an AurieModule instance to the global list and returns a pointer to it
		AurieModule* MdiAddModuleToList(
			IN const AurieModule& Module
		);

		// Queries information about a module
		EXPORTED AurieStatus MdiQueryModuleInformation(
			IN HMODULE Module,
			OPTIONAL OUT PVOID* ModuleBase,
			OPTIONAL OUT uint32_t* SizeOfModule,
			OPTIONAL OUT PVOID* EntryPoint
		);

		EXPORTED fs::path& MdiGetImagePath(
			IN AurieModule* Module
		);

		EXPORTED AurieStatus MdiGetImageFolder(
			IN AurieModule* Module,
			OUT fs::path& Path
		);

		EXPORTED AurieStatus MdiGetNextModule(
			IN AurieModule* Module,
			OUT AurieModule*& NextModule
		);

		EXPORTED PVOID MdiGetModuleBaseAddress(
			IN AurieModule* Module
		);

		EXPORTED AurieStatus MdiMapFolder(
			IN const fs::path& ImagePath,
			OUT OPTIONAL size_t* ModuleCount
		);

		inline std::list<AurieModule> g_LdrModuleList;
	}
}

#endif // AURIE_MODULE_H_