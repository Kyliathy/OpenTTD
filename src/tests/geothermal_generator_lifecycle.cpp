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
#include "../geothermal_site_cmd.h"
#include "../industry_map.h"

/** Clear the actual native machinery through ordinary command finance, without inventing a direct refund path.
 * @param tile Original whole-building anchor.
 * @return Complete native expense or unchanged refusal before any material, tile or history is discarded.
 */
static CommandCost ClearNativeGenerator(TileIndex tile)
{
	return Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile));
}

/** Cancel the exact owned active planning reservation through synchronized native dispatch.
 * @param site Original retained cold identity.
 * @param estimate Inspect cancellation without retiring the claim.
 * @return Free complete cancellation or unchanged refusal; no capital or material is refundable here.
 */
static CommandCost CancelNativeGeneratorSite(GeothermalSiteID site, bool estimate = false)
{
	return Command<Commands::CancelGeothermalSite>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, INVALID_TILE, std::make_tuple(site));
}

// Cancellation lands only with its complete body at the independently allocated revision2 identity.
static_assert(to_underlying(Commands::CancelGeothermalSite) == 215);
static_assert(std::is_same_v<CommandTraits<Commands::CancelGeothermalSite>::Args, std::tuple<GeothermalSiteID>>);

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
	if (!consumed) CHECK(ClearNativeGenerator(rig.request.tile).Failed());
	CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Actual demolition retires its original construction identity; a fresh funded building at the same numeric ID starts with an empty bill and zero work. */
TEST_CASE("Native powerhouse demolition and ID reuse cannot inherit paid work", "[geothermal-generator-reuse]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto before = *GetGeothermalGeneratorState(rig.Generator().index);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	Company::Get(_current_company)->clear_limit = 64U << 16;
	REQUIRE(ClearNativeGenerator(rig.request.tile).Succeeded());
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

/** A detached candidate must retain its real native floor/materials and paid work; validation cannot fabricate or repair a machine. */
TEST_CASE("Native powerhouse staging rejects forged work and omitted original machinery", "[geothermal-generator-stage]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
	const auto saved = CaptureGeothermalGenerators(); const auto digest = ComputeGeothermalGeneratorDigest();
	REQUIRE(ValidateGeothermalGenerators(saved));
	StageLoadedGeothermalGenerators(saved); REQUIRE(AcceptLoadedGeothermalGenerators()); CHECK(ComputeGeothermalGeneratorDigest() == digest);
	for (unsigned fault = 0; fault < 7; ++fault) {
		auto rows = saved;
		// Each fault attacks a distinct native reference or paid receipt. None
		// may publish a partial family, rewrite stock or forgive its bad field.
		switch (fault) {
			case 0: rows.clear(); break;
			case 1: rows.push_back(rows.front()); break;
			case 2: rows.front().work_ticks = 593; break;
			case 3: rows.front().materials_committed = false; break;
			case 4: rows.front().site = GeothermalSiteID{UINT32_MAX}; break;
			case 5: rows.front().last_work_tick = TimerGameTick::counter + 1; break;
			case 6: rows.front().model = GeothermalGeneratorModel::MagmaLoop; break;
		}
		CHECK_FALSE(ValidateGeothermalGenerators(rows)); StageLoadedGeothermalGenerators(rows); CHECK_FALSE(AcceptLoadedGeothermalGenerators());
		CHECK(ComputeGeothermalGeneratorDigest() == digest);
	}
}

/** Cold cancellation retires the original planning identity without acquiring a refund or undoing funded work. */
TEST_CASE("Native cold-site cancellation preserves retired and funded history", "[geothermal-generator-cancel]")
{
	GeothermalGeneratorRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	SECTION("active original plan") {
		const auto before = ComputeGeothermalSiteDigest(); const auto cash = Company::Get(_current_company)->money;
		REQUIRE(CancelNativeGeneratorSite(rig.site, true).Succeeded()); CHECK(ComputeGeothermalSiteDigest() == before);
		REQUIRE(CancelNativeGeneratorSite(rig.site).Succeeded()); CHECK_FALSE(GetGeothermalSiteClaim(rig.site)->active);
		CHECK(CancelNativeGeneratorSite(rig.site).Failed()); CHECK(rig.Fund().Failed());
		CHECK(Company::Get(_current_company)->money == cash); CHECK(Industry::GetNumItems() == 0);
		const auto fresh = ClaimGeothermalSite(rig.request); REQUIRE(fresh.error == GeothermalError::None); CHECK(fresh.id != rig.site);
	}
	SECTION("funded original plan") {
		REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
		const auto before = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
		CHECK(CancelNativeGeneratorSite(rig.site, true).Failed()); CHECK(CancelNativeGeneratorSite(rig.site).Failed());
		CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	}
	SECTION("foreign actor") {
		REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create(); foreign->money = 1000000;
		AutoRestoreBackup actor{_current_company, foreign->index}; const auto before = ComputeGeothermalSiteDigest();
		CHECK(CancelNativeGeneratorSite(rig.site).Failed()); CHECK(ComputeGeothermalSiteDigest() == before);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(CaptureUtilityPowerState().producers.empty());
}

/** A numerically reused Industry slot cannot accept even an unworked old building's detached site/control identity. */
TEST_CASE("Native generator staging cannot rebind a retired site after zero-work pool reuse", "[geothermal-generator-retired-stage]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); const auto retired = CaptureGeothermalGenerators();
	Company::Get(_current_company)->clear_limit = 64U << 16;
	const auto cash = Company::Get(_current_company)->money;
	const auto clearing = ClearNativeGenerator(rig.request.tile); REQUIRE(clearing.Succeeded());
	CHECK(clearing.GetCost() >= Money{0}); CHECK(Company::Get(_current_company)->money == cash - clearing.GetCost());
	const auto fresh = ClaimGeothermalSite(rig.request); REQUIRE(fresh.error == GeothermalError::None); rig.site = fresh.id;
	REQUIRE(rig.Fund().Succeeded()); CHECK(rig.Generator().index == retired.front().industry);
	const auto before = ComputeGeothermalGeneratorDigest(); CHECK_FALSE(ValidateGeothermalGenerators(retired));
	StageLoadedGeothermalGenerators(retired); CHECK_FALSE(AcceptLoadedGeothermalGenerators());
	CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(GetGeothermalGeneratorState(rig.Generator().index)->site == fresh.id);
	CHECK(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
}

/** Detached acceptance resolves actual physical cells and original source/control rather than trusting completion flags or allocating new ports. */
TEST_CASE("Native generator staging refuses lost source altered floor and foreign control", "[geothermal-generator-reference]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
	auto candidate = CaptureGeothermalGenerators(); const auto utility = ComputeUtilityPowerDigest();
	CHECK(GetGeothermalGeneratorPort(rig.Generator().index) == rig.request.endpoint);
	CHECK_FALSE(HasGeothermalGeneratorInstalledPorts(rig.Generator().index)); CHECK(ComputeUtilityPowerDigest() == utility);
	SECTION("lost original natural source") {
		auto geology = CaptureGeologyState(); geology.vents.clear();
		GeologyWorldState restored; REQUIRE(PrepareGeologyRestoration(geology, restored) == GeologyError::None);
		StageLoadedGeologyState(std::move(restored)); AcceptLoadedGeologyState();
	}
	SECTION("altered actual construction frame") { SetIndustryConstructionStage(rig.request.tile, 3); }
	SECTION("another live company cannot acquire this binding through staging") {
		REQUIRE(Company::CanAllocateItem()); auto *foreign = Company::Create(); foreign->money = 1000000;
		candidate.front().owner = foreign->index;
	}
	const auto before = ComputeGeothermalGeneratorDigest(); CHECK_FALSE(ValidateGeothermalGenerators(candidate));
	StageLoadedGeothermalGenerators(candidate); CHECK_FALSE(AcceptLoadedGeothermalGenerators());
	CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility);
}
