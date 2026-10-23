/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_lifecycle.cpp Native company loss, takeover, demolition and original machinery identity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_generator_fixture.h"
#include "../economy_func.h"
#include "../landscape_cmd.h"

/** Actual company takeover transfers control without replaying its capital invoice, bill consumption or already paid labour. */
TEST_CASE("Native powerhouse takeover retains original paid construction", "[geothermal-generator-takeover]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto before = *GetGeothermalGeneratorState(rig.Generator().index);
	REQUIRE(Company::CanAllocateItem()); Company *successor = Company::Create(); successor->money = 1000000;
	const auto cash = Company::Get(rig.request.owner)->money;
	ChangeOwnershipOfCompanyItems(rig.request.owner, successor->index);
	const auto *state = GetGeothermalGeneratorState(before.industry); REQUIRE(state != nullptr);
	CHECK(state->owner == successor->index); CHECK(state->site == before.site);
	CHECK(state->work_ticks == 296); CHECK(state->last_work_tick == before.last_work_tick); CHECK(state->materials_committed);
	CHECK(Company::Get(rig.request.owner)->money == cash); CHECK(successor->money == Money{1000000});
	// Native industry dispatch belongs to all companies, including when the
	// viewing/current actor still names the company that relinquished control.
	rig.Tick(296); CHECK(IsGeothermalGeneratorConstructionComplete(before.industry));
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators())); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Bankruptcy retains arrived stock or consumed-work history and cannot resume ownerless machinery or discard its accountable supplies. */
TEST_CASE("Native powerhouse abandonment preserves material and work custody", "[geothermal-generator-abandonment]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded());
	bool consumed = false;
	SECTION("arrived unfinished stock") { REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 12) == 12); }
	SECTION("already consumed whole bill") { rig.DeliverBill(); rig.Tick(296); consumed = true; }
	const auto before = *GetGeothermalGeneratorState(rig.Generator().index);
	// The actual bankruptcy caller moves the local viewer before disposing of
	// that company. Reproduce that precondition rather than weakening its guard.
	AutoRestoreBackup spectator{_local_company, COMPANY_SPECTATOR};
	ChangeOwnershipOfCompanyItems(rig.request.owner, INVALID_OWNER);
	// Native bankruptcy supplies temporary disposal funds. Subsequent work and
	// receipt refusal must leave that native result intact, without a refund.
	const auto cash = Company::Get(rig.request.owner)->money;
	REQUIRE(GetGeothermalGeneratorState(before.industry) != nullptr);
	CHECK(GetGeothermalGeneratorState(before.industry)->owner == CompanyID::Invalid());
	rig.Tick(50); CHECK(GetGeothermalGeneratorState(before.industry)->work_ticks == before.work_ticks);
	CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(before.industry)); CHECK(rig.Deliver(CargoLabel{"BATY"}, 2) == 0);
	CHECK(rig.Generator().GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == (consumed ? 0 : 12));
	CHECK(Company::Get(rig.request.owner)->money == cash); CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
	if (!consumed) CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Actual demolition retires its original construction identity; a fresh funded building at the same numeric ID starts with an empty bill and zero work. */
TEST_CASE("Native powerhouse demolition and ID reuse cannot inherit paid work", "[geothermal-generator-reuse]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto before = *GetGeothermalGeneratorState(rig.Generator().index);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	Company::Get(_current_company)->clear_limit = 64U << 16;
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK(GetGeothermalGeneratorState(before.industry) == nullptr); CHECK_FALSE(Industry::IsValidID(before.industry));
	CHECK_FALSE(GetGeothermalSiteClaim(before.site)->active); CHECK(rig.Fund().Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	const auto fresh = ClaimGeothermalSite(rig.request); REQUIRE(fresh.error == GeothermalError::None); CHECK(fresh.id != before.site);
	rig.site = fresh.id; const auto cash = Company::Get(_current_company)->money;
	REQUIRE(rig.Fund().Succeeded()); CHECK(rig.Generator().index == before.industry);
	const auto *state = GetGeothermalGeneratorState(before.industry); REQUIRE(state != nullptr);
	CHECK(state->site == fresh.id); CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed); CHECK_FALSE(state->has_work_tick);
	CHECK(Company::Get(_current_company)->money == cash - Money{180000}); rig.Tick(592);
	CHECK(GetGeothermalGeneratorState(before.industry)->work_ticks == 0); CHECK_FALSE(IsGeothermalGeneratorConstructionComplete(before.industry));
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators())); CHECK(CaptureUtilityPowerState().producers.empty());
}
