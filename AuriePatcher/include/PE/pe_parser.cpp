#include "pe_parser.hpp"
#include <fstream>

DWORD PE::MapFileToMemory(
	IN const std::filesystem::path& FilePath, 
	IN SIZE_T ReservedSize,
	OUT void*& BaseOfFile,
	OUT size_t& SizeOfFile
)
{
	// Try to open the file
	std::ifstream input_file(FilePath, std::ios::binary | std::ios::ate);

	// If we can't open it, it either doesn't exist or the permissions don't allow us to open the file.
	// I think it's fair we just truncate both to an access denied error.
	if (!input_file.is_open() || !input_file.good())
		return ERROR_ACCESS_DENIED;

	// Query the filesize and allocate memory for it
	std::streampos file_size_raw = input_file.tellg();
	if (file_size_raw > SIZE_MAX)
		return ERROR_EMPTY;

	size_t file_size = static_cast<size_t>(file_size_raw) + ReservedSize;
	char* file_in_memory = new char[file_size];

	// If we fail allocating memory, then there's simply not enough memory for us to allocate.
	if (!file_in_memory)
		return ERROR_NOT_ENOUGH_MEMORY;

	// Copy the file to the allocated buffer
	input_file.seekg(0, std::ios::beg);
	input_file.read(file_in_memory, file_size_raw);
	input_file.close();

	SizeOfFile = file_size_raw;
	BaseOfFile = file_in_memory;

	return ERROR_SUCCESS;
}

PIMAGE_SECTION_HEADER PE::AddRwxSection(IN PVOID ImageBase, IN const char* SectionName, IN SIZE_T SectionSize)
{
	auto nt_headers = RtlImageNtHeader(ImageBase);
	// Get all relevant information from the file
	const WORD section_count = nt_headers->FileHeader.NumberOfSections;
	const DWORD section_alignment = nt_headers->OptionalHeader.SectionAlignment;
	const UINT32 file_alignment = nt_headers->OptionalHeader.FileAlignment;

	// Get all relevant pointers to sections
	PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(nt_headers);
	PIMAGE_SECTION_HEADER new_section_header = &first_section[section_count];
	PIMAGE_SECTION_HEADER last_section_header = &first_section[section_count - 1];

	// Copy in the section name
	memcpy(&new_section_header->Name, SectionName, strlen(SectionName));

	// Set up the section properties
	new_section_header->Misc.VirtualSize = static_cast<DWORD>(SectionSize);
	new_section_header->VirtualAddress = P2ALIGNUP(
		last_section_header->VirtualAddress + last_section_header->Misc.VirtualSize,
		section_alignment
	);

	new_section_header->PointerToRawData = last_section_header->PointerToRawData + last_section_header->SizeOfRawData;
	new_section_header->SizeOfRawData = P2ALIGNUP(SectionSize, file_alignment);
	new_section_header->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE;

	nt_headers->FileHeader.NumberOfSections++;
	nt_headers->OptionalHeader.SizeOfImage = P2ALIGNUP(
		new_section_header->VirtualAddress + new_section_header->Misc.VirtualSize,
		section_alignment
	);

	return new_section_header;
}

void PE::RemoveLastSection(
	IN PVOID ImageBase
)
{
	auto nt_headers = RtlImageNtHeader(ImageBase);
	// Get all relevant information from the file
	const WORD section_count = nt_headers->FileHeader.NumberOfSections;
	const DWORD section_alignment = nt_headers->OptionalHeader.SectionAlignment;
	const UINT32 file_alignment = nt_headers->OptionalHeader.FileAlignment;

	// Can't have no sections
	if (section_count <= 1)
		return;

	// Get all relevant pointers to sections
	PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(nt_headers);

	PIMAGE_SECTION_HEADER second_last_header = &first_section[section_count - 2];
	PIMAGE_SECTION_HEADER last_section_header = &first_section[section_count - 1];

	nt_headers->FileHeader.NumberOfSections--;
	nt_headers->OptionalHeader.SizeOfImage = P2ALIGNUP(
		second_last_header->VirtualAddress + second_last_header->Misc.VirtualSize,
		section_alignment
	);

	// Erase contents of the section
	memset(last_section_header, 0, sizeof(IMAGE_SECTION_HEADER));
}

PIMAGE_SECTION_HEADER PE::GetSectionHeaderByName(
	IN PIMAGE_NT_HEADERS NtHeader, 
	IN const char* Name
)
{
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(NtHeader);

	// Comma operator good! Returns the value of i, but increments first_section too
	for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; section++, i++)
	{
		if (!strncmp(reinterpret_cast<const char*>(section->Name), Name, sizeof(section->Name)))
			return section;
	}

	return nullptr;
}
