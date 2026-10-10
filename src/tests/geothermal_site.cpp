/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_site.cpp Actual natural-source/profile-backed cold site quotes, with no heat/power or finished construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../geothermal.h"
#include "../utility_power.h"
#include "../landscape_cmd.h"
#include "../rail_cmd.h"
#include "../economy_func.h"
#include "../multilayer_state.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Actual generated vent and explicit fixture-open full floor/access; no natural identity/capacity or construction stock is fabricated. */
class GeothermalSiteRig : public ExcavationFixture {
public:
	GeothermalSiteRequest request; ///< Explicit site over the original generated natural source.
	std::vector<TileIndex> floor; ///< Declared unit-test input floor, separate from production paid excavation.
	/** Select one actual generated Deep source and prepare its cold geometry through the geology owner's fixture boundary. */
	GeothermalSiteRig()
	{
		ResetGeothermalSiteClaims();
		const auto &vent = GetGeologyWorldState().vents.front();
		this->request.vent = vent.id; this->request.owner = _current_company;
		this->request.tile = this->request.endpoint = TileXYL(vent.x, vent.y, vent.layer); this->request.access = DiagDirection::SE;
		// Open the complete3x3 floor and each possible native access side as
		// explicit fixture input; production preparation must open none of it.
		for (int y = -1; y <= 3; ++y) {
			for (int x = -1; x <= 3; ++x) this->floor.push_back(TileAddWrap(this->request.tile, x, y));
		}
		this->Open(this->floor);
	}
	/** Retire only this isolated fixture's cold registry before its actual geological/company owners disappear. */
	~GeothermalSiteRig() { ResetGeothermalSiteClaims(); }
};

/** Natural/profile-backed site planning quotes all cells without changing identity, reserve, cash, RNG or the common electrical authority. */
TEST_CASE("Geothermal cold site quote consumes original vent and frozen construction terms", "[geothermal-site]")
{
	GeothermalSiteRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest();
	const auto random = std::array{_random.state[0], _random.state[1]}; const auto cash = Company::Get(_current_company)->money;
	const auto quote = PrepareGeothermalSite(rig.request); REQUIRE(quote.error == GeothermalError::None);
	CHECK(quote.vent == &GetGeologyWorldState().vents.front()); CHECK(quote.vent->thermal_capacity_w == 1000000);
	REQUIRE(quote.construction != nullptr); CHECK(quote.construction->capital_cost == 180000); CHECK(quote.construction->work_ticks == 296);
	CHECK(quote.construction->inputs.size() == 3); CHECK(quote.footprint.size() == 9);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
	CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(Company::Get(_current_company)->money == cash);
}

/** Complete excavation and shared Deep/calendar/owner/endpoint rules precede future source/site binding; no refusal repairs rock or an identity. */
TEST_CASE("Geothermal cold site refuses malformed source date endpoint or incomplete floor", "[geothermal-site]")
{
	GeothermalSiteRig rig;
	SECTION("pre1980") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("foreign actor") { rig.request.owner = CompanyID::Invalid(); }
	SECTION("absent source") { rig.request.vent = GeothermalVentID{999}; }
	SECTION("Surface coordinates") { rig.request.tile = MultiLayerMap::ProjectToSurface(rig.request.tile); }
	SECTION("missing access") { rig.request.access = DiagDirection::Invalid; }
	SECTION("remote endpoint") { rig.request.endpoint = TileAddWrap(rig.request.tile, 5, 5); }
	SECTION("interior endpoint") { rig.request.endpoint = TileAddWrap(rig.request.tile, 1, 1); }
	SECTION("riser requires its separate portal owner") { rig.request.kind = MultiLayerGeothermalConstructionKind::Riser; }
	SECTION("solid far corner") {
		const auto state = CaptureGeologyState();
		GeologyWorldState candidate; REQUIRE(PrepareGeologyRestoration(state, candidate) == GeologyError::None);
		REQUIRE(candidate.terrain.Set(TileAddWrap(rig.request.tile, 2, 2), ExcavationState::Solid));
		StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
	}
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(PrepareGeothermalSite(rig.request).error != GeothermalError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** Actual cold registration/cancellation keeps monotonic planning IDs while leaving every natural/energy quantity and physical floor unchanged. */
TEST_CASE("Geothermal cold claim reserves one original floor and cancellation never renews natural heat", "[geothermal-site-claim]")
{
	GeothermalSiteRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), power = ComputeUtilityPowerDigest(), before = ComputeGeothermalSiteDigest();
	const auto estimate = ClaimGeothermalSite(rig.request, false); REQUIRE(estimate.error == GeothermalError::None);
	CHECK(ComputeGeothermalSiteDigest() == before); CHECK(GetGeothermalSiteClaimForTile(rig.request.tile) == nullptr);
	const auto claimed = ClaimGeothermalSite(rig.request); REQUIRE(claimed.error == GeothermalError::None); CHECK(claimed.id == estimate.id);
	REQUIRE(GetGeothermalSiteClaimForTile(rig.request.tile) != nullptr); CHECK(GetGeothermalSiteClaimForTile(rig.request.tile)->id == claimed.id);
	CHECK(GetGeothermalSiteClaimForTile(MultiLayerMap::ProjectToSurface(rig.request.tile)) == nullptr);
	CHECK(ClaimGeothermalSite(rig.request).error == GeothermalError::Claimed);
	CHECK(CancelGeothermalSite(claimed.id, false) == GeothermalError::None); CHECK(GetGeothermalSiteClaim(claimed.id)->active);
	REQUIRE(CancelGeothermalSite(claimed.id) == GeothermalError::None); CHECK_FALSE(GetGeothermalSiteClaim(claimed.id)->active);
	CHECK(GetGeothermalSiteClaimForTile(rig.request.tile) == nullptr);
	const auto rebuilt = ClaimGeothermalSite(rig.request); REQUIRE(rebuilt.error == GeothermalError::None); CHECK(rebuilt.id != claimed.id);
	CHECK(CaptureGeothermalSiteClaims().size() == 2);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeUtilityPowerDigest() == power);
}

/** Staged actual planning identities restore without heat/work; malformed source, duplicated floor or endpoint refuses without repairing live authority. */
TEST_CASE("Geothermal cold claim staging validates original source and exclusive geometry", "[geothermal-site-claim]")
{
	GeothermalSiteRig rig; REQUIRE(ClaimGeothermalSite(rig.request).error == GeothermalError::None);
	const auto rows = CaptureGeothermalSiteClaims(); const auto digest = ComputeGeothermalSiteDigest();
	ResetGeothermalSiteClaims(); StageLoadedGeothermalSiteClaims(rows); REQUIRE(AcceptLoadedGeothermalSiteClaims());
	CHECK(ComputeGeothermalSiteDigest() == digest);
	auto bad = rows;
	SECTION("orphan source") { bad[0].site.vent = GeothermalVentID{999}; }
	SECTION("reused planning identity") { bad[0].id = GeothermalSiteID{1}; }
	SECTION("orphan company") { bad[0].site.owner = CompanyID::Invalid(); }
	SECTION("interior endpoint") { bad[0].site.endpoint = TileAddWrap(rig.request.tile, 1, 1); }
	SECTION("duplicate active source and floor") { bad.push_back(bad.front()); bad.back().id = GeothermalSiteID{1}; }
	StageLoadedGeothermalSiteClaims(bad); CHECK_FALSE(AcceptLoadedGeothermalSiteClaims()); CHECK(ComputeGeothermalSiteDigest() == digest);
}

/** Native clearing/overbuild respects an actual cold floor reservation; cancellation permits ordinary clearing while preserving the natural source and cavity. */
TEST_CASE("Geothermal cold floor protects native clearing and building until explicit cancellation", "[geothermal-site-lifecycle]")
{
	GeothermalSiteRig rig; const auto claim = ClaimGeothermalSite(rig.request); REQUIRE(claim.error == GeothermalError::None);
	const auto source = GetGeologyWorldState().vents.front(); const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CmdLandscapeClear({}, rig.request.tile).Failed()); CHECK(CmdLandscapeClear(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(CmdBuildSingleRail({}, rig.request.tile, RAILTYPE_RAIL, Track::X, false).Failed());
	CHECK(CmdBuildSingleRail(DoCommandFlag::Execute, rig.request.tile, RAILTYPE_RAIL, Track::X, false).Failed());
	REQUIRE(CancelGeothermalSite(claim.id) == GeothermalError::None);
	REQUIRE(CmdLandscapeClear(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK(GetGeologyWorldState().terrain.Get(rig.request.tile) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().vents.front().id == source.id); CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == source.thermal_capacity_w);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	REQUIRE(ClaimGeothermalSite(rig.request).error == GeothermalError::None);
}

/** Actual company takeover cancels unbuilt planning rights before identity reuse and never removes/replenishes a natural field. */
TEST_CASE("Geothermal native takeover retires cold planning rights and preserves natural source", "[geothermal-site-lifecycle]")
{
	Company *buyer = nullptr;
	{
		GeothermalSiteRig rig; REQUIRE(ClaimGeothermalSite(rig.request).error == GeothermalError::None);
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		REQUIRE(Company::CanAllocateItem()); buyer = Company::Create();
		AutoRestoreBackup local(_local_company, CompanyID::Invalid());
		ChangeOwnershipOfCompanyItems(_current_company, buyer->index);
		CHECK(GetGeothermalSiteClaimForTile(rig.request.tile) == nullptr); CHECK_FALSE(CaptureGeothermalSiteClaims().front().active);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	}
	delete buyer;
}

/** A fresh world/schema cannot inherit a previous world's cold reservation, even when its company/source pool ordinals coincide. */
TEST_CASE("Geothermal cold planning resets with the native world metadata owner", "[geothermal-site-lifecycle]")
{
	GeothermalSiteRig rig; REQUIRE(ClaimGeothermalSite(rig.request).error == GeothermalError::None);
	ResetMultiLayerStateSchema(); CHECK(CaptureGeothermalSiteClaims().empty()); CHECK(GetGeothermalSiteClaimForTile(rig.request.tile) == nullptr);
}

/** Retired planning history still requires its actual original natural/geometry references, and metadata cannot omit their owners. */
TEST_CASE("Geothermal cancelled history refuses orphan references and undeclared source ownership", "[geothermal-site-claim]")
{
	GeothermalSiteRig rig; const auto result = ClaimGeothermalSite(rig.request); REQUIRE(result.error == GeothermalError::None);
	const auto metadata = CaptureMultiLayerStateSchema();
	CHECK((metadata.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_SITES) != 0);
	CHECK(ValidateMultiLayerStateReferences(metadata) == MultiLayerStateError::None);
	auto missing = metadata; missing.feature_flags &= ~MULTILAYER_FEATURE_VENTS;
	CHECK(ValidateMultiLayerStateReferences(missing) == MultiLayerStateError::UnsupportedFeatures);
	REQUIRE(CancelGeothermalSite(result.id) == GeothermalError::None);
	auto rows = CaptureGeothermalSiteClaims();
	SECTION("retired original source") { rows[0].site.vent = GeothermalVentID{999}; }
	SECTION("retired endpoint geometry") { rows[0].site.endpoint = TileAddWrap(rig.request.tile, 7, 7); }
	const auto before = ComputeGeothermalSiteDigest(); StageLoadedGeothermalSiteClaims(rows);
	CHECK_FALSE(AcceptLoadedGeothermalSiteClaims()); CHECK(ComputeGeothermalSiteDigest() == before);
}
