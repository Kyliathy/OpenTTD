/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file portal_build.cpp Actual sealed-pair transaction and unchanged refusal boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 4 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../landscape.h"
#include "../rail.h"
#include "../road.h"
#include "../bridge_map.h"
#include "../economy_type.h"
#include "../economy_func.h"
#include "../core/backup_type.hpp"
#include "../table/strings.h"
#include "../safeguards.h"

/** Exact public endpoint fields for comparisons across estimates and refused commands. */
struct PortalTestTile {
	std::array<uint8_t, 8> bytes; ///< Actual public type/height/metadata bytes.
	uint16_t m2, m8; ///< Actual wider metadata fields.
	/** @param index Actual fixture-owned endpoint. */
	explicit PortalTestTile(TileIndex index) : bytes{Tile{index}.type(), Tile{index}.height(), Tile{index}.m1(), Tile{index}.m3(), Tile{index}.m4(), Tile{index}.m5(), Tile{index}.m6(), Tile{index}.m7()}, m2(Tile{index}.m2()), m8(Tile{index}.m8()) {}
	/** @param other Snapshot of the same actual endpoint. @return Whether every stored field is identical. */
	bool operator==(const PortalTestTile &other) const = default;
};

/** Native rail and road create physical sealed housing without granting any geological clearing credit. */
TEST_CASE("Native portal housing preserves solid target geology", "[portal-build]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState();
	ResetRailTypes(); ResetRoadTypes();
	const auto source = TileXYL(20, 20, MapLayer::Surface);
	MapLayer destination = MapLayer::Underground;
	TransportType transport = TransportType::Rail;
	SECTION("Surface rail into Underground") {}
	SECTION("Surface road into Underground") { transport = TransportType::Road; }
	SECTION("Surface elevator leaves intervening layer untouched") { transport = TransportType::Road; destination = MapLayer::DeepUnderground; }
	SECTION("Underground rail into Deep") { destination = MapLayer::DeepUnderground; }
	const auto actual_source = transport == TransportType::Rail && destination == MapLayer::DeepUnderground ? MultiLayerMap::TileInLayer(source, MapLayer::Underground) : source;
	if (GetTileLayer(actual_source) == MapLayer::Underground) fixture.Open(std::array{actual_source, TileAddByDiagDir(actual_source, DiagDirection::NW)});
	const auto target = MultiLayerMap::TileInLayer(actual_source, destination);
	const auto intervening = PortalTestTile(MultiLayerMap::TileInLayer(source, MapLayer::Underground));
	const PortalTestTile source_before(actual_source), target_before(target);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto rng = _random;
	const auto funds = Company::Get(_current_company)->money;
	const auto rail_count = Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL];
	AutoRestoreBackup price(_price[Price::BuildTunnel], Money{17});
	const auto quote = CmdBuildLayerPortal({}, actual_source, transport, destination, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() >= 68);
	CHECK(PortalTestTile(actual_source) == source_before); CHECK(PortalTestTile(target) == target_before);
	CHECK(CaptureLayerPortalWorkRecords().empty());
	const auto result = CmdBuildLayerPortal({DoCommandFlag::Execute}, actual_source, transport, destination, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	REQUIRE(result.Succeeded()); CHECK(result.GetCost() == quote.GetCost());
	CHECK(GetLayerPortalPairTile(actual_source) == target);
	CHECK(GetLayerPortalWorkState(actual_source) == LayerPortalWorkState::Sealed);
	CHECK(GetLayerPortalWorkRecord(actual_source) == GetLayerPortalWorkRecord(target));
	CHECK(GetGeologyWorldState().terrain.Get(target) == ExcavationState::Solid);
	CHECK(CheckExcavationConstruction(std::array{target}).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	CHECK(Company::Get(_current_company)->money == funds); // The enclosing command kernel charges the returned complete cost; direct command bodies never debit cash twice.
	CHECK(_random.state[0] == rng.state[0]); CHECK(_random.state[1] == rng.state[1]);
	if (transport == TransportType::Rail) CHECK(Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL] == rail_count + 2 * GetLayerPortalPieceCount(transport));
	// Only a direct road elevator skips an intervening plane; its metadata must survive both housing writes unchanged.
	if (transport == TransportType::Road && destination == MapLayer::DeepUnderground) CHECK(PortalTestTile(MultiLayerMap::TileInLayer(source, MapLayer::Underground)) == intervening);
	ResetLayerPortalWorkState();
}

/** Both-end preparation refuses before source mutation, including the shared two-clear budget. */
TEST_CASE("Native portal second end refusal is atomic", "[portal-build]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes();
	const auto source = TileXYL(20, 20, MapLayer::Surface);
	const auto target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	SECTION("Second end bridge") { SetBridgeMiddle(target, Axis::X); }
	SECTION("Second end incompatible structure") { MakeRailLayerPortal(target, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL); }
	SECTION("Two ordinary clears must share one remaining allowance") {
		fixture.Open(std::array{target, TileAddByDiagDir(target, DiagDirection::SE)});
		Company::Get(_current_company)->clear_limit = 1U << 16;
	}
	const PortalTestTile before_source(source), before_target(target);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto work = ComputeLayerPortalWorkDigest();
	const auto funds = Company::Get(_current_company)->money;
	const auto clearing = Company::Get(_current_company)->clear_limit;
	const auto infrastructure = Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL];
	const auto rng = _random;
	CHECK(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
	CHECK(PortalTestTile(source) == before_source); CHECK(PortalTestTile(target) == before_target);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo); CHECK(ComputeLayerPortalWorkDigest() == work);
	CHECK(Company::Get(_current_company)->money == funds); CHECK(Company::Get(_current_company)->clear_limit == clearing);
	CHECK(Company::Get(_current_company)->infrastructure.rail[RAILTYPE_RAIL] == infrastructure);
	CHECK(_random.state[0] == rng.state[0]); CHECK(_random.state[1] == rng.state[1]);
	const auto edge = TileXYL(20, 0, MapLayer::Surface);
	CHECK(CmdBuildLayerPortal({DoCommandFlag::Execute}, edge, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
	ResetLayerPortalWorkState();
}

/** Actual owner capacity can be abandoned without publication; its hard limit refuses a new physical pair unchanged. */
TEST_CASE("Native portal capacity is reserved before any tile mutation", "[portal-build]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRailTypes();
	Map::Allocate(128, 128, MAX_MAP_LAYERS);
	// Actual map/geology owners are initialized together before creating any records at the larger capacity-test dimensions.
	for (auto layer : {MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
	REQUIRE(GenerateWorldGeology(420) == GeologyError::None);
	const auto source = TileXYL(80, 80, MapLayer::Surface);
	const auto target = MultiLayerMap::TileInLayer(source, MapLayer::Underground);
	{
		auto prepared = PrepareLayerPortalWorkPair(source, _current_company);
		CHECK(prepared.Ready()); CHECK(CaptureLayerPortalWorkRecords().empty());
	}
	CHECK(CaptureLayerPortalWorkRecords().empty());
	std::vector<LayerPortalWorkRecord> records;
	// Fill the real bounded owner with distinct reciprocal actual map pairs; no invented keys or shadow allocator simulates the capacity refusal.
	for (uint y = 1; y <= 64; ++y) {
		for (uint x = 1; x <= 64; ++x) {
			const auto key = TileXYL(x, y, MapLayer::Surface);
			const auto pair = MultiLayerMap::TileInLayer(key, MapLayer::Underground);
			MakeRailLayerPortal(key, _current_company, DiagDirection::SE, MapLayer::Underground, RAILTYPE_RAIL);
			MakeRailLayerPortal(pair, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL);
			LayerPortalWorkRecord record;
			record.pair_key = key; record.company = _current_company; record.revision = 1;
			record.state = LayerPortalWorkState::Sealed;
			records.push_back(record);
		}
	}
	REQUIRE(records.size() == MAX_LAYER_PORTAL_WORK_PAIRS);
	REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	const PortalTestTile before_source(source), before_target(target);
	const auto work = ComputeLayerPortalWorkDigest(), geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto rng = _random;
	CHECK_FALSE(PrepareLayerPortalWorkPair(source, _current_company).Ready());
	CHECK(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Failed());
	CHECK(PortalTestTile(source) == before_source); CHECK(PortalTestTile(target) == before_target);
	CHECK(ComputeLayerPortalWorkDigest() == work); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == rng.state[0]); CHECK(_random.state[1] == rng.state[1]);
	ResetLayerPortalWorkState();
}

/** Road tunnel/elevator housings need actual excavated road approaches and can never become a TBM origin. */
TEST_CASE("Road portal connects only excavated owned roads and refuses TBM commissioning", "[portal-road]")
{
	ExcavationFixture fixture;
	ResetLayerPortalWorkState(); ResetRoadTypes();
	MapLayer source_layer = MapLayer::Surface, target_layer = MapLayer::Underground;
	SECTION("Surface Underground tunnel") {}
	SECTION("Surface Deep elevator") { target_layer = MapLayer::DeepUnderground; }
	SECTION("Underground Deep elevator") { source_layer = MapLayer::Underground; target_layer = MapLayer::DeepUnderground; }
	const auto source = TileXYL(20, 20, source_layer), target = MultiLayerMap::TileInLayer(source, target_layer);
	if (source_layer == MapLayer::Underground) fixture.Open(std::array{source, TileAddByDiagDir(source, DiagDirection::NW)});
	REQUIRE(CmdBuildLayerPortal({DoCommandFlag::Execute}, source, TransportType::Road, target_layer, DiagDirection::SE, INVALID_RAILTYPE, ROADTYPE_ROAD).Succeeded());
	CHECK(GetTileTrackStatus(source, TransportType::Road, RoadTramType::Road).trackdirs.None());
	CHECK(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, target, true).GetErrorMessage() == STR_ERROR_EXCAVATION_SOLID_ROCK);
	LayerPortalCommissionRequest request;
	request.portal = source; request.proposal.company = _current_company; request.proposal.model_id = 0;
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidPair);
	CHECK(GetGeologyWorldState().terrain.Get(target) == ExcavationState::Solid);
	fixture.Open(std::array{target, TileAddByDiagDir(target, DiagDirection::SE)});
	// The public road constructor supplies real same-company axis roads on the prepared floors; connection creates neither road nor excavation itself.
	for (TileIndex end : {source, target}) MakeRoadNormal(TileAddByDiagDir(end, GetLayerPortalDoorDirection(end)), AxisToRoadBits(Axis::Y), ROADTYPE_ROAD, INVALID_ROADTYPE, TownID::Invalid(), _current_company, OWNER_NONE);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	REQUIRE(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, target, true).Succeeded());
	CHECK(GetTileTrackStatus(source, TransportType::Road, RoadTramType::Road).trackdirs.Any());
	CHECK(GetTileTrackStatus(target, TransportType::Road, RoadTramType::Road).trackdirs.Any());
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidPair);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	ResetLayerPortalWorkState();
}
