/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_bootstrap.cpp Portal-issued solid-start proof and issuer refusals, all without mutation.
 * These are geometry-contract tests: no paid cut, live entry or wagon exchange is claimed here.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 8B
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-f-solid-starter-contract.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "solid_starter_fixture.h"
#include "../road_cmd.h"
#include "../direction_func.h"

#include "../safeguards.h"

/** Authority snapshot that a pure issue/refusal path must leave exactly unchanged. */
struct BootstrapStateDigest {
	uint64_t geology = 0; ///< Canonical geological digest.
	uint64_t portal = 0; ///< Canonical portal work digest.
	Money cash = 0; ///< Real company funds.
	std::array<uint32_t, 2> random{}; ///< Synchronized random stream.
	size_t vehicles = 0; ///< Allocated vehicle count.

	/** @return The current authority snapshot. */
	static BootstrapStateDigest Capture()
	{
		return {ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerPortalWorkDigest(), Company::Get(_current_company)->money, {_random.state[0], _random.state[1]}, Vehicle::GetNumItems()};
	}

	/** @param other Snapshot to compare. @return Whether every field matches. */
	bool operator==(const BootstrapStateDigest &other) const = default;
};

/** A Sealed rail pair issues complete geometry before any route, train, job or payment exists. */
TEST_CASE("Sealed portal issues a pure solid-start proof before acquisition", "[portal-bootstrap]")
{
	SolidStarterFixture fixture;
	const auto before = BootstrapStateDigest::Capture();
	const auto proof = IssueLayerPortalBootstrap(fixture.request);
	REQUIRE(proof.error == LayerPortalWorkError::None);
	CHECK(proof.pair_key == fixture.source);
	CHECK(proof.pair_revision == GetLayerPortalWorkRecord(fixture.source)->revision);
	CHECK(proof.company == _current_company);
	CHECK(proof.model_id == 0);
	CHECK(proof.origin == fixture.origin->index);
	CHECK(proof.source == fixture.source);
	CHECK(proof.source_support == GetExcavationCutterEnvelope(fixture.source));
	CHECK(proof.source_support.size() == 9);
	CHECK(proof.destination == fixture.destination);
	CHECK(proof.exit_heading == DiagDirToDir(DiagDirection::SE));
	CHECK(proof.pocket == GetExcavationCutterEnvelope(fixture.destination));
	CHECK(std::ranges::binary_search(proof.pocket, fixture.destination));
	// The door cell D' touches the housing, so it is inside the pocket; the row beyond it is new rock that cut 0 also pays for.
	CHECK(std::ranges::binary_search(proof.pocket, GetLayerPortalDoorTile(fixture.destination)));
	const auto ahead = TileIndexDiffCByDir(proof.exit_heading);
	CHECK_FALSE(std::ranges::binary_search(proof.pocket, TileAddWrap(fixture.destination, 2 * ahead.x, 2 * ahead.y)));
	// The proof is geometry only: every pocket cell stays Solid, the housing stays a portal and ordinary construction still refuses.
	for (TileIndex cell : proof.pocket) CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Solid);
	CHECK(IsLayerPortalTile(fixture.destination));
	CHECK(CheckExcavatedFootprint(proof.pocket).error == ExcavationError::SolidRock);
	CHECK(RevalidateLayerPortalBootstrap(proof, fixture.request) == LayerPortalWorkError::None);
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Sealed);
	CHECK(BootstrapStateDigest::Capture() == before);

	// Geology's issuer projection: only the rock-side housing of an unassigned Sealed pair backs an unpublished proposal.
	const auto issuer = GetLayerPortalStarterIssuer(fixture.destination, _current_company, ExcavationJobID{UINT32_MAX});
	CHECK(issuer.error == LayerPortalWorkError::None);
	CHECK(issuer.pair_key == fixture.source);
	CHECK(issuer.exit_heading == proof.exit_heading);
	CHECK(GetLayerPortalStarterIssuer(fixture.source, _current_company, ExcavationJobID{UINT32_MAX}).error == LayerPortalWorkError::InvalidPair);
	CHECK(GetLayerPortalStarterIssuer(TileXYL(30, 30, MapLayer::Underground), _current_company, ExcavationJobID{UINT32_MAX}).error == LayerPortalWorkError::InvalidPair);
	CHECK(GetLayerPortalStarterIssuer(fixture.destination, _current_company, ExcavationJobID{0}).error == LayerPortalWorkError::Conflict);
	CHECK(GetLayerPortalStarterIssuer(fixture.destination, CompanyID::Invalid(), ExcavationJobID{UINT32_MAX}).error == LayerPortalWorkError::InvalidOwner);
	CHECK(BootstrapStateDigest::Capture() == before);
}

/** Each stale or invalid input refuses the fresh issue and/or the revalidation of an earlier proof, changing nothing. */
TEST_CASE("Stale, blocked, foreign and wrong-facility proofs refuse without mutation", "[portal-bootstrap]")
{
	SolidStarterFixture fixture;
	const auto proof = IssueLayerPortalBootstrap(fixture.request);
	REQUIRE(proof.error == LayerPortalWorkError::None);
	auto request = fixture.request;
	auto issue = LayerPortalWorkError::None; ///< Expected fresh issue result.
	auto revalidate = LayerPortalWorkError::None; ///< Expected revalidation of the earlier proof.
	Company *other = nullptr; ///< Foreign company, deleted after the checks.
	SECTION("Stale pair revision") {
		auto record = *GetLayerPortalWorkRecord(fixture.source);
		++record.revision;
		const std::array rows{record};
		REQUIRE(ValidateLayerPortalWorkRecords(rows, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork({record});
		AcceptLoadedLayerPortalWork();
		revalidate = LayerPortalWorkError::Conflict; // A fresh issue succeeds at the new revision; the old proof is stale.
	}
	SECTION("Blocked source floor") {
		REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileXYL(21, 20, MapLayer::Surface), ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		issue = revalidate = LayerPortalWorkError::Conflict;
	}
	SECTION("Foreign proposal owner") {
		REQUIRE(Company::CanAllocateItem());
		other = Company::Create();
		request.proposal.company = other->index;
		issue = revalidate = LayerPortalWorkError::InvalidOwner;
	}
	SECTION("Rock-side facility") {
		request.portal = fixture.destination;
		issue = revalidate = LayerPortalWorkError::InvalidPair;
	}
	SECTION("Freight wagon model") {
		request.proposal.model_id = 2;
		issue = revalidate = LayerPortalWorkError::InvalidReference;
	}
	SECTION("Before the cutter model's introduction") {
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1900}, 0, 1);
		issue = revalidate = LayerPortalWorkError::LockedDeep;
	}
	SECTION("Pocket already reserved by other work") {
		// Another job opens beside the pocket and reserves a sweep whose west column overlaps two pocket cells.
		const auto anchor = TileXYL(23, 21, MapLayer::Underground);
		fixture.world.Open(GetExcavationCutterEnvelope(anchor));
		ExcavationJob job;
		job.company = _current_company; job.model_id = 0; job.commissioning_anchor = anchor;
		ExcavationCutGeometry cut{TileXYL(22, 21, MapLayer::Underground), Direction::NE, GetExcavationCutterEnvelope(TileXYL(22, 21, MapLayer::Underground)), {anchor}};
		job.requested_tiles = {cut.cutter_position};
		job.authorized_tiles = cut.swept_tiles;
		job.route = {cut};
		REQUIRE(ReserveExcavationJob(job, true).error == ExcavationError::None);
		issue = revalidate = LayerPortalWorkError::Conflict;
	}
	const auto before = BootstrapStateDigest::Capture();
	CHECK(IssueLayerPortalBootstrap(request).error == issue);
	CHECK(RevalidateLayerPortalBootstrap(proof, request) == revalidate);
	CHECK(BootstrapStateDigest::Capture() == before);
	CHECK(GetGeologyWorldState().terrain.Get(fixture.destination) == ExcavationState::Solid);
	delete other;
}
