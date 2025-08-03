#include "debug.hpp"
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>

namespace Aurie
{
	void vDbgPrint(
		IN const char* Format, 
		IN va_list Arguments
	)
	{
		return vDbgPrintEx(
			LOG_SEVERITY_INFO,
			Format,
			Arguments
		);
	}

	void DbgPrint(
		IN const char* Format,
		IN ...
	)
	{
		va_list list;
		va_start(list, Format);

		vDbgPrint(
			Format,
			list
		);

		va_end(list);
	}

	void vDbgPrintEx(
		IN AurieLogSeverity Severity, 
		IN const char* Format, 
		IN va_list Arguments
	)
	{
		std::string buffer;
		Internal::DbgpFormatVaArgs(
			Format,
			Arguments,
			buffer
		);

		Internal::DbgpPrintStringInternal(
			Severity,
			buffer
		);
	}

	void DbgPrintEx(
		IN AurieLogSeverity Severity,
		IN const char* Format,
		IN ...
	)
	{
		va_list list;
		va_start(list, Format);

		vDbgPrintEx(
			Severity,
			Format,
			list
		);

		va_end(list);
	}

	namespace Internal
	{
		void DbgpPrintStringInternal(
			IN AurieLogSeverity LogSeverity, 
			IN const std::string& Print
		)
		{
			spdlog::log(
				static_cast<spdlog::level::level_enum>(LogSeverity),
				Print
			);
		}

		void DbgpInitLogger()
		{
			std::shared_ptr<spdlog::logger> default_logger = spdlog::default_logger();
			spdlog::sink_ptr file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("aurie.log", true);

			spdlog::set_level(spdlog::level::trace);

			default_logger->sinks().front()->set_level(spdlog::level::debug);
			file_sink->set_level(spdlog::level::trace);

			default_logger->sinks().push_back(file_sink);
			default_logger->set_pattern("%^[%T] [%l] %v%$");
			spdlog::flush_on(spdlog::level::trace);
		}

		AurieStatus DbgpFormatVaArgs(
			IN const char* Format, 
			IN va_list Arguments,
			OUT std::string& Buffer
		)
		{
			// Get required length
			size_t length_needed = 0;
			{
				va_list arg_copy;
				va_copy(arg_copy, Arguments);
				length_needed = vsnprintf(nullptr, 0, Format, arg_copy);
				va_end(arg_copy);
			}

			Buffer.resize(length_needed + 1);
			vsnprintf(Buffer.data(), length_needed + 1, Format, Arguments);
			return AURIE_SUCCESS;
		}

		BOOL WINAPI DbgpConsoleEventHandler(
			IN DWORD ControlType
		)
		{
			if (ControlType == CTRL_C_EVENT)
			{
				int result = MessageBoxA(
					0,
					"You're about to close the Aurie Framework console window.\n\n"
					"To fully exit the application, click 'Yes'.\n"
					"To close the log window without unloading Aurie, click 'No'.\n"
					"If you changed your mind, press Cancel or the X button.",
					"Aurie Framework",
					MB_ICONQUESTION | MB_SETFOREGROUND | MB_TOPMOST | MB_YESNOCANCEL
				);

				switch (result)
				{
				case IDYES:
					DbgPrintEx(LOG_SEVERITY_INFO, "Process is closing due to CTRL+C event.");
					TerminateProcess(GetCurrentProcess(), 0);
					break;
				case IDNO:
					DbgPrintEx(LOG_SEVERITY_INFO, "Log window is closing due to CTRL+C event.");
					DbgpDestroyConsole();
					break;
				default:
					break;
				}

				return TRUE;
			}

			return FALSE;
		}

		HWND DbgpCreateConsole(
			IN const char* Name
		)
		{
			AllocConsole();
			SetConsoleTitleA(Name);

			FILE* dummy_file;
			freopen_s(&dummy_file, "CONIN$", "r", stdin);
			freopen_s(&dummy_file, "CONOUT$", "w", stderr);
			freopen_s(&dummy_file, "CONOUT$", "w", stdout);

			SetConsoleCtrlHandler(
				DbgpConsoleEventHandler,
				TRUE
			);

			// Prevent Close (X) and Maximize buttons from being used.
			// This only affects the console window, not the main game.
			DeleteMenu(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_BYCOMMAND);
			DeleteMenu(GetSystemMenu(GetConsoleWindow(), false), SC_MAXIMIZE, MF_BYCOMMAND);

			return GetConsoleWindow();
		}

		void DbgpDestroyConsole()
		{
			SetConsoleCtrlHandler(
				DbgpConsoleEventHandler,
				FALSE
			);

			HWND console_window = GetConsoleWindow();
			FreeConsole();
			PostMessageW(console_window, WM_CLOSE, 0, 0);
		}
	}
}
