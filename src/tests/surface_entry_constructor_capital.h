/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_constructor_capital.h Pure exhausted-constructor lifecycle and cost admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZJ.
 * The actual hull, public commands and company books retain all ownership.
 * A legal Visit estimate grants no movement, sale proceeds or avoided upkeep.
 */
#ifndef TEST_SURFACE_ENTRY_CONSTRUCTOR_CAPITAL_H
#define TEST_SURFACE_ENTRY_CONSTRUCTOR_CAPITAL_H
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"

/** Quote the original completed builder's actual retirement and return prerequisites without mutation. */
static void QuerySurfaceConstructorCapital()
{
	VerifySurfaceGoodsRailDelivery();
	const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{49250}); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *craft = SpaceVehicle::Get(VehicleID{38});
	REQUIRE(craft->owner == _current_company); REQUIRE(craft->generation == 2); REQUIRE(GetSpaceVehicleModel(*craft) == 20);
	REQUIRE(craft->construction.placement == SpacePlacement::Orbital); REQUIRE(craft->construction.capital_paid == 100000);
	REQUIRE(!craft->vehstatus.Test(VehState::Stopped)); REQUIRE(craft->cur_speed == 0); REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(craft->cargo_payment == nullptr);
	REQUIRE(!HasSpaceManifestAssets(*craft)); REQUIRE(craft->space_orders.cursor == craft->space_orders.entries.size());
	REQUIRE(craft->position != craft->home);
	const auto idle = GetMultiLayerSpaceBudget().idle_per_day;
	REQUIRE(craft->GetRunningCost() == Money{idle} * CalendarTime::DAYS_IN_YEAR * 256);
	const auto sale = QuoteSurfaceResupply<Commands::SellVehicle>(craft->tile, craft->index, false, false, ClientID::Server);
	REQUIRE(sale.Failed());
	const auto home = craft->construction.home; REQUIRE(home.station == StationID{21}); REQUIRE(home.generation == 1);
	SpacePoint destination;
	const auto navigation = ResolveSpaceNavigationTarget(*craft, {SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation}, destination);
	REQUIRE(navigation == SpaceOrderBlock::None); REQUIRE(destination == craft->home);
	const auto visit = QuoteSurfaceResupply<Commands::EditSpaceOrder>(craft->tile, craft->index, craft->generation, SpaceOrderEdit::Replace, uint8_t{0},
		SpaceOrderAction::Visit, SpaceOrderTargetKind::LaunchCenter, uint32_t{home.station.base()}, home.generation);
	REQUIRE(visit.Succeeded()); REQUIRE(visit.GetCost() == Money{0});
	// Replacing this still-needed builder would consume new capital and real
	// components. Its depreciated resale value is neither a current balance
	// nor a net capital gain; the paid hull and its embodied inputs remain here.
	const auto replacement = QuoteSurfaceResupply<Commands::AssembleLaunchCraft>(TileIndex{2891}, home.station, home.generation, uint16_t{20});
	REQUIRE(replacement.Succeeded()); REQUIRE(replacement.GetCost() == Money{100000});
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_CAPITAL_SOURCE vehicle=38 generation=2 model=20 placement=0 stopped=false motionless=true held=0 manifest_assets=false exhausted=true capital_paid={} value={} idle_per_day={} position_x={} position_y={} home_x={} home_y={} sale_available=false sale_error={} refund_banked=0 visit_available=true visit_invoice=0 destination_is_home=true home_center=21 home_generation=1 replacement_invoice={} replacement_steel=4 replacement_goods=8 replacement_work=148 replacement_launch_priced=false ticks=0 work_permission=false\n",
		craft->construction.capital_paid, craft->value.base(), idle, craft->position.x, craft->position.y, craft->home.x, craft->home.y,
		sale.GetErrorMessage().base(), replacement.GetCost().base());
	// Every candidate prices the actual existing fleet, including this idle
	// constructor and pending ground charges. Neither hypothetical retirement
	// savings nor future fares may fund a return or another income interval.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto target = Money{40000} + operating;
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_CAPITAL_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded={} unbanked_value={} refund_counted=0 avoided_cost_counted=0 work_permission=false\n",
			limit, operating.base(), target.base(), company->money.base(), company->money >= target, craft->value.base());
	}
}
#endif /* TEST_SURFACE_ENTRY_CONSTRUCTOR_CAPITAL_H */
