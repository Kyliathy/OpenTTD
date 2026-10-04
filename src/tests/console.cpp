/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file console.cpp Tests for empty console input. @see docs/zz-reach2/upgrades/2026-10/r2rvm2-review-map-2.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_func.h"

#include "../safeguards.h"

//Blank input must not abort a dedicated server before its pending savegame load.
TEST_CASE("Console - empty commands are ignored")
{
	CHECK_NOTHROW(IConsoleCmdExec(std::string_view{}));
	const std::string_view comment = "#ignored";
	CHECK_NOTHROW(IConsoleCmdExec(comment.substr(comment.size())));
	CHECK_NOTHROW(IConsoleCmdExec(comment));
}
