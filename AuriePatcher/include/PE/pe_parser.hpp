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
}