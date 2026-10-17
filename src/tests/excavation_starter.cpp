/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file excavation_starter.cpp Solid portal start through the canonical planner, pass, reservation and cut owners.
 * The material participant is an interface fixture: it proves the geology transaction boundary, not the later
 * actual-wagon commissioning (Plan 4 Groups 6-8 and R2MLVS3MP Group 7).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 14B
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-f-solid-starter-contract.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "solid_starter_fixture.h"
#include "../direction_func.h"

#include "../safeguards.h"

/** Issued proof, exact first-cut quote and optionally a reserved/published job, all through real owners. */
class StarterRig {
public:
	SolidStarterFixture site; ///< Real sealed pair, origin and reference train.
	LayerPortalBootstrapProof proof; ///< Portal-issued geometry.
	ExcavationJob proposal; ///< First segment out of the door with exactly sized pass capacities.
	ExcavationCutResult quote; ///< Shared hardness quote of the whole first sweep, taken before reservation.
	ExcavationJobID id{UINT32_MAX}; ///< Reserved job, when requested.
	StarterParticipant participant; ///< Interface spoil/support/funds owner sized exactly to the first cut.

	/**
	 * Prepare the start and, optionally, reserve and publish it.
	 * @param reserve Reserve the proposal through ReserveExcavationJob.
	 * @param publish Also publish the pair as Commissioning the reserved job.
	 */
	explicit StarterRig(bool reserve = true, bool publish = true)
	{
		this->proof = IssueLayerPortalBootstrap(this->site.request);
		REQUIRE(this->proof.error == LayerPortalWorkError::None);
		this->proposal = this->site.FirstSegmentProposal(this->proof, this->proof.exit_heading);
		REQUIRE(this->proposal.route.size() == 1);
		this->quote = QuoteExcavationCells(this->proposal.route[0].swept_tiles, 0);
		REQUIRE(this->quote.error == ExcavationError::None);
		// Both independent wagon groups are sized to be exactly full after the first cut, with no spare tonne.
		this->proposal.pass_terms->earth_capacity_t = this->quote.earth_t;
		this->proposal.pass_terms->rock_capacity_t = this->quote.rock_t;
		this->participant.earth_space = this->quote.earth_t;
		this->participant.rock_space = this->quote.rock_t;
		if (!reserve) return;
		const auto reserved = ReserveExcavationJob(this->proposal, true);
		REQUIRE(reserved.error == ExcavationError::None);
		this->id = reserved.id;
		if (publish) this->site.Publish(this->id);
	}

	/** @return Exact live job/revision/cursor request. */
	ExcavationCutRequest Request() const
	{
		const auto *job = GetExcavationJob(this->id);
		return {this->id, job->revision, job->cursor};
	}
};

/** The first cut leaves the housing through its door, sweeps the whole solid pocket and pays for all twelve cells once. */
TEST_CASE("Solid portal start pays the whole pocket in one first cut", "[excavation-starter]")
{
	StarterRig rig(false);
	const auto &cut = rig.proposal.route[0];
	CHECK(cut.heading == rig.proof.exit_heading);
	CHECK(cut.swept_tiles.size() == 12);
	CHECK(std::ranges::includes(cut.swept_tiles, rig.proof.pocket));
	CHECK(rig.quote.new_cells == 12);

	// The canonical planner proposes the same segment and charges the pocket; sideways or detached starts refuse.
	ExcavationOutline outline;
	outline.layer = MapLayer::Underground;
	outline.requested_tiles = rig.proposal.requested_tiles;
	outline.authorized_tiles = rig.proposal.authorized_tiles;
	const auto planned = PlanExcavationRoute(outline, SolidStarterFixture::StartPose(rig.proof), 0);
	REQUIRE(planned.error == ExcavationError::None);
	REQUIRE(planned.route.size() == 1);
	CHECK(planned.route[0].heading == rig.proof.exit_heading);
	CHECK(planned.route[0].swept_tiles == cut.swept_tiles);
	CHECK(planned.estimate.new_cells == 12);
	CHECK(planned.estimate.earth_t == rig.quote.earth_t);
	CHECK(planned.estimate.rock_t == rig.quote.rock_t);
	auto sideways = SolidStarterFixture::StartPose(rig.proof);
	sideways.heading = static_cast<Direction>((to_underlying(rig.proof.exit_heading) + 1) % 8);
	CHECK(PlanExcavationRoute(outline, sideways, 0).error == ExcavationError::IllegalTurn);
	auto detached = SolidStarterFixture::StartPose(rig.proof);
	detached.history.back() = TileAddWrap(rig.proof.destination, -1, 0);
	CHECK(PlanExcavationRoute(outline, detached, 0).error == ExcavationError::InvalidRoute);

	// One exchange after the single complete cut; the loaded return starts in the issuing housing.
	const auto pass = PrepareExcavationPass(rig.proposal, *rig.proposal.pass_terms);
	REQUIRE(pass.error == ExcavationError::None);
	CHECK(pass.terms.exchange_after == std::vector<uint16_t>{1});
	CHECK(pass.terms.exchange_path == std::vector<TileIndex>{rig.proof.destination});
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portal = ComputeLayerPortalWorkDigest();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(ReserveExcavationJob(rig.proposal, false).error == ExcavationError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);

	// Execute reserves every unopened swept cell, including the housing's own rock, while its portal metadata stays.
	const auto reserved = ReserveExcavationJob(rig.proposal, true);
	REQUIRE(reserved.error == ExcavationError::None);
	rig.id = reserved.id;
	for (TileIndex cell : cut.swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavating);
	CHECK(IsLayerPortalTile(rig.proof.destination));
	CHECK(ComputeLayerPortalWorkDigest() == portal);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(CheckExcavatedFootprint(rig.proof.pocket).error == ExcavationError::ActiveCut);

	// A Sealed pair only backs proposals: the reserved job cannot cut until the pair is Commissioning it.
	CHECK(PrepareExcavationCut(rig.Request(), rig.participant.Capture()).error == ExcavationError::StaleParticipant);
	rig.site.Publish(rig.id);
	REQUIRE(AdvanceExcavationWork(rig.id, rig.quote.work_ticks, rig.participant) == ExcavationError::None);
	const auto prepared = PrepareExcavationCut(rig.Request(), rig.participant.Capture());
	REQUIRE(prepared.error == ExcavationError::None);
	CHECK(prepared.new_cells == 12);
	CHECK(prepared.support_units == 12);
	CHECK(prepared.earth_t == rig.quote.earth_t);
	CHECK(prepared.rock_t == rig.quote.rock_t);
	CHECK(prepared.cost == rig.quote.cost);
	const auto request = rig.Request();
	const auto receipt = CommitExcavationCut(request, rig.participant);
	REQUIRE(receipt.error == ExcavationError::None);
	CHECK(receipt == prepared);

	// One paid receipt opened all twelve cells; both wagon groups are exactly full and the housing is still a portal.
	for (TileIndex cell : cut.swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated);
	CHECK(IsLayerPortalTile(rig.proof.destination));
	CHECK(GetTileOwner(rig.proof.destination) == _current_company);
	CHECK(rig.participant.earth == rig.participant.earth_space);
	CHECK(rig.participant.rock == rig.participant.rock_space);
	CHECK(rig.participant.commits == 1);
	CHECK(Company::Get(_current_company)->money == cash - prepared.cost);
	CHECK(GetExcavationJob(rig.id)->state == ExcavationJobState::Completed);

	// Replaying the same request cannot open, pay or fill anything a second time.
	const auto after = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CommitExcavationCut(request, rig.participant).error == ExcavationError::AlreadyCompleted);
	CHECK(rig.participant.commits == 1);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == after);
}

/** Geometry that skips the door or a pocket cell refuses before any reservation. */
TEST_CASE("Solid start refuses sideways exits and undersized wagon groups before reservation", "[excavation-starter]")
{
	StarterRig rig(false);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	auto proposal = rig.proposal;
	auto expected = ExcavationError::None;
	SECTION("Sideways first heading") {
		proposal = rig.site.FirstSegmentProposal(rig.proof, static_cast<Direction>((to_underlying(rig.proof.exit_heading) + 2) % 8));
		proposal.pass_terms = rig.proposal.pass_terms;
		expected = ExcavationError::IllegalTurn;
	}
	SECTION("A pocket cell left out of the first sweep") {
		auto &swept = proposal.route[0].swept_tiles;
		swept.erase(std::ranges::find(swept, rig.proof.pocket.front()));
		expected = ExcavationError::InvalidRoute;
	}
	SECTION("Earth group one tonne short") {
		proposal.pass_terms->earth_capacity_t = rig.quote.earth_t - 1;
		expected = ExcavationError::InsufficientCapacity;
	}
	SECTION("Rock cannot borrow spare Earth space") {
		proposal.pass_terms->earth_capacity_t = rig.quote.earth_t + 64;
		proposal.pass_terms->rock_capacity_t = rig.quote.rock_t - 1;
		expected = ExcavationError::InsufficientCapacity;
	}
	CHECK(PrepareExcavationPass(proposal, *proposal.pass_terms).error == expected);
	CHECK(ReserveExcavationJob(proposal, false).error == expected);
	CHECK(ReserveExcavationJob(proposal, true).error == expected);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(GetGeologyWorldState().terrain.Get(rig.proof.destination) == ExcavationState::Solid);
}

/** Every first-cut participant or issuer failure leaves terrain, stock, support, cash and the reservation unchanged. */
TEST_CASE("Solid start refuses capacity, support, funds and lost issuers without mutation", "[excavation-starter]")
{
	StarterRig rig;
	REQUIRE(AdvanceExcavationWork(rig.id, rig.quote.work_ticks, rig.participant) == ExcavationError::None);
	auto expected = ExcavationError::None;
	SECTION("Earth one tonne short") {
		rig.participant.earth_space = rig.quote.earth_t - 1;
		expected = ExcavationError::InsufficientCapacity;
	}
	SECTION("Rock cannot borrow spare Earth space") {
		rig.participant.earth_space = rig.quote.earth_t + 64;
		rig.participant.rock_space = rig.quote.rock_t - 1;
		expected = ExcavationError::InsufficientCapacity;
	}
	SECTION("Last support unit missing") {
		rig.participant.supports = rig.quote.support_units - 1;
		expected = ExcavationError::MissingSupport;
	}
	SECTION("Insufficient funds") {
		Company::Get(_current_company)->money = rig.quote.cost - 1;
		expected = ExcavationError::InsufficientFunds;
	}
	SECTION("Issuer lost") {
		ResetLayerPortalWorkState();
		expected = ExcavationError::MissingSupport;
	}
	SECTION("Issuer sealed again without this job") {
		auto record = *GetLayerPortalWorkRecord(rig.site.source);
		record.state = LayerPortalWorkState::Sealed;
		record.project = ExcavationJobID{UINT32_MAX};
		record.cutter = VehicleID::Invalid();
		record.origin = StationID::Invalid();
		++record.revision;
		// The loader refuses this stale binding outright; it is staged directly only to exercise the runtime refusal.
		const std::array rows{record};
		CHECK(ValidateLayerPortalWorkRecords(rows, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::InvalidReference);
		StageLoadedLayerPortalWork({record});
		AcceptLoadedLayerPortalWork();
		expected = ExcavationError::StaleParticipant;
	}
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto supports = rig.participant.supports;
	const auto request = rig.Request();
	CHECK(PrepareExcavationCut(request, rig.participant.Capture()).error == expected);
	CHECK(CommitExcavationCut(request, rig.participant).error == expected);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(rig.participant.supports == supports);
	CHECK(rig.participant.earth == 0);
	CHECK(rig.participant.rock == 0);
	CHECK(rig.participant.commits == 0);
	for (TileIndex cell : rig.proof.pocket) CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavating);
}

/** A reserved or partly worked start reloads only together with the pair that is Commissioning it. */
TEST_CASE("Reserved and partly worked solid starts reload only with their issuer", "[excavation-starter]")
{
	StarterRig rig;
	SECTION("Initial reservation") {}
	SECTION("Partial work") {
		REQUIRE(AdvanceExcavationWork(rig.id, std::max<uint64_t>(1, rig.quote.work_ticks / 2), rig.participant) == ExcavationError::None);
	}
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portal = ComputeLayerPortalWorkDigest();
	const auto snapshot = CaptureGeologyState();
	const auto records = CaptureLayerPortalWorkRecords();
	const auto profile = GetCurrentMultiLayerProfileSelection();
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
	CHECK(ComputeGeologyStateDigest(restored) == geology);

	// The loaded pair must still be Commissioning exactly this unopened start.
	CHECK(ValidateLayerPortalWorkRecords(records, profile, restored) == LayerPortalWorkError::None);
	CHECK(ValidateLayerPortalWorkRecords(std::span<const LayerPortalWorkRecord>{}, profile, restored) == LayerPortalWorkError::InvalidReference);
	auto foreign = records;
	foreign[0].project = ExcavationJobID{rig.id.base() + 1};
	CHECK(ValidateLayerPortalWorkRecords(foreign, profile, restored) == LayerPortalWorkError::InvalidReference);
	auto sealed = records;
	sealed[0].state = LayerPortalWorkState::Sealed;
	sealed[0].project = ExcavationJobID{UINT32_MAX};
	sealed[0].cutter = VehicleID::Invalid();
	sealed[0].origin = StationID::Invalid();
	CHECK(ValidateLayerPortalWorkRecords(sealed, profile, restored) == LayerPortalWorkError::InvalidReference);

	// Publishing the validated candidates resumes the same job; the cut still needs all of its synchronized work.
	StageLoadedGeologyState(std::move(restored));
	AcceptLoadedGeologyState();
	StageLoadedLayerPortalWork(records);
	AcceptLoadedLayerPortalWork();
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerPortalWorkDigest() == portal);
	const auto *job = GetExcavationJob(rig.id);
	REQUIRE(job != nullptr);
	const uint64_t remaining = rig.quote.work_ticks - job->partial_work_ticks;
	REQUIRE(remaining > 0);
	CHECK(CommitExcavationCut(rig.Request(), rig.participant).error == ExcavationError::NoProgress);
	REQUIRE(AdvanceExcavationWork(rig.id, remaining, rig.participant) == ExcavationError::None);
	CHECK(CommitExcavationCut(rig.Request(), rig.participant).error == ExcavationError::None);
	for (TileIndex cell : rig.proof.pocket) CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated);
}
