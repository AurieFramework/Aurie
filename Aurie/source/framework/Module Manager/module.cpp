#include "module.hpp"
#include <Psapi.h>

namespace Aurie
{
	AurieStatus Internal::MdiCreateModule(
		IN const fs::path& ImagePath, 
		IN HMODULE ImageModule,
		IN AurieEntry ModuleInitialize, 
		IN AurieEntry ModulePreinitialize,
		IN AurieLoaderEntry FrameworkInitialize,
		IN uint8_t BitFlags,
		OUT AurieModule& Module
	)
	{
		AurieModule temp_module;
		AurieStatus last_status = AURIE_SUCCESS;

		// Populate known fields first
		temp_module.Flags.Bitfield = BitFlags;
		temp_module.ImagePath = ImagePath;
		temp_module.ModuleInitialize = ModuleInitialize;
		temp_module.ModulePreinitialize = ModulePreinitialize;
		temp_module.FrameworkInitialize = FrameworkInitialize;

		last_status = MdiQueryModuleInformation(
			ImageModule,
			&temp_module.ImageBase.Pointer,
			&temp_module.ImageSize,
			&temp_module.ImageEntrypoint.Pointer
		);

		if (!AurieSuccess(last_status))
		{
			return last_status;
		}

		Module = temp_module;

		return AURIE_SUCCESS;
	}

	AurieStatus Internal::MdiMapImage(
		IN const fs::path& ImagePath, 
		OUT HMODULE ImageBase
	)
	{
		// If the file doesn't exist, we have nothing to map
		std::error_code ec;
		if (!fs::exists(ImagePath, ec))
			return AURIE_FILE_NOT_FOUND;

		AurieStatus last_status = AURIE_SUCCESS;
		short target_arch = 0, self_arch = 0;
		
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
		bool has_framework_init = PpFindFileExportByName(ImagePath, "__aurie_fwk_init") != 0;
		bool has_module_entry = PpFindFileExportByName(ImagePath, "ModuleEntry") != 0;

		if (!has_framework_init || !has_module_entry)
			return AURIE_MODULE_INITIALIZATION_FAILED;

		// Load the image into memory and make sure we loaded it
		HMODULE image_module = LoadLibraryW(ImagePath.wstring().c_str());

		if (!image_module)
			return AURIE_EXTERNAL_ERROR;

		ImageBase = image_module;
		return AURIE_SUCCESS;
	}

	AurieModule* Internal::MdiAddModuleToList(
		IN const AurieModule& Module
	)
	{
		g_LdrModuleList.push_back(Module);
		return &g_LdrModuleList.back();
	}

	AurieStatus Internal::MdiQueryModuleInformation(
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

	fs::path& Internal::MdiGetImagePath(IN AurieModule* Module)
	{
		return Module->ImagePath;
	}

	AurieStatus Internal::MdiGetImageFolder(
		IN AurieModule* Module, 
		OUT fs::path& Path
	)
	{
		fs::path module_path = Internal::MdiGetImagePath(Module);

		if (!module_path.has_parent_path())
			return AURIE_INVALID_PARAMETER;

		Path = module_path.parent_path();
		return AURIE_SUCCESS;
	}

	AurieStatus Internal::MdiGetNextModule(
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

	PVOID Internal::MdiGetModuleBaseAddress(
		IN AurieModule* Module
	)
	{
		return Module->ImageBase.Pointer;
	}

	AurieStatus Internal::MdiMapFolder(
		IN const fs::path& ImagePath, 
		OUT OPTIONAL size_t* ModuleCount
	)
	{
		
	}

	AurieStatus MdMapImage(
		IN const fs::path& ImagePath, 
		OUT AurieModule*& Module
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		HMODULE image_base = nullptr;

		// Map the image
		last_status = Internal::MdiMapImage(ImagePath, image_base);

		if (!AurieSuccess(last_status))
			return last_status;

		// Find all the required functions
		uintptr_t framework_init_offset = PpFindFileExportByName(ImagePath, "__aurie_fwk_init");
		uintptr_t module_init_offset = PpFindFileExportByName(ImagePath, "ModuleEntry");
		uintptr_t module_preload_offset = PpFindFileExportByName(ImagePath, "ModulePreload");

		AurieEntry module_init = reinterpret_cast<AurieEntry>((char*)image_base + module_init_offset);
		AurieEntry module_preload = reinterpret_cast<AurieEntry>((char*)image_base + module_preload_offset);
		AurieLoaderEntry fwk_init = reinterpret_cast<AurieLoaderEntry>((char*)image_base + framework_init_offset);

		// MdiMapImage checks for __aurie_fwk_init and ModuleEntry, but doesn't check ModulePreload since it's optional
		// If ModulePreload has a null offset, it means it doesn't exist and should be nullptr.
		if (!module_preload_offset)
			module_preload = nullptr;

		// Create the module object
		AurieModule module_object = {};
		last_status = Internal::MdiCreateModule(
			ImagePath,
			image_base,
			module_init,
			module_preload,
			fwk_init,
			0,
			module_object
		);

		if (!AurieSuccess(last_status))
			return last_status;

		// Add it to our list of modules
		Module = Internal::MdiAddModuleToList(module_object);
		return AURIE_SUCCESS;
	}

	bool MdIsImageInitialized(
		IN AurieModule* Module
	)
	{
		return Module->Flags.IsInitialized;
	}

	AurieStatus MdGetImageFilename(
		IN AurieModule* Module, 
		OUT std::wstring& Filename
	)
	{
		auto& image_path = Internal::MdiGetImagePath(Module);

		if (!image_path.has_filename())
			return AURIE_INVALID_PARAMETER;

		Filename = image_path.filename().wstring();

		return AURIE_SUCCESS;
	}
}


