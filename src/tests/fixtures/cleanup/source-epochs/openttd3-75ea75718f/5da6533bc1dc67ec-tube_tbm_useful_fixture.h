/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_tbm_useful_fixture.h Ordinary-loaded paid excavation participants and actual useful modal route construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * No generated-world fixture,painted floor,prepared stock or private vehicle movement is used after loading.
 */
#ifndef TESTS_TUBE_TBM_USEFUL_FIXTURE_H
#define TESTS_TUBE_TBM_USEFUL_FIXTURE_H

#include "tube_useful_tick_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../transport_tube.h"
#include "../transport_tube_cmd.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../industry_map.h"
#include "../landscape.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include <charconv>

inline std::function<void()> *_tube_tbm_file_observer = nullptr; ///< Invocation-local native startup observer,never a saved/gameplay owner.

/** Existing native station/industry/company/link-graph landscape dispatcher,called after vehicle work by StateGameLoop. */
void CallLandscapeTick();

/**
 * Read independently retained original participant identities supplied by the wrapper.
 * @param name Exact invocation-owned environment field.
 * @return Complete ordered unsigned values; malformed or partial identities refuse.
 */
inline std::vector<uint64_t> TubeTBMValues(const char *name)
{
	const char *input = std::getenv(name);
	REQUIRE(input != nullptr);
	std::string_view remaining(input);
	std::vector<uint64_t> values;
	// Explicit bounded numeric transport identifies existing native owners;
	// it never invents pool records or searches for a convenient replacement.
	while (!remaining.empty()) {
		const auto delimiter = remaining.find(',');
		const auto field = remaining.substr(0, delimiter);
		uint64_t value = 0;
		const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value);
		REQUIRE(parsed.ec == std::errc{});
		REQUIRE(parsed.ptr == field.data() + field.size());
		values.push_back(value);
		if (delimiter == std::string_view::npos) break;
		remaining.remove_prefix(delimiter + 1);
	}
	return values;
}

/**
 * Run the current observer only after ordinary native startup and afterload.
 * @param arguments Native console invocation;empty arguments request command help.
 * @return Command admission without creating any gameplay state.
 */
inline bool ObserveTubeTBMFile(std::span<std::string_view> arguments)
{
	if (arguments.empty()) return true;
	if (_tube_tbm_file_observer != nullptr) (*_tube_tbm_file_observer)();
	return true;
}

/**
 * Load the exact original paid world through the complete application loader.
 * @param action Invocation-owned observer of original participants;its commands may fund actual subsequent work.
 */
inline void RunTubeTBMLoadedObserver(const std::function<void()> &action)
{
	const char *input = std::getenv("OTTD_TUBE_TBM_INPUT");
	REQUIRE(input != nullptr);
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	/** Observe only after native afterload;propagate test failures after the application has closed its own world. */
	std::function<void()> inspect = [&]() {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			action();
		} catch (...) { failure = std::current_exception(); }
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer{_tube_tbm_file_observer, &inspect};
	IConsole::CmdRegister("tube_tbm_file_check", ObserveTubeTBMFile);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

/** Borrowed original paid excavation owners plus command-funded useful participants in that exact ordinary-loaded world. */
struct TubeTBMUsefulWorld {
	TileIndex site = INVALID_TILE; ///< Unchanged original generated seam anchor.
	Train *cutter = nullptr; ///< Whole withdrawn original paid cutter.
	Train *hauler = nullptr; ///< Whole original emptied spoil carrier.
	Industry *earth = nullptr; ///< Actual original Surface spoil receiver.
	Industry *rock = nullptr; ///< Actual original Surface crusher/disposal owner.
	ExcavationJob job; ///< Read-only original completed history,never staged into another owner.
	Station *origin = nullptr; ///< Actual earned-floor tube loading terminal.
	Station *interchange = nullptr; ///< Actual joined same-layer transfer packet owner.
	Station *destination = nullptr; ///< Actual Surface useful receiver berth.
	Industry *mine = nullptr; ///< Native original-seam claim and finite production owner.
	Industry *factory = nullptr; ///< Actual Surface CFMT fuel acceptance,without a metal-output claim.
	Train *capsule = nullptr; ///< Actual command-purchased layer-local Mole.
	Train *freight = nullptr; ///< Actual conventional Surface-crossing locomotive.
	Train *wagon = nullptr; ///< Sole actual conventional mineral packet carrier.
	RailType tube = INVALID_RAILTYPE; ///< Native dedicated movement domain.
	CargoType cargo = INVALID_CARGO; ///< Actual original Coal slot.
	UtilityProducerID power{UINT32_MAX}; ///< Sole actually purchased external source.
	std::array<TileIndex, 2> tube_depots{}; ///< Existing original paid tube service endpoints.
	std::array<TileIndex, 2> freight_depots{}; ///< Native command-funded conventional endpoints.
	TileIndex portal = INVALID_TILE; ///< Real newly Connected freight/power shaft,not a prepared portal record.
	Money invoices = 0; ///< Observed command invoices;source lease keeps its existing debit authority.
	uint64_t earth_total = 0; ///< Original historical/physical Earth total retained during native disposal processing.
	uint64_t rock_total = 0; ///< Original historical/physical Rock total retained during native crusher processing.
	uint64_t opening_reserve = 0; ///< Actual recoverable seam balance after paid excavation;native cutting losses precede later mining output.

	/** @param x Layer-local offset from the original seam. @param y Layer-local row offset. @return Original same-plane paid target;lookup does not open floor. */
	TileIndex At(int x, int y) const { return TileAddWrap(this->site, x, y); }
	/** @param x Layer-local offset from the original seam. @param y Layer-local row offset. @return Explicit Surface projection for real conventional construction. */
	TileIndex Surface(int x, int y) const { return MultiLayerMap::ProjectToSurface(this->At(x, y)); }

	/** Resolve only the original paid saved participants and completed work;no constructor recreates excavation or logistics. */
	TubeTBMUsefulWorld()
	{
		const auto values = TubeTBMValues("OTTD_TUBE_TBM_OWNERS");
		REQUIRE(values.size() == 10);
		this->site = TileIndex{static_cast<uint32_t>(values[0])};
		this->cutter = Train::Get(VehicleID{static_cast<uint32_t>(values[2])});
		this->hauler = Train::Get(VehicleID{static_cast<uint32_t>(values[3])});
		this->earth = Industry::Get(IndustryID{static_cast<uint16_t>(values[4])});
		this->rock = Industry::Get(IndustryID{static_cast<uint16_t>(values[5])});
		const auto *job = GetExcavationJob(ExcavationJobID{static_cast<uint32_t>(values[6])});
		REQUIRE(job != nullptr);
		this->job = *job;
		REQUIRE(this->job.state == ExcavationJobState::Completed);
		REQUIRE(CaptureTBMProjects().empty());
		REQUIRE(CaptureTBMSupport().empty());
		REQUIRE(this->cutter->vehstatus.Test(VehState::Stopped));
		REQUIRE(this->hauler->vehstatus.Test(VehState::Stopped));
		this->tube_depots = {TileIndex{static_cast<uint32_t>(values[7])}, TileIndex{static_cast<uint32_t>(values[8])}};
		this->origin = Station::GetByTile(this->At(4, 2));
		this->interchange = Station::GetByTile(this->At(10, 8));
		REQUIRE(this->origin != nullptr);
		REQUIRE(this->interchange != nullptr);
		this->tube = GetTubeRailType();
		REQUIRE(this->tube != INVALID_RAILTYPE);
		this->cargo = GetCargoTypeByLabel(CT_COAL);
		REQUIRE(this->cargo != INVALID_CARGO);
		const auto *deposit = GetGeologyWorldState().deposits.Get(GetGeologyWorldState().deposits.Find(this->site));
		REQUIRE(deposit != nullptr);
		this->opening_reserve = deposit->remaining_reserve;
		const auto *earth_state = GetSpoilReceiver(this->earth->index), *rock_state = GetSpoilReceiver(this->rock->index);
		REQUIRE(earth_state != nullptr);
		REQUIRE(rock_state != nullptr);
		this->earth_total = earth_state->disposed_t + this->earth->accepted.front().waiting;
		this->rock_total = rock_state->disposed_t + rock_state->crushed_t + this->rock->accepted.front().waiting;
		REQUIRE(this->earth_total == this->job.completed_earth_t);
		REQUIRE(this->rock_total == this->job.completed_rock_t);
	}

	/**
	 * Execute one actual paid native construction/order command.
	 * @tparam Tcmd Existing synchronized native command identity.
	 * @tparam Targs Actual typed command arguments.
	 * @param args Original-world command arguments;no alternate owner is introduced.
	 * @return Native result after real invoice and signal update flush.
	 */
	template <Commands Tcmd, typename... Targs>
	auto Do(Targs... args)
	{
		auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
		INFO("original paid useful command=" << GetCommandName(Tcmd) << " refusal=" << ExtractCommandCost(result).GetErrorMessage().base());
		REQUIRE(ExtractCommandCost(result).Succeeded());
		UpdateSignalsInBuffer();
		this->invoices += ExtractCommandCost(result).GetCost();
		return result;
	}

	/** @param tile Actual owned compatible service depot. @param engine Actual native catalogue model. @param cargo Optional native refit. @return Command-purchased part at its genuine depot position. */
	Train *Buy(TileIndex tile, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto *definition = Engine::Get(engine);
		INFO("native purchase engine=" << engine.base() << " name=" << definition->name << " available=" << definition->company_avail.base()
			<< " flags=" << definition->flags.base() << " introduction=" << definition->intro_date.base() << " calendar=" << TimerGameCalendar::date.base());
		const auto result = this->Do<Commands::BuildVehicle>(tile, engine, false, cargo, ClientID::Invalid);
		return Train::Get(std::get<1>(result));
	}

	/** @param front Actual purchased front. @param pickup Native loading station. @param drop Native transfer/final station. @param depots Real service endpoints for ordinary reversal. @param transfer True requests native transfer without final settlement. */
	void Orders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload); load.SetLoadType(OrderLoadType::LoadIfPossible);
		Order unload; unload.MakeGoToStation(drop); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible); unload.SetLoadType(OrderLoadType::NoLoad);
		Order far; far.MakeGoToDepot(Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array orders{load, unload, far, home};
		// Each physical leg/reversal follows its ordinary retained order;no
		// local arrival,order skip or vehicle pose assignment supplies a leg.
		for (size_t index = 0; index < orders.size(); ++index) this->Do<Commands::InsertOrder>(front.index, static_cast<VehicleOrderID>(index), orders[index]);
		ProcessOrders(&front);
	}

	/** Fund real mine,machinery,conventional connection,paid service and stopped fronts while preserving the original earned floor. */
	void Prepare()
	{
		const auto original_floor = std::vector<uint8_t>(GetGeologyWorldState().terrain.Data().begin(), GetGeologyWorldState().terrain.Data().end());
		const auto cash = Company::Get(_current_company)->money;
		const auto seam = GetGeologyWorldState().deposits.Find(this->site);
		this->Do<Commands::BuildLayerMine>(this->site, GetLayerMineIndustryType(CT_COAL), uint32_t{0}, seam, uint32_t{1234});
		this->mine = Industry::Get(GetIndustryIndex(this->site));
		REQUIRE(GetLayerMineState(this->mine->index) != nullptr);
		this->freight_depots = {this->At(8, 7), this->Surface(8, -6)};
		this->Do<Commands::BuildRailDepot>(this->freight_depots[0], RAILTYPE_RAIL, DiagDirection::NW);
		this->Do<Commands::BuildRailStation>(this->At(8, 5), RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, this->interchange->index, false);
		this->portal = this->Surface(8, 3);
		this->Do<Commands::BuildLayerPortal>(this->portal, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
		this->Do<Commands::BuildRail>(this->At(8, 4), RAILTYPE_RAIL, Track::Y, false);
		this->Do<Commands::BuildRailDepot>(this->freight_depots[1], RAILTYPE_RAIL, DiagDirection::SE);
		this->Do<Commands::BuildRailStation>(this->Surface(8, -3), RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
		this->destination = Station::GetByTile(this->Surface(8, -3));
		// Only actual Surface gap cells receive conventional track. The shared
		// station and both real shaft endpoints retain their native identities.
		for (int row = -5; row <= 2; ++row) {
			if (IsRailStationTile(this->Surface(8, row))) continue;
			this->Do<Commands::BuildRail>(this->Surface(8, row), RAILTYPE_RAIL, Track::Y, false);
		}
		this->Do<Commands::ChangeLayerPortalConnection>(this->portal, true);
		this->Do<Commands::BuildIndustry>(this->Surface(11, -3), GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"}), uint32_t{0}, true, uint32_t{1234});
		this->factory = Industry::Get(GetIndustryIndex(this->Surface(11, -3)));
		REQUIRE(this->factory->GetCargoAccepted(this->cargo) != this->factory->accepted.end());
		std::vector<TileIndex> wire;
		// The real paid utility follows the earned eastern wing back into the
		// original cavern;neither geometric proximity nor a prepared source
		// supplies electrical work across an unbuilt connection.
		for (int column = 8; column >= 4; --column) wire.push_back(this->At(column, 4));
		for (int row = 3; row >= 0; --row) wire.push_back(this->At(4, row));
		this->Do<Commands::ConnectTubePower>(GetLayerPortalPairTile(this->portal), this->tube_depots[0], TubePowerCommandPath{wire});
		const auto sources = GetTubePowerConnections();
		REQUIRE(sources.size() == 1);
		this->power = sources.front().producer;
		this->capsule = this->Buy(this->tube_depots[0], GetTubeEngine(9));
		this->freight = this->Buy(this->freight_depots[0], GetUndergroundEngine(4));
		this->wagon = this->Buy(this->freight_depots[0], GetUndergroundEngine(5), this->cargo);
		this->Do<Commands::MoveRailVehicle>(this->wagon->index, this->freight->index, false);
		REQUIRE(BindTubePowerVehicle(this->capsule->index, this->power).has_value());
		this->Orders(*this->capsule, this->origin->index, this->interchange->index, this->tube_depots, true);
		this->Orders(*this->freight, this->interchange->index, this->destination->index, this->freight_depots, false);
		CHECK(Company::Get(_current_company)->money < cash);
		CHECK(cash - Company::Get(_current_company)->money == this->invoices + Money{static_cast<int64_t>(sources.front().invoiced_money)});
		CHECK(std::vector<uint8_t>(GetGeologyWorldState().terrain.Data().begin(), GetGeologyWorldState().terrain.Data().end()) == original_floor);
		CHECK(*GetExcavationJob(this->job.id) == this->job);
	}

	/** Advance one complete actual native clock/landscape/industry/vehicle opportunity;none of the original excavation owners is replayed. */
	void Tick() const
	{
		// Match StateGameLoop's clock,tile-loop,vehicle,then landscape order.
		// The landscape dispatcher owns station acceptance after real facility
		// construction as well as finite industry and company maintenance;
		// ticking vehicles alone leaves the new Surface receiver unannounced.
		AutoRestoreBackup actor{_current_company, OWNER_NONE};
		UpdateSignalsInBuffer();
		if (TimerManager<TimerGameCalendar>::Elapsed({})) RunVehicleCalendarDayProc();
		TimerManager<TimerGameEconomy>::Elapsed({});
		TimerManager<TimerGameTick>::Elapsed(1);
		RunTileLoop();
		CallVehicleTicks();
		CallLandscapeTick();
		UpdateLandscapingLimits();
	}

	/** @param verify_loaded_count Check the original cargo-ready count for useful continuation;false permits a complete later-file comparison with its independently retained full native chunks. Resolve only existing participants;no construction or work is replayed. */
	void ResolveUseful(bool verify_loaded_count = true)
	{
		const auto values = TubeTBMValues("OTTD_TUBE_TBM_USEFUL");
		REQUIRE(values.size() == 8);
		this->capsule = Train::Get(VehicleID{static_cast<uint32_t>(values[0])});
		this->freight = Train::Get(VehicleID{static_cast<uint32_t>(values[1])});
		this->wagon = Train::Get(VehicleID{static_cast<uint32_t>(values[2])});
		this->mine = Industry::Get(IndustryID{static_cast<uint16_t>(values[3])});
		this->factory = Industry::Get(IndustryID{static_cast<uint16_t>(values[4])});
		this->power = UtilityProducerID{static_cast<uint32_t>(values[5])};
		this->opening_reserve = values[6];
		this->freight_depots = {this->At(8, 7), this->Surface(8, -6)};
		this->portal = this->Surface(8, 3);
		this->destination = Station::GetByTile(this->Surface(8, -3));
		REQUIRE(this->destination != nullptr);
		REQUIRE(this->mine->location.tile == this->site);
		REQUIRE(this->factory->location.tile == this->Surface(11, -3));
		REQUIRE(this->wagon->First() == this->freight);
		if (verify_loaded_count) REQUIRE(this->capsule->cargo.StoredCount() == values[7]);
		REQUIRE(GetTubePowerVehicle(this->capsule->index)->source == this->power);
	}

	/** @param extra Optional actual command-purchased competing capsule,never a fabricated cargo participant. Check finite Coal custody,source provenance and the separate completed spoil leg without changing any owner. */
	void CheckConservation(const Train *extra = nullptr) const
	{
		const auto *mine_state = GetLayerMineState(this->mine->index);
		const auto *deposit = GetGeologyWorldState().deposits.Get(mine_state->deposit);
		CHECK(this->opening_reserve == deposit->remaining_reserve + mine_state->extracted);
		const uint64_t physical = this->mine->produced.front().waiting + this->origin->goods[this->cargo].TotalCount() +
			this->capsule->cargo.StoredCount() + this->interchange->goods[this->cargo].TotalCount() + this->wagon->cargo.StoredCount() +
			this->factory->GetCargoAccepted(this->cargo)->waiting + (extra == nullptr ? 0 : extra->cargo.StoredCount());
		CHECK(mine_state->extracted == physical);
		// Native transfer may merge/split packets legitimately. Every retained
		// physical fragment must keep the same actual finite mine and first
		// tube pickup instead of acquiring a second source or supply purpose.
		for (const Train *part : std::array<const Train *, 3>{this->capsule, this->wagon, extra}) {
			if (part == nullptr) continue;
			for (const auto *packet : *part->cargo.Packets()) {
				CHECK(packet->GetSource() == Source{this->mine->index, SourceType::Industry});
				CHECK(packet->GetFirstStation() == this->origin->index);
				CHECK(packet->GetCompanySupplyOrder() == 0);
			}
		}
		CHECK(GetTileLayer(this->capsule->tile) == MapLayer::Underground);
		this->CheckSpoil();
	}

	/** Preserve the original completed paid spoil leg through subsequent native receiver processing and useful freight. */
	void CheckSpoil() const
	{
		CHECK(*GetExcavationJob(this->job.id) == this->job);
		const auto *earth_state = GetSpoilReceiver(this->earth->index), *rock_state = GetSpoilReceiver(this->rock->index);
		CHECK(earth_state->disposed_t + this->earth->accepted.front().waiting == this->earth_total);
		CHECK(rock_state->disposed_t + rock_state->crushed_t + this->rock->accepted.front().waiting == this->rock_total);
		// Every original part remains withdrawn and empty;useful Coal may not
		// replace or bypass any mandatory original Earth/Rock wagon movement.
		for (const Train *front : {this->cutter, this->hauler}) {
			for (const Train *part = front; part != nullptr; part = part->Next()) CHECK(part->cargo.TotalCount() == 0);
		}
	}

	/**
	 * Save an earned native phase without constructing or advancing any owner.
	 * @param stage Explicit original loaded-ready or useful-final observation label.
	 */
	void Save(std::string_view stage) const
	{
		AutoRestoreBackup local{_local_company, _current_company};
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu};
		AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerMineDigest(), ComputeTubePowerDigest(),
			ComputeTubePowerVehicleDigest(), ComputeUtilityPowerDigest(), TimerGameTick::counter, Company::Get(_current_company)->money};
		const auto path = GetTestCaseArtifactPaths("tubes", "tbm-final-use").Output("saves", fmt::format("tube-tbm-{}.sav", stage));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerMineDigest(), ComputeTubePowerDigest(),
			ComputeTubePowerVehicleDigest(), ComputeUtilityPowerDigest(), TimerGameTick::counter, Company::Get(_current_company)->money} == before));
		fmt::print("TUBE_TBM_USEFUL_SAVE stage={} path={}\n", stage, path.string());
	}
};

#endif /* TESTS_TUBE_TBM_USEFUL_FIXTURE_H */
