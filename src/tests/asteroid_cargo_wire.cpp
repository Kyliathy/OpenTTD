/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_wire.cpp Versioned CAPA source fields and in-place same-file legacy qualification.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21.
 * Packet/facility/pod metadata below are explicitly declared isolated codec inputs; no extraction, procurement or paid delivery is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../saveload/cargo_source_sl.h"
#include "../saveload/space_cargo_preview.h"
#include "../asteroid_cargo.h"
#include "../fios.h"
#include "../space_pod.h"
#include "../space_mining_inventory.h"
#include "../hub_cargo.h"
#include "../station_base.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"

/** Mandatory payload keys cannot silently default or cross the genuine source-version boundary. */
TEST_CASE("CAPA asteroid source schema requires exact era fields", "[asteroid-cargo-wire]")
{
	const std::vector<std::string_view> old{"count", "source_type", "source_id", "feeder_share", "source", "source_xy"};
	auto modern = old; modern.push_back("asteroid_body"); modern.push_back("asteroid_mineral");
	CHECK(ValidateCargoPacketSourceSchema(old, SaveLoadVersion::SurfaceSupplyOrders));
	CHECK(ValidateCargoPacketSourceSchema(modern, SaveLoadVersion::AsteroidCargoOrigins));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(old, SaveLoadVersion::AsteroidCargoOrigins));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(modern, SaveLoadVersion::SurfaceSupplyOrders));
	// Every authority key independently remains mandatory; a duplicate key also
	// cannot impersonate the absent scalar/payload identity or original credit.
	for (size_t index = 0; index < modern.size(); ++index) {
		auto missing = modern; missing.erase(missing.begin() + index);
		CHECK_FALSE(ValidateCargoPacketSourceSchema(missing, SaveLoadVersion::AsteroidCargoOrigins));
		auto duplicate = modern; duplicate.push_back(modern[index]);
		CHECK_FALSE(ValidateCargoPacketSourceSchema(duplicate, SaveLoadVersion::AsteroidCargoOrigins));
	}
}

/** Full-width IDs and explicit portable mineral bytes survive the original source transport without aliasing ordinary native IDs. */
TEST_CASE("CAPA natural origin codec preserves full IDs and rejects dual or wrong-era payloads", "[asteroid-cargo-wire]")
{
	for (const uint32_t body : {0U, 65'534U, 65'535U, 65'536U, 131'072U, UINT32_MAX - 1}) {
		for (const CargoLabel mineral : {CargoLabel{"NORE"}, CargoLabel{"WICE"}, CargoLabel{"RARE"}}) {
			const auto source = Source::FromAsteroid(body, mineral); const auto wire = EncodeCargoSourceAsteroid(source);
			const auto decoded = DecodeCargoPacketSource(Source{Source::Invalid, SourceType::Asteroid}, wire, Money{17}, SaveLoadVersion::AsteroidCargoOrigins);
			REQUIRE(decoded); CHECK(*decoded == source); CHECK(IsValidCargoPacketSource(*decoded, Money{17}, SaveLoadVersion::AsteroidCargoOrigins));
			CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{0}, SourceType::Asteroid}, wire, Money{17}, SaveLoadVersion::AsteroidCargoOrigins));
			CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{0}, SourceType::Asteroid}, wire, Money{17}, SaveLoadVersion::SurfaceSupplyOrders));
		}
	}
	CHECK(EncodeCargoSourceAsteroid(Source::FromAsteroid(0, CargoLabel{"NORE"})).mineral == 0x4E4F5245U);
	const Source scalar{Source::Invalid, SourceType::Asteroid};
	CHECK_FALSE(DecodeCargoPacketSource(scalar, {}, Money{0}, SaveLoadVersion::AsteroidCargoOrigins));
	CHECK_FALSE(DecodeCargoPacketSource(scalar, {0, 0}, Money{0}, SaveLoadVersion::AsteroidCargoOrigins));
	CHECK_FALSE(DecodeCargoPacketSource(scalar, EncodeCargoSourceAsteroid(Source::FromAsteroid(0, CargoLabel{"COAL"})), Money{0}, SaveLoadVersion::AsteroidCargoOrigins));
	const auto legacy = DecodeCargoPacketSource(Source{SourceID{0}, SourceType::Asteroid}, {}, Money{9}, SaveLoadVersion::SurfaceSupplyOrders);
	REQUIRE(legacy); CHECK(legacy->id == 0); CHECK_FALSE(legacy->GetAsteroidOrigin());
}

/** Ordinary scalar provenance, invalid sentinels and non-paying supply credit retain their existing independent rules. */
TEST_CASE("CAPA asteroid fields leave original ordinary source and payment purpose intact", "[asteroid-cargo-wire]")
{
	for (const auto type : {SourceType::Industry, SourceType::Town, SourceType::Headquarters, SourceType::CompanySupply}) {
		for (const SourceID id : {SourceID{0}, SourceID{1}, Source::Invalid}) {
			const Source source{id, type}; const Money credit = type == SourceType::CompanySupply ? Money{0} : Money{123};
			const auto wire = EncodeCargoSourceAsteroid(source); CHECK(wire.body == UINT32_MAX); CHECK(wire.mineral == 0);
			const auto decoded = DecodeCargoPacketSource(source, wire, credit, SaveLoadVersion::AsteroidCargoOrigins);
			REQUIRE(decoded); CHECK(*decoded == source); CHECK(decoded->IsCompanySupply() == source.IsCompanySupply());
			CHECK_FALSE(DecodeCargoPacketSource(source, {0, 0x4E4F5245U}, credit, SaveLoadVersion::AsteroidCargoOrigins));
		}
	}
	CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{1}, SourceType::CompanySupply}, {}, Money{1}, SaveLoadVersion::AsteroidCargoOrigins));
	CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{MAX_COMPANIES}, SourceType::CompanySupply}, {}, Money{0}, SaveLoadVersion::AsteroidCargoOrigins));
}

/** Genuine legacy qualification changes original packet values in their existing vehicle, station and contained descent owners after a whole-family preflight. */
TEST_CASE("Legacy CAPA qualification preserves original containers and refuses missing source bodies atomically", "[asteroid-cargo-wire]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(4, false);
	REQUIRE(Station::CanAllocateItem()); auto station = std::unique_ptr<Station>(Station::Create(TileXYL(20, 20, MapLayer::Surface)));
	station->owner = craft.owner;
	REQUIRE(SpacePod::CanAllocateItem()); auto pod = std::unique_ptr<SpacePod>(SpacePod::Create());
	pod->descent = std::make_unique<SpaceDescentJob>(); pod->descent->cargo = craft.cargo_type;
	const auto production = GetSpaceMiningProductionTile(*GetMutableAsteroidBody(OrbitalBodyID{0})); REQUIRE(production);
	/** @param units Declared component freight. @return Original legacy packet with explicit source/credit/tile, without extraction or payment claims. */
	const auto packet = [&](uint16_t units) {
		REQUIRE(CargoPacket::CanAllocateItem()); auto *made = CargoPacket::Create(StationID::Invalid(), units, Source{SourceID{0}, SourceType::Asteroid});
		made->UpdateLoadingTile(*production); made->AddFeederShare(Money{11}); return made;
	};
	auto *vehicle_packet = packet(6), *station_packet = packet(4), *descent_packet = packet(2);
	craft.cargo.Append(vehicle_packet); station->goods[craft.cargo_type].GetOrCreateData().cargo.Append(station_packet, StationID::Invalid());
	pod->descent->packets.Append(descent_packet, StationID::Invalid());
	const std::array packets{vehicle_packet, station_packet, descent_packet};
	std::array<CargoPacketID, 3> identities;
	for (uint index = 0; index < packets.size(); ++index) identities[index] = packets[index]->index;
	REQUIRE(FindAsteroidBody(GetAsteroidField(), OrbitalBodyID{MAX_ORBITAL_BODIES - 1}) == nullptr);
	REQUIRE(CargoPacket::CanAllocateItem());
	auto bad = std::unique_ptr<CargoPacket>(CargoPacket::Create(StationID::Invalid(), uint16_t{1}, Source{SourceID{MAX_ORBITAL_BODIES - 1}, SourceType::Asteroid}));
	CHECK_FALSE(NormalizeLegacyAsteroidCargoPackets(GetAsteroidField()));
	for (const auto *original : packets) CHECK_FALSE(original->GetSource().GetAsteroidOrigin());
	bad.reset(); REQUIRE(NormalizeLegacyAsteroidCargoPackets(GetAsteroidField()));
	// Native container identity, credit, production and pool slots are unchanged;
	// splitting a qualified original retains its full modern source on resave.
	for (uint index = 0; index < packets.size(); ++index) {
		const auto *original = packets[index]; CHECK(original->index == identities[index]);
		CHECK(original->GetSource() == Source::FromAsteroid(0, CargoLabel{"NORE"})); CHECK(original->GetSourceTile() == *production);
		CHECK(original->GetFeederShare() == 11); CHECK(original->GetFirstStation() == StationID::Invalid());
		CHECK(DecodeCargoPacketSource(Source{original->GetSource().id, original->GetSource().type}, EncodeCargoSourceAsteroid(original->GetSource()), original->GetFeederShare(), SaveLoadVersion::AsteroidCargoOrigins) == original->GetSource());
	}
	CHECK(craft.cargo.TotalCount() == 6); CHECK(station->goods[craft.cargo_type].TotalCount() == 4); CHECK(pod->descent->packets.TotalCount() == 2);
	REQUIRE(HubCargoTransfer::Move(craft.cargo, station->goods[craft.cargo_type].GetData().cargo, 2, craft.tile));
	CHECK(craft.cargo.TotalCount() == 4); CHECK(station->goods[craft.cargo_type].TotalCount() == 6);
	for (const auto *split : *craft.cargo.Packets()) CHECK(split->GetSource() == Source::FromAsteroid(0, CargoLabel{"NORE"}));
	REQUIRE(HubCargoTransfer::Move(craft.cargo, station->goods[craft.cargo_type].GetData().cargo, 4, craft.tile));
	REQUIRE(HubCargoTransfer::Consume(station->goods[craft.cargo_type].GetData().cargo, 10));
	REQUIRE(HubCargoTransfer::Consume(pod->descent->packets, 2));
}

/** A candidate's original body alone qualifies old packet metadata; old no-CDIM admission never becomes a modern-field loophole. */
TEST_CASE("Copied legacy CAPA qualification uses only same-file mineral metadata", "[asteroid-cargo-wire]")
{
	LoadCheckData data; data.map_size_x = data.map_size_y = 64; data.map_layers = MAX_MAP_LAYERS;
	data.asteroid_present = data.asteroid_active = true; data.asteroid_next_body_id = 1;
	data.asteroid_body_rows.push_back({OrbitalBodyID{0}, AsteroidClass::Metallic});
	data.native_cargo_packets_present = data.native_station_cargo_present = data.native_vehicle_cargo_present = true;
	data.native_cargo_packet_rows.push_back({CargoPacketID{0}, 2, Source{SourceID{0}, SourceType::Asteroid}, Money{9},
		TileIndex{to_underlying(MapLayer::Space) * 64 * 64 + 100}, StationID::Invalid()});
	data.native_station_cargo_refs.push_back(1);
	const auto original_pool = CargoPacket::GetNumItems();
	REQUIRE(ValidateSpaceCargoPreviewOwners(data)); REQUIRE(NormalizeLegacyAsteroidCargoPreview(data));
	CHECK(data.native_cargo_packet_rows[0].source == Source::FromAsteroid(0, CargoLabel{"NORE"}));
	CHECK(data.native_cargo_packet_rows[0].legacy_asteroid_origin); CHECK(ValidateSpaceCargoPreviewOwners(data));
	CHECK(NormalizeLegacyAsteroidCargoPreview(data)); CHECK(CargoPacket::GetNumItems() == original_pool);
	data.native_cargo_packet_rows[0].legacy_asteroid_origin = false;
	CHECK_FALSE(ValidateSpaceAsteroidCargoPreview(data)); CHECK_FALSE(NormalizeLegacyAsteroidCargoPreview(data));
	data.native_cargo_packet_rows[0].source = Source{SourceID{0}, SourceType::Asteroid}; data.asteroid_body_rows.clear();
	CHECK_FALSE(NormalizeLegacyAsteroidCargoPreview(data)); CHECK_FALSE(data.native_cargo_packet_rows[0].source.GetAsteroidOrigin());
}
