/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_freight_fixture.h Ordinary paid freight on the exact generated normal-start world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_FREIGHT_FIXTURE_H
#define TESTS_NORMAL_GROUND_FREIGHT_FIXTURE_H

#include "normal_ground_readiness_fixture.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../misc_cmd.h"
#include "../order_cmd.h"
#include "../station_base.h"
#include "../industry_map.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../track_func.h"
#include "../direction_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_realtime.h"
#include <queue>
#include <set>
#include <map>

/** One proposed ordinary piece;the proposal never owns terrain or train routing. */
struct NormalFreightRail {
	TileIndex tile;
	Track track;
};

/** @param tile Surface platform anchor. @return Pure ordinary four-tile station estimate. */
inline CommandCost QuoteNormalFreightStation(TileIndex tile)
{
	return Command<Commands::BuildRailStation>::Do({}, tile, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{4},
		STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
}

/** @param industry Actual generated industry. @param depot Whether the source needs a buildable northern depot. @return Closest native legal platform proposal. */
inline TileIndex SelectNormalFreightStation(const Industry &industry, bool depot)
{
	std::vector<std::pair<uint, TileIndex>> candidates;
	const int ix = TileX(industry.location.tile), iy = TilePlaneY(industry.location.tile);
	std::vector<TileIndex> industrial_cells;
	for (uint dy = 0; dy < industry.location.h; ++dy) for (uint dx = 0; dx < industry.location.w; ++dx) {
		const auto cell = TileXYL(ix + dx, iy + dy, MapLayer::Surface);
		if (IsTileType(cell, TileType::Industry) && GetIndustryIndex(cell) == industry.index) industrial_cells.push_back(cell);
	}
	REQUIRE_FALSE(industrial_cells.empty());
	for (int y = iy - 6; y <= iy + industry.location.h + 6; ++y) for (int x = ix - 6; x <= ix + industry.location.w + 6; ++x) {
		if (x < 2 || y < 2 || x + 2 >= static_cast<int>(Map::SizeX()) || y + 7 >= static_cast<int>(Map::SizeY())) continue;
		const auto tile = TileXYL(x, y, MapLayer::Surface);
		bool dry = true;
		for (int offset = -1; offset <= 4; ++offset) {
			const auto cell = TileXYL(x, y + offset, MapLayer::Surface);
			dry &= IsTileType(cell, TileType::Clear) || IsTileType(cell, TileType::Trees);
		}
		if (!dry) continue;
		// A large industry's usable edge need not be its northwestern anchor.
		// Verify actual native catchment again after construction.
		uint distance = UINT_MAX;
		for (const auto cell : industrial_cells) {
			const int cx = TileX(cell), cy = TilePlaneY(cell);
			if (std::abs(x - cx) <= 3 && cy >= y - 3 && cy <= y + 6) {
				distance = std::min(distance, static_cast<uint>(std::abs(x - cx) + std::abs(y - cy)));
			}
		}
		if (distance == UINT_MAX) continue;
		if (!QuoteNormalFreightStation(tile).Succeeded()) continue;
		if (depot && !Command<Commands::BuildRailDepot>::Do({}, TileXYL(x, y - 1, MapLayer::Surface),
			RAILTYPE_RAIL, DiagDirection::SE).Succeeded()) continue;
		candidates.emplace_back(distance, tile);
	}
	REQUIRE_FALSE(candidates.empty());
	std::ranges::sort(candidates);
	return candidates.front().second;
}

/**
 * Select a buildable proposal using only native per-piece estimates.
 * @param source Four-tile source platform. @param destination Four-tile terminus.
 * @param existing_destination_approach Admit the actual same-owner conventional rail only at the final approach;new pieces still require native estimates.
 * @return Ordinary track proposals;the native pathfinder still owns every actual train movement.
 */
inline std::vector<NormalFreightRail> SelectNormalFreightRails(TileIndex source, TileIndex destination, bool existing_destination_approach = false)
{
	const auto start = TileXYL(TileX(source), TilePlaneY(source) + 4, MapLayer::Surface);
	const auto finish = TileXYL(TileX(destination), TilePlaneY(destination) - 1, MapLayer::Surface);
	std::set<TileIndex> excluded;
	for (uint y = 0; y < 4; ++y) {
		excluded.insert(TileXYL(TileX(source), TilePlaneY(source) + y, MapLayer::Surface));
		excluded.insert(TileXYL(TileX(destination), TilePlaneY(destination) + y, MapLayer::Surface));
	}
	excluded.insert(TileXYL(TileX(source), TilePlaneY(source) - 1, MapLayer::Surface));
	// A port alone loses the previous piece's tangent. Native no-90 routing
	// excludes some joined consecutive curves,even when each builds legally.
	const size_t count = static_cast<size_t>(Map::SizeX()) * Map::SizeY() * 16;
	const auto key = [](TileIndex tile, Trackdir previous) { return static_cast<size_t>(tile.base()) * 16 + to_underlying(previous); };
	const auto station_base = TrackToTrackdir(Track::Y);
	const auto station_td = TrackdirToExitdir(station_base) == DiagDirection::SE ? station_base : ReverseTrackdir(station_base);
	const auto initial = key(start, station_td);
	std::vector<int64_t> distance(count, INT64_MAX);
	std::vector<size_t> parent(count, SIZE_MAX);
	std::vector<Track> chosen(count, Track::Invalid);
	using Entry = std::pair<int64_t, size_t>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
	distance[initial] = 0;
	pending.emplace(0, initial);
	size_t last = SIZE_MAX;
	Track final_track = Track::Invalid;
	while (!pending.empty()) {
		const auto [cost, index] = pending.top(); pending.pop();
		if (cost != distance[index]) continue;
		const auto tile = TileIndex{static_cast<uint32_t>(index / 16)};
		const auto previous_td = static_cast<Trackdir>(index % 16);
		const auto incoming = ReverseDiagDir(TrackdirToExitdir(previous_td));
		if (excluded.contains(tile)) continue;
		const bool owned_finish = existing_destination_approach && tile == finish && IsPlainRailTile(tile) &&
			GetTileOwner(tile) == _current_company.base() && GetRailType(tile) == RAILTYPE_RAIL;
		if (!IsTileType(tile, TileType::Clear) && !IsTileType(tile, TileType::Trees) && !owned_finish) continue;
		for (uint part = 0; part < 6; ++part) {
			const auto track = static_cast<Track>(part);
			const auto td = TrackToTrackdir(track);
			const auto a = TrackdirToExitdir(td), b = TrackdirToExitdir(ReverseTrackdir(td));
			if (incoming != a && incoming != b) continue;
			const auto current_td = incoming == a ? ReverseTrackdir(td) : td;
			if (!TrackdirReachesTrackdirs(previous_td).Test(current_td)) continue;
			if (_settings_game.pf.forbid_90_deg && TrackdirCrossesTrackdirs(previous_td).Test(current_td)) continue;
			const auto outgoing = TrackdirToExitdir(current_td);
			const auto quote = Command<Commands::BuildRail>::Do({}, tile, RAILTYPE_RAIL, track, false);
			const bool retained_piece = owned_finish && GetTrackBits(tile).Test(track);
			if (!retained_piece && quote.Failed()) continue;
			if (tile == finish && outgoing == DiagDirection::SE &&
				(!_settings_game.pf.forbid_90_deg || !TrackdirCrossesTrackdirs(current_td).Test(station_td))) {
				last = index; final_track = track; break;
			}
			const auto step = TileIndexDiffCByDiagDir(outgoing);
			const auto next = TileAddWrap(tile, step.x, step.y);
			if (next == INVALID_TILE || TileX(next) < 1 || TilePlaneY(next) < 1 ||
				TileX(next) + 1 >= Map::SizeX() || TilePlaneY(next) + 1 >= Map::SizeY()) continue;
			const auto next_index = key(next, current_td);
			const int64_t next_cost = cost + (retained_piece ? 1 : std::max<int64_t>(1, quote.GetCost().base())) + 500;
			if (next_cost >= distance[next_index]) continue;
			distance[next_index] = next_cost; parent[next_index] = index; chosen[next_index] = track;
			pending.emplace(next_cost, next_index);
		}
		if (last != SIZE_MAX) break;
	}
	REQUIRE(last != SIZE_MAX);
	std::vector<NormalFreightRail> route{{finish, final_track}};
	while (last != initial) {
		const auto previous = parent[last]; REQUIRE(previous != SIZE_MAX);
		route.push_back({TileIndex{static_cast<uint32_t>(previous / 16)}, chosen[last]});
		last = previous;
	}
	std::ranges::reverse(route);
	std::set<TileIndex> unique;
	for (const auto &piece : route) REQUIRE(unique.insert(piece.tile).second);
	return route;
}

/**
 * Pay a normal public command and reconcile its actual invoice immediately.
 * @tparam command Native command owner. @param label Invoice category. @param args Exact native arguments.
 */
template <Commands command, typename First, typename... Rest>
inline void PayNormalFreightCommand(std::string_view label, First first, Rest... rest)
{
	const auto cost = [&]() {
		if constexpr (GetCommandFlags<command>().Test(CommandFlag::Location)) {
			return ExtractCommandCost(Command<command>::Do({}, rest...));
		} else {
			return ExtractCommandCost(Command<command>::Do({}, first, rest...));
		}
	}();
	INFO(label);
	REQUIRE(cost.Succeeded());
	const auto *company = Company::Get(_current_company);
	const auto before = company->money;
	REQUIRE(Command<command>::Post(first, rest...));
	CHECK(company->money == before - cost.GetCost());
	fmt::print("NORMAL_FREIGHT_INVOICE action={} cost={} cash={}\n", label, cost.GetCost().base(), company->money.base());
}

/** Build a genuine paid conventional coal service on the supplied untouched normal game. */
inline void BuildNormalGroundFreight()
{
	REQUIRE(_game_mode == GameMode::Normal);
	REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company));
	AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company);
	const char *label_text = std::getenv("OTTD_NORMAL_GROUND_CARGO"); REQUIRE(label_text != nullptr);
	const std::string_view label{label_text};
	REQUIRE((label == "COAL" || label == "WOOD" || label == "IORE" || label == "OIL_"));
	const bool original = label == "COAL";
	if (original) { CHECK(company->money == Money{100000}); CHECK(company->current_loan == Money{100000}); }
	else { REQUIRE(company->current_loan == Money{300000}); REQUIRE(company->money > Money{0}); }
	const auto opening_cash = company->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto coal = GetCargoTypeByLabel(label == "COAL" ? CT_COAL : label == "WOOD" ? CT_WOOD : label == "IORE" ? CT_IRON_ORE : CT_OIL);
	REQUIRE(IsValidCargoType(coal));
	const Industry *producer = nullptr, *consumer = nullptr;
	for (const auto *industry : Industry::Iterate()) {
		if (industry->IsCargoProduced(coal) && producer == nullptr) producer = industry;
		if (industry->IsCargoAccepted(coal) && consumer == nullptr) consumer = industry;
	}
	REQUIRE(producer != nullptr); REQUIRE(consumer != nullptr);
	if (original) CHECK(producer->GetCargoProduced(coal)->waiting == 0);
	const auto source = SelectNormalFreightStation(*producer, true);
	const auto destination = SelectNormalFreightStation(*consumer, false);
	const auto depot = TileXYL(TileX(source), TilePlaneY(source) - 1, MapLayer::Surface);
	const auto rails = SelectNormalFreightRails(source, destination);
	fmt::print("NORMAL_FREIGHT_PROPOSAL producer={} consumer={} cargo={} source={} destination={} depot={} rails={}\n",
		producer->index.base(), consumer->index.base(), coal, source.base(), destination.base(), depot.base(), rails.size());
	std::fflush(stdout);
	EngineID locomotive = EngineID::Invalid(), wagon = EngineID::Invalid();
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		const auto *info = RailVehInfo(engine->index);
		if (!info->railtypes.Test(RAILTYPE_RAIL)) continue;
		if (info->railveh_type == RailVehicleType::Singlehead && info->power >= 1000 &&
			(locomotive == EngineID::Invalid() || engine->GetCost() < Engine::Get(locomotive)->GetCost())) locomotive = engine->index;
		if (info->railveh_type == RailVehicleType::Wagon && engine->GetDefaultCargoType() == coal && engine->CanCarryCargo() &&
			(wagon == EngineID::Invalid() || engine->GetCost() < Engine::Get(wagon)->GetCost())) wagon = engine->index;
	}
	REQUIRE(locomotive != EngineID::Invalid()); REQUIRE(wagon != EngineID::Invalid());
	if (original) {
		REQUIRE(Command<Commands::IncreaseLoan>::Post(LoanCommand::Max, Money{0}));
		CHECK(company->money == Money{300000});
	}
	CHECK(company->current_loan == Money{300000});
	for (const auto tile : {source, destination}) PayNormalFreightCommand<Commands::BuildRailStation>("station", tile,
		RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{4}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	PayNormalFreightCommand<Commands::BuildRailDepot>("depot", depot, RAILTYPE_RAIL, DiagDirection::SE);
	for (const auto &piece : rails) PayNormalFreightCommand<Commands::BuildRail>("rail", piece.tile, RAILTYPE_RAIL, piece.track, false);
	const auto source_id = GetStationIndex(source), destination_id = GetStationIndex(destination);
	const auto *source_station = Station::Get(source_id), *destination_station = Station::Get(destination_id);
	bool covered = false;
	for (uint dy = 0; dy < producer->location.h; ++dy) for (uint dx = 0; dx < producer->location.w; ++dx) {
		const auto cell = TileXYL(TileX(producer->location.tile) + dx, TilePlaneY(producer->location.tile) + dy, MapLayer::Surface);
		if (IsTileType(cell, TileType::Industry) && GetIndustryIndex(cell) == producer->index && source_station->catchment_tiles.Contains(cell)) covered = true;
	}
	REQUIRE(covered);
	REQUIRE(destination_station->goods[coal].status.Test(GoodsEntry::State::Acceptance));
	std::set<VehicleID> owned;
	for (const auto *vehicle : Train::Iterate()) owned.insert(vehicle->index);
	PayNormalFreightCommand<Commands::BuildVehicle>("locomotive", depot, locomotive, false, INVALID_CARGO, ClientID::Invalid);
	Train *front = nullptr;
	for (auto *vehicle : Train::Iterate()) if (!owned.contains(vehicle->index) && vehicle->IsFrontEngine()) front = vehicle;
	REQUIRE(front != nullptr);
	for (uint count = 0; count < 6; ++count) {
		owned.clear(); for (const auto *vehicle : Train::Iterate()) owned.insert(vehicle->index);
		PayNormalFreightCommand<Commands::BuildVehicle>("wagon", depot, wagon, false, INVALID_CARGO, ClientID::Invalid);
		Train *part = nullptr;
		for (auto *vehicle : Train::Iterate()) if (!owned.contains(vehicle->index)) part = vehicle;
		REQUIRE(part != nullptr);
		if (part->First() != front) PayNormalFreightCommand<Commands::MoveRailVehicle>("couple", depot, part->index, front->Last()->index, false);
	}
	for (uint index = 0; index < 2; ++index) {
		Order order; order.MakeGoToStation(index == 0 ? source_id : destination_id);
		if (index == 0) order.SetLoadType(OrderLoadType::FullLoadAny);
		PayNormalFreightCommand<Commands::InsertOrder>("order", depot, front->index, static_cast<VehicleOrderID>(index), order);
	}
	PayNormalFreightCommand<Commands::StartStopVehicle>("start", depot, front->index, true);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-freight");
	for (const auto name : {"freight-ready.sav", "freight-ready-repeat.sav"}) {
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	}
	VehicleID anchor = front->index; uint fleet = 0;
	for (const auto *vehicle : Train::Iterate()) if (vehicle->owner == _current_company && vehicle->IsFrontEngine()) {
		anchor = std::min(anchor, vehicle->index); ++fleet;
	}
	fmt::print("NORMAL_FREIGHT_READY front={} purchased_front={} fleet={} locomotive={} wagon={} opening_cash={} cash={} loan={} tick={}\n", anchor.base(),
		front->index.base(), fleet, locomotive.base(), wagon.base(), opening_cash.base(), company->money.base(), company->current_loan.base(), TimerGameTick::counter);
	std::fflush(stdout);
}

/** Load the exact normal original with the ordinary application;only public commands establish the service. */
inline void RunNormalGroundFreight()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> observe = [&]() {
		try { BuildNormalGroundFreight(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &observe};
	IConsole::CmdRegister("normal_ground_freight", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

/** Read-only running totals across native economic quarter rotation. */
struct NormalFreightEarnings {
	CompanyID owner;
	int64_t quarter;
	Money seen_income, seen_expenses;
	uint64_t seen_delivered;
	Money income = 0, expenses = 0;
	uint64_t delivered = 0;
	int32_t year;
	Expenses seen_ledger, ledger{};
	CargoArray seen_cargo;
	std::array<uint64_t, NUM_CARGO> delivered_by_cargo{};

	/** @param company Actual original company with its native transport ledger. */
	NormalFreightEarnings(const Company &company) : owner(company.index), quarter(Quarter()),
		seen_income(company.cur_economy.income), seen_expenses(company.cur_economy.expenses),
		seen_delivered(company.cur_economy.delivered_cargo.GetSum<uint64_t>()), year(TimerGameEconomy::year.base()),
		seen_ledger(company.yearly_expenses[0]), seen_cargo(company.cur_economy.delivered_cargo) {}

	/** @return Native economy quarter;calendar speed never substitutes for economy time. */
	static int64_t Quarter() { return static_cast<int64_t>(TimerGameEconomy::year.base()) * 4 + TimerGameEconomy::month / 3; }

	/** Observe actual native entries,including the final prior-quarter record after rotation. */
	void Observe()
	{
		const auto *company = Company::Get(owner);
		const auto current_quarter = Quarter();
		if (current_quarter != quarter) {
			REQUIRE(current_quarter == quarter + 1);
			const auto &previous = company->old_economy[0];
			income += previous.income - seen_income;
			expenses += previous.expenses - seen_expenses;
			delivered += previous.delivered_cargo.GetSum<uint64_t>() - seen_delivered;
			for (uint type = 0; type < NUM_CARGO; ++type) {
				delivered_by_cargo[type] += previous.delivered_cargo[type] - seen_cargo[type]; seen_cargo[type] = 0;
			}
			seen_income = 0; seen_expenses = 0; seen_delivered = 0;
			quarter = current_quarter;
		}
		income += company->cur_economy.income - seen_income;
		expenses += company->cur_economy.expenses - seen_expenses;
		delivered += company->cur_economy.delivered_cargo.GetSum<uint64_t>() - seen_delivered;
		for (uint type = 0; type < NUM_CARGO; ++type) {
			delivered_by_cargo[type] += company->cur_economy.delivered_cargo[type] - seen_cargo[type];
			seen_cargo[type] = company->cur_economy.delivered_cargo[type];
		}
		seen_income = company->cur_economy.income; seen_expenses = company->cur_economy.expenses;
		seen_delivered = company->cur_economy.delivered_cargo.GetSum<uint64_t>();
		const auto current_year = TimerGameEconomy::year.base();
		for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
			const auto category = static_cast<ExpensesType>(index);
			if (current_year != year) {
				REQUIRE(current_year == year + 1);
				ledger[category] += company->yearly_expenses[1][category] - seen_ledger[category];
				seen_ledger[category] = 0;
			}
			ledger[category] += company->yearly_expenses[0][category] - seen_ledger[category];
			seen_ledger[category] = company->yearly_expenses[0][category];
		}
		year = current_year;
	}
};

/** Run a bounded real native production/carriage/delivery segment from an exact paid predecessor save. */
inline void RunNormalGroundFreightEarnings()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	const char *budget_text = std::getenv("OTTD_NORMAL_GROUND_TICKS"); REQUIRE(budget_text != nullptr);
	const std::string_view budget_argument{budget_text}; uint32_t budget = 0;
	const auto parsed = std::from_chars(budget_argument.data(), budget_argument.data() + budget_argument.size(), budget);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == budget_argument.data() + budget_argument.size());
	REQUIRE(budget > 0); REQUIRE(budget <= 65536);
	bool started = false, requested = false, finished = false;
	std::exception_ptr failure;
	CompanyID owner = CompanyID::Invalid(); VehicleID train = VehicleID::Invalid();
	Money cash = 0, loan = 0;
	uint64_t first_tick = 0; uint peak_load = 0;
	std::set<TileIndex> visited;
	std::map<VehicleID, std::set<TileIndex>> fleet_visits;
	std::map<VehicleID, uint> fleet_loads;
	std::unique_ptr<NormalFreightEarnings> earnings;
	std::optional<decltype(ComputeGeologyStateDigest(GetGeologyWorldState()))> geology;
	std::function<void()> begin = [&]() {
		try {
			REQUIRE(Company::IsValidID(_local_company)); owner = _local_company;
			const auto *company = Company::Get(owner); cash = company->money; loan = company->current_loan;
			for (const auto *vehicle : Train::Iterate()) if (vehicle->owner == owner && vehicle->IsFrontEngine()) {
				if (train == VehicleID::Invalid() || vehicle->index < train) train = vehicle->index;
				fleet_visits.emplace(vehicle->index, std::set<TileIndex>{}); fleet_loads.emplace(vehicle->index, 0);
			}
			REQUIRE(train != VehicleID::Invalid());
			earnings = std::make_unique<NormalFreightEarnings>(*company);
			geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			first_tick = TimerGameTick::counter;
			AutoRestoreBackup actor{_current_company, owner};
			if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
			started = true;
		} catch (...) { failure = std::current_exception(); }
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &begin};
	IConsole::CmdRegister("normal_ground_earn", ObserveNormalGroundNewgame);
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint) {
		if (!started || requested || failure) return;
		try {
			earnings->Observe();
			for (auto &[id, cells] : fleet_visits) {
				const auto *front = Train::Get(id); cells.insert(front->tile); visited.insert(front->tile);
				uint load = 0; for (const auto *part = front; part != nullptr; part = part->Next()) load += part->cargo.StoredCount();
				fleet_loads[id] = std::max(fleet_loads[id], load); peak_load = std::max(peak_load, load);
			}
			if (TimerGameTick::counter - first_tick >= budget) requested = true;
		} catch (...) { failure = std::current_exception(); }
	});
	// Native real-time callbacks run between complete GameLoop frames. Only
	// an ordinary pause command stops this fixed-budget driver before saving.
	const IntervalTimer<TimerGameRealtime> saver({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		if (!started || !requested || finished || failure) return;
		try {
			earnings->Observe();
			AutoRestoreBackup actor{_current_company, owner};
			REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			const auto *company = Company::Get(owner);
			CHECK(company->current_loan == loan);
			Money total_cost = 0;
			for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
				const auto cost = earnings->ledger[static_cast<ExpensesType>(index)]; total_cost += cost;
				fmt::print("NORMAL_FREIGHT_LEDGER category={} cost={}\n", index, cost.base());
			}
			CHECK(company->money == cash - total_cost);
			CHECK(earnings->income == -earnings->ledger[ExpensesType::TrainRevenue]);
			CHECK(earnings->expenses == -earnings->ledger[ExpensesType::TrainRun] - earnings->ledger[ExpensesType::Property] - earnings->ledger[ExpensesType::LoanInterest]);
			CHECK(company->money == cash + earnings->income + earnings->expenses - earnings->ledger[ExpensesType::Other]);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == *geology);
			REQUIRE(visited.size() > 20); REQUIRE(peak_load > 0);
			for (const auto &[id, cells] : fleet_visits) {
				const auto *front = Train::Get(id);
				uint held = 0, capacity = 0;
				for (const auto *part = front; part != nullptr; part = part->Next()) { held += part->cargo.StoredCount(); capacity += part->cargo_cap; }
				const bool waiting = front->current_order.IsType(OT_LOADING) && front->current_order.IsFullLoadOrder() &&
					front->cur_speed == 0 && IsTileType(front->tile, TileType::Station) &&
					GetStationIndex(front->tile) == front->last_station_visited &&
					Station::Get(front->last_station_visited)->owner == owner && held < capacity;
				fmt::print("NORMAL_FREIGHT_TRAIN front={} visited={} peak_load={} waiting={} held={} capacity={} station={}\n", id.base(),
					cells.size(), fleet_loads.at(id), waiting ? 1 : 0, held, capacity, front->last_station_visited.base());
				// Later native source closure can leave a real partially loaded
				// full-load order waiting. Preserve and report its cargo;only the
				// other services' actual CargoPayment contributes financial income.
				REQUIRE(((cells.size() > 20 && fleet_loads.at(id) > 0) || waiting));
			}
			REQUIRE(earnings->delivered > 0); REQUIRE(earnings->income > Money{0});
			uint64_t delivered_sum = 0;
			for (uint type = 0; type < NUM_CARGO; ++type) {
				const auto amount = earnings->delivered_by_cargo[type]; delivered_sum += amount;
				if (amount > 0) fmt::print("NORMAL_FREIGHT_DELIVERED cargo={} amount={}\n", type, amount);
			}
			CHECK(delivered_sum == earnings->delivered);
			const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-earnings");
			for (const auto name : {"freight-earned.sav", "freight-earned-repeat.sav"}) {
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
			}
			fmt::print("NORMAL_FREIGHT_EARNED front={} first_tick={} tick={} opening_cash={} cash={} loan={} income={} expenses={} overhead={} delivered={} visited={} peak_load={}\n",
				train.base(), first_tick, TimerGameTick::counter, cash.base(), company->money.base(), loan.base(),
				earnings->income.base(), earnings->expenses.base(), earnings->ledger[ExpensesType::Other].base(), earnings->delivered, visited.size(), peak_load);
			std::fflush(stdout); finished = true;
		} catch (...) { failure = std::current_exception(); }
	});
	const auto driver = fmt::format("null:ticks={}", budget + 3000);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", driver, "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(started); REQUIRE(finished);
}

#endif /* TESTS_NORMAL_GROUND_FREIGHT_FIXTURE_H */
