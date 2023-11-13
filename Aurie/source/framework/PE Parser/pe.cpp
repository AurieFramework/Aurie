#include "pe.hpp"
#include <fstream>

namespace Aurie
{
	AurieStatus PpQueryImageArchitecture(
		IN const std::filesystem::path& Path, 
		OUT unsigned short& ImageArchitecture
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		void* image_base = nullptr;
		size_t image_size = 0;
		unsigned short image_arch = 0;

		// Allocate the file into memory
		last_status = Internal::PpiMapFileToMemory(
			Path,
			image_base,
			image_size
		);

		// If we fail, just bail
		if (!AurieSuccess(last_status))
		{
			return last_status;
		}
		
		// Query the image architecture
		last_status = Internal::PpiQueryImageArchitecture(
			image_base,
			image_arch
		);

		// Save the image architecture we got
		ImageArchitecture = image_arch;

		// Free the file, we don't care if we errored on PpiQueryImageArchitecture
		delete[] image_base;
		return last_status;
	}

	uintptr_t PpFindFileExportByName(
		IN const fs::path& ImagePath, 
		IN const char* ImageExportName)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		void* image_base = nullptr;
		size_t image_size = 0;

		// Map the file into memory, remember to free!
		last_status = Internal::PpiMapFileToMemory(
			ImagePath,
			image_base,
			image_size
		);

		if (!AurieSuccess(last_status))
		{	
			// File mapping failed, not enough memory or the file doesn't exist?
			// See last_status for more information.
			return 0;
		}

		uintptr_t export_offset = 0;
		last_status = Internal::PpiGetExportOffset(
			image_base,
			ImageExportName,
			export_offset
		);

		delete[] image_base;

		if (!AurieSuccess(last_status))
		{
			// The export probably wasn't found.
			// See last_status for more information.
			return 0;
		}

		return export_offset;
	}

	void* PpGetFrameworkRoutine(
		IN const char* ExportName
	)
	{
		FARPROC framework_routine = GetProcAddress(
			g_ArInitialImage->ImageBase.Module,
			ExportName
		);

		return reinterpret_cast<void*>(framework_routine);
	}

	AurieStatus PpGetImageSubsystem(
		IN PVOID Image, 
		OUT unsigned short& ImageSubsystem
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		PIMAGE_NT_HEADERS nt_header = nullptr;

		last_status = Internal::PpiGetNtHeader(
			Image, 
			(void*&)nt_header
		);

		if (!AurieSuccess(last_status))
			return last_status;

		ImageSubsystem = nt_header->OptionalHeader.Subsystem;
		return AURIE_SUCCESS;
	}

	AurieStatus PpGetCurrentArchitecture(
		IN unsigned short& ImageArchitecture
	)
	{
		return Internal::PpiQueryImageArchitecture(
			g_ArInitialImage->ImageBase.Pointer,
			ImageArchitecture
		);
	}

	AurieStatus Internal::PpiMapFileToMemory(
		IN const fs::path& FilePath,
		OUT void*& BaseOfFile, 
		OUT size_t& SizeOfFile
	)
	{
		// Try to open the file
		std::ifstream input_file(FilePath, std::ios::binary | std::ios::ate);

		// If we can't open it, it either doesn't exist or the permissions don't allow us to open the file.
		// I think it's fair we just truncate both to an access denied error.
		if (!input_file.is_open() || !input_file.good())
			return AURIE_ACCESS_DENIED;

		// Query the filesize and allocate memory for it
		size_t file_size = input_file.tellg();
		char* file_in_memory = new char[file_size];

		// If we fail allocating memory, then there's simply not enough memory for us to allocate.
		if (!file_in_memory)
			return AURIE_INSUFFICIENT_MEMORY;

		// Copy the file to the allocated buffer
		input_file.seekg(0, std::ios::beg);
		input_file.read(file_in_memory, file_size);
		input_file.close();

		SizeOfFile = file_size;
		BaseOfFile = file_in_memory;

		return AURIE_SUCCESS;
	}

	AurieStatus Internal::PpiQueryImageArchitecture(
		IN void* Image, 
		OUT unsigned short& ImageArchitecture
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;
		PIMAGE_NT_HEADERS nt_header = nullptr;

		last_status = PpiGetNtHeader(
			Image, 
			(void*&)nt_header
		);

		if (!AurieSuccess(last_status))
		{
			return last_status;
		}

		ImageArchitecture = nt_header->FileHeader.Machine;

		return AURIE_SUCCESS;
	}

	EXPORTED void* Internal::PpiFindModuleExportByName(
		IN const AurieModule* Image,
		IN const char* ImageExportName
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		uintptr_t export_offset = 0;
		last_status = Internal::PpiGetExportOffset(
			Image->ImageBase.Pointer,
			ImageExportName,
			export_offset
		);

		if (!AurieSuccess(last_status))
		{
			// The export probably wasn't found.
			// See last_status for more information.
			return nullptr;
		}

		return reinterpret_cast<void*>(Image->ImageBase.Address + export_offset);
	}

	AurieStatus Internal::PpiGetNtHeader(
		IN void* Image, 
		OUT void*& NtHeader
	)
	{
		// Check the DOS header to be sure we mapped an actual PE file
		PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(Image);
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		{
			// PpiMapFileToMemory uses operator new[], so we use the array one to delete it too
			return AURIE_INVALID_SIGNATURE;
		}

		// Get the NT headers
		PIMAGE_NT_HEADERS nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(
			reinterpret_cast<char*>(Image) + dos_header->e_lfanew
		);

		// The signature field is the same offset and size for both x86 and x64.
		if (nt_header->Signature != IMAGE_NT_SIGNATURE)
		{
			return AURIE_INVALID_SIGNATURE;
		}

		NtHeader = nt_header;
		return AURIE_SUCCESS;
	}

	// https://github.com/Archie-osu/HattieDrv/blob/main/Driver/source/ioctl/pe/pe.cpp
	// See HTGetSectionBoundsByName
	AurieStatus Internal::PpiGetModuleSectionBounds(
		IN void* Image, 
		IN const char* SectionName, 
		OUT uint64_t& SectionOffset, 
		OUT size_t& SectionSize
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		PIMAGE_NT_HEADERS nt_header = nullptr;
		last_status = PpiGetNtHeader(Image, reinterpret_cast<void*&>(nt_header));

		// NT Header query failed, not a valid image?
		if (!AurieSuccess(last_status))
			return last_status;

		PIMAGE_SECTION_HEADER first_section = reinterpret_cast<PIMAGE_SECTION_HEADER>(nt_header + 1);

		for (
			PIMAGE_SECTION_HEADER current_section = first_section;
			current_section < (first_section + nt_header->FileHeader.NumberOfSections);
			current_section++
			)
		{
			// current_section->Name is not null terminated, we null terminate it.
			// This might not be an issue in normal PE files
			// but while parsing ntoskrnl.exe, this issue became apparent.
			char name_buffer[16] = { 0 };
			memcpy(name_buffer, current_section->Name, 8);

			// Actually we didn't need to null-terminate the string here, since we only compare up to 8 chars anyway
			if (!_strnicmp(name_buffer, SectionName, 8))
			{
				SectionOffset = current_section->VirtualAddress;
				SectionSize = current_section->Misc.VirtualSize;

				return AURIE_SUCCESS;
			}
		}

		return AURIE_FILE_PART_NOT_FOUND;
	}

	AurieStatus Internal::PpiGetExportOffset(
		IN void* Image, 
		IN const char* ImageExportName, 
		OUT uintptr_t& ExportOffset
	)
	{
		AurieStatus last_status = AURIE_SUCCESS;

		unsigned short target_image_arch = 0;
		last_status = Internal::PpiQueryImageArchitecture(
			Image,
			target_image_arch
		);

		if (!AurieSuccess(last_status))
		{
			// Querying the image architecture failed, might not be a PE file?
			return last_status;
		}

		void* nt_headers = nullptr;
		last_status = Internal::PpiGetNtHeader(
			Image,
			nt_headers
		);

		if (!AurieSuccess(last_status))
		{
			return last_status;
		}

		PIMAGE_EXPORT_DIRECTORY export_directory = nullptr;

		if (target_image_arch == IMAGE_FILE_MACHINE_I386)
		{
			// This NT_HEADERS object has the correct bitness, it is now safe to access the optional header
			PIMAGE_NT_HEADERS32 nt_headers_x86 = reinterpret_cast<PIMAGE_NT_HEADERS32>(nt_headers);

			// Handle object files
			if (!nt_headers_x86->FileHeader.SizeOfOptionalHeader)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// In case our file doesn't have an export header
			if (nt_headers_x86->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// Get the RVA - we can't just add this to file_in_memory because of section alignment and stuff...
			// We could if we had the file already LLA'd into memory, but that's impossible, since the file's a different arch.
			DWORD export_dir_address = nt_headers_x86->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			if (!export_dir_address)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// Get the export directory from the VA 
			export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
				reinterpret_cast<char*>(Image) + Internal::PpiRvaToFileOffset32(
					nt_headers_x86,
					export_dir_address
				)
			);
		}
		else if (target_image_arch == IMAGE_FILE_MACHINE_AMD64)
		{
			PIMAGE_NT_HEADERS64 nt_headers_x64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(nt_headers);

			// Handle object files
			if (!nt_headers_x64->FileHeader.SizeOfOptionalHeader)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// In case our file doesn't have an export header
			if (nt_headers_x64->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// Get the RVA - we can't just add this to file_in_memory because of section alignment and stuff...
			// We could if we had the file already LLA'd into memory, but that's impossible, since  the file's a different arch.
			DWORD export_dir_address = nt_headers_x64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			if (!export_dir_address)
			{
				return AURIE_FILE_PART_NOT_FOUND;
			}

			// Get the export directory from the VA 
			export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
				reinterpret_cast<char*>(Image) + Internal::PpiRvaToFileOffset64(
					nt_headers_x64,
					export_dir_address
				)
			);
		}
		else
		{
			// Unsupported architecture
			return AURIE_INVALID_ARCH;
		}

		// The file doesn't have an export directory (aka. doesn't export anything)
		if (!export_directory)
		{
			return AURIE_FILE_PART_NOT_FOUND;
		}

		// Get all our required arrays
		DWORD* function_names = reinterpret_cast<DWORD*>(
			reinterpret_cast<char*>(Image) +
			PpiRvaToFileOffset(
				reinterpret_cast<PIMAGE_NT_HEADERS>(nt_headers),
				export_directory->AddressOfNames
			)
		);

		WORD* function_name_ordinals = reinterpret_cast<WORD*>(
			reinterpret_cast<char*>(Image) +
			PpiRvaToFileOffset(
				reinterpret_cast<PIMAGE_NT_HEADERS>(nt_headers),
				export_directory->AddressOfNameOrdinals
			)
		);

		DWORD* function_addresses = reinterpret_cast<DWORD*>(
			reinterpret_cast<char*>(Image) +
			PpiRvaToFileOffset(
				reinterpret_cast<PIMAGE_NT_HEADERS>(nt_headers),
				export_directory->AddressOfFunctions
			)
		);

		// Loop over all the named exports
		for (DWORD n = 0; n < export_directory->NumberOfNames; n++)
		{
			// Get the name of the export
			const char* export_name = reinterpret_cast<char*>(Image) +
				PpiRvaToFileOffset(
					reinterpret_cast<PIMAGE_NT_HEADERS>(nt_headers), 
					function_names[n]
			);

			// Get the function ordinal for array access
			short function_ordinal = function_name_ordinals[n];

			// Get the function offset
			uint32_t function_offset = function_addresses[function_ordinal];

			// If it's our target export
			if (!_stricmp(ImageExportName, export_name))
			{
				ExportOffset = function_offset;
				return AURIE_SUCCESS;
			}
		}

		return AURIE_OBJECT_NOT_FOUND;
	}

	uint32_t Internal::PpiRvaToFileOffset(
		IN PIMAGE_NT_HEADERS ImageHeaders, 
		IN uint32_t Rva
	)
	{
		PIMAGE_SECTION_HEADER pHeaders = IMAGE_FIRST_SECTION(ImageHeaders);

		// Loop over all the sections of the file
		for (short n = 0; n < ImageHeaders->FileHeader.NumberOfSections; n++)
		{
			// ... to check if the RVA points to within that section
			// the section begins at pHeaders[n].VirtualAddress and ends at pHeaders[n].VirtualAddress + pHeaders[n].SizeOfRawData
			if (Rva >= pHeaders[n].VirtualAddress && Rva < (pHeaders[n].VirtualAddress + pHeaders[n].SizeOfRawData))
			{
				// The RVA points into this section, so return the offset inside the section's data.
				return (Rva - pHeaders[n].VirtualAddress) + pHeaders[n].PointerToRawData;
			}
		}

		return 0;
	}

	uint32_t Internal::PpiRvaToFileOffset64(
		IN PIMAGE_NT_HEADERS64 ImageHeaders,
		IN uint32_t Rva
	)
	{
		return PpiRvaToFileOffset(
			reinterpret_cast<PIMAGE_NT_HEADERS>(ImageHeaders),
			Rva
		);
	}

	uint32_t Internal::PpiRvaToFileOffset32(
		IN PIMAGE_NT_HEADERS32 ImageHeaders,
		IN uint32_t Rva
	)
	{
		return PpiRvaToFileOffset(
			reinterpret_cast<PIMAGE_NT_HEADERS>(ImageHeaders),
			Rva
		);
	}
}

