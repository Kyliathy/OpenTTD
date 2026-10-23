/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_component_edits.cpp Native physical edits invalidate derived tube reach and only affected unused rights.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group15C; occupied-facility refusal preserves native custody.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Real purchased sources and packets retain their owners; prepared floor/materials remain explicit boundary inputs.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "geothermal_tube_switch_fixture.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../pathfinder/yapf/yapf_costcache.hpp"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../order_base.h"
#include "../safeguards.h"

/** Actual original sources, a command-purchased loaded capsule and native station cleanup for physical edit boundaries. */
class TubeComponentEditRig : public GeothermalTubeSwitchRig {
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true}; ///< Explicit normal permission for the separate conventional berth.
public:
	Train *capsule; ///< Original real front; no test relocation or cached duplicate represents it.
	CargoPacketID packet; ///< Original17-unit packet remains in the actual vehicle list through every edit.
	UtilityConsumerID consumer; ///< Current original native/external binding's sole shared recipient.
	/** @param native True binds the original geothermal inlet; false selects the original external invoice owner. */
	explicit TubeComponentEditRig(bool native)
	{
		REQUIRE(Station::GetNumItems() == 0); StationClass::Reset(); RebuildStationKdtree();
		this->capsule = this->Buy(); this->packet = this->Load(*this->capsule);
		const auto bound = native ? BindTubeGeothermalPowerVehicle(this->capsule->index, this->geothermal.source) : BindTubePowerVehicle(this->capsule->index, this->external);
		REQUIRE(bound.has_value()); this->consumer = *bound;
		this->Start(*this->capsule, this->inlets[1]);
	}
	/** Clear actual station cells and ghost identities before this isolated map/company and its real capsule disappear. */
	~TubeComponentEditRig()
	{
		// Native commands retire physical cells first. The final empty station
		// identity is then destroyed through its native owner, never retained in
		// a later fixture's map or substituted for the original vehicle packet.
		while (Station::GetNumItems() != 0) {
			auto *station = *Station::Iterate().begin();
			for (TileIndex tile : station->train_station) {
				// A native station rectangle can include gaps or another facility;
				// only this exact station's actual rail cells belong to cleanup.
				if (!IsRailStationTile(tile) || GetStationIndex(tile) != station->index) continue;
				CHECK(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, tile, tile, false).Succeeded());
			}
			UpdateSignalsInBuffer(); delete station;
		}
	}
	/** Obtain one real shared opportunity without moving the original capsule, retaining its actual unused reservation. */
	void Prepare()
	{
		UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(GetUtilityAllocation(this->consumer).has_value());
		REQUIRE(GetUtilityAllocation(this->consumer)->granted_energy_j > 0);
	}
	/** @param tile Actual target cell. @param connected Expected reach from each original source, independent of the selected recipient or camera. */
	void CheckReach(TileIndex tile, bool connected) const
	{
		CHECK(IsTubePowerConnectedTile(this->external, tile) == connected);
		CHECK(IsTubeGeothermalPowerConnectedTile(this->geothermal.source, tile) == connected);
		CHECK(this->capsule->cargo.StoredCount() == 17);
		REQUIRE(this->capsule->cargo.Packets()->size() == 1);
		CHECK(this->capsule->cargo.Packets()->front()->index == this->packet);
	}
	/** @return Exact original generation/use, external invoice and native inlet identity, excluding legitimate command construction charges and unused-release bookkeeping. */
	auto History() const
	{
		const auto *native = GetUtilityPowerProducer(this->output), *external = GetUtilityPowerProducer(this->external);
		REQUIRE(native != nullptr); REQUIRE(external != nullptr);
		return std::tuple{native->generated_lifetime_j, native->consumed_lifetime_j, external->generated_lifetime_j,
			external->consumed_lifetime_j, ComputeTubePowerDigest(), ComputeTubeGeothermalPowerDigest()};
	}
	/** @return Actual original capsule/packet/order/pose and shared accounting expected to remain identical across a refused command. */
	auto Custody() const
	{
		REQUIRE(this->capsule->orders != nullptr);
		REQUIRE(this->capsule->cargo.Packets()->size() == 1);
		return std::tuple{this->capsule->index, this->capsule->tile, this->capsule->x_pos, this->capsule->y_pos,
			this->capsule->direction, this->capsule->track, this->capsule->cur_speed, this->capsule->subspeed, this->capsule->progress,
			this->capsule->orders->index, this->capsule->cur_real_order_index, this->capsule->current_order.GetType(),
			this->capsule->current_order.GetDestination(), this->capsule->cargo.StoredCount(), this->capsule->cargo.Packets()->front()->index,
			ComputeUtilityPowerDigest(), this->History(), Company::Get(_current_company)->money};
	}
};

/** A hidden real loaded capsule prevents depot demolition before any native reservation, packet, order or original accounting owner can change. */
TEST_CASE("Tube occupied depot demolition preserves original native custody", "[tube-lifecycle-occupied-depot]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	rig.Prepare(); REQUIRE(rig.capsule->IsInDepot());
	const auto depot = rig.capsule->tile;
	const auto identity = Depot::GetByTile(depot)->index;
	const auto before = rig.Custody();
	// Estimates and execution share the native occupancy guard. Neither may
	// release a live approach/grant or silently turn cargo into ownerless stock.
	for (const DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::LandscapeClear>::Do(flags, depot).Failed());
		REQUIRE(Depot::GetByTile(depot) != nullptr); CHECK(Depot::GetByTile(depot)->index == identity);
		CHECK(rig.Custody() == before); rig.CheckReach(rig.inlets[1], true);
	}
}

/** Actual normal motion earns an occupied tube platform; native removal refuses before changing the arriving packet, ordinary order or unused/settled utility owners. */
TEST_CASE("Tube occupied platform removal preserves original native custody", "[tube-lifecycle-occupied-platform]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	const auto platform = TileXYL(32, 25, MapLayer::DeepUnderground);
	// The same unused branch cable as the accepted interchange fixture must
	// leave through its actual owner before a native station occupies this cell.
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, platform, UtilityEnergyDomain::Electricity).Succeeded());
	REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, platform, GetTubeRailType(), Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto station = GetStationIndex(platform);
	uint ticks = 0;
	// Only native orders, power and movement may place the real capsule on the
	// platform. A physical relocation would bypass the occupancy boundary here.
	while (rig.capsule->tile != platform && ticks++ < 1000) rig.Run(std::array{rig.capsule});
	REQUIRE(ticks < 1000); REQUIRE_FALSE(rig.capsule->IsInDepot());
	const auto before = rig.Custody();
	// Both command modes retain the complete live station/capsule and its
	// current source opportunity; a refusal cannot invalidate derived reach.
	for (const DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(Command<Commands::RemoveFromRailStation>::Do(flags, platform, platform, false).Failed());
		REQUIRE(IsRailStationTile(platform)); CHECK(GetStationIndex(platform) == station);
		CHECK(rig.Custody() == before); rig.CheckReach(rig.inlets[1], true);
	}
	// The accepted refusal leaves the platform genuinely occupied. Complete
	// normal travel into the destination depot before the fixture's ordinary
	// station cleanup; deleting/relocating the front would hide that occupancy.
	while ((!rig.capsule->IsInDepot() || rig.capsule->tile != rig.inlets[1]) && ticks++ < 1400) rig.Run(std::array{rig.capsule});
	REQUIRE(ticks < 1400); rig.CheckReach(rig.inlets[1], true);
}

/** Removing/rebuilding the real joining guideway changes both source components and revokes only their unused original work. */
TEST_CASE("Tube physical guideway edits invalidate both original source components", "[tube-component-track-edit]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	const auto track = TileXYL(32, 25, MapLayer::DeepUnderground);
	rig.CheckReach(rig.inlets[1], true); rig.Prepare(); const auto history = rig.History();
	REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, track, Track::Y).Succeeded());
	rig.CheckReach(rig.inlets[1], false);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0); CHECK(rig.History() == history);
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, track, GetTubeRailType(), Track::Y, false).Succeeded());
	rig.CheckReach(rig.inlets[1], true); CHECK(rig.History() == history);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
}

/** A real distant physical component changes native routing but cannot revoke an unrelated original capsule's reservation. */
TEST_CASE("Tube unrelated paid guideway edit retains the original component grant", "[tube-component-unrelated-edit]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	rig.CheckReach(rig.inlets[1], true); rig.Prepare(); const auto history = rig.History();
	const auto grant = GetUtilityAllocation(rig.consumer)->granted_energy_j;
	const auto unrelated = TileXYL(28, 30, MapLayer::DeepUnderground); rig.Open(std::array{unrelated});
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, unrelated, GetTubeRailType(), Track::Y, false).Succeeded());
	rig.CheckReach(unrelated, false); rig.CheckReach(rig.inlets[1], true);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == grant); CHECK(rig.History() == history);
	const auto revision = CSegmentCostCacheBase::s_rail_change_counter;
	// Repeated component observations are read-only. Only native physical
	// events may churn pathfinder topology; queries cannot create map rescans.
	for (uint repeat = 0; repeat < 20; ++repeat) rig.CheckReach(rig.inlets[1], true);
	CHECK(CSegmentCostCacheBase::s_rail_change_counter == revision);
}

/** A real tube platform and joined conventional berth keep native packet identity, while removing the tube cell splits both original components. */
TEST_CASE("Tube native interchange edits invalidate physical platform reach", "[tube-component-interchange-edit]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	const auto platform = TileXYL(32, 25, MapLayer::DeepUnderground), ordinary = TileXYL(28, 25, MapLayer::DeepUnderground);
	// The compact source fixture has a paid cable under this unused branch.
	// Native building admission forbids hiding it under an undeclared station
	// port. Remove that branch through its real owner before the cache boundary;
	// the original source0 cable and inlet at32,24 remain unchanged.
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, platform, UtilityEnergyDomain::Electricity).Succeeded());
	rig.CheckReach(rig.inlets[1], true); rig.Prepare(); auto history = rig.History();
	const auto built = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, platform, GetTubeRailType(), Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false);
	INFO("native platform refusal StringID=" << built.GetErrorMessage().base());
	REQUIRE(built.Succeeded());
	const auto station = GetStationIndex(platform);
	rig.CheckReach(rig.inlets[1], true); CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0); CHECK(rig.History() == history);
	rig.Open(std::array{ordinary, TileAddWrap(ordinary, -1, 0), TileAddWrap(ordinary, 1, 0)});
	Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_RAIL); rig.Prepare(); history = rig.History();
	const auto grant = GetUtilityAllocation(rig.consumer)->granted_energy_j;
	REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, ordinary, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, station, false).Succeeded());
	CHECK(GetStationIndex(ordinary) == station); REQUIRE(GetTubeTerminal(platform, _current_company).has_value());
	CHECK(GetTubeTerminal(platform, _current_company)->station == station);
	CHECK_FALSE(IsTubeRailType(GetRailType(ordinary)));
	rig.CheckReach(ordinary, false); rig.CheckReach(rig.inlets[1], true);
	CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == grant); CHECK(rig.History() == history);
	REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, platform, platform, false).Succeeded());
	rig.CheckReach(rig.inlets[1], false); CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0); CHECK(rig.History() == history);
}

/** Clearing/rebuilding the actual empty destination depot cannot retain stale reach or reuse an already expired work opportunity. */
TEST_CASE("Tube native depot replacement rebuilds both original components", "[tube-component-depot-edit]")
{
	const bool native = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); TubeComponentEditRig rig{native};
	const auto depot = rig.inlets[1]; rig.CheckReach(depot, true); rig.Prepare(); const auto history = rig.History();
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, depot).Succeeded());
	rig.CheckReach(depot, false); CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0); CHECK(rig.History() == history);
	// Native depot construction needs its empty physical floor. Remove only
	// the unused destination branch's installed cable before normal rebuilding;
	// the original source inlet/path at32,24 remains the same paid owner.
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, depot, UtilityEnergyDomain::Electricity).Succeeded());
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, GetTubeRailType(), DiagDirection::NW).Succeeded());
	REQUIRE(Depot::GetByTile(depot) != nullptr);
	rig.CheckReach(depot, true); CHECK(GetUtilityAllocation(rig.consumer)->granted_energy_j == 0); CHECK(rig.History() == history);
}
