/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_refusal.cpp Actual loaded capsule refuses valid opposite-layer tubes and original conventional shafts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15A.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Group12B; real road/elevator shafts retain the capsule domain boundary.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor, commissioned portal and loaded17-unit stock remain boundary inputs, not a bootstrap/useful-route proof.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "movement_fixture.h"
#include "../cargopacket.h"
#include "../tile_cmd.h"
#include "../utility_power.h"
#include "../road.h"
#include "../road_map.h"
#include "../safeguards.h"

/** A real loaded powered capsule cannot jump to another valid rock layer or enter either original conventional shaft endpoint. */
TEST_CASE("Tube layer refusal preserves loaded native custody despite otherwise valid tube targets", "[tube-refusal-cross-layer]")
{
	TubePrototypeRig world;
	auto *capsule = world.BuyCapsule();
	world.InstallPaidFixturePower(*capsule);
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	packet->UpdateLoadingTile(capsule->tile); capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	capsule->ConsistChanged(CCF_LOADUNLOAD);
	const auto same = TileXYL(45, 42, MapLayer::Underground);
	const auto other = MultiLayerMap::TileInLayer(same, MapLayer::DeepUnderground);
	world.BuildTrack(same, world.tube); world.BuildTrack(other, world.tube);
	REQUIRE(CheckTubeVehicleAdmission(capsule->engine_type, other, capsule->cargo_type).Succeeded());
	REQUIRE(CanTubeTrainEnterTile(*capsule, same));
	const auto connections = GetTubePowerConnections(); REQUIRE(connections.size() == 1);
	const auto &source = connections.front(); REQUIRE(IsTubePowerConnected(source.producer, *capsule));
	const auto portal = source.portal, upper = GetLayerPortalPairTile(portal);
	REQUIRE(IsLayerPortalTile(portal)); REQUIRE(IsLayerPortalTile(upper));
	std::vector<TileIndex> targets{other, portal, upper};
	ResetRoadTypes();
	uint offset = 0;
	// Exercise every supported road shaft, including both elevator geometries,
	// as an actually Connected pair. Otherwise a sealed door or missing road
	// could explain refusal instead of the capsule's independent movement domain.
	for (const auto &[from, to] : {std::pair{MapLayer::Surface, MapLayer::Underground},
		std::pair{MapLayer::Surface, MapLayer::DeepUnderground}, std::pair{MapLayer::Underground, MapLayer::DeepUnderground}}) {
		const auto start = TileXYL(24 + offset * 4, 24, from), end = MultiLayerMap::TileInLayer(start, to);
		++offset;
		// These explicit open floors/approaches remain boundary inputs. Portal
		// creation/connect commands must not excavate or move the loaded capsule.
		std::vector<TileIndex> floors{end, TileAddByDiagDir(end, DiagDirection::SE)};
		if (from != MapLayer::Surface) {
			floors.push_back(start); floors.push_back(TileAddByDiagDir(start, DiagDirection::NW));
		}
		world.Open(floors);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, start, TransportType::Road, to,
			DiagDirection::SE, INVALID_RAILTYPE, ROADTYPE_ROAD).Succeeded());
		for (const auto endpoint : {start, end}) { // Native same-owner axis road approaches make both physical housings usable by their intended domain.
			MakeRoadNormal(TileAddByDiagDir(endpoint, GetLayerPortalDoorDirection(endpoint)), AxisToRoadBits(Axis::Y),
				ROADTYPE_ROAD, INVALID_ROADTYPE, TownID::Invalid(), _current_company, OWNER_NONE);
		}
		REQUIRE(CmdChangeLayerPortalConnection(DoCommandFlag::Execute, start, true).Succeeded());
		REQUIRE(GetLayerPortalWorkState(start) == LayerPortalWorkState::Connected);
		REQUIRE(GetTileTrackStatus(start, TransportType::Road, RoadTramType::Road).trackdirs.Any());
		REQUIRE(GetTileTrackStatus(end, TransportType::Road, RoadTramType::Road).trackdirs.Any());
		targets.push_back(start); targets.push_back(end);
	}
	// Broad cached mechanics cannot override the original layer/portal domain
	// rule. Both target tube cells are legal and built; neither solid ground,
	// model date nor missing source explains the refusal being tested.
	capsule->compatible_railtypes.Set(world.tube); capsule->compatible_railtypes.Set(RAILTYPE_RAIL);
	/** @return Exact original physical/order/packet/source/geology/cash/time/RNG observations; refusal may change none of these owners. */
	const auto observe = [&] {
		return std::tuple{capsule->index, capsule->tile, capsule->x_pos, capsule->y_pos, capsule->z_pos,
			capsule->direction, capsule->track, capsule->cur_speed, capsule->subspeed, capsule->progress,
			capsule->dest_tile, capsule->current_order.GetType(), capsule->current_order.GetDestination(),
			capsule->cargo.StoredCount(), capsule->cargo.Packets()->front()->index,
			Vehicle::GetNumItems(), CargoPacket::GetNumItems(), Company::Get(_current_company)->money,
			ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
			ComputeLayerPortalWorkDigest(), ComputeGeologyStateDigest(GetGeologyWorldState()), TimerGameTick::counter,
			_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	};
	const auto before = observe();
	REQUIRE(IsTrainOnItsTiles(capsule));
	// Probe both original rail ends, all real road/elevator ends and an
	// otherwise admissible Deep tube; a native loaded part cannot use a shaft.
	// The existing native tile-entry owner must reject every candidate without
	// moving the original capsule or silently granting cross-layer transport.
	for (const auto target : targets) {
		CHECK_FALSE(CanTubeTrainEnterTile(*capsule, target));
		CHECK(VehicleEnterTile(capsule, target, 0, 0).Test(VehicleEnterTileState::CannotEnter));
		CHECK(observe() == before);
		CHECK(IsTrainOnItsTiles(capsule));
	}
	CHECK(capsule->cargo.Packets()->front()->index == packet->index);
	CHECK(IsTubePowerConnected(source.producer, *capsule));
}
