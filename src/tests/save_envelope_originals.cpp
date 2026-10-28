/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_originals.cpp Exact retained source bytes and live-world nonmutation witnesses for preflight.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 */
#include "../stdafx.h"
#include <filesystem>
#include <fstream>
#include "../3rdparty/nlohmann/json.hpp"
#include "save_envelope_fixture.h"
#include "../fileio_type.h"
#include "transport_tube_fixture.h"
#include "../core/random_func.hpp"
#include "../saveload/saveload.h"

extern SaveLoadVersion _sl_version; ///< Native loader's original candidate version, observed only to prove detached inspection leaves it unchanged.

/** Independent catalogue expectations classify retained originals; no file is rewritten, ticked or fully loaded. */
TEST_CASE("Save envelope inspector retains exact A and B original identities", "[save-envelope-originals]")
{
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	std::ifstream input(root / "src/tests/fixtures/saveload/lineage-originals.json");
	REQUIRE(input.good());
	const auto catalogue = nlohmann::json::parse(input);
	// Every original has an independent pre-implementation digest/version and
	// exact observed common metadata. A path locates bytes; it is never classifier evidence.
	for (const auto &entry : catalogue.at("observations")) {
		INFO(entry.at("path"));
		auto file = FileHandle::Open((root / entry.at("path").get<std::string>()).string(), "rb"); REQUIRE(file.has_value());
		const auto raw = CreateSaveFileReader(std::move(*file));
		const auto facts = InspectSaveWireFacts(raw);
		CHECK(facts.framing_complete); CHECK(facts.common_transport_valid);
		CHECK(facts.original_version == entry.at("version").get<uint16_t>());
		CHECK(EnvelopeDigest(facts.source_digest) == entry.at("sha256").get<std::string>());
		const auto &families = entry.at("facts").at("families");
		CHECK(facts.feature_flags == families.at("MLMD").at("metadata").at("feature_flags").get<uint32_t>());
		CHECK(facts.space_content_version == families.at("SDEF").at("content_version").get<uint16_t>());
		SaveOriginHint hint{entry.at("origin") == "A" ? SaveFormat::Orbital : SaveFormat::Geothermal, facts.source_digest};
		const auto result = ClassifySaveFormat(facts, hint);
		CHECK(result.status == SaveLineageStatus::Accepted);
		CHECK(result.format == (facts.original_version <= 424 ? SaveFormat::Base : hint.format));
		CHECK(result.original_version == facts.original_version);
	}
	// Real deliberately incomplete donor families must refuse. Either bounded
	// framing or pure mandatory-family policy may detect the missing owner.
	for (const auto &entry : catalogue.at("negative_examples")) {
		INFO(entry.at("path"));
		auto file = FileHandle::Open((root / entry.at("path").get<std::string>()).string(), "rb"); REQUIRE(file.has_value());
		try {
			const auto facts = InspectSaveWireFacts(CreateSaveFileReader(std::move(*file)));
			CHECK(ClassifySaveFormat(facts).status == SaveLineageStatus::Corrupt);
		} catch (const SaveFramingError &) {
			SUCCEED("Detached bounded framing refused the original malformed family");
		}
	}
}

/** An actual native capsule/depot/company remain identical across successful and failing detached inspection; no initialization or chunk callback runs. */
TEST_CASE("Save envelope preflight cannot change a running native world", "[save-envelope-originals]")
{
	TubePrototypeRig rig;
	auto *capsule = rig.BuyCapsule();
	const auto id = capsule->index;
	const auto cash = Company::Get(_current_company)->money;
	const auto tile = capsule->tile;
	const auto map = Map::Size();
	const auto tick = TimerGameTick::counter;
	const std::array<uint32_t, 2> rng{_random.state[0], _random.state[1]};
	const auto version = _sl_version;
	const auto input = EnvelopeStream(EnvelopeHex("544553540000000310203000000000"));
	CHECK(InspectSaveWireFacts(EnvelopeNetworkReader(input)).framing_complete);
	// Codec creation, decode corruption and table-header failure all remain
	// detached. Native SlError's pointer-null/feature-reset cleanup is forbidden here.
	for (const auto tag : {"OTTZ", "OTTX", "OTTD", "BAD!"}) {
		CHECK_THROWS_AS(InspectSaveWireFacts(EnvelopeNetworkReader(EnvelopeStream({1, 2, 3, 4}, tag))), SaveFramingError);
	}
	CHECK_THROWS_AS(InspectSaveWireFacts(EnvelopeNetworkReader(EnvelopeStream(EnvelopeHex("4d4c4d44030000000000")))), SaveFramingError);
	CHECK(Vehicle::GetIfValid(id) == capsule); CHECK(capsule->tile == tile); CHECK(capsule->IsInDepot());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Map::Size() == map);
	CHECK(TimerGameTick::counter == tick); CHECK(_random.state[0] == rng[0]); CHECK(_random.state[1] == rng[1]); CHECK(_sl_version == version);
}
