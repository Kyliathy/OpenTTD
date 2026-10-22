/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_commission.cpp Commissioning through the real command (Plan 4 Group 2.1/2.4, r2mlvs3 Group 8): a legal
 * Surface purchase acquires the consist, reserves the job, binds the pair and publishes the project once, for the
 * machinery and wagon price; every refusal comes before any part is built, with funds, geology, pair and RNG unchanged.
 * Depots service a commissioned cutter but never sell one.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 3B (same-owner area quotes)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 17 (actual script commissioning)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_commission_fixture.h"
#include "../layer_portal_cmd.h"
#include "../tbm_area_preview.h"
#include "../layer_portal_map.h"
#include "../vehicle_func.h"
#include "../vehicle_cmd.h"
#include "../timer/timer_game_calendar.h"
#include "../table/strings.h"
#include "../core/math_func.hpp"
#include "../script/api/script_rail.hpp"
#include "../script/api/script_companymode.hpp"
#include "../script/api/script_testmode.hpp"
#include "../script/api/script_execmode.hpp"
#include "../script/script_suspend.hpp"
#include "../game/game_instance.hpp"

#include "../safeguards.h"


/** The UI and command share the complete canonical material/work and purchase quote, without any allocation. */
TEST_CASE("Commissioning area preview quotes the exact owning command and future paid work", "[underground-area-quote]")
{
	TBMCommissionRig c;
	const auto before = c.rig.Digest();
	const auto quote = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false);
	REQUIRE(quote.purchase.Succeeded());
	CHECK(quote.purchase.GetCost() == c.Commission({}).GetCost());
	CHECK_FALSE(quote.area.partial);
	CHECK_FALSE(quote.area.plan.route.empty());
	CHECK(quote.area.totals.new_cells > 0);
	CHECK(quote.area.totals.earth_t > 0);
	CHECK(quote.area.totals.rock_t > 0);
	CHECK(quote.area.totals.support_units > 0);
	CHECK_FALSE(quote.area.turn_envelope.empty());
	CHECK(quote.request.proposal.requested_tiles == quote.area.proposal.requested_tiles);
	CHECK(quote.request.proposal.route == quote.area.plan.route);
	CHECK(c.rig.Digest() == before);
	const auto wrong_layer = PreviewTBMCommissionArea(_current_company, c.portal, 0, 2, 2, c.rig.fixture.origin->index,
		MultiLayerMap::TileInLayer(c.start, MapLayer::Surface), c.end, false);
	CHECK(wrong_layer.purchase.Failed());
	CHECK(c.rig.Digest() == before);
	const auto foreign = PreviewTBMCommissionArea(CompanyID::Invalid(), c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false);
	CHECK(foreign.purchase.Failed());
	CHECK(c.rig.Digest() == before);
}

/** One purchase publishes the whole project once; the pair then refuses a second one. */
TEST_CASE("A legal Surface commissioning buys, reserves, binds and publishes once", "[tbm-commission]")
{
	TBMCommissionRig c;
	const StationID origin = c.rig.fixture.origin->index;
	const auto before = c.rig.Digest();
	const size_t projects = CaptureTBMProjects().size();

	const auto quote = c.Commission(DoCommandFlags{});
	REQUIRE(quote.Succeeded());
	const Money price = GetTBMCutterModel(0)->purchase_cost + Engine::Get(GetTBMEngine(2))->GetCost() * 2 + Engine::Get(GetTBMEngine(3))->GetCost() * 2;
	CHECK(quote.GetCost() == price);
	CHECK(c.rig.Digest() == before); // A quote builds, reserves and binds nothing.
	CHECK(CaptureTBMProjects().size() == projects);

	const auto bought = c.Commission(DoCommandFlag::Execute);
	REQUIRE(bought.Succeeded());
	CHECK(bought.GetCost() == price);
	REQUIRE(CaptureTBMProjects().size() == projects + 1);
	const TBMProject project = CaptureTBMProjects().back();
	CHECK(project.phase == TBMPhase::Commissioning);
	CHECK(project.blocked == TBMBlockedReason::None);
	CHECK(project.portal == c.portal);
	CHECK(project.origin == origin);
	CHECK(project.exchange_bay == c.portal); // The source side is the exchange until a bay is chosen.
	const Train *front = Train::GetIfValid(project.cutter);
	REQUIRE(front != nullptr);
	CHECK(front->vehstatus.Test(VehState::Stopped));
	CHECK(GetTBMProjectForCutter(front->index) == GetTBMProject(project.id));
	CHECK(InspectTBMConsist(*front, project).error == TBMBlockedReason::None);
	const auto *job = GetExcavationJob(project.job);
	REQUIRE(job != nullptr);
	CHECK(job->commissioning_anchor == c.rock);
	CHECK(job->company == _current_company);
	const auto *record = GetLayerPortalWorkRecord(c.portal);
	REQUIRE(record != nullptr);
	CHECK(record->state == LayerPortalWorkState::Commissioning);
	CHECK(record->project == project.job);
	CHECK(record->cutter == project.cutter);
	CHECK(record->origin == origin);
	CHECK(ValidateTBMProject(project) == TBMProjectError::None);
	CHECK(ValidateLayerPortalWorkRecords(CaptureLayerPortalWorkRecords(), GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
	CHECK(CheckLayerPortalWorkAccess(c.portal, front->index, project.job) == LayerPortalWorkError::None); // Its own cutter is admitted.

	// The pair is taken now: a second purchase refuses with nothing changed.
	const auto after = c.rig.Digest();
	CHECK(c.Commission(DoCommandFlag::Execute).Failed());
	CHECK(c.rig.Digest() == after);

	// Both commissioned projects reload with the exchange store.
	const TBMExchangeSave keep;
	c.rig.Reload();
	REQUIRE(keep.Load());
	CHECK(CaptureTBMProjects().size() == projects + 1);
	CHECK(CaptureTBMProjects().back() == project);
}

/** Each refusal comes before any part, job or binding exists, and names its reason. */
TEST_CASE("Commissioning refuses before building anything", "[tbm-commission]")
{
	TBMCommissionRig c;
	const auto before = c.rig.Digest();
	const size_t projects = CaptureTBMProjects().size();

	SECTION("an origin without receivers") {
		REQUIRE(Station::CanAllocateItem());
		Station *bare = Station::Create(TileXYL(40, 10, MapLayer::Surface));
		bare->owner = _current_company;
		bare->facilities.Set(StationFacility::Train);
		CHECK(c.Commission(DoCommandFlag::Execute, bare->index).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_RECEIVER);
	}
	SECTION("the rock-side end") {
		CHECK(CmdCommissionTBM(DoCommandFlag::Execute, c.rock, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_PORTAL);
	}
	SECTION("a pair that already commissions") {
		CHECK(CmdCommissionTBM(DoCommandFlag::Execute, c.rig.fixture.request.portal, 0, 2, 2, c.rig.fixture.origin->index, c.start, c.end, false).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_PORTAL);
	}
	SECTION("before the cutter is introduced") {
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1800}, 0, 1));
		CHECK(c.Commission(DoCommandFlag::Execute).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_MODEL);
	}
	SECTION("without the money") {
		AutoRestoreBackup cash(Company::Get(_current_company)->money, Money{0});
		CHECK(c.Commission(DoCommandFlag::Execute).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_FUNDS);
	}
	SECTION("without Rock wagons") {
		CHECK(c.Commission(DoCommandFlag::Execute, StationID::Invalid(), 2, 0).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_CONSIST);
	}
	SECTION("only the housing as floor") {
		CHECK(CmdCommissionTBM(DoCommandFlag::Execute, c.portal, 0, 2, 2, c.rig.fixture.origin->index, c.rock, c.rock, false).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_AREA);
	}
	SECTION("floor on the wrong layer") {
		CHECK(CmdCommissionTBM(DoCommandFlag::Execute, c.portal, 0, 2, 2, c.rig.fixture.origin->index, TileXYL(33, 21, MapLayer::Surface), TileXYL(33, 23, MapLayer::Surface), false).GetErrorMessage() == STR_ERROR_TBM_COMMISSION_AREA);
	}
	CHECK(c.rig.Digest() == before);
	CHECK(CaptureTBMProjects().size() == projects);
	CHECK(GetLayerPortalWorkState(c.portal) == LayerPortalWorkState::Sealed);
}

/** Servicing keeps the project and its lock; a depot never sells a cutter (task 2.3). */
TEST_CASE("Depots service a commissioned cutter but never sell one", "[tbm-commission]")
{
	TBMCommissionRig c;
	REQUIRE(c.Commission(DoCommandFlag::Execute).Succeeded());
	const TBMProject project = CaptureTBMProjects().back();
	Train *front = Train::Get(project.cutter);

	// A service visit restores reliability like any train's; the project and its consist lock are untouched.
	front->reliability = 0;
	VehicleServiceInDepot(front);
	CHECK(front->reliability == front->GetEngine()->reliability);
	CHECK(*GetTBMProject(project.id) == project);
	CHECK(CheckTBMTrainRearrangement(*front, nullptr).Failed());

	// Neither the cutter nor its clone can be bought in the depot it may visit.
	const size_t vehicles = Vehicle::GetNumItems();
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, c.rig.depot, GetTBMEngine(project.model_id), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, c.rig.depot, front->index, false)).Failed());
	CHECK(Vehicle::GetNumItems() == vehicles);
}

/** The commissioning rig plus an Underground-to-Deep rail pair whose Underground end stands on excavated floor. */
class TBMDeepCommissionRig : public TBMCommissionRig {
public:
	AutoRestoreBackup<Money> funds{Company::Get(_current_company)->money, Money{50000000}}; ///< Room for the Deep machinery and every hopper count tried.
	TileIndex deep_portal = TileXYL(44, 30, MapLayer::Underground); ///< Underground end, where the Deep cutter assembles.
	TileIndex deep_rock = INVALID_TILE; ///< Deep housing.
	TileIndex deep_start = INVALID_TILE; ///< First Deep corridor cell beyond the housing.
	TileIndex deep_end = INVALID_TILE; ///< Third Deep corridor cell.

	/** Open the Underground assembly ground and build the pair through the real portal command on 1 January 1980. */
	TBMDeepCommissionRig()
	{
		std::vector<TileIndex> floor;
		// The whole 3x3 assembly ground around the Underground end must be open; the Deep side stays solid rock.
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) floor.push_back(TileAddWrap(this->deep_portal, dx, dy));
		}
		this->rig.fixture.world.Open(floor);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->deep_portal, TransportType::Rail, MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		this->deep_rock = GetLayerPortalPairTile(this->deep_portal);
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(this->deep_rock));
		this->deep_start = TileAddWrap(this->deep_rock, step.x, step.y);
		this->deep_end = TileAddWrap(this->deep_rock, step.x * 3, step.y * 3);
		REQUIRE(IsValidTile(this->deep_end));
	}

	/**
	 * Commission at the Deep pair through the command.
	 * @param flags Estimate or execute.
	 * @param consist Hopper counts.
	 * @param model Cutter model; the Deep TBM by default.
	 * @return The command result.
	 */
	CommandCost CommissionDeep(DoCommandFlags flags, TBMConsistRequest consist, uint16_t model = 1) const
	{
		return CmdCommissionTBM(flags, this->deep_portal, model, consist.earth_wagons, consist.rock_wagons, this->rig.fixture.origin->index, this->deep_start, this->deep_end, false);
	}

	/** @return The smallest hopper counts whose consist holds the Deep pocket, or none when no tried count fits. */
	TBMConsistRequest FitDeepConsist() const
	{
		// The pocket's spoil depends on the generated Deep rock, so the consist comes from the command's own quote.
		for (uint8_t rock = 2; rock <= 6; ++rock) {
			for (uint8_t earth = 2; earth <= 4; ++earth) {
				if (this->CommissionDeep(DoCommandFlags{}, {earth, rock}).Succeeded()) return {earth, rock};
			}
		}
		return {};
	}
};

/** Deep commissioning shares the 1980 gate of every Deep path, and assembles on accessible Underground ground (task 2.3). */
TEST_CASE("A Deep cutter commissions at an Underground portal only from 1 January 1980", "[tbm-commission]")
{
	TBMDeepCommissionRig c;
	const auto consist = c.FitDeepConsist();
	REQUIRE(consist.earth_wagons != 0);
	const auto before = c.rig.Digest();
	{
		AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31));
		CHECK(c.CommissionDeep(DoCommandFlag::Execute, consist).GetErrorMessage() == STR_ERROR_EXCAVATION_DEEP_LOCKED);
	}
	CHECK(c.CommissionDeep(DoCommandFlag::Execute, consist, 0).Failed()); // The standard cutter cannot work Deep rock.
	CHECK(c.rig.Digest() == before);

	REQUIRE(c.CommissionDeep(DoCommandFlag::Execute, consist).Succeeded());
	const TBMProject project = CaptureTBMProjects().back();
	CHECK(project.model_id == 1);
	CHECK(project.portal == c.deep_portal);
	CHECK(project.phase == TBMPhase::Commissioning);
	CHECK(GetExcavationJob(project.job)->commissioning_anchor == c.deep_rock);
	CHECK(GetLayerPortalWorkState(c.deep_portal) == LayerPortalWorkState::Commissioning);
	CHECK(GetTileLayer(Train::Get(project.cutter)->tile) == MapLayer::Underground); // Assembled on the accessible upper end.
	CHECK(ValidateTBMProject(project) == TBMProjectError::None);
}

/** Actual script actor scope, retaining the GameInstance's command, storage and suspension owners. */
class TBMCommissionScriptScope : public ScriptObject {
	ActiveInstance active; ///< Native active instance, restored before the runtime is destroyed.
public:
	/** @param game Actual Game Script runtime whose public commissioning binding is exercised. */
	explicit TBMCommissionScriptScope(GameInstance &game) : active(game) {}

	/** @return The actual runtime's most recent synchronized command result. */
	static bool LastCommandSucceeded() { return GetLastCommandRes(); }
};

/** Script estimates and successful purchases use the same real Deep calendar gate and acquisition transaction. */
TEST_CASE("Scripts commission actual cutters through the ordinary synchronized command", "[portal-script-commission]")
{
	TBMDeepCommissionRig c;
	const auto consist = c.FitDeepConsist();
	REQUIRE(consist.earth_wagons != 0);
	GameInstance game;
	TBMCommissionScriptScope active(game);
	ScriptCompanyMode company(static_cast<ScriptCompany::CompanyID>(_current_company.base()));
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Actual script commands run under ordinary unpaused permission.
	const StationID origin = c.rig.fixture.origin->index;
	/** @return The actual script API result for this owned sealed Deep site. */
	const auto commission = [&] {
		return ScriptRail::CommissionTBM(c.deep_portal, 1, consist.earth_wagons, consist.rock_wagons, origin, c.deep_start, c.deep_end, false);
	};
	const auto before = c.rig.Digest();
	{
		ScriptTestMode estimate;
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
		CHECK_FALSE(commission());
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
		CHECK(commission());
		CHECK_FALSE(ScriptRail::CommissionTBM(c.deep_portal, 1, 256, consist.rock_wagons, origin, c.deep_start, c.deep_end, false));
		CHECK_FALSE(ScriptRail::CommissionTBM(c.deep_portal, -1, consist.earth_wagons, consist.rock_wagons, origin, c.deep_start, c.deep_end, false));
	}
	CHECK(c.rig.Digest() == before);
	{
		ScriptExecMode execute;
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
		CHECK_FALSE(commission());
		CHECK(c.rig.Digest() == before);
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
		bool suspended = false;
		try {
			commission();
		} catch (Script_Suspend &resume) {
			// A real successful single-player script command suspends until its callback; this is its normal protocol.
			suspended = true;
			CHECK(resume.GetSuspendCallback() != nullptr);
		}
		REQUIRE(suspended);
		REQUIRE(TBMCommissionScriptScope::LastCommandSucceeded());
	}
	const auto project = CaptureTBMProjects().back();
	CHECK(project.portal == c.deep_portal);
	CHECK(project.model_id == 1);
	CHECK(project.phase == TBMPhase::Commissioning);
	CHECK(GetLayerPortalWorkState(c.deep_portal) == LayerPortalWorkState::Commissioning);
	CHECK(ValidateTBMProject(project) == TBMProjectError::None);
	CHECK(Vehicle::GetNumItems() == before.vehicles + 2 + consist.earth_wagons + consist.rock_wagons);
}

/** One measured first cut. */
struct TBMCutMeasure {
	uint64_t ticks = 0; ///< Controller ticks from the start of boring to the commit.
	Money cut_cost = 0; ///< Money the commit charged.
	uint64_t spoil_t = 0; ///< Earth and Rock aboard after the commit.
	Money running_cost = 0; ///< Recurring active cost those ticks accrued.
};

/**
 * Bore the newest project's first cut and measure it. The cutter cannot drive into its pocket yet (master Group 7B),
 * so the project enters Bore through the staged-load boundary, as the work rigs do.
 * @param rig Rig whose store holds the project last.
 * @param id The project.
 * @return The measurement, which must equal the cut's own quote.
 */
static TBMCutMeasure MeasureFirstCut(const TBMWorkRig &rig, TBMProjectID id)
{
	rig.Reload(TBMPhase::Bore);
	const ExcavationJobID job = GetTBMProject(id)->job;
	Train *front = Train::Get(GetTBMProject(id)->cutter);
	front->vehstatus.Reset(VehState::Stopped);
	const auto estimate = EstimateExcavationCut(job);
	REQUIRE(estimate.error == ExcavationError::None);
	TBMCutMeasure measure;
	const Money cash = Company::Get(_current_company)->money;
	// One controller step per tick until the commit; no cutter is slower than one work tick per tick.
	while (GetExcavationJob(job)->cursor == 0 && measure.ticks <= estimate.work_ticks) {
		REQUIRE(AdvanceTBMWork(id) == TBMProjectError::None);
		REQUIRE(GetTBMProject(id)->blocked == TBMBlockedReason::None);
		++measure.ticks;
	}
	REQUIRE(GetExcavationJob(job)->cursor == 1);
	measure.cut_cost = cash - Company::Get(_current_company)->money;
	measure.spoil_t = TBMExchangeRig::Aboard(front);
	measure.running_cost = ComputeTBMRecurringCharge(*GetTBMProject(id)).cost;
	// The cutter does its model's rate of geological work per tick: one for the standard cutter, more for the Deep one.
	const auto *model = GetTBMCutterModel(GetTBMProject(id)->model_id);
	REQUIRE(model != nullptr);
	CHECK(measure.ticks == CeilDiv(estimate.work_ticks * model->work_rate_denominator, model->work_rate_numerator));
	CHECK(measure.cut_cost == estimate.cost);
	CHECK(measure.spoil_t == estimate.earth_t + estimate.rock_t);
	return measure;
}

/** Time and full cost of the first cut of a standard and a Deep corridor, for r2mlvs15 (Plan 4 Group 20.1). */
TEST_CASE("The first cut of a standard and a Deep corridor is measured", "[tbm-measure]")
{
	TBMDeepCommissionRig c;
	const auto deep = c.FitDeepConsist();
	REQUIRE(deep.earth_wagons != 0);
	const auto standard_price = c.Commission(DoCommandFlag::Execute);
	REQUIRE(standard_price.Succeeded());
	const TBMProjectID standard = CaptureTBMProjects().back().id;
	const auto standard_cut = MeasureFirstCut(c.rig, standard);
	const auto deep_price = c.CommissionDeep(DoCommandFlag::Execute, deep);
	REQUIRE(deep_price.Succeeded());
	const TBMProjectID deep_id = CaptureTBMProjects().back().id;
	const auto deep_cut = MeasureFirstCut(c.rig, deep_id);
	/**
	 * Report one corridor's measurement.
	 * @param name Corridor.
	 * @param consist Its hoppers.
	 * @param price Machinery and hopper price.
	 * @param cut First-cut measurement.
	 */
	const auto report = [](const char *name, TBMConsistRequest consist, Money price, const TBMCutMeasure &cut) {
		WARN("MLVS_TBM_MEASURE corridor=" << name << " earth_hoppers=" << int{consist.earth_wagons} << " rock_hoppers=" << int{consist.rock_wagons} << " purchase=" << static_cast<int64_t>(price) << " first_cut_ticks=" << cut.ticks << " first_cut_cost=" << static_cast<int64_t>(cut.cut_cost) << " first_cut_spoil_t=" << cut.spoil_t << " running_cost=" << static_cast<int64_t>(cut.running_cost));
	};
	report("standard", {2, 2}, standard_price.GetCost(), standard_cut);
	report("deep", deep, deep_price.GetCost(), deep_cut);
	CHECK(standard_cut.ticks != 0);
	CHECK(deep_cut.ticks != 0);
}
