/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_validation.cpp Historical origins and candidate-only mineral/ownership joins.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid_cargo.h"
#include "../cargo_source_digest.h"
#include "../fios.h"
#include "../saveload/space_cargo_preview.h"
#include "../safeguards.h"

/** Issuance proves history while a surviving exact ID independently constrains its mineral. */
TEST_CASE("Historical asteroid origins use full issuance and surviving composition", "[asteroid-cargo-validation]")
{
	const CargoLabel ore{"NORE"}, carbon{"WICE"};
	const std::array rows{AsteroidIdentityRecord{OrbitalBodyID{70'000}, AsteroidClass::Metallic}};
	CHECK(ValidateAsteroidCargoOrigin({70'000, ore}, rows, 70'001));
	CHECK_FALSE(ValidateAsteroidCargoOrigin({70'000, carbon}, rows, 70'001));
	CHECK(ValidateAsteroidCargoOrigin({69'999, ore}, rows, 70'001));
	CHECK(ValidateAsteroidCargoOrigin({UINT32_MAX - 1, ore}, {}, UINT32_MAX));
	CHECK_FALSE(ValidateAsteroidCargoOrigin({UINT32_MAX, ore}, {}, UINT32_MAX));
	CHECK_FALSE(ValidateAsteroidCargoOrigin({70'001, ore}, rows, 70'001));
	CHECK_FALSE(ValidateAsteroidCargoOrigin({69'999, CargoLabel{"FAIL"}}, rows, 70'001));
	CHECK_FALSE(ValidateAsteroidCargoOrigin({69'999, {}}, rows, 70'001));
	const Source legacy{SourceID{0}, SourceType::Asteroid};
	const std::array legacy_rows{AsteroidIdentityRecord{OrbitalBodyID{0}, AsteroidClass::Metallic}};
	CHECK(ResolveAsteroidCargoOrigin(legacy, legacy_rows, 1) == AsteroidCargoOrigin{0, ore});
	CHECK_FALSE(ResolveAsteroidCargoOrigin(legacy, {}, 1));
	CHECK_FALSE(ResolveAsteroidCargoOrigin(Source{SourceID{0}, SourceType::Industry}, legacy_rows, 1));
	CHECK(ResolveAsteroidCargoOrigin(Source::FromAsteroid(0, ore), {}, 1) == AsteroidCargoOrigin{0, ore});
}

/** @param data Empty detached candidate to populate. The arbitrary cargo slot belongs only to this candidate; no CargoSpec lookup or native packet allocation participates. */
static void PrepareHistoricalCargoPreview(LoadCheckData &data)
{
	data.map_size_x = 64; data.map_size_y = 64; data.map_layers = MAX_MAP_LAYERS;
	data.asteroid_present = true; data.asteroid_active = true; data.asteroid_next_body_id = 70'001;
	data.native_cargo_packets_present = true; data.native_station_cargo_present = true; data.native_vehicle_cargo_present = true;
	data.native_cargo_dimensions_present = true;
	data.native_cargo_dimension_rows.push_back({CargoType{5}, CargoLabel{"NORE"}, 16});
	data.native_cargo_packet_rows.push_back({CargoPacketID{2}, 2, Source::FromAsteroid(70'000, CargoLabel{"NORE"}), Money{0},
		TileIndex{to_underlying(MapLayer::Space) * 64 * 64 + 100}, StationID::Invalid()});
	data.native_station_cargo_refs.push_back(3);
	data.native_cargo_owner_rows.push_back({3, CargoType{5}, StationID::Invalid()});
}

/** A historical packet is valid in exactly one compatible same-file owner, with its original production receipt. */
TEST_CASE("Candidate asteroid custody rejects missing meaning bad history and duplicate owners", "[asteroid-cargo-validation]")
{
	LoadCheckData data; PrepareHistoricalCargoPreview(data);
	const auto native_count = CargoPacket::GetNumItems();
	CHECK(ValidateSpaceCargoPreviewOwners(data));
	SECTION("Ordinary carrier uses the candidate cargo slot") {
		data.native_station_cargo_refs.clear(); data.native_ordinary_vehicle_cargo_refs.push_back(3);
		CHECK(ValidateSpaceCargoPreviewOwners(data));
		data.native_cargo_dimension_rows[0].label = CargoLabel{"WICE"};
		CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data));
	}
	SECTION("Space freight uses its own copied cargo slot") {
		data.native_station_cargo_refs.clear(); data.native_cargo_owner_rows.clear();
		auto &row = data.native_space_vehicle_rows.emplace_back(); row.freight_cargo = CargoType{5}; row.cargo_packet_refs.push_back(3);
		CHECK(ValidateSpaceCargoPreviewOwners(data));
		row.freight_cargo = CargoType{6}; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data));
	}
	SECTION("Duplicate physical owner") { data.native_ordinary_vehicle_cargo_refs.push_back(3); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Orphan packet") { data.native_station_cargo_refs.clear(); data.native_cargo_owner_rows.clear(); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Future identity") { data.asteroid_next_body_id = 70'000; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Wrong mineral") { data.native_cargo_packet_rows[0].source = Source::FromAsteroid(70'000, CargoLabel{"WICE"}); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Surviving body contradicts source") { data.asteroid_body_rows.push_back({OrbitalBodyID{70'000}, AsteroidClass::Rich}); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Surface production") { data.native_cargo_packet_rows[0].source_tile = TileIndex{100}; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Invented first pickup") { data.native_cargo_packet_rows[0].first_station = StationID{0}; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Missing container meaning") { data.native_cargo_owner_rows.clear(); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Missing configured meaning") { data.native_cargo_dimension_rows.clear(); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Absent family with stale rows") { data.native_cargo_dimensions_present = false; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Absent family with no rows") { data.native_cargo_dimensions_present = false; data.native_cargo_dimension_rows.clear(); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Unused ordinary meaning") { data.native_cargo_owner_rows.push_back({4, CargoType{5}, StationID::Invalid()}); CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); }
	SECTION("Legacy absence never borrows an open-world body") {
		data.native_cargo_packet_rows[0].source = Source{SourceID{0}, SourceType::Asteroid};
		CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data));
		data.asteroid_body_rows.push_back({OrbitalBodyID{0}, AsteroidClass::Metallic});
		CHECK(ValidateSpaceCargoPreviewOwners(data));
		data.native_cargo_dimension_rows.clear(); data.native_cargo_dimensions_present = false;
		CHECK(ValidateSpaceCargoPreviewOwners(data)); // Explicit genuine old-era full-load-only ordinal check.
	}
	CHECK(CargoPacket::GetNumItems() == native_count);
	data.Clear(); CHECK(data.native_cargo_owner_rows.empty()); CHECK(data.native_cargo_packet_rows.empty());
}

/** Existing diagnostic streams remain identical for old/ordinary sources; full identity and mineral distinguish modern origins. */
TEST_CASE("Source digest includes complete modern provenance without changing legacy input", "[asteroid-cargo-validation]")
{
	/** @param source Original source value to observe. @return Exact scalar stream presented to each existing digest's byte encoder. */
	auto stream = [](const Source &source) {
		std::vector<uint64_t> values;
		/** @param value Canonical input scalar, recorded without interpreting native pointer or cargo state. */
		auto add = [&](uint64_t value) { values.push_back(value); };
		AppendCargoSourceDigest(source, add); return values;
	};
	// Existing purpose ordinals and legacy asteroid IDs must keep immutable
	// diagnostic fixture expectations byte-for-byte unchanged.
	for (uint8_t value = 0; value <= to_underlying(SourceType::CompanySupply); ++value) {
		CHECK(stream(Source{SourceID{1}, static_cast<SourceType>(value)}) == std::vector<uint64_t>{value, 1});
	}
	const auto original = stream(Source::FromAsteroid(65'536, CargoLabel{"NORE"}));
	CHECK(original == std::vector<uint64_t>{3, Source::Invalid, 0x4153544F52494731, 65'536, 'N', 'O', 'R', 'E'});
	CHECK(original != stream(Source::FromAsteroid(131'072, CargoLabel{"NORE"})));
	CHECK(original != stream(Source::FromAsteroid(65'536, CargoLabel{"WICE"})));
}
