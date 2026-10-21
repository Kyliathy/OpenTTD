/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tube_test_environment.cpp Explicit window/font prerequisites for isolated native tube command checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * Earlier full-test ordering supplied these prerequisites incidentally. This
 * focused executable initializes them without changing completed provider cases.
 */

#include "../stdafx.h"
#define CATCH_CONFIG_EXTERNAL_INTERFACES
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "../safeguards.h"

/** Initialize ordinary station sign/widget metadata before this isolated executable creates native facilities. */
class TubeTestEnvironment final : public Catch::TestEventListenerBase {
public:
	using TestEventListenerBase::TestEventListenerBase;

	/**
	 * Prepare the native UI prerequisites once, independently of test ordering.
	 * @param info Catch's actual run description; initialization does not alter test selection.
	 */
	void testRunStarting([[maybe_unused]] const Catch::TestRunInfo &info) override
	{
		MockEnvironment::Instance();
		LoadGeneratedWindowTestLanguage();
	}
};

CATCH_REGISTER_LISTENER(TubeTestEnvironment)
