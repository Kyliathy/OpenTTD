/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_goods_capacity.h Pure dedicated Goods capacity and capital proposal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEI.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
 * Existing native command, packet, railway and accounting owners remain authoritative.
 */
#ifndef TESTS_SPACE_GOODS_CAPACITY_H
#define TESTS_SPACE_GOODS_CAPACITY_H

/** Price a new callback-free body's complete first operating interval.
 * @param engine Actual native model whose complete annual display cost is used.
 * @param ticks Full proposed first interval; new bodies have no pending running ticks.
 * @return Upward-rounded native annual charge with the existing two-unit safety margin.
 */
static Money SpaceGoodsNewUpkeep(const Engine *engine, uint ticks)
{
	const auto annual = engine->GetRunningCost().base(); REQUIRE(annual >= 0);
	constexpr int64_t denominator = CalendarTime::DAYS_IN_YEAR * Ticks::DAY_TICKS;
	return Money{(annual * ticks + denominator - 1) / denominator + 2};
}

/** Retain actual station acceptance, waiting packets and their original provenance.
 * @param station Existing station13source or15receiver; no inventory is allocated.
 * @param goods Current native Goods cargo identity.
 */
static void SpaceGoodsStation(const Station *station, CargoType goods)
{
	const auto &entry = station->goods[goods];
	uint packets = 0, quantity = 0;
	// Waiting freight is demand, never money. Preserve each native packet's
	// first station/source tile and routing bucket instead of inventing cargo.
	if (entry.HasData()) {
		for (const auto &[next, list] : *entry.GetData().cargo.Packets()) {
			for (const auto *packet : list) {
				++packets; quantity += packet->Count();
				fmt::print("SPACE_GOODS_PACKET station={} id={} next={} first={} source_tile={} quantity={}\n",
					station->index.base(), packet->index.base(), next.base(), packet->GetFirstStation().base(), packet->GetSourceTile().base(), packet->Count());
			}
		}
	}
	fmt::print("SPACE_GOODS_STATION station={} tile={} available={} packet_quantity={} packets={} accepts={} platform={} owner={}\n",
		station->index.base(), station->train_station.tile.base(), entry.AvailableCount(), quantity, packets,
		entry.status.Test(GoodsEntry::State::Acceptance), station->GetPlatformLength(station->train_station.tile), station->owner.base());
}

/** Read real original Goods/mixed capacity without changing any coupled body.
 * @param head Existing49Goods or52mixed front engine.
 * @param goods Actual native Goods cargo identity.
 */
static void SpaceGoodsExisting(const Train *head, CargoType goods)
{
	uint parts = 0, capacity = 0, held = 0;
	// Keep each actual part in its paid consist. Other freight capacity cannot
	// be described as additional Goods service or liquidated to fund this quote.
	for (const auto *part = head; part != nullptr; part = part->Next()) {
		++parts;
		if (part->cargo_type != goods) continue;
		capacity += part->cargo_cap; held += part->cargo.TotalCount();
	}
	fmt::print("SPACE_GOODS_EXISTING head={} parts={} capacity={} held={} length={} remaining_days={}\n",
		head->index.base(), parts, capacity, held, head->gcache.cached_total_length, head->max_age.base() - head->age.base());
}

/** Read real demand, native model availability, route fit and complete capital invoices. */
static void QuerySpaceGoodsCapacity()
{
	VerifySurfaceFundedFarmGoods();
	const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{182302}); REQUIRE(company->current_loan == Money{5000000});
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(goods));
	const TileIndex depot{1638}; REQUIRE(IsRailDepotTile(depot)); REQUIRE(GetTileOwner(depot) == _current_company);
	const auto *source = Station::Get(StationID{13}); const auto *receiver = Station::Get(StationID{15});
	REQUIRE(source->owner == _current_company); REQUIRE(receiver->owner == _current_company);
	SpaceGoodsStation(source, goods); SpaceGoodsStation(receiver, goods);
	SpaceGoodsExisting(Train::Get(VehicleID{49}), goods); SpaceGoodsExisting(Train::Get(VehicleID{52}), goods);
	constexpr uint ticks = 2000;
	const auto bill = QuoteSurfaceFixedPriceIncome(ticks);
	const auto operating = bill.upkeep + bill.interest + bill.station;
	const uint platform = std::min(source->GetPlatformLength(source->train_station.tile), receiver->GetPlatformLength(receiver->train_station.tile)) * TILE_SIZE;
	// Record both expected historical models even if native availability has
	// expired. A genuine refusal remains visible when a compatible fallback wins.
	for (const EngineID id : {EngineID{24}, EngineID{32}}) {
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, id, false, id == EngineID{32} ? goods : INVALID_CARGO, ClientID::Server);
		fmt::print("SPACE_GOODS_EXPECTED engine={} available={} callback_free={} quoted={} error={} invoice={} capacity={}\n",
			id.base(), IsEngineBuildable(id, VehicleType::Train, _current_company), Engine::Get(id)->GetGRF() == nullptr,
			std::get<0>(quote).Succeeded(), std::get<0>(quote).GetErrorMessage(), std::get<0>(quote).GetCost().base(), std::get<2>(quote));
	}
	EngineID wagon = EngineID::Invalid(); Money wagon_cost{0}; uint wagon_capacity = 0, wagon_length = 0;
	// Prefer the actual old Goods32when it remains buildable. Otherwise compare
	// only real callback-free native wagons that the depot can build for Goods.
	for (const auto *engine : Engine::Iterate()) {
		// Other vehicle types, callbacks and unavailable designs cannot provide a
		// predictable native invoice or a genuine currently purchasable wagon.
		if (engine->type != VehicleType::Train || engine->GetGRF() != nullptr || !IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		const auto &info = engine->VehInfo<RailVehicleInfo>();
		if (info.railveh_type != RailVehicleType::Wagon) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, engine->index, false, goods, ClientID::Server);
		fmt::print("SPACE_GOODS_WAGON engine={} quoted={} error={} invoice={} capacity={} annual={} length={} life_days={} age_limited=false\n",
			engine->index.base(), std::get<0>(quote).Succeeded(), std::get<0>(quote).GetErrorMessage(), std::get<0>(quote).GetCost().base(),
			std::get<2>(quote), engine->GetRunningCost().base(), 8 - info.shorten_factor, engine->GetLifeLengthInDays().base());
		// A real Goods-capable native wagon is required. CmdBuildRailWagon sets no
		// max_age, and AgeVehicle excludes non-engine wagons from its lifetime /
		// reliability branch. Their calendar age is not a primary service limit;
		// the proposed powered head's complete useful life is checked below.
		if (std::get<0>(quote).Failed() || std::get<2>(quote) == 0) continue;
		const auto cost = std::get<0>(quote).GetCost();
		// Preserve the requested model when possible; fallbacks minimize the
		// native purchase plus first interval instead of inventing a price for32.
		if (wagon == EngineID::Invalid() || engine->index == EngineID{32} || (wagon != EngineID{32} && cost + SpaceGoodsNewUpkeep(engine, ticks) < wagon_cost + SpaceGoodsNewUpkeep(Engine::Get(wagon), ticks))) {
			wagon = engine->index; wagon_cost = cost; wagon_capacity = std::get<2>(quote); wagon_length = 8 - info.shorten_factor;
		}
	}
	EngineID head = EngineID::Invalid(); Money head_cost{0}, best_head_interval{0}; uint head_length = 0;
	// Select a real powered callback-free head; passenger carrying motor units
	// cannot silently substitute for the dedicated freight consist being priced.
	for (const auto *engine : Engine::Iterate()) {
		// Only available native freight heads have predictable full purchase and
		// upkeep charges; arbitrary GRF callbacks cannot authorize this proposal.
		if (engine->type != VehicleType::Train || engine->GetGRF() != nullptr || engine->GetPower() == 0 || engine->CanCarryCargo() || !IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		const auto &info = engine->VehInfo<RailVehicleInfo>();
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(depot, depot, engine->index, false, INVALID_CARGO, ClientID::Server);
		const auto rails = GetAllPoweredRailTypes(info.railtypes);
		const bool reaches_source = SurfaceResupplyRailConnected(depot, source->train_station.tile, rails);
		const bool reaches_receiver = SurfaceResupplyRailConnected(depot, receiver->train_station.tile, rails);
		const uint length = (8 - info.shorten_factor) * (info.railveh_type == RailVehicleType::Multihead ? 2 : 1);
		const auto life = engine->GetLifeLengthInDays().base();
		fmt::print("SPACE_GOODS_HEAD engine={} quoted={} error={} invoice={} annual={} source_connected={} receiver_connected={} length={} life_days={}\n",
			engine->index.base(), std::get<0>(quote).Succeeded(), std::get<0>(quote).GetErrorMessage(), std::get<0>(quote).GetCost().base(), engine->GetRunningCost().base(), reaches_source, reaches_receiver, length, life);
		// A cheap head on disconnected rail or too short a useful life supplies
		// no service. The smallest requested lot must fit both real platforms.
		if (std::get<0>(quote).Failed() || !reaches_source || !reaches_receiver || life <= static_cast<int>(bill.days) || length + 2 * wagon_length > platform) continue;
		const auto invoice = std::get<0>(quote).GetCost(); const auto interval = invoice + SpaceGoodsNewUpkeep(engine, ticks);
		// Keep the explicit expected24quote above, then select the cheapest real
		// eligible full-interval head. An expensive old preference must not hide
		// another genuinely fundable compatible native service proposal.
		if (head == EngineID::Invalid() || interval < best_head_interval) {
			head = engine->index; head_cost = invoice; best_head_interval = interval; head_length = length;
		}
	}
	// Without both real purchased body quotes, every lot remains a refusal;
	// partial native prices never count as a complete new service invoice.
	const bool complete = head != EngineID::Invalid() && wagon != EngineID::Invalid();
	const Money head_upkeep = head == EngineID::Invalid() ? Money{0} : SpaceGoodsNewUpkeep(Engine::Get(head), ticks);
	const Money wagon_upkeep = wagon == EngineID::Invalid() ? Money{0} : SpaceGoodsNewUpkeep(Engine::Get(wagon), ticks);
	fmt::print("SPACE_GOODS_BUDGET cash={} debt={} depot=1638 ticks={} days={} months={} upkeep={} interest={} station={} capital_floor=100000 operating_floor=40000 head={} wagon={} head_invoice={} wagon_invoice={} head_upkeep={} wagon_upkeep={} wagon_capacity={} head_length={} wagon_length={} platform_length={} quote_complete={} year_boundary={} future_fares=0 refunds=0 credit=0 paid=0\n",
		company->money.base(), company->current_loan.base(), ticks, bill.days, bill.months, bill.upkeep.base(), bill.interest.base(), bill.station.base(),
		head.base(), wagon.base(), head_cost.base(), wagon_cost.base(), head_upkeep.base(), wagon_upkeep.base(), wagon_capacity, head_length, wagon_length, platform, complete, bill.year_boundary);
	// Each proposal includes every bought body, its whole first operating bill,
	// the existing company's full bill and restored capital floor. Native order
	// and coupling commands have no money fee; their later real admission remains
	// required. No future fare, salvage or loan room can pay this gross invoice.
	for (const uint count : {2, 4, 8, 12}) {
		const auto invoice = head_cost + wagon_cost * count; const auto added = head_upkeep + wagon_upkeep * count;
		const auto target = Money{100000} + operating + invoice + added;
		const uint length = head_length + wagon_length * count; const bool fits = length <= platform;
		// Demand, accepting delivery, real models, physical fit and cash must all
		// hold before a later capacity producer can even seek full native admission.
		const bool funded = complete && fits && source->goods[goods].AvailableCount() > 0 && receiver->goods[goods].status.Test(GoodsEntry::State::Acceptance) && company->money >= target;
		fmt::print("SPACE_GOODS_LOT wagons={} capacity={} invoice={} added_upkeep={} existing_operating={} target={} cash={} length={} platform_length={} fits={} quote_complete={} funded={} spending_permission=false\n",
			count, wagon_capacity * count, invoice.base(), added.base(), operating.base(), target.base(), company->money.base(), length, platform, fits, complete, funded);
	}
}

/** Query both owner snapshots while retaining the exact task-owned clock/revision marker. */
TEST_CASE("Dedicated Goods capacity has only real native whole capital admission", "[.space-goods-capacity]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "goods-capacity"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Keep query failures visible without allowing any loop tick or paid mutation. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company)); REQUIRE(_pause_mode != PauseModes{});
			const auto tick = TimerGameTick::counter;
			SaveSurfaceResupply(paths.saves / "before.sav"); QuerySpaceGoodsCapacity(); SaveSurfaceResupply(paths.saves / "after.sav");
			REQUIRE(TimerGameTick::counter == tick);
			fmt::print("SPACE_OPERATION_RESULT task=Plan11/15CSGZVEEI phase=goods-capacity-query ticks_before={} ticks_after={} revision_text={} revision_modified={} revision_newgrf={} save_version={} inherited_paid_scope=false\n",
				tick, TimerGameTick::counter, SpaceCheckpointRevisionText(), _openttd_revision_modified, _openttd_newgrf_version, to_underlying(SAVEGAME_VERSION));
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("space_goods_capacity", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TESTS_SPACE_GOODS_CAPACITY_H */
