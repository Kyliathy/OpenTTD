/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_construction.cpp Actual funding, material shortage and once-per-tick paid riser construction boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_riser_fixture.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Complete command estimates preserve cash/RNG/portal/energy owners, while actual native funding settles capital once without building free capacity. */
TEST_CASE("Native riser funding charges once and preserves both actual portal endpoints", "[geothermal-riser-construction]")
{
	NativeRiserRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto utility = ComputeUtilityPowerDigest(), portal = ComputeLayerPortalWorkDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	const auto quote = rig.Fund(true);
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == Money{120000});
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	REQUIRE(rig.Fund().Succeeded());
	CHECK(Company::Get(_current_company)->money == cash - Money{120000});
	auto &industry = rig.Riser();
	REQUIRE(industry.accepted.size() == 4); CHECK(industry.produced.empty());
	CHECK(industry.location.w == 2); CHECK(industry.location.h == 2);
	CHECK(industry.founder == rig.request.owner); CHECK(industry.owner == OWNER_NONE);
	const auto *state = GetGeothermalRiserState(industry.index); REQUIRE(state != nullptr);
	CHECK(state->site == rig.request); CHECK(state->destination == rig.destination);
	CHECK(state->portal_revision == GetLayerPortalWorkRecord(rig.request.portal)->revision);
	CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed); CHECK_FALSE(IsGeothermalRiserOperational(industry.index));
	CHECK(rig.Fund().Failed()); CHECK(Company::Get(_current_company)->money == cash - Money{120000});
	CHECK(GetLayerPortalPairTile(rig.request.portal) == rig.destination);
	CHECK(ComputeLayerPortalWorkDigest() == portal); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeUtilityPowerDigest() == utility);
}

/** Cash/date/ownership/whole-site refusal precedes every native allocation, portal write or simulation RNG consumption. */
TEST_CASE("Native riser funding refuses incomplete capital dates and physical participants atomically", "[geothermal-riser-construction]")
{
	NativeRiserRig rig;
	SECTION("short cash") { Company::Get(_current_company)->money = 119999; }
	SECTION("locked date") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("foreign actor") { _current_company = OWNER_DEITY; }
	SECTION("foreign destination") { SetTileOwner(rig.destination, CompanyID{1}); }
	SECTION("occupied corner") { MakeRailNormal(TileAddWrap(rig.request.tile, 1, 1), rig.request.owner, TrackBits{Track::X}, RAILTYPE_RAIL); }
	SECTION("nonadjacent attachment") { rig.request.port = TileAddWrap(rig.request.tile, 1, 1); }
	const auto cash = Company::Get(rig.request.owner)->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), utility = ComputeUtilityPowerDigest(), portal = ComputeLayerPortalWorkDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	CHECK(CmdBuildGeothermalRiser(DoCommandFlag::Execute, rig.request.tile, rig.request.portal, rig.request.port, rig.request.access, 123).Failed());
	CHECK(Industry::GetNumItems() == 0); CHECK(FindGeothermalRiserForPortal(rig.request.portal) == IndustryID::Invalid());
	CHECK(Company::Get(rig.request.owner)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeLayerPortalWorkDigest() == portal);
}

/** Missing Batteries retains every arrived input; full native stock is consumed once before exactly296 synchronized labour ticks, independently of power or repeated calls. */
TEST_CASE("Native riser consumes four real construction inputs and296 once-only ticks", "[geothermal-riser-construction]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Riser();
	const auto cash = Company::Get(_current_company)->money;
	const auto utility = ComputeUtilityPowerDigest(), portal = ComputeLayerPortalWorkDigest();
	CHECK(rig.Deliver(CargoLabel{"STEL"}, 100) == 4); CHECK(rig.Deliver(CargoLabel{"RFMT"}, 100) == 4); CHECK(rig.Deliver(CargoLabel{"ELEC"}, 100) == 2);
	rig.Tick(300); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 0);
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 4);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(rig.Deliver(CargoLabel{"BATY"}, 100) == 2); CHECK(rig.Deliver(CargoLabel{"STEL"}, 1) == 0);
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Editor); rig.Tick(); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 0);
	}
	++TimerGameTick::counter; AdvanceGeothermalRiser(industry); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 0);
	rig.Tick(); REQUIRE(GetGeothermalRiserState(industry.index)->work_ticks == 1);
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); AdvanceGeothermalRiser(industry); AdvanceGeothermalRiser(industry);
		CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 1);
	}
	// Native accepted history remains evidence of each actual receipt after
	// the whole bill is consumed; no private construction stock shadows it.
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
		const auto slot = industry.GetCargoAccepted(GetCargoTypeByLabel(input.label)); REQUIRE(slot != industry.accepted.end());
		CHECK(slot->waiting == 0); REQUIRE(slot->history != nullptr); CHECK((*slot->history)[THIS_MONTH].accepted == input.quantity);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	rig.Tick(10); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 1);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	rig.Tick(294); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 295); CHECK_FALSE(IsGeothermalRiserOperational(industry.index));
	rig.Tick(); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 296); CHECK(IsGeothermalRiserOperational(industry.index));
	// The complete native floor finishes together, while the existing source
	// and receiving portal housings retain their independent traffic geometry.
	for (const auto tile : industry.location) CHECK(IsIndustryCompleted(tile));
	rig.Tick(50); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 296); CHECK(rig.Deliver(CargoLabel{"BATY"}, 1) == 0);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeLayerPortalWorkDigest() == portal);
}

/** Direct elevators fund machinery only on their real sending floor; no intermediate Underground cavity or numerical transfer is created. */
TEST_CASE("Direct elevator riser construction leaves intermediate rock and electrical accounting unchanged", "[geothermal-riser-construction]")
{
	NativeRiserRig rig(true);
	const auto middle = MultiLayerMap::TileInLayer(rig.request.portal, MapLayer::Underground);
	const auto utility = ComputeUtilityPowerDigest();
	REQUIRE(GetGeologyWorldState().terrain.Get(middle) == ExcavationState::Solid);
	REQUIRE(rig.Fund().Succeeded());
	CHECK(GetGeologyWorldState().terrain.Get(middle) == ExcavationState::Solid);
	CHECK(GetGeothermalRiserState(rig.Riser().index)->destination == rig.destination);
	CHECK(ComputeUtilityPowerDigest() == utility);
}
