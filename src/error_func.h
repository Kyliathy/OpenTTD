/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file error_func.h Error reporting related functions. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, bounded unattended diagnostics */

#ifndef ERROR_FUNC_H
#define ERROR_FUNC_H

#include "3rdparty/fmt/format.h"

/** @param str Fatal user-facing error text. @attention Terminates the process after reporting the error. */
[[noreturn]] void UserErrorI(const std::string &str);

/**
 * Configure a process-local reporter for unattended native fatal errors.
 * @param handler Receives the actual fatal text after crash-log ownership is updated; null restores normal reporting.
 *        A reporter may terminate the process. If it returns, normal error presentation and abort continue.
 * @note This startup-only presentation hook owns no synchronized gameplay or video state.
 */
void SetFatalErrorReporter(void (*handler)(const std::string &));

/**
 * Error handling for fatal non-user errors.
 * @param str the string to print.
 * @attention Function does not return.
 */
[[noreturn]] void FatalErrorI(const std::string &str);

#define UserError(format_string, ...) UserErrorI(fmt::format(FMT_STRING(format_string) __VA_OPT__(,) __VA_ARGS__))
#define FatalError(format_string, ...) FatalErrorI(fmt::format(FMT_STRING(format_string) __VA_OPT__(,) __VA_ARGS__))

#endif /* ERROR_FUNC_H */
