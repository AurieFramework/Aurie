#include <iostream>
#include <fstream>
#include "pe_parser.hpp"
#include <Zydis/Zydis.h>

#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)


using PFN_LoadLibraryW = decltype(&::LoadLibraryW);
using PFN_SuspendThread = decltype(&::SuspendThread);
using PFN_RtlPcToFileHeader = decltype(&::RtlPcToFileHeader);
using PFN_RtlImageNtHeader = decltype(&::PE::RtlImageNtHeader);
using PFN_ProcessEntrypoint = void(*)(void*, uintptr_t);

constexpr SIZE_T payload_size = 0x1000;
struct AurieRwxPage
{
	PFN_LoadLibraryW m_LoadLibraryW;
	PFN_SuspendThread m_SuspendThread;
	PFN_RtlPcToFileHeader m_RtlPcToFileHeader;
	PFN_RtlImageNtHeader m_RtlImageNtHeader;

	DWORD m_OriginalEntrypointAddress;
	wchar_t m_PathToLoad[MAX_PATH];

	char m_ShellcodeBytes[0x400];
};
static_assert(sizeof(AurieRwxPage) <= payload_size);

// No CFG for this function. This is what gets executed instead of the process entrypoint.
#pragma runtime_checks("", off)
__declspec(safebuffers) __declspec(guard(nocf))
void ArProcessEntrypoint(struct _PEB* Rcx, uintptr_t EntrypointAddress)
{
	const AurieRwxPage* shellcode_page = reinterpret_cast<AurieRwxPage*>(EntrypointAddress & ~(0xFFF));
	
	// Get the image base address using RtlPcToFileHeader
	PVOID image_base = nullptr;
	shellcode_page->m_RtlPcToFileHeader(
		reinterpret_cast<PVOID>(EntrypointAddress),
		&image_base
	);

	// Get the DOS and NT image headers
	PIMAGE_NT_HEADERS nt_header = shellcode_page->m_RtlImageNtHeader(
		image_base
	);

	shellcode_page->m_LoadLibraryW(shellcode_page->m_PathToLoad);
	shellcode_page->m_SuspendThread(NtCurrentThread());

	return reinterpret_cast<PFN_ProcessEntrypoint>(
		static_cast<char*>(image_base) + shellcode_page->m_OriginalEntrypointAddress
	)(Rcx, EntrypointAddress);
}
#pragma runtime_checks("", restore)


int wmain(int argc, wchar_t** argv)
{
	if (argc < 3)
	{
		printf("Usage: %S executable aurie_core_path [-w]\n", argv[0]);
		return ERROR_BAD_ARGUMENTS;
	}

	bool wait_on_error = false;
	if (argc == 4 && !_wcsicmp(argv[2], L"-w"))
		wait_on_error = true;

	printf("Using executable: \"%S\"\n", argv[1]);
	printf("Using AurieCore path: \"%S\"\n", argv[2]);

	PVOID file_base = nullptr;
	SIZE_T file_size = 0;

	// Read the file into memory.
	DWORD last_error = PE::MapFileToMemory(
		argv[1],
		P2ALIGNUP(payload_size, 0x1000),
		file_base,
		file_size
	);

	// If we failed doing that, no point continuing.
	if (last_error)
	{
		printf("Error occured while mapping the file.\n");

		if (wait_on_error) std::cin.ignore();
		return last_error;
	}

	// Print some info about where the file is loaded.
	printf("File mapped to 0x%p => size 0x%llX\n", file_base, file_size);

	PIMAGE_NT_HEADERS nt_headers = PE::RtlImageNtHeader(file_base);
	if (!nt_headers)
	{
		printf("Image is not a valid PE file.\n");

		if (wait_on_error) std::cin.ignore();
		return last_error;
	}
	
	// Add an RWX section called .aurie to the executable
	// TODO: Check if this section already exists.

	PIMAGE_SECTION_HEADER new_section = PE::GetSectionHeaderByName(nt_headers, ".aurie");
	bool repeated_install = new_section != nullptr;
	if (!new_section)
	{
		new_section = PE::AddRwxSection(
			file_base,
			".aurie",
			payload_size
		);
	}

	printf(".aurie section at %p\n", new_section);

	// Get the pointer to the RWX section we created.
	// We specify the format of it via AurieRwxPage.
	AurieRwxPage* rwx_page = reinterpret_cast<AurieRwxPage*>(
		static_cast<char*>(file_base) + new_section->PointerToRawData
	);

	printf("RWX page at %p\n", rwx_page);

	if (repeated_install)
		nt_headers->OptionalHeader.AddressOfEntryPoint = rwx_page->m_OriginalEntrypointAddress;

	memset(rwx_page, 0, payload_size);
	rwx_page->m_LoadLibraryW = LoadLibraryW;
	rwx_page->m_RtlPcToFileHeader = RtlPcToFileHeader;
	rwx_page->m_SuspendThread = SuspendThread;
	rwx_page->m_RtlImageNtHeader = PE::RtlImageNtHeader;
	rwx_page->m_OriginalEntrypointAddress = nt_headers->OptionalHeader.AddressOfEntryPoint;

	// Copy in the path of the DLL that the process has to load
	wcscpy_s(
		rwx_page->m_PathToLoad,
		argv[2]
	);

	// Write the new entrypoint.
	nt_headers->OptionalHeader.AddressOfEntryPoint =
		new_section->VirtualAddress + offsetof(AurieRwxPage, m_ShellcodeBytes);

	ZydisDisassembledInstruction disassembled_instruction;
	ZyanStatus status = ZydisDisassembleIntel(
		nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 ? ZYDIS_MACHINE_MODE_LONG_64 : ZYDIS_MACHINE_MODE_LEGACY_32,
		reinterpret_cast<ZyanU64>(ArProcessEntrypoint),
		ArProcessEntrypoint,
		0x1000,
		&disassembled_instruction
	);

	if (!ZYAN_SUCCESS(status))
		return ERROR_INVALID_ADDRESS;
	
	printf("First shellcode instruction is %s\n", disassembled_instruction.text);

	ZyanU64 address_to_copy_from = reinterpret_cast<ZyanU64>(ArProcessEntrypoint);
	if (disassembled_instruction.info.mnemonic == ZYDIS_MNEMONIC_JMP)
	{
		ZydisCalcAbsoluteAddress(
			&disassembled_instruction.info,
			&disassembled_instruction.operands[0],
			reinterpret_cast<ZyanU64>(ArProcessEntrypoint),
			&address_to_copy_from
		);
	}

	// Copy shellcode into it
	memcpy(rwx_page->m_ShellcodeBytes, (PVOID)address_to_copy_from, sizeof(rwx_page->m_ShellcodeBytes));
	std::ofstream out_file(argv[1], std::ios::binary);
	out_file.write((const char*)file_base, file_size);
		
	return ERROR_SUCCESS;
}