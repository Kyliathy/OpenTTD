/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_wire.cpp Versioned CAPA source fields and in-place same-file legacy qualification.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 15 and 17B; every case names its accepted source format (merged 446 for typed origins).
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
#include "save_supply_wire_fixture.h"
#include <filesystem>
#include <fstream>
#include "../safeguards.h"

/** @param dialect Original writer. @param version Unchanged envelope version. @return That accepted source's cargo format. */
static CargoSourceFormat AcceptedFormat(SaveDialect dialect, uint16_t version)
{
	return CargoSourceFormat::FromLineage({SaveLineageStatus::Accepted, dialect, version});
}

/** Mandatory payload keys cannot silently default or cross the genuine source-version boundary. */
TEST_CASE("CAPA asteroid source schema requires exact era fields", "[asteroid-cargo-wire]")
{
	const std::vector<std::string_view> old{"count", "source_type", "source_id", "feeder_share", "source", "source_xy"};
	auto modern = old; modern.push_back("asteroid_body"); modern.push_back("asteroid_mineral");
	CHECK(ValidateCargoPacketSourceSchema(old, SaveLoadVersion::SurfaceSupplyOrders));
	CHECK(ValidateCargoPacketSourceSchema(modern, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(old, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(modern, SaveLoadVersion::SurfaceSupplyOrders));
	// Every authority key independently remains mandatory; a duplicate key also
	// cannot impersonate the absent scalar/payload identity or original credit.
	for (size_t index = 0; index < modern.size(); ++index) {
		auto missing = modern; missing.erase(missing.begin() + index);
		CHECK_FALSE(ValidateCargoPacketSourceSchema(missing, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
		auto duplicate = modern; duplicate.push_back(modern[index]);
		CHECK_FALSE(ValidateCargoPacketSourceSchema(duplicate, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	}
}

/** Full-width IDs and explicit portable mineral bytes survive the original source transport without aliasing ordinary native IDs. */
TEST_CASE("CAPA natural origin codec preserves full IDs and rejects dual or wrong-era payloads", "[asteroid-cargo-wire]")
{
	for (const uint32_t body : {0U, 65'534U, 65'535U, 65'536U, 131'072U, UINT32_MAX - 1}) {
		for (const CargoLabel mineral : {CargoLabel{"NORE"}, CargoLabel{"WICE"}, CargoLabel{"RARE"}}) {
			const auto source = Source::FromAsteroid(body, mineral); const auto wire = EncodeCargoSourceAsteroid(source);
			const auto decoded = DecodeCargoPacketSource(Source{Source::Invalid, SourceType::Asteroid}, wire, Money{17}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION));
			REQUIRE(decoded); CHECK(*decoded == source); CHECK(IsValidCargoPacketSource(*decoded, Money{17}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
			CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{0}, SourceType::Asteroid}, wire, Money{17}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
			CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{0}, SourceType::Asteroid}, wire, Money{17}, SaveLoadVersion::SurfaceSupplyOrders));
		}
	}
	CHECK(EncodeCargoSourceAsteroid(Source::FromAsteroid(0, CargoLabel{"NORE"})).mineral == 0x4E4F5245U);
	const Source scalar{Source::Invalid, SourceType::Asteroid};
	CHECK_FALSE(DecodeCargoPacketSource(scalar, {}, Money{0}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	CHECK_FALSE(DecodeCargoPacketSource(scalar, {0, 0}, Money{0}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	CHECK_FALSE(DecodeCargoPacketSource(scalar, EncodeCargoSourceAsteroid(Source::FromAsteroid(0, CargoLabel{"COAL"})), Money{0}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
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
			const auto decoded = DecodeCargoPacketSource(source, wire, credit, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION));
			REQUIRE(decoded); CHECK(*decoded == source); CHECK(decoded->IsCompanySupply() == source.IsCompanySupply());
			CHECK_FALSE(DecodeCargoPacketSource(source, {0, 0x4E4F5245U}, credit, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
		}
	}
	CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{1}, SourceType::CompanySupply}, {}, Money{1}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
	CHECK_FALSE(DecodeCargoPacketSource(Source{SourceID{MAX_COMPANIES}, SourceType::CompanySupply}, {}, Money{0}, AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)));
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
		CHECK(DecodeCargoPacketSource(Source{original->GetSource().id, original->GetSource().type}, EncodeCargoSourceAsteroid(original->GetSource()), original->GetFeederShare(), AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION)) == original->GetSource());
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

/** One number, two meanings: the same CAPA key set is right for S441 and wrong for merged 441, and the writer decides. */
TEST_CASE("CAPA source format follows the accepted writer, never the number", "[asteroid-cargo-wire]")
{
	const std::vector<std::string_view> plain{"count", "source_type", "source_id", "feeder_share", "source", "source_xy"};
	auto typed = plain; typed.push_back("asteroid_body"); typed.push_back("asteroid_mineral");
	auto half = plain; half.push_back("asteroid_body");
	const auto merged441 = AcceptedFormat(SaveDialect::Merged, MERGED_SAVE_LINEAGE_VERSION);
	const auto donor434 = AcceptedFormat(SaveDialect::DonorB, 434);
	const auto merged446 = AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION);
	CHECK_FALSE(merged441.typed_asteroid_origins); CHECK_FALSE(donor434.typed_asteroid_origins);
	CHECK(merged446.typed_asteroid_origins);
	// The typed source needs both keys exactly once; every untyped source forbids them.
	CHECK(ValidateCargoPacketSourceSchema(typed, merged446));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(plain, merged446));
	CHECK_FALSE(ValidateCargoPacketSourceSchema(half, merged446));
	for (const auto &untyped : {merged441, donor434}) {
		CHECK(ValidateCargoPacketSourceSchema(plain, untyped));
		CHECK_FALSE(ValidateCargoPacketSourceSchema(typed, untyped));
		CHECK_FALSE(ValidateCargoPacketSourceSchema(half, untyped));
	}
	// A high body ID decodes only where typed origins exist. Merged 441 keeps D's
	// legacy scalar for the later same-file upgrade, and refuses any typed payload.
	const auto high = Source::FromAsteroid(UINT32_MAX - 1, CargoLabel{"NORE"});
	const Source unqualified{Source::Invalid, SourceType::Asteroid};
	CHECK(DecodeCargoPacketSource(unqualified, EncodeCargoSourceAsteroid(high), Money{0}, merged446) == high);
	CHECK_FALSE(DecodeCargoPacketSource(unqualified, EncodeCargoSourceAsteroid(high), Money{0}, merged441));
	const Source legacy{SourceID{3}, SourceType::Asteroid};
	CHECK(DecodeCargoPacketSource(legacy, {}, Money{0}, merged441) == legacy);
	CHECK_FALSE(DecodeCargoPacketSource(legacy, {}, Money{0}, merged446)); // A typed source never carries an unqualified scalar.
	// An ordinary source with an illicit asteroid payload refuses in every format.
	const Source town{TownID{2}, SourceType::Town};
	for (const auto &format : {merged441, donor434, merged446}) {
		CHECK(DecodeCargoPacketSource(town, {}, Money{5}, format) == town);
		CHECK_FALSE(DecodeCargoPacketSource(town, {7, 0x4E4F5245U}, Money{5}, format));
	}
	// Bought supplies belong to A's era onwards; historical B never wrote that purpose.
	const Source bought{CompanyID{1}, SourceType::CompanySupply};
	CHECK(DecodeCargoPacketSource(bought, {}, Money{0}, merged441) == bought);
	CHECK_FALSE(DecodeCargoPacketSource(bought, {}, Money{0}, donor434));
}

/** Real typed-origin CAPA rows, from development-build saves that are themselves never loaded (m2o28 Group 17B):
 * their CAPA shape is the one merged 446 writes, so every row decodes with the 446 format and typed rows refuse under 441.
 */
TEST_CASE("CAPA rows of real typed-origin saves decode through the merged 446 format", "[asteroid-cargo-wire]")
{
#ifndef WITH_LIBLZMA
	WARN("The typed-origin originals are OTTX; this build has no LZMA decoder"); return;
#else
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	const auto space = AcceptedFormat(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION); // The typed-origin CAPA shape.
	const auto merged441 = AcceptedFormat(SaveDialect::Merged, MERGED_SAVE_LINEAGE_VERSION);
	size_t typed_rows = 0;
	// Each committed original is decompressed by a fresh codec and its CAPA table
	// read field by field from its own descriptor; no native handler runs.
	for (const char *name : {"typed-origin-packets-445.sav", "mining-continuation-445.sav", "field-lifecycle-445.sav"}) {
		INFO(name);
		std::ifstream input(root / "src/tests/fixtures/saveload/space-fixes-445" / name, std::ios::binary); REQUIRE(input.good());
		const std::vector<uint8_t> raw{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
		const auto decoded = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(raw));
		const auto chunks = SupplyWireChunks(decoded); // Kept alive: SupplyWireFind returns a reference into it.
		const auto &cargo = SupplyWireFind(chunks, "CAPA");
		REQUIRE_FALSE(cargo.rows.empty());
		// The first framed row is the descriptor: (type, gamma name length, name) until type 0.
		std::vector<std::pair<std::string, uint8_t>> columns;
		size_t cursor = cargo.rows[0].payload;
		/** @return Next descriptor byte, bounded by the header row. */
		const auto byte = [&]() { REQUIRE(cursor < cargo.rows[0].end); return decoded[cursor++]; };
		for (uint8_t type = byte(); type != 0; type = byte()) {
			const auto length = ReadSaveGamma(byte);
			std::string column;
			for (uint32_t index = 0; index < length; ++index) column.push_back(static_cast<char>(byte()));
			columns.emplace_back(std::move(column), type);
		}
		std::vector<std::string_view> names;
		for (const auto &column : columns) names.push_back(column.first);
		CHECK(ValidateCargoPacketSourceSchema(names, space));
		CHECK_FALSE(ValidateCargoPacketSourceSchema(names, merged441));
		// Every real packet row carries flat scalars in descriptor order; empty rows are pool holes.
		for (size_t index = 1; index < cargo.rows.size(); ++index) {
			const auto &row = cargo.rows[index];
			if (row.payload == row.end) continue;
			Source scalar; CargoSourceAsteroidWire wire; Money credit{0};
			size_t offset = row.payload;
			// Walk the descriptor once per row, picking out the five source fields.
			for (const auto &[column, type] : columns) {
				static constexpr uint8_t widths[]{0, 1, 1, 2, 2, 4, 4, 8, 8, 2};
				REQUIRE(type < std::size(widths));
				const auto value = SupplyWireGet(decoded, offset, widths[type]);
				if (column == "source_type") scalar.type = static_cast<SourceType>(value);
				if (column == "source_id") scalar.id = static_cast<SourceID>(value);
				if (column == "feeder_share") credit = Money{static_cast<int64_t>(value)};
				if (column == "asteroid_body") wire.body = static_cast<uint32_t>(value);
				if (column == "asteroid_mineral") wire.mineral = static_cast<uint32_t>(value);
				offset += widths[type];
			}
			REQUIRE(offset == row.end);
			const auto source = DecodeCargoPacketSource(scalar, wire, credit, space);
			REQUIRE(source.has_value());
			if (!source->GetAsteroidOrigin()) continue;
			++typed_rows;
			CHECK_FALSE(DecodeCargoPacketSource(scalar, wire, credit, merged441));
		}
	}
	CHECK(typed_rows > 0); // The typed-origin fixture carries natural packets.
#endif
}

