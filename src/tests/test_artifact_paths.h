/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file test_artifact_paths.h Shared development-only artifact and native data-home boundary.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-10.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10/18B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef TESTS_TEST_ARTIFACT_PATHS_H
#define TESTS_TEST_ARTIFACT_PATHS_H

#include <filesystem>
#include <string>
#include <string_view>

/** One feature view of a reserved invocation; runtime state remains process-local. */
struct TestArtifactPaths {
	std::filesystem::path root; ///< Source checkout; never inferred from documentation depth.
	std::string feature; ///< Explicit player-facing owner supplied by the producer.
	std::string scenario; ///< Named feature case, independent of filename prefixes.
	std::string run_id; ///< Invocation shared across feature views and native children.
	std::filesystem::path images; ///< Captures for this feature/scenario/invocation.
	std::filesystem::path saves; ///< Newly generated checkpoints, never source fixtures.
	std::filesystem::path data; ///< Selected generated payloads, never immutable inputs.
	std::filesystem::path reports; ///< Machine-readable evidence for this feature.
	std::filesystem::path evidence; ///< Required current-run transcripts, excluding frames.
	std::filesystem::path scratch; ///< Disposable intermediates owned by this case.
	std::filesystem::path logs; ///< Associated repository log directory.
	std::filesystem::path runtime_dir; ///< Isolated native process data home.
	std::filesystem::path config_path; ///< Writable native-art configuration copy.

	/**
	 * Resolve an output without allowing a producer to redirect its ownership.
	 * @param kind One of images, saves, data, reports, evidence, scratch or logs.
	 * @param name Artifact basename, including its extension.
	 * @return Canonical output inside this case's declared directory.
	 */
	std::filesystem::path Output(std::string_view kind, std::string_view name) const;
};

/** @return Canonical source checkout enclosing the working directory or explicit test root. */
std::filesystem::path FindTestRepositoryRoot();

/**
 * Validate a routing component without interpreting it as a path.
 * @param value Feature, scenario, profile, process or run identifier.
 * @return The validated identifier; invalid names throw before artifact writes.
 */
std::string ValidateTestArtifactIdentifier(std::string_view value);

/**
 * Refuse traversal or junction redirection beyond an artifact owner.
 * @param path Existing or future artifact path.
 * @param owner Authorized directory, already verified within the checkout.
 * @return Canonical path strictly below owner.
 */
std::filesystem::path RequireTestArtifactWithin(const std::filesystem::path &path, const std::filesystem::path &owner);

/**
 * Obtain feature paths in the inherited invocation or a fresh direct-native run.
 * @param feature Explicit feature from the producer catalogue.
 * @param scenario Explicit scenario owned by that feature.
 * @return Created output directories and the process-local runtime identity.
 */
TestArtifactPaths GetTestArtifactPaths(std::string_view feature, std::string_view scenario);

/**
 * Obtain the wrapper's explicit case context for checkpoints and shared captures.
 * @param feature Direct-native default feature when no wrapper supplied one.
 * @param scenario Direct-native default scenario when no wrapper supplied one.
 * @return Paths for the explicitly inherited case, without filename inference.
 */
TestArtifactPaths GetTestCaseArtifactPaths(std::string_view feature, std::string_view scenario);

/**
 * Resolve an immutable fixture, explicit prior-run input or legacy regression save.
 * Indexed retained identities take precedence over an existing old path; their
 * full SHA256 must match. Unindexed prior-run paths remain direct selections.
 * @param input Existing source path or stable key in the input manifest.
 * @return Existing input path; does not grant the caller write permission.
 */
std::filesystem::path ResolveTestInputPath(const std::filesystem::path &input);

/**
 * Resolve an explicitly bound source sprite payload before native loader startup.
 * @param family Exact original binary family expected by the selected native case.
 * @return Hash-verified current data path with matching source report/run/configuration.
 * @throws std::runtime_error On missing/foreign/changed payloads, families or configurations.
 * No historical expected directory or general saved-world alias lookup is used.
 */
std::filesystem::path GetTestExpectedSprites(std::string_view family);

/**
 * Materialize native configuration/content in this invocation's process home.
 * @param template_name Config basename under immutable templates or the existing build.
 * @return Writable runtime config path, never the source template.
 */
std::filesystem::path GetTestRuntimeConfig(std::string_view template_name = "native-art.cfg");

/**
 * Record the actual producer and feature of a completed native capture.
 * @param path Existing captured artifact, already inside a feature run.
 * @param writer Exact native function producing the pixels.
 * @return None; append provenance for the wrapper's final hash-bound report.
 */
void RecordNativeTestArtifact(const std::filesystem::path &path, std::string_view writer);

#endif /* TESTS_TEST_ARTIFACT_PATHS_H */
