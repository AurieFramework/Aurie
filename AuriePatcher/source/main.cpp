#include <iostream>
#include <fstream>
#include <PE/pe_parser.hpp>
#include <NT/nt.hpp>

#define AURIE_SECTION_NAME ".aurie"

#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
using PFN_OriginalEntrypoint = void(*)();

// Export for reinstall purposes
extern "C" __declspec(dllexport) DWORD g_OldOEP = 0;
extern "C" __declspec(dllexport) wchar_t g_AuriePath[MAX_PATH] = {};

static SIZE_T GetCurrentExecutableSize()
{
	HMODULE current_executable = GetModuleHandleA(nullptr);
	PIMAGE_NT_HEADERS nt_headers = PE::RtlImageNtHeader(
		current_executable
	);

	return P2ALIGNUP(nt_headers->OptionalHeader.SizeOfImage, USN_PAGE_SIZE);
}

static USHORT GetCurrentMachine()
{
	HMODULE current_executable = GetModuleHandleA(nullptr);
	PIMAGE_NT_HEADERS nt_headers = PE::RtlImageNtHeader(
		current_executable
	);

	return nt_headers->FileHeader.Machine;
}

// This is the function that runs as the entrypoint for a given executable.
// Due to this, I impose the restriction of only NTDLL.dll API calls being made from here.
// Any WinAPI function that invokes TLS callbacks will crash the executable.
// 
// Also, exports have to be resolved manually here, as after a system reboot,
// ASLR changes addresses of system DLLs.
static void ArProcessInitialize()
{
	// We cannot use RtlImageNtHeader, so this does the trick.
	auto get_nt_header = [](PVOID ImageBase) -> PIMAGE_NT_HEADERS
		{
			return reinterpret_cast<PIMAGE_NT_HEADERS>(
				static_cast<char*>(ImageBase) + static_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew
			);
		};

	auto find_module_entry = [](const wchar_t* Name) -> PLDR_DATA_TABLE_ENTRY
		{
			// NtCurrentTeb is inlined, so can be called safely. 
			_TEB* teb = NtCurrentTeb();
			PLIST_ENTRY list_entry = &teb->ProcessEnvironmentBlock->Ldr->InLoadOrderModuleList;
			do
			{
				PLDR_DATA_TABLE_ENTRY table_entry = CONTAINING_RECORD(list_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

				// Make sure we have a buffer and that the size is at least what's needed to actually store our string.
				// Also, _wcsnicmp and wcslen should be linked statically (as it's CRT and we enable static linking).
				if (table_entry->BaseDllName.Buffer && table_entry->BaseDllName.MaximumLength >= (wcslen(Name) * sizeof(wchar_t)))
					if (!_wcsnicmp(table_entry->BaseDllName.Buffer, Name, wcslen(Name)))
						return table_entry;

				list_entry = list_entry->Flink;
			} while (&teb->ProcessEnvironmentBlock->Ldr->InLoadOrderModuleList);

			return nullptr;
		};

	auto find_export = [get_nt_header]<typename T>(PVOID Dll, const char* ExportName) -> T*
		{
			PIMAGE_NT_HEADERS nt_headers = get_nt_header(Dll);
			return PE::GetExport<T>(
				Dll,
				nt_headers,
				ExportName
			);
		};

	PVOID game_image_base = NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;

	const auto oep = reinterpret_cast<PFN_OriginalEntrypoint>(
		static_cast<char*>(game_image_base) + g_OldOEP
	);

	if (NtCurrentTeb()->ProcessEnvironmentBlock->BeingDebugged)
	{
		auto dbg_print = find_export.template operator()<decltype(DbgPrintEx)>(
			find_module_entry(L"ntdll.dll")->DllBase,
			"DbgPrintEx"
		);

		auto dbg_breakpoint = find_export.template operator()<decltype(DbgBreakPoint)>(
			find_module_entry(L"ntdll.dll")->DllBase,
			"DbgBreakPoint"
		);

		dbg_print(0, 0, "[AurieInit] Process Image loaded at %p\n", game_image_base);
		dbg_print(0, 0, "[AurieInit] OEP %p\n", oep);
		dbg_print(0, 0, "[AurieInit] DLL to load: %S\n", g_AuriePath);

		dbg_breakpoint();
	}

	// This is not in a DLL, so can be safely called.
	UNICODE_STRING path = {};
	RtlInitUnicodeString(&path, g_AuriePath);

	PVOID handle = 0;
	auto ldr_load_dll = find_export.template operator()<decltype(LdrLoadDll)>(
		find_module_entry(L"ntdll.dll")->DllBase,
		"LdrLoadDll"
	);

	NTSTATUS last_status = ldr_load_dll(
		nullptr,
		nullptr,
		&path,
		&handle
	);

	if (!NT_SUCCESS(last_status))
		return oep();

	auto nt_suspend_thread = find_export.template operator()<decltype(NtSuspendThread) > (
		find_module_entry(L"ntdll.dll")->DllBase,
		"NtSuspendThread"
	);

	nt_suspend_thread(NtCurrentThread(), nullptr);

	return oep();
}

int wmain(int argc, wchar_t** argv)
{
	// If not enough arguments provided, print the usage and exit.
	if (argc < 4)
	{
		printf("Usage: %S executable_path native_dll_path [install|remove]\n", argv[0]);
		return ERROR_BAD_ARGUMENTS;
	}

	const PVOID my_executable_base = GetModuleHandleA(nullptr);
	const SIZE_T my_executable_size = GetCurrentExecutableSize();

	printf("Using executable: \"%S\"\n", argv[1]);
	printf("Using native DLL path: \"%S\"\n", argv[2]);

	PVOID file_base = nullptr;
	size_t file_size = 0;

	// Read the file into memory.
	// We reserve payload_size bytes, all while respecting page alignment.
	DWORD last_error = PE::MapFileToMemory(
		argv[1],
		my_executable_size,
		file_base,
		file_size
	);

	// If we failed doing that, no point continuing.
	if (last_error)
	{
		printf("Error occured while mapping the file.\n");

		return last_error;
	}

	// Print some info about where the file is loaded.
	printf("File mapped to 0x%p => size 0x%llX\n", file_base, file_size);

	// Get NT headers from the file.
	// If this fails, the image is invalid.
	PIMAGE_NT_HEADERS nt_headers = PE::RtlImageNtHeader(file_base);
	if (!nt_headers)
	{
		printf("Image is not a valid PE file.\n");
		return ERROR_FILE_CORRUPT;
	}

	// Make sure that the game is x64.
	const bool is_arch_supported = nt_headers->FileHeader.Machine == GetCurrentMachine();
	
	// If not installed, add the section.
	if (!is_arch_supported)
	{
		printf("Image architecture is not supported.\n");
		return ERROR_FILE_CORRUPT;
	}

	// If we're installing, prepare the section and write our executable there.
	if (!_wcsicmp(argv[3], L"install"))
	{
		// Try to find the Aurie section inside the executable.
		// This returns nullptr if none exists, signalling that Aurie isn't yet installed.
		PIMAGE_SECTION_HEADER new_section = PE::GetSectionHeaderByName(nt_headers, AURIE_SECTION_NAME);
		const bool new_install = (new_section == nullptr);

		// If not installed, add the section.
		if (new_install)
		{
			new_section = PE::AddRwxSection(
				file_base,
				AURIE_SECTION_NAME,
				my_executable_size
			);

			// Add to the file size only if file is extended
			file_size += my_executable_size;
		}

		// Print the address
		printf("%s section at %p\n", AURIE_SECTION_NAME, new_section);

		if (new_section->SizeOfRawData < my_executable_size)
		{
			printf("%s section size is inconsistent. Reinstall the game.\n", AURIE_SECTION_NAME);
			return ERROR_BUFFER_OVERFLOW;
		}

		// If we're updating an already existing install, we need to revert the AddressOfEntrypoint shit.
		if (!new_install)
		{
			char* aurie_image = static_cast<char*>(file_base) + new_section->PointerToRawData;
			auto stored_oep = PE::GetExport<decltype(g_OldOEP)>(aurie_image, PE::RtlImageNtHeader(aurie_image), "g_OldOEP");

			printf("g_OldOEP stored at %p, contains value 0x%X.\n", stored_oep, *stored_oep);

			// Set the patched executable's PE headers OEP to the old one, so that the code below
			// can work as if it was run on a new install.
			nt_headers->OptionalHeader.AddressOfEntryPoint = *stored_oep;
		}

		// Write g_OldOEP, which will be copied to the executable now.
		// Also write g_AuriePath.
		g_OldOEP = nt_headers->OptionalHeader.AddressOfEntryPoint;
		wcscpy_s(g_AuriePath, argv[2]);

		// Null the section data
		memset(
			static_cast<char*>(file_base) + new_section->PointerToRawData,
			0,
			new_section->SizeOfRawData
		);

		// Write new section data
		memcpy(
			static_cast<char*>(file_base) + new_section->PointerToRawData,
			GetModuleHandleA(nullptr),
			my_executable_size
		);

		const uint64_t offset_to_function =
			reinterpret_cast<uint64_t>(ArProcessInitialize) - reinterpret_cast<uint64_t>(my_executable_base);

		nt_headers->OptionalHeader.AddressOfEntryPoint = static_cast<DWORD>(new_section->VirtualAddress + offset_to_function);
	}

	else if (!_wcsicmp(argv[3], L"remove"))
	{
		PIMAGE_SECTION_HEADER aurie_section = PE::GetSectionHeaderByName(nt_headers, AURIE_SECTION_NAME);

		if (!aurie_section)
		{
			printf("The target executable is either not patched or corrupt.\n");
			return ERROR_FILE_CORRUPT;
		}

		char* aurie_image = static_cast<char*>(file_base) + aurie_section->PointerToRawData;
		auto stored_oep = PE::GetExport<decltype(g_OldOEP)>(aurie_image, PE::RtlImageNtHeader(aurie_image), "g_OldOEP");
		printf("g_OldOEP stored at %p, contains value 0x%X.\n", stored_oep, *stored_oep);

		// Set the patched executable's PE headers OEP to the old one.
		nt_headers->OptionalHeader.AddressOfEntryPoint = *stored_oep;
		file_size -= aurie_section->SizeOfRawData;

		PE::RemoveLastSection(file_base);
	}
	else
	{
		printf("Unknown operation.\n");
		return ERROR_UNKNOWN_FEATURE;
	}

	std::ofstream out_file(argv[1], std::ios::binary);
	out_file.write(static_cast<const char*>(file_base), file_size);
	out_file.close();

	printf("Done!\n");
	
	return ERROR_SUCCESS;
}