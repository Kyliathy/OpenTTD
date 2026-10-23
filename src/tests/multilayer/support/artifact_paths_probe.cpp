/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file artifact_paths_probe.cpp Small executable linking the actual native path owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups10/18B.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group25 runtime parity.
 * Exercises routing only; it links no game/world implementation and claims no pixels.
 */

#include "../../test_artifact_paths.h"
#include "../../../3rdparty/nlohmann/json.hpp"
#include <iostream>

/**
 * Describe one actual native feature view using portable path spellings.
 * @param paths Actual shared-owner result for a reserved feature/scenario.
 * @return JSON identities compared independently with the Python owner.
 */
static nlohmann::json DescribePaths(const TestArtifactPaths &paths)
{
	return {{"root", paths.root.generic_string()}, {"run_id", paths.run_id},
		{"images", paths.images.generic_string()}, {"saves", paths.saves.generic_string()},
		{"data", paths.data.generic_string()}, {"reports", paths.reports.generic_string()},
		{"evidence", paths.evidence.generic_string()}, {"scratch", paths.scratch.generic_string()},
		{"logs", paths.logs.generic_string()}, {"runtime_dir", paths.runtime_dir.generic_string()},
		{"config_path", paths.config_path.generic_string()}};
}

/**
 * Invoke explicit actual routing/input/provenance boundaries in a small process.
 * @param argc Argument count including the executable name.
 * @param argv Mode: paths, runtime [template], input <path>, expected <family>, output <role> <basename>, record <path>.
 * @return Zero for an accepted boundary, two for malformed invocation, one on refusal.
 */
int main(int argc, char **argv)
{
	if (argc < 2) return 2;
	try {
		const std::string mode = argv[1];
		// Runtime parity exercises the actual native preparation owner, using
		// only the caller's declared fixture/process home and no game execution.
		if (mode == "runtime" && (argc == 2 || argc == 3)) {
			std::cout << nlohmann::json{{"path", GetTestRuntimeConfig(argc == 3 ? argv[2] : "native-art.cfg").generic_string()}}.dump() << '\n';
			return 0;
		}
		// Input resolution is read-only and must not reserve a runtime invocation.
		if (mode == "input" && argc == 3) {
			std::cout << nlohmann::json{{"path", ResolveTestInputPath(argv[2]).generic_string()}}.dump() << '\n';
			return 0;
		}
		// The actual shared source-input boundary is tested without linking any
		// game/decoder/world owner or claiming application loading acceptance.
		if (mode == "expected" && argc == 3) {
			std::cout << nlohmann::json{{"path", GetTestExpectedSprites(argv[2]).generic_string()}}.dump() << '\n';
			return 0;
		}
		const auto paths = GetTestArtifactPaths("tbm", "routing-check");
		if (mode == "paths") {
			std::cout << nlohmann::json{{"tbm", DescribePaths(paths)},
				{"geology", DescribePaths(GetTestArtifactPaths("geology", "routing-check"))}}.dump() << '\n';
			return 0;
		}
		// Explicit argument arity keeps missing values from being interpreted as
		// filenames or another mode, matching the wrapper's structured arguments.
		if (mode == "output" && argc == 4) {
			std::cout << nlohmann::json{{"path", paths.Output(argv[2], argv[3]).generic_string()}}.dump() << '\n';
			return 0;
		}
		if (mode == "record" && argc == 3) {
			RecordNativeTestArtifact(argv[2], "artifact_paths_probe::routing_only");
			return 0;
		}
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
	return 2;
}
