/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_workflow_fixture.h Accepted scenario preconditions and actual native ticks for visible workflows. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 9B-9D; r2mlvs4-tunnel-boring-machines.md, Group 20 (paid initial logistics and complete cost); r2mlvs5-spoil-wagons-and-haulage.md, Group 13A (native spoil physics) */

#ifndef TESTS_UNDERGROUND_WORKFLOW_FIXTURE_H
#define TESTS_UNDERGROUND_WORKFLOW_FIXTURE_H

#include "excavation_fixture.h"
#include "movement_fixture.h"
#include "../underground_vehicle.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../layer_industry_baseline.h"
#include "../landscape.h"
#include "../economy_base.h"
#include "../spoil_receiver.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../industry_cmd.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../depot_base.h"
#include "../roadstop_base.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../tbm_cmd.h"
#include "../layer_portal_map.h"
#include "../tbm_controls.h"
#include "../town_kdtree.h"
#include "../table/strings.h"
#include "../timer/timer_game_economy.h"

/** Existing native synchronized industry dispatcher, declared by the main game loop. */
void OnTick_Industry();
/** Existing native landscape traversal cursor; initialized once as part of this accepted headless scenario. */
extern TileIndex _cur_tileloop_tile;

/** A generated seam with existing funded Surface spoil logistics; the target mine floor begins as untouched rock. */
class UndergroundWorkflowWorld : public ExcavationFixture {
	bool paid_setup; ///< Economics scenarios invoice initial infrastructure through the actual command kernel.
	decltype(_engine_mngr.mappings) engine_mappings = _engine_mngr.mappings; ///< Caller EIDS ownership.
	decltype(_industry_mngr.mappings) industry_mappings = _industry_mngr.mappings; ///< Caller IIDS ownership.
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Caller TIDS ownership.
	AutoRestoreBackup<decltype(_sorted_industry_types)> industry_order{_sorted_industry_types}; ///< Caller derived funding-list order, restored with its original definitions.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Ordinary train acquisition limits.
	AutoRestoreBackup<UnitID> trucks{_settings_game.vehicle.max_roadveh, UnitID{500}}; ///< Ordinary road acquisition limits.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Ordinary consist length.
	AutoRestoreBackup<decltype(_settings_game.vehicle.freight_trains)> freight_weight{_settings_game.vehicle.freight_trains, 1}; ///< Native minimum/default freight multiplier; zero-initialized unit settings would erase real spoil mass.
	AutoRestoreBackup<CommandPauseLevel> pause_build{_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions}; ///< Headless raster pause does not disable visible construction commands.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{10000}}; ///< Nonzero ordinary train price.
	AutoRestoreBackup<Money> wagon_price{_price[Price::BuildVehicleWagon], Money{1000}}; ///< Nonzero ordinary wagon price.
	AutoRestoreBackup<Money> road_price{_price[Price::BuildVehicleRoad], Money{1000}}; ///< Nonzero ordinary truck price.
	AutoRestoreBackup<Money> industry_price{_price[Price::BuildIndustry], Money{10000}}; ///< Nonzero facility construction.
	AutoRestoreBackup<Money> raw_price{_price[Price::BuildIndustryRaw], Money{10000}}; ///< Nonzero raw facility construction.
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< No floating raster-only loading text in command/event tests.
	AutoRestoreBackup<TimerGameEconomy::DateFract> economy_fraction{TimerGameEconomy::date_fract}; ///< Restore the actual daily scheduling slot after physical stepping.
	AutoRestoreBackup<TimerGameTick::TickCounter> tick_counter{TimerGameTick::counter}; ///< Restore caller simulation-frame numbering.
	AutoRestoreBackup<TileIndex> landscape_cursor{_cur_tileloop_tile, TileIndex{1}}; ///< Normal nonzero landscape traversal seed.
	AutoRestoreBackup<decltype(_economy.fluct)> recession{_economy.fluct, 1}; ///< Scenario starts outside recession.
	AutoRestoreBackup<uint16_t> scale{_settings_game.economy.industry_cargo_scale, uint16_t{100}}; ///< Declared ordinary production scale.
	decltype(_sorted_cargo_specs) sorted_cargo = _sorted_cargo_specs; ///< Caller derived presentation pointers, restored before its span.
	size_t standard_cargo_count = _sorted_standard_cargo_specs.size(); ///< Rebind the caller's span to restored vector storage, never a freed allocation.
	AutoRestoreBackup<decltype(_sorted_cargo_types)> cargo_order{_sorted_cargo_types}; ///< Caller presentation ordering.
	AutoRestoreBackup<decltype(_standard_cargo_mask)> standard_mask{_standard_cargo_mask}; ///< Caller derived presentation mask.
	AutoRestoreBackup<decltype(CargoSpec::town_production_cargoes)> town_cargo{CargoSpec::town_production_cargoes}; ///< Caller derived town cargo pointers.
public:
	PathfinderDefaults pathfinder; ///< Actual route-search defaults for physical train/road movement.
	MapLayer layer; ///< Explicit target mine/work plane.
	CargoLabel resource; ///< Actual finite mine mineral.
	TileIndex site = INVALID_TILE; ///< Generated deposit point, never a manufactured seam.
	TileIndex portal = INVALID_TILE; ///< Real upper commissioning magazine.
	TileIndex rock = INVALID_TILE; ///< Actual target-plane housing.
	Station *origin = nullptr; ///< Real owned Surface station accepting both spoil streams.
	Train *hauler = nullptr; ///< Existing ordinary logistics consist awaiting visible assignment/start.
	Industry *earth = nullptr; ///< Funded Earth receiver.
	Industry *stone = nullptr; ///< Funded Rock receiver.
	uint ticks = 0; ///< Native simulation steps, not wall-clock waits.
	Money setup_cost = 0; ///< Actual invoiced initial logistics; pre-owned workflow scenarios leave this zero.
	uint64_t measured_active_ticks = 0; ///< Observed canonical unblocked controller durations across daily invoice resets.
	uint64_t measured_idle_ticks = 0; ///< Observed canonical blocked controller durations across daily invoice resets.
	bool simulate_industries = false; ///< Enable the full normal industry/landscape owners when the visible mine has been built.
	bool simulate_company_limits = false; ///< Optional normal per-frame construction allowance owner for multi-stage paid construction scenarios.

	/**
	 * @param layer Actual Underground or Deep target; existing upper excavated access is a scenario precondition.
	 * @param paid_setup Invoice every initial logistics command and use the caller's declared native economy configuration.
	 * @param full_save Reserve dense original EIDS identities and use the distributed cargo slots for actual game-file transport.
	 * @param hardness_version Zero preserves historical workflow measurements; current regional cases select their saved algorithm explicitly.
	 * @param world_seed Independent geological input for the actual generated world.
	 */
	explicit UndergroundWorkflowWorld(MapLayer layer = MapLayer::Underground, bool paid_setup = false, bool full_save = false, uint16_t hardness_version = 0, uint32_t world_seed = 420) : ExcavationFixture(full_save, hardness_version, world_seed), paid_setup(paid_setup), layer(layer), resource(layer == MapLayer::Underground ? CargoLabel{"COAL"} : CargoLabel{"RARE"})
	{
		// A measured project uses canonical native price calculation rather than the narrow GUI fixture's price overrides.
		if (this->paid_setup) RecomputePrices();
		REQUIRE(Engine::GetNumItems() == 0);
		REQUIRE(Vehicle::GetNumItems() == 0);
		// This accepted scenario has dry Surface land above sea level; the base geology fixture's zero-height grass would flood during real landscape ticks.
		for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
			if (IsInnerTile(tile)) SetTileHeight(tile, 1);
		}
		ResetRailTypes();
		ResetRoadTypes();
		ResetLayerPortalWorkState();
		ResetTBMProjectState();
		ResetIndustries();
		ResetLayerIndustryState();
		ResetLayerProcessorState();
		StationClass::Reset();
		RoadStopClass::Reset();
		RebuildStationKdtree();
		// Real game loading reconstructs every original engine before appending native definitions; save-ready worlds must match that identity transport.
		if (full_save) {
			_engine_mngr.ResetToDefaultMapping();
			SetupEngines();
		}
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseSpoilReceiverDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseLayerMineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		// Moving spoil trains use the distributed package's weight/freight properties (build_content.py 0x0F/0x15),
		// including economics and visible/hidden runs. Pure geology fixtures may omit mass; physical workflows cannot.
		for (CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
			auto *cargo = CargoSpec::Get(GetCargoTypeByLabel(label));
			cargo->weight = 16;
			cargo->is_freight = true;
		}
		// Complete the reduced fixture's actual cargo presentation before opening buy/refit controls; labels/capabilities remain registry-owned.
		for (CargoSpec *cargo : CargoSpec::Iterate()) cargo->town_production_effect = TownProductionEffect::None;
		CargoSpec::Get(GetCargoTypeByLabel(CT_COAL))->name = STR_CARGO_PLURAL_COAL;
		InitializeSortedCargoSpecs();
		// Resolve the original power station's accepted label as normal industry finalization does for the fixture's shuffled runtime slots.
		auto &power = *const_cast<IndustrySpec *>(GetIndustrySpec(1));
		for (size_t slot = 0; slot < power.accepts_cargo_label.size(); ++slot) {
			if (const auto *label = std::get_if<CargoLabel>(&power.accepts_cargo_label[slot])) power.accepts_cargo[slot] = GetCargoTypeByLabel(*label);
		}
		// Catchment reads tile acceptance, not just the industry input list; finalize each original layout's labels as the actual GRF loader does.
		for (const auto &layout : power.layouts) {
			for (const auto &cell : layout) {
				if (cell.gfx == INDUSTRYTILE_NOANIM) continue;
				auto &spec = *const_cast<IndustryTileSpec *>(GetIndustryTileSpec(cell.gfx));
				for (size_t slot = 0; slot < spec.accepts_cargo_label.size(); ++slot) {
					if (const auto *label = std::get_if<CargoLabel>(&spec.accepts_cargo_label[slot])) spec.accepts_cargo[slot] = GetCargoTypeByLabel(*label);
				}
			}
		}
		SortIndustryTypes();
		Company::Get(_current_company)->money = 10000000;
		uint minimum_x = 0;
		// Room for the actual receiver widths determines the left logistics margin; a fixed larger margin can exclude the guaranteed Deep seam.
		for (SpoilReceiverKind kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
			for (const auto &cell : GetIndustrySpec(GetSpoilReceiverIndustryType(kind))->layouts.front()) minimum_x = std::max<uint>(minimum_x, cell.ti.x + 2);
		}
		// The guaranteed Deep rare seam is near the west side; its existing receivers sit east of the line, while the Underground scenario keeps its left-side layout.
		minimum_x = layer == MapLayer::Underground ? std::max<uint>(minimum_x, 12) : 5;
		// A stale availability flag cannot bypass technology, but this funded scenario starts with its introduced ordinary designs offered.
		for (uint16_t model = 2; model <= 8; ++model) Engine::Get(GetUndergroundEngine(model))->company_avail.Set(_current_company);
		// Choose a real generated seam with room for the whole workflow and its existing Surface logistics, never editing a geological span.
		for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
			if (deposit.resource != this->resource) continue;
			for (const auto &span : deposit.spans) {
				// Interior selection leaves room for the upper access and later Surface receiving station without wrapping map planes.
				const uint minimum_y = layer == MapLayer::Underground ? 12 : 15;
				if (span.y < minimum_y || span.y > 43 || span.first_x > 48 || span.last_x < minimum_x) continue;
				this->site = TileXYL(std::max<uint>(span.first_x, minimum_x), span.y, layer);
				break;
			}
			if (this->site != INVALID_TILE) break;
		}
		REQUIRE(this->site != INVALID_TILE);
		REQUIRE(Town::CanAllocateItem());
		Town *town = Town::Create(TileXYL(10, 10, MapLayer::Surface));
		town->townnametype = SPECSTR_TOWNNAME_START;
		RebuildTownKdtree();
		const uint x = TileX(this->site) + 1, y = TilePlaneY(this->site) - 2;
		const MapLayer upper = layer == MapLayer::Underground ? MapLayer::Surface : MapLayer::Underground;
		this->portal = TileXYL(x, y, upper);
		if (upper == MapLayer::Underground) {
			// An established Underground access line is the Deep scenario's starting world; Deep target cells stay solid until native boring.
			std::vector<TileIndex> upper_floor;
			// The already established upper magazine needs the Deep cutter's real three-cell assembly width; no target-plane floor is opened here.
			for (uint row = y - 4; row <= y + 1; ++row) {
				for (uint column = x - 1; column <= x + 1; ++column) upper_floor.push_back(TileXYL(column, row, upper));
			}
			this->Open(upper_floor);
			const auto access = TileXYL(x, y - 3, MapLayer::Surface);
			REQUIRE(this->PrepareInitial<Commands::BuildLayerPortal>(access, TransportType::Rail, upper, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
			// Both actual approaches precede connection; camera selection grants no route.
			for (uint row = y - 2; row < y; ++row) REQUIRE(this->PrepareInitial<Commands::BuildRail>(TileXYL(x, row, upper), RAILTYPE_RAIL, Track::Y, false).Succeeded());
			REQUIRE(this->PrepareInitial<Commands::BuildRail>(TileXYL(x, y - 4, MapLayer::Surface), RAILTYPE_RAIL, Track::Y, false).Succeeded());
			REQUIRE(this->PrepareInitial<Commands::ChangeLayerPortalConnection>(access, true).Succeeded());
		}
		REQUIRE(this->PrepareInitial<Commands::BuildLayerPortal>(this->portal, TransportType::Rail, layer, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		this->rock = GetLayerPortalPairTile(this->portal);
		const uint surface_y = upper == MapLayer::Surface ? y : y - 3;
		const uint platform_length = layer == MapLayer::Underground ? 3 : 4;
		const TileIndex station = TileXYL(x, surface_y - 6, MapLayer::Surface);
		REQUIRE(this->PrepareInitial<Commands::BuildRailStation>(station, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, static_cast<uint8_t>(platform_length), STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false).Succeeded());
		this->origin = Station::GetByTile(station);
		// The owned Surface line connects magazine, origin and pre-existing hauler depot through actual construction commands.
		for (uint row = surface_y - 9; row < surface_y; ++row) {
			// Leave the actual platform intact, with room for the complete layer-appropriate spoil hauler outside the magazine/depot during loading.
			if (row >= surface_y - 6 && row < surface_y - 6 + platform_length) continue;
			// The Deep world's Surface-to-Underground connection already built and verified this same owned approach before connecting its pair.
			if (upper == MapLayer::Underground && row == surface_y - 1) continue;
			REQUIRE(this->PrepareInitial<Commands::BuildRail>(TileXYL(x, row, MapLayer::Surface), RAILTYPE_RAIL, Track::Y, false).Succeeded());
		}
		const TileIndex depot = TileXYL(x, surface_y - 10, MapLayer::Surface);
		REQUIRE(this->PrepareInitial<Commands::BuildRailDepot>(depot, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
		// Both existing disposal receivers are genuinely funded and included in normal station catchment before commissioning.
		for (SpoilReceiverKind kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
			uint extent = 0;
			// Place the entire actual receiver layout beside the railway; its artwork template can be wider than the visible anchor tile.
			for (const auto &cell : GetIndustrySpec(GetSpoilReceiverIndustryType(kind))->layouts.front()) extent = std::max<uint>(extent, cell.ti.x);
			const uint receiver_x = layer == MapLayer::Underground ? x - extent - 2 : x + 2;
			const TileIndex cell = TileXYL(receiver_x, kind == SpoilReceiverKind::Earth ? surface_y - 7 : surface_y - 2, MapLayer::Surface);
			const auto result = this->PrepareInitial<Commands::BuildIndustry>(cell, GetSpoilReceiverIndustryType(kind), uint32_t{0}, true, uint32_t{1234});
			INFO("receiver=" << to_underlying(kind) << " at " << TileX(cell) << ',' << TilePlaneY(cell) << " site=" << TileX(this->site) << ',' << TilePlaneY(this->site));
			INFO("error=" << result.GetErrorMessage().base());
			REQUIRE(result.Succeeded());
			Industry *industry = Industry::GetByTile(cell);
			if (kind == SpoilReceiverKind::Earth) this->earth = industry;
			else this->stone = industry;
		}
		this->origin->RecomputeCatchment();
		UpdateStationAcceptance(this->origin, false);
		this->hauler = Train::From(this->BuyPreexisting(depot, 4));
		// Only initial logistics are prepared here; the later mine carrier must be purchased/refitted through its visible controls.
		const auto consist = GetDefaultTBMConsist(layer);
		for (const auto [model, count] : {std::pair<uint16_t, uint8_t>{2, consist.earth_wagons}, {3, consist.rock_wagons}}) {
			for (uint wagon_index = 0; wagon_index < count; ++wagon_index) {
				Vehicle *wagon = this->BuyPreexisting(depot, model);
				REQUIRE(this->PrepareInitial<Commands::MoveRailVehicle>(wagon->index, this->hauler->Last()->index, false).Succeeded());
			}
		}
		CHECK(GetGeologyWorldState().terrain.Get(this->site) == ExcavationState::Solid);
	}

	/** Retire actual cargo/order/spatial references before the accepted geological/company world disappears. */
	~UndergroundWorkflowWorld()
	{
		// Retire each complete physical chain once so real order, packet and group ownership is unwound before stations.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		ResetTBMProjectState();
		ResetLayerPortalWorkState();
		// Ordinary facility destruction removes catchment, packet and claim references while geology and towns still exist.
		for (Station *station : Station::Iterate()) delete station;
		for (Industry *industry : Industry::Iterate()) delete industry;
		_roadstop_pool.CleanPool();
		_depot_pool.CleanPool();
		_group_pool.CleanPool();
		_engine_pool.CleanPool();
		_town_pool.CleanPool();
		RebuildTownKdtree();
		ResetSpoilReceiverState();
		ResetLayerIndustryState();
		ResetLayerProcessorState();
		ResetIndustries();
		_engine_mngr.mappings = std::move(this->engine_mappings);
		_industry_mngr.mappings = std::move(this->industry_mappings);
		_industile_mngr.mappings = std::move(this->tile_mappings);
		// Native sorting can reallocate the vector; restore its caller pointers first and rebuild the public span against that actual storage.
		_sorted_cargo_specs = std::move(this->sorted_cargo);
		_sorted_standard_cargo_specs = {_sorted_cargo_specs.data(), this->standard_cargo_count};
	}

	/** @param depot Existing owned logistics depot. @param model Actual ordinary design. @return Genuine command-created initial vehicle. */
	Vehicle *BuyPreexisting(TileIndex depot, uint16_t model)
	{
		const auto [cost, id, capacity, mail, cargo] = this->PrepareInitial<Commands::BuildVehicle>(depot, GetUndergroundEngine(model), false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(cost.Succeeded());
		return Vehicle::Get(id);
	}

	/** Advance genuine station loading, vehicle movement, TBM work and receiver intake once, without resetting any phase or position. */
	void Tick()
	{
		const auto before = this->paid_setup ? CaptureTBMProjects() : std::vector<TBMProject>{};
		// Rotate the native daily scheduling slot once per physical frame; a fixed slot would age/service one vehicle on every test tick.
		TimerGameEconomy::date_fract = static_cast<TimerGameEconomy::DateFract>((TimerGameEconomy::date_fract + 1) % Ticks::DAY_TICKS);
		++TimerGameTick::counter;
		if (this->simulate_industries) {
			// Actual landscape traversal finishes construction and dispatches finite output; the synchronized owner alone credits mining work.
			RunTileLoop();
			OnTick_Industry();
		} else {
			AdvanceSpoilReceiver(*this->earth);
			AdvanceSpoilReceiver(*this->stone);
		}
		// Each ordinary physical owner runs once; depot autoreplace dispatch and station loading follow the real game loop's vehicle step.
		CallVehicleTicks();
		// Multi-stage construction consumes the ordinary company allowance. Its normal per-frame owner replenishes
		// that allowance at the declared saved rate; fixtures must not reset the balance to bypass a refusal.
		if (this->simulate_company_limits) UpdateLandscapingLimits();
		// Count the canonical post-step billing classification only for projects that were active at frame entry;
		// completed history never accrues another duration, and daily invoices do not erase observed elapsed work.
		for (const auto &project : before) {
			if (IsTBMProjectFinished(project.phase)) continue;
			const auto *after = GetTBMProject(project.id);
			REQUIRE(after != nullptr);
			if (after->blocked == TBMBlockedReason::None) ++this->measured_active_ticks;
			else ++this->measured_idle_ticks;
		}
		++this->ticks;
	}

private:
	/**
	 * Construct initial scenario logistics through the owning command, optionally paying its complete native invoice.
	 * @tparam Tcmd Existing synchronized command identity.
	 * @tparam Targs Native typed command argument types.
	 * @param args Actual initial infrastructure/vehicle arguments; no private map or inventory writes replace construction.
	 * @return The native command result, including its unchanged refusal and invoice.
	 */
	template <Commands Tcmd, typename... Targs>
	auto PrepareInitial(Targs... args) -> std::invoke_result_t<typename CommandTraits<Tcmd>::ProcType, DoCommandFlags, Targs...>
	{
		// Economics cases execute the full command kernel so estimates, actual funds and the invoice share one authority;
		// ordinary workflow cases retain their declared pre-owned initial logistics and their existing direct-owner construction.
		auto result = this->paid_setup ? Command<Tcmd>::Do(DoCommandFlag::Execute, args...) : CommandTraits<Tcmd>::proc(DoCommandFlag::Execute, args...);
		if (this->paid_setup) this->setup_cost += ExtractCommandCost(result).GetCost();
		return result;
	}
};

/**
 * Play the shared visible excavation/physical spoil-delivery workflow in the caller's accepted world.
 * @param world Prepared initial scenario; the target floor remains solid until its actual paid cuts.
 * @param stopped_ticks Actual native ticks held before the player starts either train; zero adds no artificial delay.
 * @param observe Read-only observer after each real native step, for actual state/ledger/file evidence.
 * @param control Optional player action schedule after observation; it may issue owning commands but never advances a simulation owner or writes physical/controller fields.
 * @param wide Request the existing five-column paid chamber instead of the narrow mine corridor; native planning and turning guards still apply.
 * @return The real completed project identity; controls are closed while their world references are still alive.
 */
TBMProjectID RunUndergroundExcavationWorkflow(UndergroundWorkflowWorld &world, uint stopped_ticks = 0,
	const std::function<void(const UndergroundWorkflowWorld &)> &observe = {},
	const std::function<void(const UndergroundWorkflowWorld &)> &control = {}, bool wide = false);

#endif /* TESTS_UNDERGROUND_WORKFLOW_FIXTURE_H */
