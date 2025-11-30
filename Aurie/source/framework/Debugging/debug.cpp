#include "debug.hpp"
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>

// Bro I swear wtf are they smoking at Microsoft
// and more importantly, is there anything left for me?
// https://learn.microsoft.com/en-us/answers/questions/1184393/when-including-comctl32-lib-causes-the-ordinal-345
#include <CommCtrl.h>
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

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
			std::stringstream stream(Print);
			std::string split_string;

			while (std::getline(stream, split_string, '\n')) 
			{
				// Remove extraneous newlines
				std::erase(split_string, '\r');
				std::erase(split_string, '\n');

				// Remove nullbytes since spdlog likes to stuff those in files, which confuses text editors.
				std::erase(split_string, '\0');

				// Don't print empty strings.
				if (split_string.empty())
					continue;

				spdlog::log(
					static_cast<spdlog::level::level_enum>(LogSeverity),
					split_string
				);
			}
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
				int button_pressed_index = 0;
				TASKDIALOGCONFIG config = { 0 };
				const TASKDIALOG_BUTTON buttons[] = {
					{ IDYES, L"Close application" },
					{ IDNO, L"Close log window" }
				};
				config.cbSize = sizeof(config);
				config.pszWindowTitle = L"Aurie Framework";
				config.hInstance = NULL;
				config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
				config.pszMainIcon = TD_INFORMATION_ICON;
				config.pszMainInstruction = L"Framework console closed";
				config.pszContent =
					L"You've attempted to close the Aurie console window.\n"
					L"It isn't possible to re-open it without restarting the game.\n"
					L"Please pick your desired action or cancel the closure.";
				config.pButtons = buttons;
				config.cButtons = ARRAYSIZE(buttons);

				TaskDialogIndirect(&config, &button_pressed_index, NULL, NULL);
				switch (button_pressed_index)
				{
				case IDYES:
					// Close application button
					DbgPrintEx(LOG_SEVERITY_INFO, "Process is closing due to CTRL+C event.");
					TerminateProcess(GetCurrentProcess(), 0);
					return TRUE;
				case IDNO:
					// Close log window button
					DbgPrintEx(LOG_SEVERITY_INFO, "Log window is closing due to CTRL+C event.");
					DbgpDestroyConsole();
					return TRUE;
				default:
					// Maybe cancel? idfk
					return TRUE;
				}
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
