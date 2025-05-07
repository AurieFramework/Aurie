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

		Internal::DbgpQueueString(
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
		void DbgpQueueString(
			IN AurieLogSeverity LogSeverity, 
			IN const std::string& Print
		)
		{
			AurieLogEntry entry = {
				.Creator = 0,
				.StringToPrint = Print,
				.Severity = LogSeverity,
			};

			g_ConsolePrintQueue.enqueue(entry);
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
			spdlog::flush_every(std::chrono::seconds(1));
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

			return GetConsoleWindow();
		}

		void DbgpDestroyConsole()
		{
			HWND console_window = GetConsoleWindow();
			FreeConsole();
			PostMessageW(console_window, WM_CLOSE, 0, 0);
		}

		void DbgpPrintWorkerThread()
		{
			while (!g_ShouldExitWorkerThread)
			{
				if (g_ConsolePrintQueue.size_approx() == 0)
				{
					Sleep(1);
					continue;
				}

				// Empty the print queue
				AurieLogEntry log_entry;
				if (g_ConsolePrintQueue.try_dequeue(log_entry))
				{
					const std::string& printable = log_entry.StringToPrint;
					spdlog::log(
						static_cast<spdlog::level::level_enum>(log_entry.Severity),
						printable
					);
				}
			}
		}
	}
}
