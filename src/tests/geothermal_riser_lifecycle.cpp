/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_lifecycle.cpp Actual native sealing, takeover, abandonment and original-housing demolition/rebuild boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_riser_fixture.h"
#include "utility_power_fixture.h"
#include "../economy_func.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Native sealing suspends incomplete/completed machinery, while reopening that same actual pair retains paid stock/work without another capital or energy event. */
TEST_CASE("Native riser sealing and reopening preserve the same paid machinery", "[geothermal-riser-lifecycle]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Riser();
	bool complete = false;
	SECTION("unfinished stock") { REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 4) == 4); }
	SECTION("completed machinery") { rig.DeliverBill(); rig.Tick(296); complete = true; }
	const auto before = *GetGeothermalRiserState(industry.index);
	const auto cash = Company::Get(_current_company)->money;
	const auto power = ComputeUtilityPowerDigest();
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, rig.request.portal, false).Succeeded());
	CHECK_FALSE(IsGeothermalRiserOperational(industry.index));
	CHECK_FALSE(GetGeothermalRiserState(industry.index)->portal_retired);
	CHECK(GetGeothermalRiserState(industry.index)->portal_revision == GetLayerPortalWorkRecord(rig.request.portal)->revision);
	rig.Tick(20); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == before.work_ticks);
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 2) == 0);
	const auto sealed = *GetGeothermalRiserState(industry.index);
	CHECK(Command<Commands::ChangeLayerPortalConnection>::Do({}, rig.request.portal, true).Succeeded());
	CHECK(*GetGeothermalRiserState(industry.index) == sealed);
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, rig.request.portal, true).Succeeded());
	CHECK(GetGeothermalRiserState(industry.index)->last_work_tick == before.last_work_tick);
	if (!complete) {
		CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 4);
		REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4);
		REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2);
		REQUIRE(rig.Deliver(CargoLabel{"BATY"}, 2) == 2);
		rig.Tick(296);
	}
	CHECK(IsGeothermalRiserOperational(industry.index));
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeUtilityPowerDigest() == power);
}

/** The actual whole-company merger restamps only the same physical pair, retaining paid partial work and construction receipts while moving native infrastructure once. */
TEST_CASE("Native riser takeover retains partial work and both housing owners", "[geothermal-riser-lifecycle]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
	const auto id = rig.Riser().index;
	const auto before = *GetGeothermalRiserState(id);
	REQUIRE(Company::CanAllocateItem()); std::unique_ptr<Company> successor{Company::Create()};
	successor->money = 1000000; successor->clear_limit = 64U << 16;
	const auto infrastructure = Company::Get(rig.request.owner)->infrastructure.rail[RAILTYPE_RAIL];
	ChangeOwnershipOfCompanyItems(rig.request.owner, successor->index);
	const auto *state = GetGeothermalRiserState(id); REQUIRE(state != nullptr);
	CHECK(state->site.owner == successor->index); CHECK_FALSE(state->portal_retired);
	CHECK(state->portal_revision == GetLayerPortalWorkRecord(rig.request.portal)->revision);
	CHECK(state->portal_revision == before.portal_revision + 1);
	CHECK(state->work_ticks == before.work_ticks); CHECK(state->last_work_tick == before.last_work_tick); CHECK(state->materials_committed);
	CHECK(GetTileOwner(rig.request.portal) == successor->index); CHECK(GetTileOwner(rig.destination) == successor->index);
	CHECK(Company::Get(rig.request.owner)->infrastructure.rail[RAILTYPE_RAIL] == 0);
	CHECK(successor->infrastructure.rail[RAILTYPE_RAIL] == infrastructure);
	// All-company native ticks keep running while the UI actor still names the
	// original company; presentation ownership cannot suspend the taken-over job.
	rig.Tick(148); CHECK(IsGeothermalRiserOperational(id));
	CHECK(GetGeothermalRiserState(id)->work_ticks == 296);
	CHECK(Industry::Get(id)->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 0);
}

/** Bankruptcy preserves arrived native supplies and paid labour while permanently retiring machinery control, for removed rail and retained ownerless-road housings alike. */
TEST_CASE("Native riser abandonment retains material custody and cannot regain readiness", "[geothermal-riser-lifecycle]")
{
	bool road = false;
	SECTION("rail housings removed") {}
	SECTION("road housing retained ownerless") { road = true; }
	NativeRiserRig rig(road); REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Riser();
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 4) == 4);
	const auto before = *GetGeothermalRiserState(industry.index);
	const auto power = ComputeUtilityPowerDigest();
	ChangeOwnershipOfCompanyItems(rig.request.owner, INVALID_OWNER);
	const auto *state = GetGeothermalRiserState(industry.index); REQUIRE(state != nullptr);
	CHECK(state->site.owner == CompanyID::Invalid()); CHECK(state->portal_retired);
	CHECK(state->work_ticks == before.work_ticks); CHECK(state->last_work_tick == before.last_work_tick);
	rig.Tick(50); CHECK_FALSE(IsGeothermalRiserOperational(industry.index));
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 2) == 0);
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 4);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(ComputeUtilityPowerDigest() == power);
	if (road) {
		CHECK(IsLayerPortalTile(rig.request.portal)); CHECK(GetTileOwner(rig.request.portal) == OWNER_NONE);
		CHECK(GetLayerPortalWorkState(rig.request.portal) == LayerPortalWorkState::Sealed);
	} else {
		CHECK_FALSE(IsLayerPortalTile(rig.request.portal)); CHECK_FALSE(IsLayerPortalTile(rig.destination));
	}
}

/** Demolishing/rebuilding the actual portal may reuse coordinates and numeric stamps, but cannot revive old machinery or refund previously consumed shared energy. */
TEST_CASE("Native riser original portal loss survives coordinate and revision reuse", "[geothermal-riser-lifecycle]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto id = rig.Riser().index;
	const auto original = *GetGeothermalRiserState(id);
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External); REQUIRE(source.has_value());
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 20, std::array{*source}); REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*source, 1, 1000, 1) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*consumer, 20}}) == UtilityPowerError::None);
	}
	UtilityCompanyWork expense(_current_company);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {*consumer, 1}, 10), &expense).error == UtilityPowerError::None);
	const auto power = ComputeUtilityPowerDigest();
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.destination).Succeeded());
	REQUIRE(GetGeothermalRiserState(id)->portal_retired); CHECK_FALSE(IsGeothermalRiserOperational(id));
	CHECK(GetGeothermalRiserState(id)->portal_revision == original.portal_revision);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, rig.destination, TransportType::Rail, MapLayer::DeepUnderground,
		DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, true, true) == LayerPortalWorkError::None);
	CHECK(GetLayerPortalWorkRecord(rig.request.portal)->revision == original.portal_revision);
	CHECK_FALSE(IsGeothermalRiserOperational(id)); CHECK(GetGeothermalRiserState(id)->portal_retired);
	CHECK(GetGeothermalRiserState(id)->work_ticks == 296); CHECK(GetGeothermalRiserState(id)->materials_committed);
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(GetUtilityPowerProducer(*source)->consumed_lifetime_j == 10);
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK(GetGeothermalRiserState(id) == nullptr); CHECK_FALSE(IsGeothermalRiserOperational(id));
	CHECK(FindGeothermalRiserForPortal(rig.request.portal) == IndustryID::Invalid());
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(rig.Fund().Succeeded()); CHECK(Company::Get(_current_company)->money == cash - Money{120000});
	const auto *rebuilt = GetGeothermalRiserState(rig.Riser().index); REQUIRE(rebuilt != nullptr);
	CHECK_FALSE(rebuilt->portal_retired); CHECK_FALSE(rebuilt->materials_committed); CHECK(rebuilt->work_ticks == 0);
	CHECK_FALSE(IsGeothermalRiserOperational(rig.Riser().index)); CHECK(ComputeUtilityPowerDigest() == power);
}
