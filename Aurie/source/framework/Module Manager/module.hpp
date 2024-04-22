#ifndef AURIE_MODULE_H_
#define AURIE_MODULE_H_

#include "../framework.hpp"
#include <vector>
#include <functional>

namespace Aurie
{
	// Maps an image into memory and returns the pointer to its AurieModule struct
	// Used for runtime loading only
	EXPORTED AurieStatus MdMapImage(
		IN const fs::path& ImagePath, 
		OUT AurieModule*& Module
	);

	// Maps an image into memory and returns the pointer to its AurieModule struct
	// Used to load both runtime and preloaded modules
	AurieStatus MdMapImageEx(
		IN const fs::path& ImagePath,
		IN bool IsRuntimeLoad,
		OUT AurieModule*& Module
	);

	// Checks whether an image's ModulePreinitialize method ran
	EXPORTED bool MdIsImagePreinitialized(
		IN AurieModule* Module
	);

	// Checks whether an image's ModuleInitialize method ran
	EXPORTED bool MdIsImageInitialized(
		IN AurieModule* Module
	);

	// Checks whether an image was loaded at runtime
	EXPORTED bool MdIsImageRuntimeLoaded(
		IN AurieModule* Module
	);

	// Maps every module in a given folder
	EXPORTED AurieStatus MdMapFolder(
		IN const fs::path& FolderPath,
		IN bool Recursive
	);

	// Gets the filename from an image
	EXPORTED AurieStatus MdGetImageFilename(
		IN AurieModule* Module,
		OUT std::wstring& Filename
	);

	// Unmaps and deallocates every image
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
			IN bool ProcessExports,
			IN uint8_t BitFlags,
			OUT AurieModule& Module
		);
		
		// Internal method to check whether a module is marked for purge
		bool MdpIsModuleMarkedForPurge(
			IN AurieModule* Module
		);

		// Internal method to mark a module for purge
		void MdpMarkModuleForPurge(
			IN AurieModule* Module
		);

		// Internal method that unloads all modules marked for purge
		void MdpPurgeMarkedModules();

		// Internal routine responsible for ensuring module compatibility
		AurieStatus MdpMapImage(
			IN const fs::path& ImagePath,
			OUT HMODULE& ImageBase
		);

		// Internal routine that builds a list of modules to be loaded from a folder
		// given a predicate routine that decides whether or not to include them in the list
		void MdpBuildModuleList(
			IN const fs::path& BaseFolder,
			IN bool Recursive,
			IN std::function<bool(const fs::directory_entry& Entry)> Predicate,
			OUT std::vector<fs::path>& Files
		);

		// Adds an AurieModule instance to the global list and returns a pointer to it
		AurieModule* MdpAddModuleToList(
			IN AurieModule&& Module
		);

		// Queries information about a module
		EXPORTED AurieStatus MdpQueryModuleInformation(
			IN HMODULE Module,
			OPTIONAL OUT PVOID* ModuleBase,
			OPTIONAL OUT uint32_t* SizeOfModule,
			OPTIONAL OUT PVOID* EntryPoint
		);

		// Queries the full path for a given AurieModule
		EXPORTED fs::path& MdpGetImagePath(
			IN AurieModule* Module
		);

		// Queries the folder path for a given AurieModule
		EXPORTED AurieStatus MdpGetImageFolder(
			IN AurieModule* Module,
			OUT fs::path& Path
		);

		// Walks the linked list of loaded modules
		EXPORTED AurieStatus MdpGetNextModule(
			IN AurieModule* Module,
			OUT AurieModule*& NextModule
		);

		// Queries the base address of a given AurieModule
		EXPORTED PVOID MdpGetModuleBaseAddress(
			IN AurieModule* Module
		);

		// Looks up the AurieModule given a path to the module
		EXPORTED AurieStatus MdpLookupModuleByPath(
			IN const fs::path& ModulePath,
			OUT AurieModule*& Module
		);

		// Internal method responsible for initializing AurieModule function pointers
		AurieStatus MdpProcessImageExports(
			IN const fs::path& ImagePath,
			IN HMODULE ImageBaseAddress,
			OUT AurieModule* ModuleImage
		);

		// Internal method for unmapping images
		AurieStatus MdpUnmapImage(
			IN AurieModule* Module,
			IN bool RemoveFromList,
			IN bool CallUnloadRoutine
		);

		// Internal method for dispatching AurieEntry function pointers
		AurieStatus MdpDispatchEntry(
			IN AurieModule* Module,
			IN AurieEntry Entry
		);

		// Internal routine that maps every module in a given folder
		void MdpMapFolder(
			IN const fs::path& Folder,
			IN bool Recursive,
			IN bool IsRuntimeLoad,
			OPTIONAL OUT size_t* NumberOfMappedModules
		);

		inline std::list<AurieModule> g_LdrModuleList;
	}
}

#endif // AURIE_MODULE_H_