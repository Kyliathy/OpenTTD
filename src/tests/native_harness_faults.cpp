/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */
/** @file native_harness_faults.cpp Explicit hidden arithmetic-fault child for native harness diagnostics.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group12D.
 * This case is selected only by the isolated failing-child observer; ordinary test selection excludes it.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../safeguards.h"

#ifdef _MSC_VER
/** Raise a real native integer fault with its original frame retained; no game/world state is initialized. */
__declspec(noinline) static void TriggerHarnessArithmeticFault()
{
	volatile int numerator = 1;
	volatile int denominator = 0;
	volatile int quotient = numerator / denominator;
	(void)quotient;
}
#endif

/** Intentionally fail only this explicitly selected child, proving diagnostics retain the fault and failing exit. */
TEST_CASE("Explicit native harness arithmetic fault retains original stack", "[.native-harness-fault]")
{
#ifdef _MSC_VER
	TriggerHarnessArithmeticFault();
#else
	SUCCEED("The selected native diagnostic contract is Windows/MSVC only.");
#endif
}
