/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file test_artifact_paths.cpp Native half of the development artifact routing contract.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-10.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../core/format.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "../safeguards.h"

namespace {

/**
 * Read one invocation parameter without retaining environment-owned pointers.
 * @param name Public test environment variable.
 * @param fallback Value used for direct native execution without a wrapper.
 * @return Independent copy of the selected value.
 */
std::string ArtifactEnvironment(const char *name, std::string_view fallback)
{
	const char *value = std::getenv(name);
	return value == nullptr ? std::string(fallback) : std::string(value);
}

/**
 * Reserve a native invocation once so all feature views share its identity.
 * @param root Actual source checkout.
 * @return Inherited identifier or a fresh atomically reserved direct-run identifier.
 */
std::string ReserveNativeInvocation(const std::filesystem::path &root)
{
	const auto owner = RequireTestArtifactWithin(root / "mlm/runtime", root);
	const auto reservations = RequireTestArtifactWithin(owner / ".runs", owner);
	std::filesystem::create_directories(reservations);
	const auto inherited = ArtifactEnvironment("OTTD_TEST_RUN_ID", "");
	if (!inherited.empty()) {
		const auto marker = RequireTestArtifactWithin(reservations / ValidateTestArtifactIdentifier(inherited), reservations);
		if (!std::filesystem::is_directory(marker)) throw std::runtime_error("native child has no reserved test invocation");
		return inherited;
	}
	const auto ticks = std::chrono::system_clock::now().time_since_epoch().count();
	// Atomic directory creation distinguishes simultaneous direct-native runs;
	// an existing invocation is never selected merely because its name is close.
	for (unsigned int attempt = 0; attempt < 1000; ++attempt) {
		const auto name = fmt::format("native-{}-{}", ticks, attempt);
		if (std::filesystem::create_directory(reservations / name)) return name;
	}
	throw std::runtime_error("cannot reserve a unique native test invocation");
}

/**
 * Resolve this process's runtime home independently of screenshot feature.
 * @param root Canonical source checkout.
 * @param run_id Current reserved invocation.
 * @return Process-local native data home under mlm/runtime.
 */
std::filesystem::path NativeRuntimeDirectory(const std::filesystem::path &root, std::string_view run_id)
{
	const auto owner = RequireTestArtifactWithin(root / "mlm/runtime", root);
	const auto profile = ValidateTestArtifactIdentifier(ArtifactEnvironment("OTTD_TEST_PROFILE", "native"));
	const auto process = ValidateTestArtifactIdentifier(ArtifactEnvironment("OTTD_TEST_PROCESS", "main"));
	const auto expected = RequireTestArtifactWithin(owner / profile / run_id / process, owner);
	const auto supplied = ArtifactEnvironment("OTTD_TEST_RUNTIME_DIR", "");
	// The parent may provide a concrete runtime directory, but it must describe
	// this exact process identity rather than another peer's writable data home.
	if (!supplied.empty() && RequireTestArtifactWithin(supplied, owner) != expected) {
		throw std::runtime_error("runtime directory disagrees with native invocation identity");
	}
	return expected;
}

} // namespace

/** @copydoc FindTestRepositoryRoot */
std::filesystem::path FindTestRepositoryRoot()
{
	const auto supplied = ArtifactEnvironment("OTTD_TEST_REPOSITORY", "");
	auto candidate = std::filesystem::weakly_canonical(supplied.empty() ? std::filesystem::current_path() : std::filesystem::path(supplied));
	// Only a source checkout owns the test artifacts. Ancestor walking supports
	// builds and nested process homes without depending on their nesting depth.
	while (!candidate.empty()) {
		if (std::filesystem::is_regular_file(candidate / "CMakeLists.txt") && std::filesystem::is_directory(candidate / "src")) return candidate;
		const auto parent = candidate.parent_path();
		if (parent == candidate) break;
		candidate = parent;
	}
	throw std::runtime_error("no source checkout for native test artifacts");
}

/** @copydoc ValidateTestArtifactIdentifier */
std::string ValidateTestArtifactIdentifier(std::string_view value)
{
	if (value.empty()) throw std::runtime_error("empty artifact identifier");
	// An identifier is one namespace component, never a drive, separator or
	// traversal expression. This rule is shared with the Python producer.
	for (const char c : value) {
		const bool letter = c >= 'a' && c <= 'z';
		const bool digit = c >= '0' && c <= '9';
		if (!letter && !digit && c != '-' && c != '_') throw std::runtime_error("invalid artifact identifier");
	}
	const char first = value.front();
	if (first == '-' || first == '_') throw std::runtime_error("artifact identifier must start with a letter or digit");
	return std::string(value);
}

/** @copydoc RequireTestArtifactWithin */
std::filesystem::path RequireTestArtifactWithin(const std::filesystem::path &path, const std::filesystem::path &owner)
{
	const auto boundary = std::filesystem::weakly_canonical(owner);
	const auto candidate = std::filesystem::weakly_canonical(path);
	const auto relative = candidate.lexically_relative(boundary);
	// Canonicalization resolves existing symlinks/junctions; equality and a
	// parent component cannot grant output ownership outside the declared case.
	if (relative.empty() || relative == "." || relative.is_absolute()) throw std::runtime_error("artifact is not below its owner");
	for (const auto &part : relative) {
		if (part == "..") throw std::runtime_error("artifact escapes its owner");
	}
	return candidate;
}

/** @copydoc GetTestArtifactPaths */
TestArtifactPaths GetTestArtifactPaths(std::string_view feature, std::string_view scenario)
{
	const auto root = FindTestRepositoryRoot();
	static const auto run_id = ReserveNativeInvocation(root);
	const auto feature_name = ValidateTestArtifactIdentifier(feature);
	const auto scenario_name = ValidateTestArtifactIdentifier(scenario);
	const auto reviews = RequireTestArtifactWithin(root / "mlm/reviews", root);
	const auto directory = RequireTestArtifactWithin(reviews / feature_name / scenario_name / run_id, reviews);
	const auto log_root = RequireTestArtifactWithin(root / "logs", root);
	const auto logs = RequireTestArtifactWithin(log_root / feature_name / scenario_name / run_id, log_root);
	const auto runtime = NativeRuntimeDirectory(root, run_id);
	TestArtifactPaths result{root, feature_name, scenario_name, run_id,
		directory / "images", directory / "saves", directory / "reports", directory / "scratch", logs, runtime, runtime / "native-art.cfg"};
	// Each case owns only these output directories. Creating a feature view
	// never materializes or overwrites a fixture, config template or media source.
	for (const auto &path : {result.images, result.saves, result.reports, result.scratch, result.logs, result.runtime_dir}) {
		std::filesystem::create_directories(RequireTestArtifactWithin(path, root));
	}
	return result;
}

/** @copydoc GetTestCaseArtifactPaths */
TestArtifactPaths GetTestCaseArtifactPaths(std::string_view feature, std::string_view scenario)
{
	return GetTestArtifactPaths(ArtifactEnvironment("OTTD_TEST_FEATURE", feature), ArtifactEnvironment("OTTD_TEST_SCENARIO", scenario));
}

/** @copydoc TestArtifactPaths::Output */
std::filesystem::path TestArtifactPaths::Output(std::string_view kind, std::string_view name) const
{
	const std::filesystem::path basename{name};
	// A caller selects a file identity inside its declared case, not a new
	// directory that might name an input fixture or another feature's output.
	if (basename.empty() || basename.filename() != basename || basename == "." || basename == "..") throw std::runtime_error("artifact requires a basename");
	const std::filesystem::path *directory = nullptr;
	if (kind == "images") directory = &this->images;
	if (kind == "saves") directory = &this->saves;
	if (kind == "reports") directory = &this->reports;
	if (kind == "scratch") directory = &this->scratch;
	if (kind == "logs") directory = &this->logs;
	if (directory == nullptr) throw std::runtime_error("unknown test artifact kind");
	return RequireTestArtifactWithin(*directory / basename, *directory);
}

/** @copydoc ResolveTestInputPath */
std::filesystem::path ResolveTestInputPath(const std::filesystem::path &input)
{
	const auto root = FindTestRepositoryRoot();
	auto candidate = RequireTestArtifactWithin(input.is_absolute() ? input : root / input, root);
	const auto manifest = root / "src/tests/fixtures/multilayer-inputs.json";
	// Stable input keys resolve through an explicit relocation index. They do
	// not authorize scanning history, choosing a latest save or recreating a
	// compatibility file in the obsolete flat fixture directory.
	if (!std::filesystem::is_regular_file(candidate) && std::filesystem::is_regular_file(manifest)) {
		std::ifstream stream(manifest);
		const auto document = nlohmann::json::parse(stream);
		const auto key = candidate.lexically_relative(root).generic_string();
		const auto &inputs = document.at("inputs");
		if (inputs.contains(key)) candidate = RequireTestArtifactWithin(root / inputs.at(key).at("path").get<std::string>(), root);
	}
	if (!std::filesystem::is_regular_file(candidate)) throw std::runtime_error("declared test input is missing: " + candidate.string());
	// Existing Classic/supplied regression worlds remain read-only inputs; the
	// additional roots are not permitted destinations for generated test saves.
	for (const auto owner : {"src/tests/fixtures", "mlm/reviews", "regression", "build/save"}) {
		try {
			return RequireTestArtifactWithin(candidate, RequireTestArtifactWithin(root / owner, root));
		} catch (const std::runtime_error &) {
			// Try the next explicitly allowed input owner, not an arbitrary fallback.
		}
	}
	throw std::runtime_error("test input is outside declared owners");
}

/** @copydoc GetTestRuntimeConfig */
std::filesystem::path GetTestRuntimeConfig(std::string_view template_name)
{
	const auto context = GetTestArtifactPaths(ArtifactEnvironment("OTTD_TEST_FEATURE", "multilayer-art"), ArtifactEnvironment("OTTD_TEST_SCENARIO", "native-startup"));
	const std::filesystem::path name{template_name};
	if (name.filename() != name) throw std::runtime_error("runtime template must be a basename");
	auto source = context.root / "src/tests/fixtures/config/multilayer" / name;
	if (!std::filesystem::is_regular_file(source)) source = context.root / "build" / name;
	const auto destination = RequireTestArtifactWithin(context.runtime_dir / name, context.runtime_dir);
	if (!std::filesystem::exists(destination)) std::filesystem::copy_file(source, destination);
	// Config-local discovery must still find the exact repository distribution.
	// Separate copies isolate mutable startup scripts and peer data homes; a
	// repeated preparation call must not replace a caller's new startup command.
	for (const auto folder : {"baseset", "newgrf", "lang", "scripts"}) {
		const auto original = context.root / "build" / folder;
		const auto target = context.runtime_dir / folder;
		if (std::filesystem::is_directory(original) && !std::filesystem::exists(target)) {
			std::filesystem::copy(original, target, std::filesystem::copy_options::recursive);
		}
	}
	return destination;
}

/** @copydoc RecordNativeTestArtifact */
void RecordNativeTestArtifact(const std::filesystem::path &path, std::string_view writer)
{
	const auto root = FindTestRepositoryRoot();
	const auto owner = RequireTestArtifactWithin(root / "mlm/reviews", root);
	const auto actual = RequireTestArtifactWithin(path, owner);
	const auto relative = actual.lexically_relative(owner);
	std::vector<std::string> parts;
	for (const auto &part : relative) parts.push_back(part.string());
	// A capture must identify feature, scenario, invocation and images before
	// its filename; a flat directory cannot masquerade as traceable evidence.
	if (parts.size() != 5 || parts[3] != "images") throw std::runtime_error("native capture lacks feature/run ownership");
	const auto context = GetTestArtifactPaths(parts[0], parts[1]);
	if (context.run_id != parts[2]) throw std::runtime_error("native capture belongs to another invocation");
	const nlohmann::json record{{"_documentation", "Owner: docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md"},
		{"native_writer", writer}, {"feature", parts[0]}, {"scenario", parts[1]}, {"run_id", parts[2]},
		{"path", actual.lexically_relative(root).generic_string()}};
	std::ofstream output(context.reports / "native-artifacts.jsonl", std::ios::app);
	if (!output.good()) throw std::runtime_error("cannot record native capture provenance");
	output << record.dump() << '\n';
}
