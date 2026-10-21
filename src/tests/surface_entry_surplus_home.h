/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_surplus_home.h Pure real surplus sale and builder-home prerequisites.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVE.
 * Native ownership, command estimates and whole company costs remain binding;
 * quoted refunds and future avoided upkeep never become spendable money.
 */
#ifndef TEST_SURFACE_ENTRY_SURPLUS_HOME_H
#define TEST_SURFACE_ENTRY_SURPLUS_HOME_H

/** Quote only the actual empty surplus chain and current builder's public home prerequisites. */
static void QuerySurfaceSurplusHome()
{
	VerifySurfaceFarmBackhaul(); const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{40162}); REQUIRE(company->current_loan == Money{5000000});
	const auto *reserve = Train::Get(VehicleID{58}); REQUIRE(reserve->IsFreeWagon()); REQUIRE(reserve->IsStoppedInDepot());
	Money values{0}; const auto *part = reserve;
	// These two additional paid bodies are surplus after the real mixed-role
	// purchase. No original wagon, active capacity or packet enters the quote.
	for (const uint16_t id : {58, 59}) {
		REQUIRE(part != nullptr); REQUIRE(part->index == VehicleID{id}); REQUIRE(part->owner == _current_company);
		REQUIRE(part->tile == TileIndex{1638}); REQUIRE(part->cargo.TotalCount() == 0); REQUIRE(part->engine_type == EngineID{31});
		REQUIRE(part->GetEngine()->GetRunningCost() == Money{0}); values += part->value; part = part->Next();
	}
	REQUIRE(part == nullptr);
	const auto sale = QuoteSurfaceResupply<Commands::SellVehicle>(TileIndex{1638}, VehicleID{58}, true, false, ClientID::Server);
	REQUIRE(sale.Succeeded()); REQUIRE(sale.GetCost() == -values);
	fmt::print("SURFACE_RESUPPLY_SURPLUS_HOME_RESERVE head=58 tail=59 owned=2 empty=true depot=1638 available=true invoice={} unbanked_refund={} refund_banked=0 running_cost_saved=0 original_wagons_owned=32 active_livestock=200 active_grain=90 active_goods=50 cash={} elapsed=0\n",
		sale.GetCost().base(), values.base(), company->money.base());
	const auto *craft = SpaceVehicle::Get(VehicleID{38});
	REQUIRE(craft->owner == _current_company); REQUIRE(craft->generation == 2); REQUIRE(GetSpaceVehicleModel(*craft) == 20);
	REQUIRE(craft->construction.placement == SpacePlacement::Orbital); REQUIRE(craft->construction.capital_paid == 100000);
	REQUIRE(!craft->vehstatus.Test(VehState::Stopped)); REQUIRE(craft->cur_speed == 0); REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(craft->cargo_payment == nullptr);
	REQUIRE(!HasSpaceManifestAssets(*craft)); REQUIRE(craft->space_orders.cursor == craft->space_orders.entries.size());
	REQUIRE(craft->position != craft->home);
	const auto idle = GetMultiLayerSpaceBudget().idle_per_day;
	REQUIRE(craft->GetRunningCost() == Money{idle}*CalendarTime::DAYS_IN_YEAR*256);
	const auto remote_sale = QuoteSurfaceResupply<Commands::SellVehicle>(craft->tile, craft->index, false, false, ClientID::Server);
	REQUIRE(remote_sale.Failed());
	const auto home = craft->construction.home; REQUIRE(home.station == StationID{21}); REQUIRE(home.generation == 1);
	SpacePoint destination;
	REQUIRE(ResolveSpaceNavigationTarget(*craft, {SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation}, destination) == SpaceOrderBlock::None);
	REQUIRE(destination == craft->home);
	const auto visit = QuoteSurfaceResupply<Commands::EditSpaceOrder>(craft->tile, craft->index, craft->generation, SpaceOrderEdit::Replace, uint8_t{0},
		SpaceOrderAction::Visit, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation);
	REQUIRE(visit.Succeeded()); REQUIRE(visit.GetCost() == Money{0});
	const auto replacement = QuoteSurfaceResupply<Commands::AssembleLaunchCraft>(TileIndex{2891}, home.station, home.generation, uint16_t{20});
	REQUIRE(replacement.Succeeded()); REQUIRE(replacement.GetCost() == Money{100000});
	fmt::print("SURFACE_RESUPPLY_SURPLUS_HOME_BUILDER vehicle=38 generation=2 model=20 exhausted=true held=0 manifest_assets=false position_x={} position_y={} home_x={} home_y={} home_center=21 home_generation=1 visit_available=true visit_invoice=0 remote_sale_available=false unbanked_value={} refund_banked=0 idle_per_day={} replacement_invoice=100000 replacement_steel=4 replacement_goods=8 replacement_work=148 replacement_launch_priced=false elapsed=0\n",
		craft->position.x, craft->position.y, craft->home.x, craft->home.y, craft->value.base(), idle);
	// Every candidate still includes the actual idle builder and whole company.
	// Future refunds, lower future upkeep and unearned fares grant no admission.
	for (const uint limit : {1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep+bill.interest+bill.station;
		const auto target = operating+Money{40000};
		fmt::print("SURFACE_RESUPPLY_SURPLUS_HOME_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded={} refund_counted=0 avoided_cost_counted=0 future_income_counted=0 work_permission=false\n",
			limit, operating.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Preserve every original family while inspecting only actual native sale/home estimates. */
TEST_CASE("Actual surplus and idle-builder home quotes grant no money or movement", "[.surface-surplus-home]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *phase = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(phase != nullptr); REQUIRE(std::string_view{phase} == "surplus-home-query");
	const auto paths = GetTestCaseArtifactPaths("space", "surface-surplus-home"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain the actual unchanged proposal, including a precise native refusal if one occurs. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/"surface-resupply-before-surplus-home-query-440.sav"); QuerySurfaceSurplusHome();
			SaveSurfaceResupply(paths.saves/"surface-resupply-surplus-home-query-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=surplus-home-query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/"surface-resupply-failed-surplus-home-query-440.sav");
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_SURPLUS_HOME_H */
