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
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 15 (the run-start load uses no Catch assertion, so the JUnit reporter completes)
 */

#include "../stdafx.h"
#define CATCH_CONFIG_EXTERNAL_INTERFACES
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include <stdexcept>
#include "../safeguards.h"

/**
 * Load the generated English pack without Catch assertions. No test case runs yet when the listener starts, and the
 * JUnit reporter cannot record an assertion outside a test section: it dereferenced an empty section stack (SIGSEGV).
 * @return Whether the pack was read, validated and made current; the static metadata outlives the run, as the language
 *         owner keeps a pointer to it.
 */
static bool LoadRunStartLanguage()
{
	static LanguageMetadata metadata;
	metadata.file = std::filesystem::current_path() / "lang" / "english.lng";
	auto file = FileHandle::Open(metadata.file.string(), "rb");
	// Each step needs the previous one: an open file, a whole header, a valid pack, then an accepted current language.
	return file.has_value() && fread(static_cast<LanguagePackHeader *>(&metadata), sizeof(LanguagePackHeader), 1, *file) == 1 &&
		metadata.IsValid() && ReadLanguagePack(&metadata);
}

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
		// Catch's session reports an exception here and ends the run with an error code instead of crashing a reporter.
		if (!LoadRunStartLanguage()) throw std::runtime_error("generated lang/english.lng is missing or invalid; run from the build directory");
	}
};

CATCH_REGISTER_LISTENER(TubeTestEnvironment)
