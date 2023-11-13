#ifndef AURIE_MODULE_H_
#define AURIE_MODULE_H_

#include "../framework.hpp"
#include <vector>
#include <functional>

namespace Aurie
{
	// Maps an image into memory and returns the pointer to its AurieModule struct
	EXPORTED AurieStatus MdMapImage(
		IN const fs::path& ImagePath, 
		OUT AurieModule*& Module
	);

	EXPORTED bool MdIsImagePreinitialized(
		IN AurieModule* Module
	);

	// Checks whether an image is initialized or not
	EXPORTED bool MdIsImageInitialized(
		IN AurieModule* Module
	);

	EXPORTED AurieStatus MdMapFolder(
		IN const fs::path& FolderPath,
		IN bool Recursive
	);

	// Gets the filename from an image
	EXPORTED AurieStatus MdGetImageFilename(
		IN AurieModule* Module,
		OUT std::wstring& Filename
	);

	EXPORTED AurieStatus MdUnmapImage(
		IN AurieModule* Module
	);

	namespace Internal
	{
		// Creates an AurieModule object - for internal use only
		// Keep in mind struct AurieModule should be opaque
		AurieStatus MdpCreateModule(
			IN const fs::path& ImagePath,
			IN HMODULE ImageModule,
			IN AurieEntry ModuleInitialize,
			IN AurieEntry ModulePreinitialize,
			IN AurieEntry ModuleUnload,
			IN AurieLoaderEntry FrameworkInitialize,
			IN uint8_t BitFlags,
			OUT AurieModule& Module
		);
		
		bool MdpIsModuleMarkedForPurge(
			IN AurieModule* Module
		);

		void MdpMarkModuleForPurge(
			IN AurieModule* Module
		);

		void MdpPurgeMarkedModules();

		// Internal routine responsible for ensuring module compatibility
		AurieStatus MdpMapImage(
			IN const fs::path& ImagePath,
			OUT HMODULE& ImageBase
		);

		void MdpBuildModuleList(
			IN const fs::path& BaseFolder,
			IN bool Recursive,
			IN std::function<bool(const fs::directory_entry& Entry)> Predicate,
			OUT std::vector<fs::path>& Files
		);

		// Adds an AurieModule instance to the global list and returns a pointer to it
		AurieModule* MdpAddModuleToList(
			IN const AurieModule& Module
		);

		// Queries information about a module
		EXPORTED AurieStatus MdpQueryModuleInformation(
			IN HMODULE Module,
			OPTIONAL OUT PVOID* ModuleBase,
			OPTIONAL OUT uint32_t* SizeOfModule,
			OPTIONAL OUT PVOID* EntryPoint
		);

		EXPORTED fs::path& MdpGetImagePath(
			IN AurieModule* Module
		);

		EXPORTED AurieStatus MdpGetImageFolder(
			IN AurieModule* Module,
			OUT fs::path& Path
		);

		EXPORTED AurieStatus MdpGetNextModule(
			IN AurieModule* Module,
			OUT AurieModule*& NextModule
		);

		EXPORTED PVOID MdpGetModuleBaseAddress(
			IN AurieModule* Module
		);

		EXPORTED AurieStatus MdpLookupModuleByPath(
			IN const fs::path& ModulePath,
			OUT AurieModule*& Module
		);

		AurieStatus MdpUnmapImage(
			IN AurieModule* Module,
			IN bool RemoveFromList,
			IN bool CallUnloadRoutine
		);

		AurieStatus MdpDispatchEntry(
			IN AurieModule* Module,
			IN AurieEntry Entry
		);

		void MdpMapFolder(
			IN const fs::path& Folder,
			IN bool Recursive,
			OPTIONAL OUT size_t* NumberOfMappedModules
		);

		inline std::list<AurieModule> g_LdrModuleList;
	}
}

#endif // AURIE_MODULE_H_