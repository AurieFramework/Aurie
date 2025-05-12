#ifndef AURIE_DEBUG_H_
#define AURIE_DEBUG_H_

#include "../framework.hpp"
#include <spdlog/spdlog.h>

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
		void DbgpPrintStringInternal(
			IN AurieLogSeverity LogSeverity,
			IN const std::string& Print
		);

		void DbgpInitLogger();

		AurieStatus DbgpFormatVaArgs(
			IN const char* Format,
			IN va_list Arguments,
			OUT std::string& Buffer
		);

		BOOL WINAPI DbgpConsoleEventHandler(
			IN DWORD ControlType
		);

		EXPORTED HWND DbgpCreateConsole(
			IN const char* Name
		);

		EXPORTED void DbgpDestroyConsole();
	}
}

#endif // AURIE_DEBUG_H_