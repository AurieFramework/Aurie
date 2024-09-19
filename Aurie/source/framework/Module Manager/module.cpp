#include "module.hpp"
#include <Psapi.h>

namespace Aurie
{
	AurieStatus Internal::MdpCreateModule(
		IN const fs::path& ImagePath, 
		IN HMODULE ImageModule,
		IN bool ProcessExports,
		IN uint8_t BitFlags,
		OUT AurieModule& Module
	)
	{
		AurieModule temp_module;
		AurieStatus last_status = AURIE_SUCCESS;

		// Populate known fields first
		temp_module.Flags.Bitfield = BitFlags;
		temp_module.ImagePath = ImagePath;

		if (ProcessExports)
		{
			last_status = MdpProcessImageExports(
				ImagePath,
				ImageModule,
				&temp_module
			);

			if (!AurieSuccess(last_status))
			{
				return last_status;
			}
		}

		last_status = MdpQueryModuleInformation(
			ImageModule,
			&temp_module.ImageBase.Pointer,
			&temp_module.ImageSize,
			&temp_module.ImageEntrypoint.Pointer
		);

		if (!AurieSuccess(last_status))
		{
			return last_status;
		}

		Module = std::move(temp_module);

		return AURIE_SUCCESS;
	}

	bool Internal::MdpIsModuleMarkedForPurge(
		IN AurieModule* Module
	)
	{
		return Module->Flags.MarkedForPurge;
	}

	void Internal::MdpMarkModuleForPurge(
		IN AurieModule* Module
	)
	{
		Module->Flags.MarkedForPurge = true;
	}

	void Internal::MdpPurgeMarkedModules()
	{
		// Loop through all the modules marked for purge
		for (auto& module : g_LdrModuleList)
		{
			// Unmap the module, but don't call the unload routine, and don't remove it from the list
			if (MdpIsModuleMarkedForPurge(&module))
				MdpUnmapImage(&module, false, false);
		}

		// Remove the now unloaded modules from our list
		// Note we can't do this in the for loop, since that'd invalidate the iterators
		g_LdrModuleList.remove_if(
			[](AurieModule& Module) -> bool
			{
				return MdpIsModuleMarkedForPurge(&Module);
			}
		);
	}

	AurieStatus Internal::MdpMapImage(
		IN const fs::path& ImagePath, 
		OUT HMODULE& ImageBase
	)
	{
		// If the file doesn't exist, we have nothing to map
		std::error_code ec;
		if (!fs::exists(ImagePath, ec))
			return AURIE_FILE_NOT_FOUND;

		AurieStatus last_status = AURIE_SUCCESS;
		unsigned short target_arch = 0, self_arch = 0;
		
		// Query the target image architecture
		last_status = PpQueryImageArchitecture(
			ImagePath,
			target_arch
		);

		if (!AurieSuccess(last_status))
			return last_status;

		// Query the current architecture
		last_status = PpGetCurrentArchitecture(
			self_arch
		);

		// If we fail to query the current architecture, we bail.
		if (!AurieSuccess(last_status))
			return last_status;

		// Don't try to load modules which are the wrong architecture
		if (target_arch != self_arch)
			return AURIE_INVALID_ARCH;

		// Make sure the image has the required exports
		bool has_framework_init = PpFindFileExportByName(ImagePath, "__AurieFrameworkInit") != 0;
		bool has_module_entry = PpFindFileExportByName(ImagePath, "ModuleInitialize") != 0;
		bool has_module_preinit = PpFindFileExportByName(ImagePath, "ModulePreinitialize") != 0;

		// If the image doesn't have a framework init function, we can't load it.
		if (!has_framework_init)
			return AURIE_INVALID_SIGNATURE;

		// If we don't have a module entry OR a module preinitialize function, we can't load.
		bool has_either_entry = has_module_entry || has_module_preinit;
		if (!has_either_entry)
			return AURIE_INVALID_SIGNATURE;

		AurieModule* potential_loaded_copy = nullptr;
		last_status = MdpLookupModuleByPath(
			ImagePath,
			potential_loaded_copy
		);

		// If there's a module that's already loaded from the same path, deny loading it twice
		if (AurieSuccess(last_status))
			return AURIE_OBJECT_ALREADY_EXISTS;

		// Load the image into memory and make sure we loaded it
		HMODULE image_module = LoadLibraryW(ImagePath.wstring().c_str());

		if (!image_module)
			return AURIE_EXTERNAL_ERROR;

		ImageBase = image_module;
		return AURIE_SUCCESS;
	}

	void Internal::MdpBuildModuleList(
		IN const fs::path& BaseFolder, 
		IN bool Recursive, 
		IN std::function<bool(const fs::directory_entry& Entry)> Predicate,
		OUT std::vector<fs::path>& Files
	)
	{
		std::error_code ec;
		Files.clear();

		if (Recursive)
		{
			for (auto& entry : fs::recursive_directory_iterator(BaseFolder, ec))
				if (Predicate(entry))
					Files.push_back(entry.path());

			return;
		}

		for (auto& entry : fs::recursive_directory_iterator(BaseFolder, ec))
			if (Predicate(entry))
				Files.push_back(entry.path());	
	}

	AurieModule* Internal::MdpAddModuleToList(
		IN AurieModule&& Module
	)
	{
		return &g_LdrModuleList.emplace_back(std::move(Module));
	}

	AurieStatus Internal::MdpQueryModuleInformation(
		IN HMODULE Module, 
		OPTIONAL OUT PVOID* ModuleBase,
		OPTIONAL OUT uint32_t* SizeOfModule, 
		OPTIONAL OUT PVOID* EntryPoint
	)
	{
		// Query the information by asking Windows
		MODULEINFO module_info = {};
		if (!GetModuleInformation(
			GetCurrentProcess(),
			Module,
			&module_info,
			sizeof(module_info)
		))
		{
			return AURIE_EXTERNAL_ERROR;
		}

		// Fill in what the caller wants
		if (ModuleBase)
			*ModuleBase = module_info.lpBaseOfDll;

		if (SizeOfModule)
			*SizeOfModule = module_info.SizeOfImage;

		if (EntryPoint)
			*EntryPoint = module_info.EntryPoint;

		return AURIE_SUCCESS;
	}

	fs::path& Internal::MdpGetImagePath(
		IN AurieModule* Module
	)
	{
		return Module->ImagePath;
	}

	AurieStatus Internal::MdpGetImageFolder(
		IN AurieModule* Module, 
		OUT fs::path& Path
	)
	{
		fs::path& module_path = Internal::MdpGetImagePath(Module);

		if (!module_path.has_parent_path())
			return AURIE_INVALID_PARAMETER;

		Path = module_path.parent_path();
		return AURIE_SUCCESS;
	}

	AurieStatus Internal::MdpGetNextModule(
		IN AurieModule* Module, 
		OUT AurieModule*& NextModule
	)
	{
		// Find the module in our list (gets an iterator)
		auto list_iterator = std::find(
			g_LdrModuleList.begin(),
			g_LdrModuleList.end(),
			*Module
		);

		// Make sure that module is indeed in our list
		if (list_iterator == std::end(g_LdrModuleList))
			return AURIE_INVALID_PARAMETER;

		// Compute the distance from the beginning of the list to the module
		size_t distance = std::distance(g_LdrModuleList.begin(), list_iterator);

		// Advance to the next element
		distance = (distance + 1) % g_LdrModuleList.size();
		AurieModule& next_module = *std::next(g_LdrModuleList.begin(), distance);

		NextModule = &next_module;
		
		return AURIE_SUCCESS;
	}

	PVOID Internal::MdpGetModuleBaseAddress(
		IN AurieModule* Module
	)
	{
		return Module->ImageBase.Pointer;
	}

	EXPORTED AurieStatus Internal::MdpLookupModuleByPath(
		IN const fs::path& ModulePath,
		OUT AurieModule*& Module
	)
	{
		auto iterator = std::find_if(
			g_LdrModuleList.begin(),
			g_LdrModuleList.end(),
			[ModulePath](const AurieModule& Module) -> bool
			{
				return fs::equivalent(ModulePath, Module.ImagePath);
			}
		);

		if (iterator == g_LdrModuleList.end())
			return AURIE_OBJECT_NOT_FOUND;

		Module = &(*iterator);
		
		return AURIE_SUCCESS;
	}

	AurieStatus Internal::MdpProcessImageExports(
		IN const fs::path& ImagePath, 
		IN HMODULE ImageBaseAddress, 
		IN OUT AurieModule* ModuleImage
	)
	{
		// Find all the required functions
		uintptr_t framework_init_offset = PpFindFileExportByName(ImagePath, "__AurieFrameworkInit");
		uintptr_t module_init_offset = PpFindFileExportByName(ImagePath, "ModuleInitialize");

		uintptr_t module_callback_offset = PpFindFileExportByName(ImagePath, "ModuleOperationCallback");
		uintptr_t module_preload_offset = PpFindFileExportByName(ImagePath, "ModulePreinitialize");
		uintptr_t module_unload_offset = PpFindFileExportByName(ImagePath, "ModuleUnload");

		// Cast the problems away
		char* image_base = reinterpret_cast<char*>(ImageBaseAddress);

		AurieEntry module_init = reinterpret_cast<AurieEntry>(image_base + module_init_offset);
		AurieEntry module_preload = reinterpret_cast<AurieEntry>(image_base + module_preload_offset);
		AurieEntry module_unload = reinterpret_cast<AurieEntry>(image_base + module_unload_offset);
		AurieLoaderEntry framework_init = reinterpret_cast<AurieLoaderEntry>(image_base + framework_init_offset);
		AurieModuleCallback module_callback = reinterpret_cast<AurieModuleCallback>(image_base + module_callback_offset);

		// If the offsets are zero, the function wasn't found, which means we shouldn't populate the field.
		if (module_init_offset)
			ModuleImage->ModuleInitialize = module_init;

		if (module_preload_offset)
			ModuleImage->ModulePreinitialize = module_preload;

		if (framework_init_offset)
			ModuleImage->FrameworkInitialize = framework_init;

		if (module_callback_offset)
			ModuleImage->ModuleOperationCallback = module_callback;

		if (module_unload_offset)
			ModuleImage->ModuleUnload = module_unload;

		// We always need __AurieFrameworkInit to exist.
		// We also need either a ModuleInitialize or a ModulePreinitialize function.
		return ((module_init_offset || module_preload_offset) && framework_init_offset) ? AURIE_SUCCESS : AURIE_FILE_PART_NOT_FOUND;
	}

	// The ignoring of return values here is on purpose, we just have to power through
	// and unload / free what we can.
	AurieStatus Internal::MdpUnmapImage(
		IN AurieModule* Module,
		IN bool RemoveFromList,
		IN bool CallUnloadRoutine
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		// We don't have to do anything else, since SafetyHook will handle everything for us.
		// Truly a GOATed library, thank you @localcc for telling me about it love ya
		Module->Hooks.clear();

		// Call the unload entry if needed
		if (CallUnloadRoutine)
		{
			last_status = MdpDispatchEntry(
				Module,
				Module->ModuleUnload
			);
		}

		// Remove the module's operation callback
		Module->ModuleOperationCallback = nullptr;

		// Destory all interfaces created by the module
		for (auto& module_interface : Module->InterfaceTable)
		{
			if (module_interface.Interface)
				module_interface.Interface->Destroy();
		}

		// Wipe them off the interface table
		// Note these can't be freed, they're allocated by the owner module
		Module->InterfaceTable.clear();

		// Free all memory allocated by the module (except persistent memory)
		for (auto& memory_allocation : Module->MemoryAllocations)
		{
			MmpFreeMemory(
				memory_allocation.OwnerModule,
				memory_allocation.AllocationBase,
				false
			);
		}

		// Remove all the allocation entries, they're now invalid
		Module->MemoryAllocations.clear();

		// Free the module
		FreeLibrary(Module->ImageBase.Module);

		// Remove the module from our list if needed
		if (RemoveFromList)
			g_LdrModuleList.remove(*Module);

		return last_status;
	}

	AurieStatus Internal::MdpDispatchEntry(
		IN AurieModule* Module,
		IN AurieEntry Entry
	)
	{
		// Ignore dispatch attempts for the initial module
		if (Module == g_ArInitialImage)
			return AURIE_SUCCESS;

		ObpDispatchModuleOperationCallbacks(
			Module, 
			Entry, 
			true
		);

		AurieStatus module_status = Module->FrameworkInitialize(
			g_ArInitialImage,
			PpGetFrameworkRoutine,
			Entry,
			MdpGetImagePath(Module),
			Module
		);

		ObpDispatchModuleOperationCallbacks(
			Module, 
			Entry, 
			false
		);

		return module_status;
	}

	void Internal::MdpMapFolder(
		IN const fs::path& Folder, 
		IN bool Recursive,
		IN bool IsRuntimeLoad,
		OPTIONAL OUT size_t* NumberOfMappedModules
	)
	{
		std::vector<fs::path> modules_to_map;

		MdpBuildModuleList(
			Folder,
			Recursive,
			[](const fs::directory_entry& entry) -> bool
			{
				if (!entry.is_regular_file())
					return false;

				if (!entry.path().has_filename())
					return false;

				if (!entry.path().filename().has_extension())
					return false;

				if (entry.path().filename().extension().compare(L".dll"))
					return false;

				return true;
			},
			modules_to_map
		);

		std::sort(
			modules_to_map.begin(),
			modules_to_map.end()
		);

		size_t loaded_count = 0;
		for (auto& module : modules_to_map)
		{
			AurieModule* loaded_module = nullptr;

			if (AurieSuccess(MdMapImageEx(module, IsRuntimeLoad, loaded_module)))
				loaded_count++;
		}

		if (NumberOfMappedModules)
			*NumberOfMappedModules = loaded_count;
	}

	AurieStatus MdMapImage(
		IN const fs::path& ImagePath, 
		OUT AurieModule*& Module
	)
	{
		return MdMapImageEx(
			ImagePath,
			true,
			Module
		);
	}

	AurieStatus MdMapImageEx(
		IN const fs::path& ImagePath,
		IN bool IsRuntimeLoad,
		OUT AurieModule*& Module
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		HMODULE image_base = nullptr;

		// Map the image
		last_status = Internal::MdpMapImage(ImagePath, image_base);

		if (!AurieSuccess(last_status))
			return last_status;

		// Create the module object
		AurieModule module_object = {};
		last_status = Internal::MdpCreateModule(
			ImagePath,
			image_base,
			true,
			0,
			module_object
		);

		// Verify image integrity
		last_status = Internal::MmpVerifyCallback(module_object.ImageBase.Module, module_object.FrameworkInitialize);
		if (!AurieSuccess(last_status))
			return last_status;

		module_object.Flags.IsRuntimeLoaded = IsRuntimeLoad;

		// Add the module to the module list before running module code
		// No longer safe to access module_object
		Module = Internal::MdpAddModuleToList(std::move(module_object));

		// If we're loaded at runtime, we have to call the module methods manually
		if (IsRuntimeLoad)
		{
			// Dispatch Module Preinitialize to not break modules that depend on it (eg. YYTK)
			last_status = Internal::MdpDispatchEntry(
				Module,
				Module->ModulePreinitialize
			);

			// Remove module if Preinitialize failed
			if (!AurieSuccess(last_status))
			{
				Internal::MdpMarkModuleForPurge(Module); 
				Internal::MdpPurgeMarkedModules();

				return last_status;
			}

			Module->Flags.IsPreloaded = true;

			// Check the environment we are in. This is to detect plugins loaded by other plugins
			// from their ModulePreinitialize routines. In such cases, we don't want
			// to call the loaded plugin's ModuleInitialize, as the process isn't yet
			// initialized.
			bool are_we_within_early_launch = false;
			ElIsProcessSuspended(are_we_within_early_launch);

			// If we're within early launch, ModuleInitialize should not be called.
			// Instead, it will be called in ArProcessAttach when the module initializes.
			if (are_we_within_early_launch)
			{
				return AURIE_SUCCESS;
			}

			// Dispatch Module Initialize
			last_status = Internal::MdpDispatchEntry(
				Module,
				Module->ModuleInitialize
			);

			// Remove module if Initialize failed
			if (!AurieSuccess(last_status))
			{
				Internal::MdpMarkModuleForPurge(Module);
				Internal::MdpPurgeMarkedModules();

				return last_status;
			}

			Internal::MdpPurgeMarkedModules();

			Module->Flags.IsInitialized = true;

			// Module is now fully initialized
		}
		
		return AURIE_SUCCESS;
	}

	bool MdIsImageRuntimeLoaded(
		IN AurieModule* Module
	)
	{
		return Module->Flags.IsRuntimeLoaded;
	}

	bool MdIsImageInitialized(
		IN AurieModule* Module
	)
	{
		return Module->Flags.IsInitialized;
	}

	AurieStatus MdMapFolder(
		IN const fs::path& FolderPath, 
		IN bool Recursive
	)
	{
		if (!fs::exists(FolderPath))
			return AURIE_FILE_NOT_FOUND;

		Internal::MdpMapFolder(
			FolderPath,
			Recursive,
			true,
			nullptr
		);

		return AURIE_SUCCESS;
	}

	AurieStatus MdGetImageFilename(
		IN AurieModule* Module, 
		OUT std::wstring& Filename
	)
	{
		auto& image_path = Internal::MdpGetImagePath(Module);

		if (!image_path.has_filename())
			return AURIE_INVALID_PARAMETER;

		Filename = image_path.filename().wstring();

		return AURIE_SUCCESS;
	}

	bool MdIsImagePreinitialized(
		IN AurieModule* Module
	)
	{
		return Module->Flags.IsPreloaded;
	}

	AurieStatus MdUnmapImage(
		IN AurieModule* Module
	)
	{
		if (Module == g_ArInitialImage)
			return AURIE_ACCESS_DENIED;

		return Internal::MdpUnmapImage(Module, true, true);
	}
}
