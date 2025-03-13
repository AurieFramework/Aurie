#include "KInjector.hpp"
#include <fstream>

namespace KInjector
{
	bool Inject(
		IN ULONG ProcessId,
		IN PCWSTR DllName
	)
	{
		HANDLE process_handle = OpenProcess(
			PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
			false,
			ProcessId
		);

		if (!process_handle)
		{
			// Opening the process failed.
			return false;
		}

		bool injection_status = Inject(process_handle, DllName);

		CloseHandle(process_handle);

		return injection_status;
	}

	bool Inject(
		IN HANDLE ProcessHandle,
		IN PCWSTR DllName
	)
	{
		wchar_t dll_path[MAX_PATH] = { 0 };
		if (!GetFullPathNameW(
			DllName,
			MAX_PATH,
			dll_path,
			nullptr
		))
		{
			// Getting the path to the DLL failed.
			return false;
		}

		// Query the architecture of both processes
		Architecture current_process_arch = Architecture::Unknown;
		Architecture target_process_arch = Architecture::Unknown;

		if (!Internal::GetProcessArchitecture(
			GetCurrentProcess(),
			current_process_arch
		))
		{
			return false;
		}

		if (!Internal::GetProcessArchitecture(
			ProcessHandle,
			target_process_arch
		))
		{
			return false;
		}

		// If the two architectures match, we know LoadLibraryW resides
		// at the same address in the target process as it does in our process.
		if (current_process_arch == target_process_arch)
		{
			return Internal::InjectInternal(
				ProcessHandle,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryW),
				DllName
			);
		}

		// If the current process isn't x64, we can't inject into an x64 process.
		// This is due to CreateToolhelp32Snapshot returning ERROR_PARTIAL_COPY.
		// Note this only happens if the injector is x86 and the target is x64, not vice versa.
		if (current_process_arch != Architecture::x64)
		{
			return false;
		}

		// Try to get the kernel32.dll base in the target process
		// We can then parse the PE file and get the offset for LoadLibraryW.
		Int64 kernel32_base = reinterpret_cast<int64_t>(Internal::GetModuleBaseAddress(
			ProcessHandle,
			L"kernel32.dll"
		));

		// Locate the path to the module if possible
		std::wstring kernel32_path = Internal::GetModulePath(
			ProcessHandle,
			L"kernel32.dll"
		);

		if (!static_cast<int64_t>(kernel32_base) || kernel32_path.empty())
		{
			// The process doesn't yet have kernel32.dll loaded, as it might have been created in a suspended state.
			// In such case, CreateToolhelp32Snapshot fails with ERROR_PARTIAL_COPY.
			// 
			// However, if we create a thread at the address that kernel32!LoadLibraryW is supposed to be at,
			// the Windows loader will automatically load in kernel32.dll for us and the process won't crash.

			bool found_kernel32 = Internal::ForEachProcess(
				[target_process_arch, &kernel32_base, &kernel32_path](const PROCESSENTRY32W& ProcessEntry) -> bool
				{
					// Open a handle to the process
					ULONG process_id = ProcessEntry.th32ProcessID;
					HANDLE process_handle = OpenProcess(
						PROCESS_QUERY_LIMITED_INFORMATION,
						false,
						process_id
					);

					// Make sure the process opened successfully
					if (!process_handle)
						return false;

					// Query the architecture of the target process
					Architecture process_architecture = Architecture::Unknown;
					if (!Internal::GetProcessArchitecture(
						process_handle,
						process_architecture
					))
					{
						CloseHandle(process_handle);
						return false;
					}

					// If the architecture of the process we're looping through
					// matches the architecture of the process into which we want to inject,
					// we can capture the base address of kernel32.dll, as it will be the same
					// in both processes.
					if (process_architecture == target_process_arch)
					{
						PVOID process_kernel32_base = Internal::GetModuleBaseAddress(
							process_handle,
							L"kernel32.dll"
						);

						std::wstring process_kernel32_path = Internal::GetModulePath(
							process_handle,
							L"kernel32.dll"
						);

						// Make sure we actually got the base address
						// If we did, we can stop enumeration.
						if (process_kernel32_base && !process_kernel32_path.empty())
						{
							kernel32_base = reinterpret_cast<int64_t>(process_kernel32_base);
							kernel32_path = process_kernel32_path;
							CloseHandle(process_handle);

							return true;
						}
					}

					CloseHandle(process_handle);
					return false;
				}
			);

			// If we looped through all the processes and didn't find kernel32.dll in any of them,
			// we can't inject. There might be other workarounds, you're free to add them yourself
			// or PR this repo so others can benefit from your findings.
			if (!found_kernel32)
				return false;
		}

		// By the time we get here, we know kernel32.dll's base address in the target process.
		// We now have to parse the PE file on disk, and figure out the export offset.
		uintptr_t function_offset = Internal::PE::GetExportOffset(
			kernel32_path.c_str(),
			"LoadLibraryW"
		);

		// We failed to find LoadLibraryW in kernel32.dll?
		// This shouldn't happen, and if it does, it might be worth to check
		// if the path gets resolved correctly. 
		// 
		// For x64: %SystemRoot%\\System32\\kernel32.dll
		// For x86: %SystemRoot%\\SysWoW64\\kernel32.dll
		if (!function_offset)
		{
			return false;
		}

		// We now know the base address of kernel32.dll in the target process,
		// and the offset to LoadLibraryW. We can inject!
		return Internal::InjectInternal(
			ProcessHandle,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(reinterpret_cast<char*>(static_cast<int64_t>(kernel32_base)) + function_offset),
			dll_path
		);
	}

	namespace Internal
	{
		bool GetProcessArchitecture(
			IN HANDLE ProcessHandle,
			OUT Architecture& ProcessArchitecture
		)
		{
			BOOL is_wow64_process = false;

			if (!IsWow64Process(
				ProcessHandle,
				&is_wow64_process
			))
			{
				// Process might be terminating?
				// I don't actually see how this might fail.
				ProcessArchitecture = Architecture::Unknown;
				return false;
			}

			ProcessArchitecture = is_wow64_process ? Architecture::x86 : Architecture::x64;
			return true;
		}

		bool ForEachProcess(
			IN std::function<bool(const PROCESSENTRY32W&)> Callback
		)
		{
			HANDLE process_snapshot = CreateToolhelp32Snapshot(
				TH32CS_SNAPPROCESS,
				0
			);

			if (process_snapshot == INVALID_HANDLE_VALUE)
				return false;

			PROCESSENTRY32W process_entry = {}; process_entry.dwSize = sizeof(process_entry);

			Process32FirstW(process_snapshot, &process_entry);
			do
			{
				if (Callback(process_entry))
				{
					CloseHandle(process_snapshot);
					return true;
				}

			} while (Process32NextW(process_snapshot, &process_entry));

			CloseHandle(process_snapshot);
			return false;
		}

		bool ForEachModule(
			IN HANDLE ProcessHandle,
			IN std::function<bool(const MODULEENTRY32W&)> Callback
		)
		{
			HANDLE module_snapshot = CreateToolhelp32Snapshot(
				TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
				GetProcessId(ProcessHandle)
			);

			if (module_snapshot == INVALID_HANDLE_VALUE)
				return false;

			MODULEENTRY32W module_entry = {}; module_entry.dwSize = sizeof(module_entry);

			Module32FirstW(module_snapshot, &module_entry);
			do
			{
				if (Callback(module_entry))
				{
					CloseHandle(module_snapshot);
					return true;
				}

			} while (Module32NextW(module_snapshot, &module_entry));

			CloseHandle(module_snapshot);
			return false;
		}

		std::wstring GetModulePath(
			IN HANDLE ProcessHandle,
			IN PCWSTR ModuleName
		)
		{
			std::wstring module_path = L"";

			ForEachModule(
				ProcessHandle,
				[ModuleName, &module_path](const MODULEENTRY32W& ModuleEntry) -> bool
				{
					if (!_wcsicmp(ModuleName, ModuleEntry.szModule))
					{
						module_path = ModuleEntry.szExePath;
						return true;
					}

					return false;
				}
			);

			return module_path;
		}

		PVOID GetModuleBaseAddress(
			IN HANDLE ProcessHandle,
			IN PCWSTR ModuleName
		)
		{
			PVOID module_base = nullptr;

			ForEachModule(
				ProcessHandle,
				[ModuleName, &module_base](const MODULEENTRY32W& ModuleEntry) -> bool
				{
					if (!_wcsicmp(ModuleName, ModuleEntry.szModule))
					{
						module_base = ModuleEntry.modBaseAddr;
						return true;
					}

					return false;
				}
			);

			return module_base;
		}

		bool InjectInternal(
			IN HANDLE ProcessHandle,
			IN LPTHREAD_START_ROUTINE StartRoutine,
			IN PCWSTR DllPath
		)
		{
			Int64 dll_path_size = wcslen(DllPath) * sizeof(wchar_t);

			// Allocate memory in the target process
			LPVOID path_buffer = VirtualAllocEx(
				ProcessHandle,
				nullptr,
				MAX_PATH * sizeof(wchar_t),
				MEM_COMMIT | MEM_RESERVE,
				PAGE_READWRITE
			);

			if (!path_buffer)
			{
				// Allocation failed, the process might be on a quota?
				return false;
			}

			// Write the path to our DLL into the newly allocated memory.
			// This memory will serve as the argument to LoadLibraryW.
			if (!WriteProcessMemory(
				ProcessHandle,
				path_buffer,
				DllPath,
				static_cast<SIZE_T>(dll_path_size),
				nullptr
			))
			{
				// If we failed, just free the memory and bail.
				VirtualFreeEx(
					ProcessHandle,
					path_buffer,
					0,
					MEM_RELEASE
				);

				return false;
			}

			// Create the actual thread
			HANDLE remote_thread = CreateRemoteThread(
				ProcessHandle,
				nullptr,
				0,
				StartRoutine,
				path_buffer,
				0,
				nullptr
			);

			if (!remote_thread)
			{
				// Creating the remote thread failed.
				VirtualFreeEx(
					ProcessHandle,
					path_buffer,
					0,
					MEM_RELEASE
				);

				return false;
			}

			WaitForSingleObject(
				remote_thread,
				INFINITE
			);

			CloseHandle(remote_thread);

			VirtualFreeEx(
				ProcessHandle,
				path_buffer,
				0,
				MEM_RELEASE
			);

			return true;
		}

		namespace PE
		{
			uintptr_t GetExportOffset(IN PCWSTR ImagePath, IN PCSTR FunctionName)
			{
				// Try to open our file
				std::ifstream input_file(ImagePath, std::ios::binary | std::ios::ate);
				if (!input_file.is_open() || !input_file.good())
				{
					return 0;
				}

				// Get the size of the open file (we opened ATE so we can use tellg to get the current position => filesize)
				std::streampos input_file_size = input_file.tellg();

				// I think we need more than this to fit stuff in?
				if (input_file_size <= 1024)
				{
					return 0;
				}

				char* file_in_memory = new char[static_cast<size_t>(input_file_size)];

				if (!file_in_memory)
				{
					return 0;
				}

				// Seek to the beginning of the file (offset 0 from the beginning)
				input_file.seekg(0, std::ios::beg);

				// Read the whole file into memory, and close the stream
				input_file.read(file_in_memory, input_file_size);
				input_file.close();

				// Check if the DOS header is valid and follow the NT header pointer.
				PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(file_in_memory);
				if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
				{
					delete[] file_in_memory;
					return 0;
				}

				// We don't know if the OptionalHeader is x86 or x64 yet, unsafe to access it just yet!!!
				// Also verify the NT signature is fine.
				PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(file_in_memory + dos_header->e_lfanew);
				if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
				{
					delete[] file_in_memory;
					return 0;
				}

				PIMAGE_EXPORT_DIRECTORY export_directory = nullptr;

				// I386
				if (nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
				{
					// This NT_HEADERS object has the correct bitness, it is now safe to access the optional header
					PIMAGE_NT_HEADERS32 nt_headers_x86 = reinterpret_cast<PIMAGE_NT_HEADERS32>(nt_headers);

					// Handle object files
					if (!nt_headers_x86->FileHeader.SizeOfOptionalHeader)
					{
						delete[] file_in_memory;
						return 0;
					}

					// In case our file doesn't have an export header
					if (nt_headers_x86->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
					{
						delete[] file_in_memory;
						return 0;
					}

					// Get the RVA - we can't just add this to file_in_memory because of section alignment and stuff...
					// We could if we had the file already LLA'd into memory, but that's impossible, since  the file's a different arch.
					DWORD export_dir_address = nt_headers_x86->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
					if (!export_dir_address)
					{
						delete[] file_in_memory;
						return 0;
					}

					// Get the export directory from the VA 
					export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
						file_in_memory + RvaToFileOffset(
							nt_headers,
							export_dir_address
						)
						);
				}
				else if (nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
				{
					PIMAGE_NT_HEADERS64 nt_headers_x64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(nt_headers);

					// Handle object files
					if (!nt_headers_x64->FileHeader.SizeOfOptionalHeader)
					{
						delete[] file_in_memory;
						return 0;
					}

					// In case our file doesn't have an export header
					if (nt_headers_x64->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
					{
						delete[] file_in_memory;
						return 0;
					}

					// Get the RVA - we can't just add this to file_in_memory because of section alignment and stuff...
					// We could if we had the file already LLA'd into memory, but that's impossible, since  the file's a different arch.
					DWORD export_dir_address = nt_headers_x64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
					if (!export_dir_address)
					{
						delete[] file_in_memory;
						return 0;
					}

					// Get the export directory from the VA 
					export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
						file_in_memory + RvaToFileOffset(
							nt_headers,
							export_dir_address
						)
						);
				}
				else
				{
					// Intel Itanium not supported, sorry...
					delete[] file_in_memory;
					return 0;
				}

				if (!export_directory)
				{
					delete[] file_in_memory;
					return 0;
				}

				// Get all our required arrays
				DWORD* function_names = reinterpret_cast<DWORD*>(
					file_in_memory +
					RvaToFileOffset(
						nt_headers,
						export_directory->AddressOfNames
					)
					);

				WORD* function_name_ordinals = reinterpret_cast<WORD*>(
					file_in_memory +
					RvaToFileOffset(
						nt_headers,
						export_directory->AddressOfNameOrdinals
					)
					);

				DWORD* function_addresses = reinterpret_cast<DWORD*>(
					file_in_memory +
					RvaToFileOffset(
						nt_headers,
						export_directory->AddressOfFunctions
					)
					);

				// Loop over all the named exports
				for (DWORD n = 0; n < export_directory->NumberOfNames; n++)
				{
					// Get the name of the export
					const char* export_name = file_in_memory + RvaToFileOffset(nt_headers, function_names[n]);

					// Get the function ordinal for array access
					unsigned short function_ordinal = function_name_ordinals[n];

					// Get the function offset
					uintptr_t function_offset = function_addresses[function_ordinal];

					// If it's our target export
					if (!_stricmp(FunctionName, export_name))
					{
						delete[] file_in_memory;
						return function_offset;
					}
				}

				delete[] file_in_memory;
				return 0;
			}

			DWORD RvaToFileOffset(
				IN PIMAGE_NT_HEADERS Headers,
				IN DWORD Rva
			)
			{
				PIMAGE_SECTION_HEADER pHeaders = IMAGE_FIRST_SECTION(Headers);

				// Loop over all the sections of the file
				for (WORD n = 0; n < Headers->FileHeader.NumberOfSections; n++)
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
		}
	}
}