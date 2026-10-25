/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_performance_fixture.h Prepared physical tube limits with actual native capsules,service,movement and invoices.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group21DDC.
 * Explicit restored floor/Connected portals,funding and boundary-loaded packets are not normal bootstrap or useful cargo production.
 */
#ifndef TESTS_TUBE_PERFORMANCE_FIXTURE_H
#define TESTS_TUBE_PERFORMANCE_FIXTURE_H

#include "ground_job_fixture.h"
#include "transport_tube_power_fixture.h"
#include "movement_fixture.h"
#include "../order_func.h"
#include "../core/string_consumer.hpp"
#include "../timer/timer_game_tick.h"
#include "../economy_func.h"

/** @param name Required exact selector. @return Parsed unsigned value,never an implicit scale fallback. */
inline uint TubePerformanceArgument(const char *name)
{
	const char *value = std::getenv(name); REQUIRE(value != nullptr);
	const auto number = ParseInteger<uint>(value); REQUIRE(number.has_value()); return *number;
}

/** Explicit isolated physical world reuses native pools and commands;only those owners hold money,packets,grants and work. */
class TubePerformanceRig : public ExcavationFixture {
	decltype(_engine_mngr.mappings) mappings = _engine_mngr.mappings; ///< Restore actual native mapping owner after teardown.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{5000}}; ///< Explicit high-fleet policy below the actual binding cap.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Native one-capsule consist admission.
	AutoRestoreBackup<Money> price{_price[Price::BuildVehicleTrain], Money{100000}}; ///< Nonzero actual kernel purchase invoices.
	AutoRestoreBackup<TimerGameCalendar::Date> date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2005}, 0, 1)}; ///< All five real selected introductions are reached.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{2005}}; ///< Coherent native technology calendar.
	AutoRestoreBackup<TimerGameEconomy::Date> economy{TimerGameEconomy::date, TimerGameEconomy::Date{100}}; ///< One real invoice day for the fixed work.
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter, uint64_t{0}}; ///< Restore actual synchronized opportunity after isolated native work.
	PathfinderDefaults pathfinder; ///< Ordinary default native routing limits.
public:
	RailType tube = INVALID_RAILTYPE; ///< Shared registry-issued tube railtype.
	std::vector<TileIndex> depots, destinations; ///< Actual command-built facility identities.
	std::vector<UtilityProducerID> sources; ///< Sole shared-owner-issued installed sources.
	std::vector<Train *> capsules; ///< Nonowning native vehicle observations.
	std::vector<CargoPacketID> packets; ///< Original native custody observations,never a cargo registry.
	std::vector<ExcavationJobID> jobs; ///< Real reservations used only for declared partial-work stress.

	/** @param connections Native inlet count. @param path_length Real installed cells per source. @param vehicles Real purchased capsules. @param job_count Real partial jobs. @param motion Actual native route versus repeated prepared depot repair. */
	TubePerformanceRig(uint connections, uint path_length, uint vehicles, uint job_count, bool motion)
	{
		MockEnvironment::Instance();
		REQUIRE(Engine::GetNumItems() == 0); REQUIRE(Vehicle::GetNumItems() == 0);
		ResetTubePowerState(); ResetUtilityPowerState(); ResetLayerPortalWorkState();
		PrepareGroundJobs(*this, 512, 0);
		Company::Get(_current_company)->money = Money{1000000000}; // Explicit prepared stress funding,never an earned bootstrap claim.
		Company::Get(_current_company)->clear_limit = 60000U << 16; // Explicit setup policy admits the physically bounded native construction commands.
		this->tube = InitialiseTubeRailType(this->ActivatedContent()); REQUIRE(this->tube != INVALID_RAILTYPE);
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		Company::Get(_current_company)->avail_railtypes.Set(this->tube);
		// Expose the actual native model availability explicitly; these prepared inputs make no bootstrap claim.
		for (uint16_t model = 9; model <= 13; ++model) Engine::Get(GetTubeEngine(model))->company_avail.Set(_current_company);
		std::vector<TileIndex> floor, portals, anchors;
		// Place disjoint explicitly supplied starter rooms for the real reserved geological work.
		for (uint index = 0; index < job_count; ++index) {
			const auto anchor = TileXYL(64 + (index % 16) * 8, 440 + (index / 16) * 8, MapLayer::Underground);
			anchors.push_back(anchor);
			for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(anchor, x, y));
		}
		std::vector<std::vector<TileIndex>> paths;
		std::vector<LayerPortalWorkRecord> records;
		const uint columns = 508 / (path_length + 1), stride = motion ? 70 : 10;
		// Each declared inlet installs its own physical route/source; no duplicate identity multiplies capacity.
		for (uint index = 0; index < connections; ++index) {
			const uint x = 2 + (index % columns) * (path_length + 1), y = 2 + (index / columns) * stride;
			REQUIRE(x + path_length < 511); REQUIRE(y + (motion ? 64 : 8) < 511);
			const auto portal = TileXYL(x, y, MapLayer::Underground);
			portals.push_back(portal); floor.push_back(portal);
			std::vector<TileIndex> path;
			// Retain every actual paid path cell so connection length is a physical workload dimension.
			for (uint offset = 0; offset < path_length; ++offset) path.push_back(TileXYL(x + offset, y + 1, MapLayer::Underground));
			floor.insert(floor.end(), path.begin(), path.end());
			const auto depot = path.back(); this->depots.push_back(depot); paths.push_back(std::move(path));
			const uint end_y = y + (motion ? 64 : 8);
			this->destinations.push_back(TileXYL(x + path_length - 1, end_y, MapLayer::Underground));
			// Open the declared native travel corridor only; no work outside the selected component is claimed.
			for (uint track_y = y + 2; track_y <= end_y; ++track_y) floor.push_back(TileXYL(x + path_length - 1, track_y, MapLayer::Underground));
		}
		if (!floor.empty()) this->Open(floor);
		// Admit each real prepared job through the canonical reservation owner.
		for (auto anchor : anchors) {
			const auto reservation = ReserveExcavationJob(BudgetJob(anchor), true);
			REQUIRE(reservation.error == ExcavationError::None); this->jobs.push_back(reservation.id);
		}
		// Each declared inlet installs its own physical route/source; no duplicate identity multiplies capacity.
		for (uint index = 0; index < connections; ++index) {
			const auto portal = portals[index], surface = MultiLayerMap::TileInLayer(portal, MapLayer::Surface);
			CAPTURE(index, connections, path_length, this->depots[index].base());
			MakeRailLayerPortal(surface, _current_company, DiagDirection::SE, MapLayer::Underground, RAILTYPE_RAIL);
			MakeRailLayerPortal(portal, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL);
			LayerPortalWorkRecord record; record.pair_key = surface; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
			records.push_back(record);
			REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->depots[index], this->tube, DiagDirection::SE).Succeeded());
			if (motion) {
				// Build the actual continuous guideway that native fronts must traverse.
				for (uint y = TilePlaneY(this->depots[index]) + 1; y < TilePlaneY(this->destinations[index]); ++y) {
					const auto rail = CmdBuildSingleRail(DoCommandFlag::Execute, TileXYL(TileX(this->depots[index]), y, MapLayer::Underground), this->tube, Track::Y, false);
					INFO("rail y=" << y << " error=" << rail.GetErrorMessage().base()); REQUIRE(rail.Succeeded());
				}
				REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->destinations[index], this->tube, DiagDirection::NW).Succeeded());
			}
		}
		std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
		// Each declared inlet installs its own physical route/source; no duplicate identity multiplies capacity.
		for (uint index = 0; index < connections; ++index) {
			const auto connection = ConnectTubeExternalPower(portals[index], this->depots[index], paths[index]);
			REQUIRE(connection.error == TubePowerError::None); this->sources.push_back(connection.producer);
		}
		// Buy and bind each actual production capsule through native commands, retaining invoices and custody.
		for (uint index = 0; index < vehicles; ++index) {
			const uint inlet = index % connections;
			const auto depot = this->depots[inlet];
			const auto engine = GetTubeEngine(9 + index % 5);
			const auto before = Company::Get(_current_company)->money;
			const auto [result, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
				depot, std::make_tuple(depot, engine, false, INVALID_CARGO, ClientID{1}));
			REQUIRE(result.Succeeded()); REQUIRE(result.GetCost() > Money{0}); REQUIRE(Company::Get(_current_company)->money == before - result.GetCost());
			Train *train = Train::Get(id); this->capsules.push_back(train);
			REQUIRE(BindTubePowerVehicle(id, this->sources[inlet]).has_value());
			REQUIRE(CargoPacket::CanAllocateItem());
			auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{1}, Source{Source::Invalid, SourceType::Industry});
			packet->UpdateLoadingTile(depot); this->packets.push_back(packet->index);
			train->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep); train->CargoChanged();
			if (motion) {
				// Use a real two-depot order cycle so native pathing, not a stand-in motion owner, earns movement.
				for (uint order_index = 0; order_index < 2; ++order_index) {
					Order order; order.MakeGoToDepot(Depot::GetByTile(order_index == 0 ? this->destinations[inlet] : depot)->index, {OrderDepotTypeFlag::PartOfOrders});
					REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, id, order_index, order).Succeeded());
				}
				ProcessOrders(train);
				REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, id, true).Succeeded());
			}
		}
		UpdateSignalsInBuffer();
	}

	/** Destroy actual vehicle/packet/order owners before their real depots,utility accounts and map disappear. */
	~TubePerformanceRig()
	{
		// Retire native vehicle/order/cargo ownership before their world and engine pools disappear.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		ResetTubePowerState(); ResetUtilityPowerState(); ResetLayerPortalWorkState();
		_group_pool.CleanPool(); _depot_pool.CleanPool(); _engine_pool.CleanPool();
		_engine_mngr.mappings = std::move(this->mappings);
	}
};

/** Actual physical services use the one shared epoch and native whole-work participants under fixed budgets. */
TEST_CASE("Physical tube inlets and native capsule work expose bounded conserved performance", "[.tube-work-performance]")
{
	const auto connections = TubePerformanceArgument("OTTD_TUBE_CONNECTIONS"), path_length = TubePerformanceArgument("OTTD_TUBE_PATH");
	const auto vehicles = TubePerformanceArgument("OTTD_TUBE_VEHICLES"), jobs = TubePerformanceArgument("OTTD_TUBE_JOBS");
	const auto motion = TubePerformanceArgument("OTTD_TUBE_MOTION");
	REQUIRE(connections <= MAX_TUBE_POWER_CONNECTIONS); REQUIRE((path_length == 2 || path_length == 64 || path_length == 128));
	REQUIRE(connections * path_length <= MAX_TUBE_POWER_PATH_CELLS); REQUIRE(vehicles <= MAX_UTILITY_INTERVAL_ACCOUNTS);
	REQUIRE((vehicles == 0 || connections > 0)); REQUIRE((motion == 0 || motion == 1)); REQUIRE((motion == 0 || vehicles == connections));
	const auto setup_begin = std::chrono::steady_clock::now();
	TubePerformanceRig world(connections, path_length, vehicles, jobs, motion != 0);
	const auto setup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - setup_begin).count();
	const auto initial_cash = Company::Get(_current_company)->money;
	const auto initial_expenses = Company::Get(_current_company)->yearly_expenses;
	const auto &terms = GetMultiLayerTubePowerContract();
	BudgetSupport support;
	std::vector<uint8_t> outcomes(vehicles);
	std::vector<std::tuple<TileIndex, int, int>> positions(vehicles);
	AutoRestoreBackup unpaused(_pause_mode, PauseModes{});
	uint64_t moved = 0, serviced = 0, waiting = 0, measured_ticks = 0;
	const uint rounds = motion ? 64 : 1;
	// Warm one native batch, then retain five equally sized measured batches without renewing work in the clock observer.
	for (uint sample = 0; sample < 6; ++sample) {
		int64_t preparation_ns = 0, work_ns = 0, jobs_ns = 0, checks_ns = 0;
		uint64_t sample_moved = 0, sample_serviced = 0, sample_waiting = 0;
		// Every round is one real common allocation and physical work opportunity; timing cannot supply energy or movement.
		for (uint round = 0; round < rounds; ++round) {
			const auto previous_preparation_ns = preparation_ns, previous_work_ns = work_ns, previous_jobs_ns = jobs_ns;
			const auto previous_checks_ns = checks_ns;
			const auto previous_moved = sample_moved, previous_serviced = sample_serviced, previous_waiting = sample_waiting;
			// Repeated prepared repairs are declared fixture input,not simulated
			// wear or a second movement owner. Real service changes the native train.
			if (!motion) for (auto *train : world.capsules) train->reliability = 0;
			// Public construction can queue signals outside the full application dispatcher.
			UpdateSignalsInBuffer(); ++TimerGameTick::counter;
			const auto prepare_begin = std::chrono::steady_clock::now();
			if (vehicles != 0) {
				PrepareTubePowerTick(); REQUIRE(GetTubePowerPreparedTick() == TimerGameTick::counter);
			} else {
				// Explicit source-only owner measurement:the ordinary global tick
				// intentionally does no work without an active registered consumer.
				{ AutoRestoreBackup system(_current_company, OWNER_DEITY); if (IsUtilityPowerAllocated()) REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); }
				// Reconcile every actual installed source against its retained shared-owner history.
				for (auto source : world.sources) REQUIRE(PublishTubeExternalPower(source, GetUtilityPowerInterval(), 1) == TubePowerError::None);
				AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
			}
			preparation_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - prepare_begin).count();
			if (motion) for (size_t index = 0; index < world.capsules.size(); ++index) {
				const auto *train = world.capsules[index]; positions[index] = {train->tile, train->x_pos, train->y_pos};
			}
			const auto work_begin = std::chrono::steady_clock::now();
			// Observe each original capsule and packet separately so aggregate movement cannot hide lost or duplicated custody.
			for (size_t index = 0; index < world.capsules.size(); ++index) {
				auto *train = world.capsules[index];
				if (motion) outcomes[index] = train->Tick();
				else { CapsuleDepotService participant(*train); outcomes[index] = AdvanceTubePowerTick(*train, participant); }
			}
			work_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - work_begin).count();
			// Diagnostics observe the actual completed batch. Their additional
			// binding scans are retained separately,not charged to native work.
			const auto checks_begin = std::chrono::steady_clock::now();
			// Observe each original capsule and packet separately so aggregate movement cannot hide lost or duplicated custody.
			for (size_t index = 0; index < world.capsules.size(); ++index) {
				auto *train = world.capsules[index];
				if (motion) {
					REQUIRE(outcomes[index]); REQUIRE_FALSE(train->vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(train));
					if (positions[index] != std::tuple{train->tile, train->x_pos, train->y_pos}) ++sample_moved;
				} else {
					if (outcomes[index]) {
						++sample_serviced; REQUIRE(train->reliability == train->GetEngine()->reliability);
					} else {
						++sample_waiting; REQUIRE(train->reliability == 0); REQUIRE(GetTubePowerVehicle(train->index)->wait == TubePowerWait::Grant);
					}
				}
				REQUIRE(train->cargo.StoredCount() == 1); REQUIRE(train->cargo.Packets()->size() == 1);
				REQUIRE(train->cargo.Packets()->front()->index == world.packets[index]);
			}
			checks_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - checks_begin).count();
			const auto jobs_begin = std::chrono::steady_clock::now();
			for (auto job : world.jobs) REQUIRE(AdvanceExcavationWork(job, 1, support) == ExcavationError::None);
			jobs_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - jobs_begin).count();
			if (sample != 0) fmt::print("TUBE_WORK_TICK index={} round={} preparation_ns={} work_ns={} jobs_ns={} moved={} serviced={} waiting={} checks_ns={}\n",
				sample - 1, round, preparation_ns - previous_preparation_ns, work_ns - previous_work_ns, jobs_ns - previous_jobs_ns,
				sample_moved - previous_moved, sample_serviced - previous_serviced, sample_waiting - previous_waiting, checks_ns - previous_checks_ns);
		}
		moved += sample_moved; serviced += sample_serviced; waiting += sample_waiting;
		if (sample != 0) {
			measured_ticks += rounds;
			fmt::print("TUBE_WORK_SAMPLE index={} ticks={} preparation_ns={} work_ns={} jobs_ns={} moved={} serviced={} waiting={} checks_ns={}\n",
				sample - 1, rounds, preparation_ns, work_ns, jobs_ns, sample_moved, sample_serviced, sample_waiting, checks_ns);
		}
	}
	const auto state = CaptureUtilityPowerState();
	uint64_t generated = 0, consumed = 0, invoice = 0, remainders = 0;
	// Reconcile every actual installed source against its retained shared-owner history.
	for (auto source : world.sources) {
		const auto *producer = GetUtilityPowerProducer(source); const auto *connection = GetTubePowerConnection(source);
		REQUIRE(producer->generated_lifetime_j == 6 * rounds * terms.capacity_w * 27 / 1000);
		REQUIRE(connection->usage_remainder_j == producer->consumed_lifetime_j % terms.joules_per_money);
		REQUIRE(connection->invoiced_money == terms.lease_per_day + producer->consumed_lifetime_j / terms.joules_per_money);
		generated += producer->generated_lifetime_j; consumed += producer->consumed_lifetime_j;
		invoice += connection->invoiced_money; remainders += connection->usage_remainder_j;
	}
	REQUIRE(consumed <= generated);
	const auto usage_cost = static_cast<int64_t>(invoice - connections * terms.lease_per_day);
	REQUIRE(Company::Get(_current_company)->money == initial_cash - Money{usage_cost});
	// Check all original expense categories across every retained financial year; no invoice can hide in another category.
	for (size_t year = 0; year < initial_expenses.size(); ++year) for (auto expense : EnumRange{ExpensesType::End})
		REQUIRE(Company::Get(_current_company)->yearly_expenses[year][expense] == initial_expenses[year][expense] + Money{year == 0 && expense == ExpensesType::Other ? usage_cost : 0});
	if (!motion) REQUIRE(consumed == serviced * 540);
	if (motion && vehicles != 0) REQUIRE(moved > 0);
	for (auto id : world.jobs) {
		const auto *job = GetExcavationJob(id); REQUIRE(job != nullptr); REQUIRE(job->cursor == 0); REQUIRE(job->completed_earth_t == 0); REQUIRE(job->completed_rock_t == 0);
	}
	REQUIRE(ValidateTubePowerConnections(CaptureTubePowerConnections(), GetCurrentMultiLayerProfileSelection()) == TubePowerError::None);
	REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	REQUIRE(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	fmt::print("TUBE_WORK_COMPLETE width=512 connections={} path_length={} path_cells={} vehicles={} jobs={} motion={} warmup_ticks={} measured_ticks={} samples=5 generated_j={} consumed_j={} invoiced={} usage_cost={} remainder_j={} serviced={} waiting={} moved={} setup_ns={} exact=1 rendering=0\n",
		connections, path_length, connections * path_length, vehicles, jobs, motion, rounds, measured_ticks,
		generated, consumed, invoice, usage_cost, remainders, serviced, waiting, moved, setup_ns);
}

#endif /* TESTS_TUBE_PERFORMANCE_FIXTURE_H */
