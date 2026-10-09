/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_advice.cpp Native order review and current controller status. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group6; docs/zz-reach2/architecture/vehicles/archi-vehicles.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm_controls.h"
#include "../tbm_gui.h"
#include "../tbm_cmd.h"
#include "../order_cmd.h"
#include "../news_func.h"
#include "../news_gui.h"

#include "../safeguards.h"

/** Actual commissioned cutter, native hauler, origin order and receivers for read-only review tests. */
class TBMAdviceWorld : public UndergroundWorkflowWorld {
	AutoRestoreBackup<OrderReviewSystem> review{_settings_client.gui.order_review_system, OrderReviewSystem::All}; ///< Explicit enabled review, including stopped test participants.
	AutoRestoreBackup<CompanyID> local{_local_company, _current_company}; ///< Advice is emitted only for the real local owner.
public:
	TBMProjectID project; ///< Actual command-created controller.
	Train *cutter = nullptr; ///< Native commissioned machinery.
	/** Commission through the real shared quote/command and give the hauler its actual origin order. */
	TBMAdviceWorld()
	{
		const TileIndex end = GetLayerPortalDoorTile(this->rock);
		const auto quote = PreviewTBMCommissionArea(_current_company, this->portal, 0, 2, 2, this->origin->index,
			this->rock, end, false, ExcavationPlanIntent::StraightTunnel);
		REQUIRE(quote.purchase.Succeeded());
		REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, this->portal, uint16_t{0}, uint8_t{2}, uint8_t{2}, this->origin->index,
			this->rock, end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
		this->project = CaptureTBMProjects().front().id;
		this->cutter = Train::Get(GetTBMProject(this->project)->cutter);
		Order order;
		order.MakeGoToStation(this->origin->index);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->hauler->index, 0, order).Succeeded());
		REQUIRE(AssignTBMHauler(this->project, this->hauler->index, true) == TBMExchangeError::None);
		InitNewsItemStructs();
	}
	/** Remove test news before its actual vehicle references leave the world. */
	~TBMAdviceWorld() { InitNewsItemStructs(); }
};

/** @param vehicle Actual train under native review. @return Advice produced at the next normal review boundary, with previous advice for that train removed. */
static std::vector<std::string> ReviewAdvice(Train &vehicle)
{
	DeleteVehicleNews(vehicle.index, AdviceType::Order);
	vehicle.day_counter = 20;
	CheckOrders(&vehicle);
	std::vector<std::string> texts;
	for (const auto &news : GetNews()) {
		const auto *id = std::get_if<VehicleID>(&news.ref1);
		if (id != nullptr && *id == vehicle.index) texts.push_back(news.GetStatusText()); // Other trains' messages must not satisfy this vehicle's review.
	}
	return texts;
}

/** Active assignments replace the station-count rule; releasing immediately restores ordinary order review. */
TEST_CASE("Managed work avoids too-few-orders advice and release restores review", "[tbm-advice]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMAdviceWorld world;
	CHECK(GetTBMOrderProblem(*world.hauler) == INVALID_STRING_ID);
	CHECK(GetTBMOrderProblem(*world.cutter) == INVALID_STRING_ID);
	CHECK(ReviewAdvice(*world.hauler).empty());
	CHECK(ReviewAdvice(*world.cutter).empty());
	const auto status = GetTBMVehicleStatus(*world.cutter);
	REQUIRE(status.has_value());
	CHECK(status->find("No orders") == std::string::npos);
	CHECK(GetTBMVehicleStatus(*world.hauler).has_value());
	REQUIRE(ControlTBMProject(world.project, TBMProjectAction::Pause, true) == TBMProjectError::None);
	CHECK(GetTBMVehicleStatus(*world.cutter)->find("paused") != std::string::npos);
	CHECK(*GetTBMVehicleStatus(*world.cutter) == GetString(STR_TBM_VEHICLE_WAIT_VEL, PackVelocity(0, VehicleType::Train), STR_PORTAL_HOLD_PAUSED));
	world.cutter->current_order.MakeGoToDepot(DepotID::Invalid(), OrderDepotTypeFlags{});
	CHECK_FALSE(GetTBMVehicleStatus(*world.cutter).has_value()); // Native service status retains priority.
	world.cutter->current_order.Free();
	REQUIRE(ReleaseTBMHauler(world.hauler->index, true) == TBMExchangeError::None);
	CHECK_FALSE(GetTBMOrderProblem(*world.hauler).has_value());
	const auto ordinary = ReviewAdvice(*world.hauler);
	REQUIRE(ordinary.size() == 1);
	CHECK(ordinary.front().find("too few orders") != std::string::npos);
	REQUIRE(AssignTBMHauler(world.project, world.hauler->index, true) == TBMExchangeError::None);
	CHECK(ReviewAdvice(*world.hauler).empty());
}

/** Actual missing destinations, ownership or wagons remain actionable; manual invalid/duplicate orders retain their native diagnosis. */
TEST_CASE("Managed review reports genuine route faults", "[tbm-advice]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMAdviceWorld world;
	std::string expected;
	SECTION("origin order removed") {
		REQUIRE(CmdDeleteOrder(DoCommandFlag::Execute, world.hauler->index, 0).Succeeded());
		expected = "Add an order";
	}
	SECTION("origin ownership lost") {
		world.origin->owner = OWNER_NONE;
		expected = "origin";
	}
	SECTION("receiver acceptance lost") {
		world.origin->goods[GetCargoTypeByLabel(CargoLabel{"ERTH"})].status.Reset(GoodsEntry::State::Acceptance);
		expected = "acceptance";
	}
	SECTION("bay ownership lost") {
		SetTileOwner(world.portal, OWNER_NONE);
		expected = "bay";
	}
	SECTION("wagon owner changed") {
		world.hauler->Last()->owner = CompanyID::Invalid();
		expected = "different owners";
	}
	SECTION("ordinary dummy order") {
		world.hauler->GetOrder(0)->MakeDummy(); // Same saved order state produced when its destination is removed.
		expected = "invalid";
	}
	SECTION("ordinary duplicate order") {
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, world.hauler->index, 1, *world.hauler->GetOrder(0)).Succeeded());
		expected = "duplicate";
	}
	const auto advice = ReviewAdvice(*world.hauler);
	REQUIRE(advice.size() == 1);
	CHECK(advice.front().find(expected) != std::string::npos);
	CHECK(advice.front().find("too few orders") == std::string::npos);
	world.hauler->Last()->owner = world.hauler->owner; // Restore the declared ownership fault before native fixture teardown.
}

/** A controller on one shared-order peer must not silence an ordinary peer's incomplete schedule. */
TEST_CASE("Shared orders retain ordinary peer review beside a managed hauler", "[tbm-advice]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMAdviceWorld world;
	Train *ordinary = Train::From(world.BuyPreexisting(world.hauler->tile, 4));
	REQUIRE(CmdCloneOrder(DoCommandFlag::Execute, CO_SHARE, ordinary->index, world.hauler->index).Succeeded());
	CHECK(ReviewAdvice(*world.hauler).empty());
	const auto advice = ReviewAdvice(*ordinary);
	REQUIRE(advice.size() == 1);
	CHECK(advice.front().find("too few orders") != std::string::npos);
	REQUIRE(ReleaseTBMHauler(world.hauler->index, true) == TBMExchangeError::None);
	CHECK_FALSE(GetTBMOrderProblem(*world.hauler).has_value());
	Train *first = Train::From(world.hauler->FirstShared());
	Train *next = Train::From(first->NextShared());
	CHECK(ReviewAdvice(*first).size() == 1);
	CHECK(ReviewAdvice(*next).empty());
}
