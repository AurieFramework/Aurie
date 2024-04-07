#ifndef AURIE_PE_H_
#define AURIE_PE_H_

#include "../framework.hpp"
#include <Windows.h>

namespace Aurie
{
	// Queries a file's image architecture
	EXPORTED AurieStatus PpQueryImageArchitecture(
		IN const fs::path& Path,
		OUT unsigned short& ImageArchitecture
	);

	// Finds the file offset of an export
	EXPORTED uintptr_t PpFindFileExportByName(
		IN const fs::path& Path,
		IN const char* ImageExportName
	);

	// GetProcAddress on the Aurie Framework module
	EXPORTED void* PpGetFrameworkRoutine(
		IN const char* ExportName
	);

	// Queries the image architecture of the running Aurie Framework module
	EXPORTED AurieStatus PpGetCurrentArchitecture(
		IN unsigned short& ImageArchitecture
	);

	// Queries the image subsystem of a mapped image
	EXPORTED AurieStatus PpGetImageSubsystem(
		IN PVOID Image,
		OUT unsigned short& ImageSubsystem
	);

	namespace Internal
	{
		// Finds an export of a given AurieModule by name
		EXPORTED void* PpiFindModuleExportByName(
			IN const AurieModule* Image,
			IN const char* ImageExportName
		);

		// Maps a file on a given path to memory (remember to free it!)
		AurieStatus PpiMapFileToMemory(
			IN const fs::path& FilePath,
			OUT void*& BaseOfFile, // must call delete[] to free memory
			OUT size_t& SizeOfFile
		);

		// Gets the Machine field from the NT header of an image
		EXPORTED AurieStatus PpiQueryImageArchitecture(
			IN void* Image,
			OUT unsigned short& ImageArchitecture
		);

		// Get NT Header of image (also verifies signatures)
		EXPORTED AurieStatus PpiGetNtHeader(
			IN void* Image,
			OUT void*& NtHeader
		);

		// Gets the module section bounds of a mapped image
		EXPORTED AurieStatus PpiGetModuleSectionBounds(
			IN void* Image,
			IN const char* SectionName,
			OUT uint64_t& SectionBase,
			OUT size_t& SectionSize
		);

		// Queries an export offset from the start of a file
		// This offset stays the same when the file is mapped
		AurieStatus PpiGetExportOffset(
			IN void* Image,
			IN const char* ImageExportName,
			OUT uintptr_t& ExportOffset
		);

		// Convert an section RVA to an offset from the image base
		EXPORTED uint32_t PpiRvaToFileOffset(
			IN PIMAGE_NT_HEADERS ImageHeaders,
			IN uint32_t Rva
		);

		// Wrapper for a different type
		uint32_t PpiRvaToFileOffset32(
			IN PIMAGE_NT_HEADERS32 ImageHeaders,
			IN uint32_t Rva
		);

		// Wrapper for a different type
		uint32_t PpiRvaToFileOffset64(
			IN PIMAGE_NT_HEADERS64 ImageHeaders,
			IN uint32_t Rva
		);
	}
}

#endif // AURIE_PE_H_