#pragma once
#include <filesystem>
#include <Windows.h>
namespace PE
{
	#pragma comment(lib, "ntdll.lib")
	EXTERN_C NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(
		IN PVOID ModuleAddress
	);

	// Generic align-up, assumes Alignment to be a power of two
	#pragma warning(disable : 4146)
	#define P2ALIGNUP(Value, Alignment) (-(-((SIZE_T)Value) & -((SIZE_T)Alignment)))


	DWORD MapFileToMemory(
		IN const std::filesystem::path& FilePath,
		IN SIZE_T ReservedSize,
		OUT void*& BaseOfFile,
		OUT size_t& SizeOfFile
	);

	PIMAGE_SECTION_HEADER AddRwxSection(
		IN PVOID ImageBase,
		IN const char* SectionName,
		IN SIZE_T SectionSize
	);

	void RemoveLastSection(
		IN PVOID ImageBase
	);

	PIMAGE_SECTION_HEADER GetSectionHeaderByName(
		IN PIMAGE_NT_HEADERS NtHeader,
		IN const char* Name
	);

	template <typename T>
	T* GetExport(
		IN PVOID Dll,
		IN PIMAGE_NT_HEADERS NtHeader,
		IN const char* ExportName
	)
	{
		// In case our file doesn't have an export header
		if (NtHeader->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
			return nullptr;

		auto export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
			static_cast<char*>(Dll) + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
			);

		// Get all our required arrays
		DWORD* function_names = reinterpret_cast<DWORD*>(
			static_cast<char*>(Dll) + export_directory->AddressOfNames
			);

		WORD* function_name_ordinals = reinterpret_cast<WORD*>(
			static_cast<char*>(Dll) + export_directory->AddressOfNameOrdinals
			);

		DWORD* function_addresses = reinterpret_cast<DWORD*>(
			static_cast<char*>(Dll) + export_directory->AddressOfFunctions
			);

		// Loop over all the named exports
		for (DWORD n = 0; n < export_directory->NumberOfNames; n++)
		{
			// Get the name of the export
			const char* export_name = static_cast<char*>(Dll) + function_names[n];

			// Get the function ordinal for array access
			short function_ordinal = function_name_ordinals[n];

			// Get the function offset
			DWORD function_offset = function_addresses[function_ordinal];

			// If it's our target export
			if (!_stricmp(ExportName, export_name))
			{
				return reinterpret_cast<T*>(static_cast<char*>(Dll) + function_offset);
			}
		}

		return nullptr;
	};
}