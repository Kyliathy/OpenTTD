/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file artifact_paths.cpp Focused native checks for development artifact ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 3.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include <fstream>
#include <stdexcept>

#include "../safeguards.h"

/** Feature partitioning must preserve each writer's bytes within one invocation. */
TEST_CASE("Artifact paths separate features without changing native process identity", "[artifact-paths]")
{
	const auto tbm = GetTestArtifactPaths("tbm", "routing-check");
	const auto geology = GetTestArtifactPaths("geology", "routing-check");
	CHECK(tbm.run_id == geology.run_id);
	CHECK(tbm.runtime_dir == geology.runtime_dir);
	CHECK(tbm.images != geology.images);
	const auto first = tbm.Output("scratch", "identity.bin");
	const auto second = geology.Output("scratch", "identity.bin");
	std::ofstream(first) << "tbm";
	std::ofstream(second) << "geology";
	std::ifstream input(first);
	std::string value;
	input >> value;
	// Windows requires the owned reader to release its handle before this test
	// can remove its scratch file; retained fixture inputs are never opened here.
	input.close();
	CHECK(value == "tbm");
	CHECK_THROWS_AS(tbm.Output("images", "../input.sav"), std::runtime_error);
	CHECK_THROWS_AS(tbm.Output("fixtures", "input.sav"), std::runtime_error);
	// Only this test's disposable scratch files are removed; its input owners
	// and other feature outputs remain outside the cleanup operation.
	std::filesystem::remove(first);
	std::filesystem::remove(second);
}

/** Native and Python routing must reject the same traversal/identifier shapes. */
TEST_CASE("Artifact path ownership rejects traversal and invalid identifiers", "[artifact-paths]")
{
	const auto root = FindTestRepositoryRoot();
	CHECK(ValidateTestArtifactIdentifier("tbm-exit_2") == "tbm-exit_2");
	CHECK_THROWS_AS(ValidateTestArtifactIdentifier("../fixtures"), std::runtime_error);
	CHECK_THROWS_AS(ValidateTestArtifactIdentifier("_hidden"), std::runtime_error);
	CHECK_THROWS_AS(RequireTestArtifactWithin(root / "src/tests/fixtures", root / "mlm/reviews"), std::runtime_error);
	CHECK_THROWS_AS(RequireTestArtifactWithin(root, root), std::runtime_error);
	CHECK_THROWS_AS(ResolveTestInputPath(root / "CMakeLists.txt"), std::runtime_error);
}
