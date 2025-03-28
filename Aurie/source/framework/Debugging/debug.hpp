#ifndef AURIE_DEBUG_H_
#define AURIE_DEBUG_H_

#include "../framework.hpp"
#include <spdlog/spdlog.h>
#include <concurrentqueue.h>

namespace Aurie
{
	EXPORTED void vDbgPrint(
		IN const char* Format,
		IN va_list Arguments
	);

	EXPORTED void DbgPrint(
		IN const char* Format,
		IN ...
	);

	EXPORTED void vDbgPrintEx(
		IN AurieLogSeverity Severity,
		IN const char* Format,
		IN va_list Arguments
	);

	EXPORTED void DbgPrintEx(
		IN AurieLogSeverity Severity,
		IN const char* Format,
		IN ...
	);

	namespace Internal
	{
		void DbgpQueueString(
			IN AurieLogSeverity LogSeverity,
			IN const std::string& Print
		);

		void DbgpInitLogger();

		AurieStatus DbgpFormatVaArgs(
			IN const char* Format,
			IN va_list Arguments,
			OUT std::string& Buffer
		);

		EXPORTED HWND DbgpCreateConsole(
			IN const char* Name
		);

		EXPORTED void DbgpDestroyConsole();

		// Console printing blocks if Quick Edit mode is enabled and text is selected.
		// This thread exclusively handles writing output to the console.
		// To ensure proper order, a queue (g_ConsolePrintQueue) is used.
		void DbgpPrintWorkerThread();

		inline bool g_ShouldExitWorkerThread = false;
		inline HANDLE g_PrintWorkerThreadHandle = nullptr;
		inline moodycamel::ConcurrentQueue<AurieLogEntry> g_ConsolePrintQueue;
	}
}

#endif // AURIE_DEBUG_H_