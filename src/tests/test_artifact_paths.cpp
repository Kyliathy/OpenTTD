/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file test_artifact_paths.cpp Native half of the development artifact routing contract.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-10.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10/18B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "test_input_sha256.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../core/format.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
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

/**
 * Read one exact generated sprite input role without reserving or creating outputs.
 * @param root Selected source checkout.
 * @param name Explicit environment field containing an existing input path.
 * @param role Required data or reports owner.
 * @param run_id Explicit source/native invocation shared by producer and consumer.
 * @return Existing contained file; old caches and other writers' runs refuse.
 */
std::filesystem::path SelectedSpriteInput(const std::filesystem::path &root, const char *name, std::string_view role, std::string_view run_id)
{
	const auto value = ArtifactEnvironment(name, "");
	if (value.empty()) throw std::runtime_error("expected sprite input requires an explicit path");
	const auto owner = RequireTestArtifactWithin(root / "mlm/reviews", root);
	const auto path = RequireTestArtifactWithin(value, owner);
	std::vector<std::string> parts;
	// Every input retains the writer's actual feature/scenario/run/role. Merely
	// existing somewhere under reviews cannot grant current source authority.
	for (const auto &part : path.lexically_relative(owner)) parts.push_back(part.string());
	if (parts.size() != 5 || parts[2] != run_id || parts[3] != role || !std::filesystem::is_regular_file(path)) {
		throw std::runtime_error("expected sprite input has a missing file or foreign invocation/role");
	}
	return path;
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
		directory / "images", directory / "saves", directory / "data", directory / "reports", directory / "evidence", directory / "scratch", logs, runtime, runtime / "native-art.cfg"};
	// Each case owns only these output directories. Creating a feature view
	// never materializes or overwrites a fixture, config template or media source.
	for (const auto &path : {result.images, result.saves, result.data, result.reports, result.evidence, result.scratch, result.logs, result.runtime_dir}) {
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
	if (kind == "data") directory = &this->data;
	if (kind == "reports") directory = &this->reports;
	if (kind == "evidence") directory = &this->evidence;
	if (kind == "scratch") directory = &this->scratch;
	if (kind == "logs") directory = &this->logs;
	if (directory == nullptr) throw std::runtime_error("unknown test artifact kind");
	return RequireTestArtifactWithin(*directory / basename, *directory);
}

/**
 * Read one complete input-catalogue epoch, shared safely across fixture readers.
 * @param path Exact repository-owned multilayer-inputs.json.
 * @return Immutable parsed document, invalidated when size or timestamp changes.
 */
static std::shared_ptr<const nlohmann::json> TestInputCatalogue(const std::filesystem::path &path)
{
	static std::mutex mutex;
	static std::filesystem::path selected;
	static std::filesystem::file_time_type modified;
	static uintmax_t length = 0;
	static std::shared_ptr<const nlohmann::json> document;
	const std::lock_guard lock(mutex);
	const auto current_modified = std::filesystem::last_write_time(path);
	const auto current_length = std::filesystem::file_size(path);
	// Many native cases select inputs in one invocation. Reparse only a changed
	// catalogue, while every selected payload still receives a full-byte hash.
	if (document != nullptr && selected == path && modified == current_modified && length == current_length) return document;
	std::ifstream stream(path);
	const auto loaded = std::make_shared<const nlohmann::json>(nlohmann::json::parse(stream));
	const auto schema = loaded->value("schema_version", 0);
	// Both established versions use exact identities. Unknown schemas or a
	// partially replaced file must not cause fallback to workstation history.
	if ((schema != 1 && schema != 2) || !loaded->at("inputs").is_object()) throw std::runtime_error("unsupported input catalogue schema");
	if (std::filesystem::last_write_time(path) != current_modified || std::filesystem::file_size(path) != current_length) throw std::runtime_error("input catalogue changed while loading");
	selected = path;
	modified = current_modified;
	length = current_length;
	document = loaded;
	return document;
}

/** @copydoc ResolveTestInputPath */
std::filesystem::path ResolveTestInputPath(const std::filesystem::path &input)
{
	const auto root = FindTestRepositoryRoot();
	auto candidate = RequireTestArtifactWithin(input.is_absolute() ? input : root / input, root);
	const auto manifest = root / "src/tests/fixtures/multilayer-inputs.json";
	std::string expected;
	// Retained stable keys win even when the old path exists. Unindexed prior
	// runs remain explicit inputs, with no latest-file search or missing-key scan.
	if (std::filesystem::is_regular_file(manifest)) {
		const auto document = TestInputCatalogue(manifest);
		const auto key = candidate.lexically_relative(root).generic_string();
		const auto &inputs = document->at("inputs");
		if (inputs.contains(key)) {
			const auto &reference = inputs.at(key);
			expected = reference.at("sha256").get<std::string>();
			// Full lowercase SHA256 is the shared identity contract; malformed
			// declarations cannot degrade into an unhashed direct-file selection.
			if (expected.size() != 64 || expected.find_first_not_of("0123456789abcdef") != std::string::npos) throw std::runtime_error("indexed input requires an exact SHA256");
			candidate = RequireTestArtifactWithin(root / reference.at("path").get<std::string>(), root);
		}
	}
	if (!std::filesystem::is_regular_file(candidate)) throw std::runtime_error("declared test input is missing: " + candidate.string());
	// Existing Classic/supplied regression worlds remain read-only inputs; the
	// additional roots are not permitted destinations for generated test saves.
	bool contained = false;
	for (const auto owner : {"src/tests/fixtures", "savegames/retained", "mlm/reviews", "regression", "build/save"}) {
		try {
			RequireTestArtifactWithin(candidate, RequireTestArtifactWithin(root / owner, root));
			contained = true;
			break;
		} catch (const std::runtime_error &) {
			// Try the next explicitly allowed input owner, not an arbitrary fallback.
		}
	}
	if (!contained) throw std::runtime_error("test input is outside declared owners");
	// Hash the actual retained payload after owner validation. A missing or
	// corrupt retained copy must fail, even if the old original is still present.
	if (!expected.empty() && TestInputSha256(candidate) != expected) throw std::runtime_error("indexed input hash changed: " + input.string());
	return candidate;
}

/** @copydoc GetTestExpectedSprites */
std::filesystem::path GetTestExpectedSprites(std::string_view family)
{
	const auto root = FindTestRepositoryRoot();
	const std::array<std::string_view, 5> families{"m1", "space", "orbital", "launch-fleet", "ground"};
	const auto supplied_family = ArtifactEnvironment("OTTD_TEST_EXPECTED_SPRITES_FAMILY", "");
	// Each hidden loader has one explicit protocol family. Another family's
	// valid binary or a path-only environment setting cannot supply this case.
	if (std::find(families.begin(), families.end(), family) == families.end() || supplied_family != family) {
		throw std::runtime_error("expected sprite input family is missing or disagrees with the native case");
	}
#ifdef NDEBUG
	constexpr std::string_view configuration = "RelWithDebInfo";
#else
	constexpr std::string_view configuration = "Debug";
#endif
	// Supported native proof profiles are Debug and RelWithDebInfo. Bind to
	// the actually compiled profile, never a caller's different result label.
	if (ArtifactEnvironment("OTTD_TEST_CONFIGURATION", "") != configuration ||
			ArtifactEnvironment("OTTD_TEST_EXPECTED_SPRITES_CONFIGURATION", "") != configuration) {
		throw std::runtime_error("expected sprite input configuration disagrees with the native build");
	}
	const auto run_id = ValidateTestArtifactIdentifier(ArtifactEnvironment("OTTD_TEST_RUN_ID", ""));
	const auto marker = RequireTestArtifactWithin(root / "mlm/runtime/.runs" / run_id, root / "mlm/runtime/.runs");
	if (!std::filesystem::is_directory(marker)) throw std::runtime_error("expected sprite input has no reserved invocation");
	const auto payload = SelectedSpriteInput(root, "OTTD_TEST_EXPECTED_SPRITES", "data", run_id);
	const auto manifest = SelectedSpriteInput(root, "OTTD_TEST_EXPECTED_SPRITES_MANIFEST", "reports", run_id);
	const auto payload_sha = ArtifactEnvironment("OTTD_TEST_EXPECTED_SPRITES_SHA256", "");
	const auto manifest_sha = ArtifactEnvironment("OTTD_TEST_EXPECTED_SPRITES_MANIFEST_SHA256", "");
	// Both exact identities are required before reading a typed report or
	// entering native startup. Source and report paths remain one actual writer.
	if (payload.extension() != ".bin" || payload.parent_path().parent_path() != manifest.parent_path().parent_path() ||
			payload_sha.size() != 64 || payload_sha.find_first_not_of("0123456789abcdef") != std::string::npos ||
			manifest_sha.size() != 64 || manifest_sha.find_first_not_of("0123456789abcdef") != std::string::npos ||
			TestInputSha256(payload) != payload_sha || TestInputSha256(manifest) != manifest_sha) {
		throw std::runtime_error("expected sprite input identity or writer changed");
	}
	std::ifstream input(manifest);
	const auto document = nlohmann::json::parse(input);
	const auto recipe = std::string(family == "launch-fleet" ? "launch" : family) + "-expected-payload";
	const auto declared = RequireTestArtifactWithin(root / document.at("expected_payload").at("path").get<std::string>(), root / "mlm/reviews");
	// Source completion is distinct from native acceptance. Reject altered
	// family/recipe/run/status claims before the real decoder reads any sprite.
	if (document.value("kind", "") != "selected_sprite_expectations" || document.value("family", "") != family ||
			document.value("recipe", "") != recipe || document.value("run_id", "") != run_id ||
			document.value("comparison", "") != "independent_source_bytes" ||
			document.value("configuration_reuse", "") != "source_bytes_only" ||
			document.value("native_loading_verified", true) || document.value("publishes_source", true) ||
			declared != payload || document.at("expected_payload").value("sha256", "") != payload_sha) {
		throw std::runtime_error("expected sprite input source report family/recipe/status changed");
	}
	return payload;
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
