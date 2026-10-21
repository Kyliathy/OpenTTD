/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_constructor_home.h Fully funded original idle-builder home arrival.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
 * Public orders, native physical navigation, actual books and public stop own
 * this boundary. A later sale remains a separate unbanked capital proposal.
 */
#ifndef TEST_SURFACE_ENTRY_CONSTRUCTOR_HOME_H
#define TEST_SURFACE_ENTRY_CONSTRUCTOR_HOME_H
#include "../autoreplace_cmd.h"

/** Verify only the original empty builder and its exact old or new home queue.
 * @param ordered Whether the genuine public Visit21/gen1must already be installed.
 * @return Actual original38/gen2builder, retaining all its paid material history.
 */
static const SpaceVehicle *VerifySurfaceConstructorHome(bool ordered)
{
	VerifySurfaceFarmBackhaul(true); const auto *craft = SpaceVehicle::Get(VehicleID{38});
	REQUIRE(craft->owner == _current_company); REQUIRE(craft->generation == 2); REQUIRE(GetSpaceVehicleModel(*craft) == 20);
	REQUIRE(craft->construction.placement == SpacePlacement::Orbital); REQUIRE(craft->construction.capital_paid == 100000);
	REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(craft->cargo_payment == nullptr); REQUIRE(!HasSpaceManifestAssets(*craft));
	const auto home = craft->construction.home; REQUIRE(home.station == StationID{21}); REQUIRE(home.generation == 1);
	SpacePoint destination;
	REQUIRE(ResolveSpaceNavigationTarget(*craft, {SpaceOrderTargetKind::LaunchCenter, 21, 1}, destination) == SpaceOrderBlock::None);
	REQUIRE(destination == craft->home); REQUIRE(craft->space_orders.entries.size() == 1);
	const auto &order = craft->space_orders.entries.front();
	if (ordered) {
		REQUIRE(order.action == SpaceOrderAction::Visit); REQUIRE(order.target.kind == SpaceOrderTargetKind::LaunchCenter);
		REQUIRE(order.target.id == 21); REQUIRE(order.target.generation == 1);
	} else {
		REQUIRE(order.action == SpaceOrderAction::Construct); REQUIRE(order.target.kind == SpaceOrderTargetKind::Blueprint);
		REQUIRE(order.target.id == 0); REQUIRE(order.target.generation == 1);
		REQUIRE(craft->space_orders.cursor == craft->space_orders.entries.size());
	}
	return craft;
}

/** Inspect genuine home completion without assigning any physical or saved progress.
 * @param craft Verified original builder following its exact public home Visit.
 * @return Whether native navigation really reached home and exhausted that instruction.
 */
static bool SurfaceConstructorReachedHome(const SpaceVehicle *craft)
{
	return craft->position == craft->home && craft->space_orders.cursor == craft->space_orders.entries.size();
}

/** Price one complete home-only interval through the unchanged full-company cost owner.
 * @param ordered Whether the actual public home Visit is already installed.
 * @param ceiling Maximum candidate interval, at most250ticks and never earned work.
 * @return Longest fully funded250/64candidate; zero preserves an unchanged refusal.
 */
static uint QuoteSurfaceConstructorHome(bool ordered, uint ceiling)
{
	const auto *craft = VerifySurfaceConstructorHome(ordered); const auto *company = Company::Get(_current_company);
	REQUIRE(ceiling > 0); REQUIRE(ceiling <= 250); REQUIRE(!_settings_game.economy.inflation); REQUIRE(_economy.interest_rate == 2);
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->engine_renew_list == nullptr);
	const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months}*30) : 0;
	REQUIRE(craft->max_age.base()-craft->age.base() > 365+lead);
	// A physical Visit cannot start any productive active-rate action. The
	// actual replacement policy also refuses, so arrival cannot buy a hidden
	// replacement or add an unpriced hull to this strictly bounded proposal.
	const auto automatic = QuoteSurfaceResupply<Commands::AutoreplaceVehicle>(craft->tile, craft->index);
	REQUIRE(automatic.Failed());
	uint space = 0;
	// All current bill participants must retain callback-free fixed rates.
	// Only38may travel; the exact stored54rocket remains idle/exhausted at0.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Stopped ground reserves remain outside active charges, as in the
		// original owner. The orbital builder keeps paying even after its stop.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped)) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		REQUIRE(vehicle->GetEngine()->GetGRF() == nullptr);
		if (vehicle->type != VehicleType::Space) continue;
		++space; const auto *body = SpaceVehicle::From(vehicle);
		if (body->index == VehicleID{38}) {
			REQUIRE(body == craft); REQUIRE(body->GetRunningCost() == Money{100}*CalendarTime::DAYS_IN_YEAR*256);
		} else {
			REQUIRE(body->index == VehicleID{54}); REQUIRE(body->generation == 1);
			REQUIRE(body->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(body->GetRunningCost() == Money{0});
			REQUIRE(body->space_orders.cursor == body->space_orders.entries.size());
		}
	}
	REQUIRE(space == 2);
	// Reuse the exact old complete algorithm after these home-specific guards;
	// its upkeep/pending ticks, interest, stations and useful lives stay binding.
	for (const uint limit : {250, 64}) {
		if (limit > ceiling) continue;
		const auto bill = QuoteSurfaceModuleCapitalWork(0, limit); const auto operating = bill.upkeep+bill.interest+bill.station;
		const auto target = operating+Money{40000}; const bool funded = company->money >= target;
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_HOME_ADMISSION ceiling={} ticks={} operating={} reserve=40000 target={} cash={} funded={} idle_per_day=100 automatic_replacement=false refund_counted=0 avoided_cost_counted=0 future_income_counted=0\n",
			ceiling, limit, operating.base(), target.base(), company->money.base(), funded);
		if (funded) return limit;
	}
	return 0;
}

/** Issue only the fully funded genuine builder's zero-cost public home Visit. */
static void OrderSurfaceConstructorHome()
{
	REQUIRE(QuoteSurfaceConstructorHome(false, 250) > 0); const auto *craft = VerifySurfaceConstructorHome(false);
	const auto position = craft->position; const auto tick = TimerGameTick::counter; const auto cash = Company::Get(_current_company)->money;
	PaySurfaceResupply<Commands::EditSpaceOrder>(craft->tile, craft->index, craft->generation, SpaceOrderEdit::Replace, uint8_t{0},
		SpaceOrderAction::Visit, SpaceOrderTargetKind::LaunchCenter, uint32_t{21}, uint64_t{1});
	VerifySurfaceConstructorHome(true); REQUIRE(craft->position == position); REQUIRE(TimerGameTick::counter == tick);
	REQUIRE(Company::Get(_current_company)->money == cash);
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_HOME_ORDERED vehicle=38 generation=2 home=21 home_generation=1 invoice=0 elapsed=0 cash={} refund_banked=0\n", cash.base());
}

/** Earn a freshly fully funded actual home interval and stop publicly only at genuine arrival. */
static void WorkSurfaceConstructorHome()
{
	const auto *craft = VerifySurfaceConstructorHome(true); REQUIRE(!SurfaceConstructorReachedHome(craft));
	const auto ceiling = SurfaceChemicalWorkLimit(); const auto limit = QuoteSurfaceConstructorHome(true, ceiling);
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_HOME_SELECTION ceiling={} ticks={} found={}\n", ceiling, limit, limit > 0);
	if (limit == 0) return;
	/** @return Whether native navigation reached the exact original home without spending the protected reserve. */
	const auto done = [&] {
		REQUIRE(Company::Get(_current_company)->money >= Money{40000});
		REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(!HasSpaceManifestAssets(*craft)); return SurfaceConstructorReachedHome(craft);
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done);
	const bool arrived = SurfaceConstructorReachedHome(craft);
	if (arrived) {
		REQUIRE(!craft->vehstatus.Test(VehState::Stopped));
		PaySurfaceResupply<Commands::StartStopVehicle>(craft->tile, craft->index, true); REQUIRE(craft->vehstatus.Test(VehState::Stopped));
		const auto sale = QuoteSurfaceResupply<Commands::SellVehicle>(craft->tile, craft->index, false, false, ClientID::Server);
		REQUIRE(sale.Succeeded()); REQUIRE(sale.GetCost() == -craft->value);
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_HOME_SALE available=true invoice={} unbanked_refund={} refund_banked=0 idle_per_day=100 avoided_cost_realized=0\n", sale.GetCost().base(), craft->value.base());
	}
	VerifySurfaceConstructorHome(true);
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_HOME_SERVICE ticks={} arrived={} stopped={} cursor={} entries={} x={} y={} home_x={} home_y={} cash={} reserve=40000 refund_banked=0 idle_per_day=100\n",
		work.ticks, arrived, craft->vehstatus.Test(VehState::Stopped), craft->space_orders.cursor, craft->space_orders.entries.size(), craft->position.x, craft->position.y,
		craft->home.x, craft->home.y, Company::Get(_current_company)->money.base());
}

/** Execute only one selected actual home order or finite fully funded work boundary. */
TEST_CASE("Original idle builder reaches home only through fully funded native navigation", "[.surface-constructor-home]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	REQUIRE((phase == "constructor-home-order" || phase == "constructor-home-work"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-constructor-home"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve each exact native endpoint or partial failure without supplying movement or cash. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/("surface-resupply-before-"+phase+"-440.sav"));
			if (phase == "constructor-home-order") OrderSurfaceConstructorHome(); else WorkSurfaceConstructorHome();
			SaveSurfaceResupply(paths.saves/("surface-resupply-"+phase+"-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/("surface-resupply-failed-"+phase+"-440.sav"));
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_CONSTRUCTOR_HOME_H */
