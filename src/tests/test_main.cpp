/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file test_main.cpp Entry point for all the unit tests.
 * With MSVC a failed assertion prints the call stack and ends the run without a modal dialog, so an unattended
 * focused burst never waits for a click.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Global Mandatory Reading (bounded test runs)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, actual window failure diagnosis
 */

#include "../stdafx.h"

#define CATCH_CONFIG_MAIN
#define DO_NOT_USE_WMAIN
#include "../3rdparty/catch2/catch.hpp"

#ifdef _MSC_VER
#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#pragma comment(lib, "dbghelp.lib")

/**
 * Print the call stack to stderr when a failed assertion aborts the run; the abort then ends the process.
 * @param signal The abort signal.
 */
static void PrintAbortStack(int)
{
	void *frames[48];
	const USHORT count = CaptureStackBackTrace(0, 48, frames, nullptr);
	HANDLE process = GetCurrentProcess();
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
	SymInitialize(process, nullptr, TRUE);
	alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
	SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(buffer);
	// Each frame shows its function and, where the debug symbols have it, its source line.
	for (USHORT i = 0; i < count; ++i) {
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 255;
		const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
		IMAGEHLP_LINE64 line{};
		line.SizeOfStruct = sizeof(line);
		DWORD displacement = 0;
		const bool named = SymFromAddr(process, address, nullptr, symbol) != FALSE;
		const bool lined = SymGetLineFromAddr64(process, address, &displacement, &line) != FALSE;
		fprintf(stderr, "  #%u %s %s:%lu\n", i, named ? symbol->Name : "?", lined ? line.FileName : "?", lined ? line.LineNumber : 0UL);
	}
	fflush(stderr);
}

/**
 * Print an actual access-violation stack before Catch's fatal-condition handler unwinds it.
 * @param exception Windows exception context; ordinary C++ exceptions remain with their existing handlers.
 * @return Continue normal exception handling after recording the fault; never turn a crash into a passing test.
 */
static LONG WINAPI PrintAccessViolationStack(EXCEPTION_POINTERS *exception)
{
	if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) PrintAbortStack(0);
	return EXCEPTION_CONTINUE_SEARCH;
}

/**
 * Send failed assertions and aborts to stderr instead of the Abort/Retry/Ignore box, as the null video driver does
 * for the game, and print the stack on abort.
 * @return Always true; only the settings matter.
 */
static bool QuietTestAssertions()
{
	_set_error_mode(_OUT_TO_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	signal(SIGABRT, PrintAbortStack);
	AddVectoredExceptionHandler(1, PrintAccessViolationStack); // Record the failing native frame before fatal-condition reporting discards its original call stack.
	return true;
}

static const bool _quiet_test_assertions = QuietTestAssertions(); ///< Set before any test runs.
#endif /* _MSC_VER */
