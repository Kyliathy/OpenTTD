/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_fixture.h The exchange rig plus the native spoil receivers: both definitions installed through
 * their real owner, and receiver industries placed tile by tile in the pool within the origin station's catchment,
 * as a funded receiver would stand there. The industry construction command needs a town, which these fixtures lack.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md
 */

#ifndef TESTS_SPOIL_RECEIVER_FIXTURE_H
#define TESTS_SPOIL_RECEIVER_FIXTURE_H

#include "tbm_exchange_fixture.h"
#include "../spoil_receiver.h"
#include "../industry.h"
#include "../industry_map.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../station_func.h"
#include "../company_base.h"
#include "../town.h"
#include "../economy_func.h"
#include "../economy_base.h"
#include "../settings_type.h"
#include "../core/backup_type.hpp"

/** The exchange rig with installed receiver definitions; every receiver it builds is removed with it. */
class SpoilReceiverRig : public TBMExchangeRig {
	decltype(_industry_mngr.mappings) industry_mapping = _industry_mngr.mappings; ///< The caller's IIDS configuration.
	decltype(_industile_mngr.mappings) tile_mapping = _industile_mngr.mappings; ///< The caller's TIDS configuration.
public:
	TileIndex origin_tile = INVALID_TILE; ///< The origin's real rail station tile.
	Town *town = nullptr; ///< The origin's town, which every delivery credits.

	/**
	 * Install the receiver definitions and give the origin a real station tile.
	 * @param segments Straight cuts of the project.
	 */
	explicit SpoilReceiverRig(size_t segments = 1) : TBMExchangeRig(segments)
	{
		this->InstallReceivers();
	}

	/**
	 * Install the receiver definitions and give the origin a real station tile, for a project whose route turns as given.
	 * @param turns Change of heading at each advance (see TBMProjectRig).
	 */
	explicit SpoilReceiverRig(const std::vector<DirDiff> &turns) : TBMExchangeRig(turns)
	{
		this->InstallReceivers();
	}

	/** Install the receiver definitions, then the origin's station tile and town. */
	void InstallReceivers()
	{
		REQUIRE(Industry::GetNumItems() == 0);
		ResetIndustries();
		REQUIRE(InitialiseSpoilReceiverDefinitions(this->fixture.package));
		this->origin_tile = this->MakeOriginStationTile();
		REQUIRE(Town::CanAllocateItem());
		this->town = Town::Create(TileXYL(10, 10, MapLayer::Surface));
		this->fixture.origin->town = this->town;
	}

	/** Remove the receivers and restore the original definitions and mappings. */
	~SpoilReceiverRig()
	{
		_industry_pool.CleanPool();
		_town_pool.CleanPool();
		Industry::industries = {}; // Cleaning the pool skips the destructors that would empty the per-type lists.
		ResetSpoilReceiverState();
		ResetIndustries();
		_industry_mngr.mappings = std::move(this->industry_mapping);
		_industile_mngr.mappings = std::move(this->tile_mapping);
	}

	/**
	 * Place a receiver industry on the Surface from its definition's layout, accepting its spoil, founded by the
	 * current company, and give it its state as DoCreateNewIndustry does.
	 * @param kind Receiver kind.
	 * @param x Western layout corner.
	 * @param y Northern layout corner.
	 * @return The industry.
	 */
	Industry *BuildReceiver(SpoilReceiverKind kind, uint x, uint y) const
	{
		const IndustryType type = GetSpoilReceiverIndustryType(kind);
		REQUIRE(type != IT_INVALID);
		const IndustrySpec *spec = GetIndustrySpec(type);
		REQUIRE(Industry::CanAllocateItem());
		const TileIndex corner = TileXYL(x, y, MapLayer::Surface);
		Industry *industry = Industry::Create(corner);
		industry->type = type;
		industry->founder = _current_company;
		Industry::industries[type].insert(industry->index);
		// Every layout cell becomes an industry tile of this industry, extending its area.
		for (const auto &cell : spec->layouts.front()) {
			const TileIndex tile = TileXYL(x + cell.ti.x, y + cell.ti.y, MapLayer::Surface);
			MakeIndustry(tile, industry->index, cell.gfx, 0, WaterClass::Invalid);
			industry->location.Add(tile);
		}
		industry->accepted.emplace_back().cargo = spec->accepts_cargo[0];
		RegisterSpoilReceiver(*industry);
		return industry;
	}

	/** Recompute the origin's catchment and acceptance, as the daily station loop would after a receiver opens. */
	void RefreshOrigin() const
	{
		this->fixture.origin->RecomputeCatchment();
		UpdateStationAcceptance(this->fixture.origin, false);
	}

	/**
	 * @param industry Receiver industry.
	 * @return Its spoil stock.
	 */
	static uint16_t &Stock(Industry &industry)
	{
		REQUIRE(!industry.accepted.empty());
		return industry.accepted.front().waiting;
	}

	/**
	 * Tick a receiver through whole intervals.
	 * @param industry Receiver industry.
	 * @param intervals Complete intervals to run.
	 */
	static void RunIntervals(Industry &industry, uint intervals)
	{
		for (uint tick = 0; tick < intervals * SPOIL_RECEIVER_INTERVAL_TICKS; ++tick) AdvanceSpoilReceiver(industry);
	}

	/** @return The current company's cash. */
	static Money Cash() { return Company::Get(_current_company)->money; }
};

/**
 * Stop a train at a station as an arriving train does, run the station's loading until it finishes, then leave: the
 * payment is settled and the station forgets the train, as LeaveStation does.
 * @param train Train standing on one of the station's tiles.
 * @param station The station.
 * @param unload Order unload type.
 * @param load Order load type.
 */
inline void RunStationStop(Train *train, Station *station, OrderUnloadType unload, OrderLoadType load)
{
	train->last_station_visited = station->index;
	train->current_order.MakeLoading(false);
	train->current_order.SetUnloadType(unload);
	train->current_order.SetLoadType(load);
	PrepareUnload(train);
	for (int step = 0; step < 1000 && !train->vehicle_flags.Test(VehicleFlag::LoadingFinished); ++step) LoadUnloadStation(station);
	REQUIRE(train->vehicle_flags.Test(VehicleFlag::LoadingFinished));
	delete train->cargo_payment;
	station->loading_vehicles.remove(train);
}

/** A rig whose hauler stands loaded at its origin, with an Earth and a Rock receiver in the origin's catchment. */
struct LoadedAtOrigin {
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< No filling text effect without a GUI.
	SpoilReceiverRig rig; ///< Receivers, cutter and hauler.
	Industry *earth = nullptr; ///< Earth receiver.
	Industry *rock = nullptr; ///< Rock crusher.
	ExcavationCutResult cut{}; ///< The spoil the hauler carries.

	/** Bore, exchange and drive the hauler to its origin. */
	LoadedAtOrigin()
	{
		this->earth = this->rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->rock = this->rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		this->rig.RefreshOrigin();
		this->cut = this->rig.BoreOneCut();
		REQUIRE(this->rig.Tick().phase == TBMPhase::WaitForEmpties);
		this->rig.Meet();
		REQUIRE(this->rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
		REQUIRE(this->rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
		this->rig.PlaceOnTrack(this->rig.hauler, TileXYL(25, 20, MapLayer::Surface));
		REQUIRE(this->rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
		this->rig.PlaceOnTrack(this->rig.hauler, this->rig.origin_tile);
		REQUIRE(this->rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
	}

	/**
	 * Unload the hauler at its origin and leave without loading anything back.
	 * @param unload Order unload type.
	 */
	void Unload(OrderUnloadType unload) const
	{
		RunStationStop(this->rig.hauler, this->rig.fixture.origin, unload, OrderLoadType::NoLoad);
	}

	/** @return Tonnes the hauler still carries. */
	uint64_t Aboard() const { return TBMExchangeRig::Aboard(this->rig.hauler); }
};

#endif /* TESTS_SPOIL_RECEIVER_FIXTURE_H */
