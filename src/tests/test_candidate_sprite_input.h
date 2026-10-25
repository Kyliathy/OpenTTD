/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file test_candidate_sprite_input.h Observe the actual SpriteFile bytes opened by native art startup.
 * @see docs/zz-reach2/architecture/archi-historical-debug.md
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
 * Container identity and independent pixel expectations remain separate inputs.
 * This test-only observation restores the real stream cursor and owns no renderer/cache state.
 */
#ifndef TESTS_TEST_CANDIDATE_SPRITE_INPUT_H
#define TESTS_TEST_CANDIDATE_SPRITE_INPUT_H

#include "test_artifact_paths.h"
#include "test_input_sha256.h"
#include "../spriteloader/sprite_file_type.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include <cstdlib>
#include <fstream>

/** Restore the actual opened sprite stream after read-only container identity observation. */
class CandidateSpriteCursor final {
	SpriteFile &file; ///< Actual cache-origin stream, never reopened by guessed filename.
	size_t position; ///< Original native buffered cursor.
public:
	/** @param file Actual opened cache-origin stream whose position must survive observation. */
	explicit CandidateSpriteCursor(SpriteFile &file) : file(file), position(file.GetPos()) {}
	/** Restore the original cursor; no decoded sprite or game state is changed. */
	~CandidateSpriteCursor() { this->file.SeekTo(this->position, SEEK_SET); }
};

/** Verify and record the exact bytes of the already opened native candidate origin.
 * @param origin Actual shared SpriteFile returned by GetOriginFile for the observed sprite range.
 * @return None; missing/changed/foreign candidate bindings throw before any acceptance record.
 * Canonical-only legacy invocations without candidate environment remain separately classified.
 */
inline void ObserveTestCandidateSpriteFile(SpriteFile &origin)
{
	const char *binding_name = std::getenv("OTTD_TEST_CANDIDATE_BASESET");
	const char *binding_sha = std::getenv("OTTD_TEST_CANDIDATE_BASESET_SHA256");
	// A legacy canonical invocation has no candidate claim. Partial binding must
	// never silently fall back to that mode and certify the wrong container.
	if (binding_name == nullptr && binding_sha == nullptr) return;
	if (binding_name == nullptr || binding_sha == nullptr) throw std::runtime_error("Incomplete native candidate binding");
	const auto context = GetTestCaseArtifactPaths("multilayer-art", "candidate-origin");
	const auto binding = RequireTestArtifactWithin(binding_name, context.data);
	if (TestInputSha256(binding) != binding_sha) throw std::runtime_error("Native candidate binding identity changed");
	std::ifstream source(binding);
	const auto document = nlohmann::json::parse(source);
#ifdef NDEBUG
	constexpr std::string_view configuration = "RelWithDebInfo";
#else
	constexpr std::string_view configuration = "Debug";
#endif
	const char *process_name = std::getenv("OTTD_TEST_PROCESS");
	if (process_name == nullptr) throw std::runtime_error("Native candidate has no process identity");
	const auto process = ValidateTestArtifactIdentifier(process_name);
	// Only this process/configuration's prepared runtime can supply the bytes.
	// Preparation cannot label itself as a native loading or publication result.
	if (document.value("kind", "") != "candidate_baseset_binding" || document.value("schema_version", 0) != 1 ||
			document.value("run_id", "") != context.run_id || document.value("process", "") != process ||
			document.value("configuration", "") != configuration || document.value("native_loading_verified", true) ||
			document.value("publishes_source", true)) throw std::runtime_error("Native candidate run/configuration/status disagrees");
	const auto runtime = RequireTestArtifactWithin(context.root / document.at("runtime_grf").at("path").get<std::string>(), context.runtime_dir / "baseset");
	const auto expected = document.at("runtime_grf").at("sha256").get<std::string>();
	// Source GRF identity is independent of source pixel expectations. The actual
	// opened stream must match both its prepared copy and original generated input.
	if (runtime != context.runtime_dir / "baseset/openttd.grf" || TestInputSha256(runtime) != expected ||
			document.at("candidate_source").at("sha256") != expected || origin.GetStartPos() != 0) {
		throw std::runtime_error("Native candidate runtime/source/container origin disagrees");
	}
	for (const auto &key : {"runtime_checksum", "companions"}) {
		// Required checksum and original descriptor/base files remain exact around
		// native startup; an absent companion cannot become an optional diagnostic.
		const auto entries = std::string_view(key) == "companions" ? document.at(key) : nlohmann::json::array({document.at(key)});
		for (const auto &entry : entries) {
			const auto path = RequireTestArtifactWithin(context.root / entry.at("path").get<std::string>(), context.runtime_dir / "baseset");
			if (TestInputSha256(path) != entry.at("sha256").get<std::string>()) throw std::runtime_error("Native candidate companion changed");
		}
	}
	CandidateSpriteCursor restore(origin);
	origin.SeekTo(origin.GetStartPos(), SEEK_SET);
	const size_t length = origin.GetEndPos() - origin.GetStartPos();
	Sha256 hash;
	std::array<uint8_t, 65536> block{};
	size_t remaining = length;
	// Hash the actual already-opened stream, not a second file opened by a
	// basename search. Every container byte participates, including nonpixel data.
	while (remaining != 0) {
		const size_t count = std::min(remaining, block.size());
		const size_t before = origin.GetPos();
		origin.ReadBlock(block.data(), count);
		if (origin.GetPos() != before + count) throw std::runtime_error("Short native candidate origin read");
		hash.Update(std::span{block}.first(count));
		remaining -= count;
	}
	static constexpr char hex[] = "0123456789abcdef";
	std::string actual;
	for (const auto byte : hash.Digest()) { actual += hex[byte >> 4]; actual += hex[byte & 15]; }
	if (actual != expected) throw std::runtime_error("Actual opened native GRF differs from the selected candidate");
	const auto output = context.Output("reports", "candidate-loaded-" + process + ".json");
	// Native observations are fresh outputs, never a rewritten preparation or
	// earlier frame. Pixel acceptance still requires the real original loader loop.
	if (std::filesystem::exists(output)) throw std::runtime_error("Native candidate observation already exists");
	nlohmann::json observed{{"_documentation", "@see r2fc-file-cleanup3.md; actual opened SpriteFile bytes, pixels checked separately."},
		{"schema_version", 1}, {"kind", "native_candidate_grf_observation"}, {"run_id", context.run_id},
		{"configuration", configuration}, {"process", process}, {"binding", {{"path", binding.lexically_relative(context.root).generic_string()}, {"sha256", binding_sha}}},
		{"runtime_grf", document.at("runtime_grf")}, {"origin_name", origin.GetFilename()}, {"opened_bytes", length},
		{"opened_bytes_sha256", actual}, {"native_origin_bytes_verified", true}, {"pixels_verified", false}};
	std::ofstream sink(output);
	if (!(sink << observed.dump(2) << '\n')) throw std::runtime_error("Cannot retain native candidate origin observation");
}

#endif /* TESTS_TEST_CANDIDATE_SPRITE_INPUT_H */
