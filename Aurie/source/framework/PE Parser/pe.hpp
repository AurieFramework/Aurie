#ifndef AURIE_PE_H_
#define AURIE_PE_H_

#include "../framework.hpp"
#include <Windows.h>

namespace Aurie
{
	EXPORTED AurieStatus PpQueryImageArchitecture(
		IN const fs::path& Path,
		OUT unsigned short& ImageArchitecture
	);

	EXPORTED uintptr_t PpFindFileExportByName(
		IN const fs::path& Path,
		IN const char* ImageExportName
	);

	EXPORTED void* PpGetFrameworkRoutine(
		IN const char* ExportName
	);

	EXPORTED AurieStatus PpGetCurrentArchitecture(
		IN unsigned short& ImageArchitecture
	);

	namespace Internal
	{
		// Finds an export by name
		EXPORTED void* PppFindModuleExportByName(
			IN const AurieModule* Image,
			IN const char* ImageExportName
		);

		// Maps a file on a given path to memory (remember to free it!)
		AurieStatus PppMapFileToMemory(
			IN const fs::path& FilePath,
			OUT void*& BaseOfFile, // must call delete[] to free memory
			OUT size_t& SizeOfFile
		);

		// Gets the Machine field from the NT header of an image
		EXPORTED AurieStatus PppQueryImageArchitecture(
			IN void* Image,
			OUT unsigned short& ImageArchitecture
		);

		// Get NT Header of image (also verifies signatures)
		EXPORTED AurieStatus PppGetNtHeader(
			IN void* Image,
			OUT void*& NtHeader
		);

		EXPORTED AurieStatus PppGetModuleSectionBounds(
			IN void* Image,
			IN const char* SectionName,
			OUT uint64_t& SectionBase,
			OUT size_t& SectionSize
		);

		AurieStatus PppGetExportOffset(
			IN void* Image,
			IN const char* ImageExportName,
			OUT uintptr_t& ExportOffset
		);

		// Convert an section RVA to an offset from the image base
		EXPORTED uint32_t PppRvaToFileOffset(
			IN PIMAGE_NT_HEADERS ImageHeaders,
			IN uint32_t Rva
		);

		// Wrapper for a different type
		uint32_t PppRvaToFileOffset32(
			IN PIMAGE_NT_HEADERS32 ImageHeaders,
			IN uint32_t Rva
		);

		// Wrapper for a different type
		uint32_t PppRvaToFileOffset64(
			IN PIMAGE_NT_HEADERS64 ImageHeaders,
			IN uint32_t Rva
		);
	}
}

#endif // AURIE_PE_H_