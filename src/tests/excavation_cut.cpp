/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_cut.cpp Complete transaction, demolition and resumed-job conservation fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 11/15/17 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../road_cmd.h"
#include "../road.h"
#include "../landscape_cmd.h"
#include "../tile_map.h"

#include "../safeguards.h"

/** Controlled external stock/support owner using the real fixture company cash; actual wagon/controller integration remains Plan 4/5. */
class CutFixtureParticipant final : public ExcavationMaterialParticipant, public ExcavationWorkParticipant {
public:
	uint64_t earth = 0, rock = 0, revision = 0; ///< Explicit external fixture stock and stamp.
	uint32_t supports = 64, commits = 0; ///< Actual fixture material supply and committed-receipt count.
	bool validate_allowed = true, commit_allowed = true; ///< Controlled unchanged refusals, never a partial commit.
	CompanyID company = _current_company; ///< Real cash owner; no second money balance exists.
	/** @return Actual independent stock spaces/support units and real company funds without mutation. */
	ExcavationMaterialSnapshot Capture() const override
	{
		return {this->revision, 64 - this->earth, 128 - this->rock, this->supports, Company::Get(this->company)->money};
	}
	/**
	 * Revalidate the actual fixture owner identity and every complete quantity.
	 * @param expected Captured external state.
	 * @param delta Complete proposed Earth/Rock/support/work charge.
	 * @return Whether the unchanged participant still accepts this exact transaction.
	 */
	bool Validate(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) const override
	{
		const auto now = this->Capture();
		// Independent capacities, support and actual funds must all still match the captured owner; no validation side effects occur.
		return this->validate_allowed && expected.revision == now.revision && expected.earth_space_t == now.earth_space_t && expected.rock_space_t == now.rock_space_t && expected.support_units == now.support_units && expected.available_funds == now.available_funds && delta.earth_t <= now.earth_space_t && delta.rock_t <= now.rock_space_t && delta.support_units <= now.support_units && delta.cost <= now.available_funds;
	}
	/**
	 * Atomically apply only a still-valid complete external receipt.
	 * @param expected Exact external owner/stamp from preflight.
	 * @param delta Complete generated stock, support consumption and real company charge.
	 * @return True after the whole receipt, or unchanged false; no allocation or throwing operation occurs.
	 */
	bool Commit(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) noexcept override
	{
		if (!this->commit_allowed || !this->Validate(expected, delta)) return false; // A deliberately refused commit cannot consume even one unit.
		this->earth += delta.earth_t;
		this->rock += delta.rock_t;
		this->supports -= delta.support_units;
		Company::Get(this->company)->money -= delta.cost;
		++this->revision;
		++this->commits;
		return true;
	}
	/**
	 * Validate the fixture's existing support floor for synchronized work accumulation.
	 * @param job Actual owned geometry job.
	 * @param cut Exact next segment.
	 * @return Whether actual support terrain remains open; no vehicle binding is claimed by this fixture.
	 */
	bool IsSupported(const ExcavationJob &job, const ExcavationCutGeometry &cut) const override
	{
		if (!Company::IsValidID(job.company)) return false;
		for (TileIndex tile : cut.support_tiles) {
			if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return false;
		}
		return true;
	}
};

/** Actual native reservation with an explicitly open 3x3 starter room and two accepted forward cutter envelopes. */
class CutFixtureRig {
public:
	ExcavationFixture world; ///< Real profile/company/geology owners.
	TileIndex anchor = INVALID_TILE; ///< Explicit supported starting pose.
	ExcavationJobID id{UINT32_MAX}; ///< Actual owner-allocated reservation.
	CutFixtureParticipant participant; ///< External stock/support and actual company cash.
	/**
	 * Prepare bounded real owner geometry and a paid-work-ready reservation.
	 * @param seam Use an actual generated Coal seam with exactly one recoverable tonne left.
	 */
	explicit CutFixtureRig(bool seam = false)
	{
		if (seam) {
			const auto &span = GetGeologyWorldState().deposits.Records().front().spans.front();
			this->anchor = TileXYL(span.first_x - 1, span.y + 1, MapLayer::Underground);
			auto snapshot = CaptureGeologyState();
			snapshot.deposits.front().remaining_reserve = 1;
			GeologyWorldState restored;
			REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
			StageLoadedGeologyState(std::move(restored));
			AcceptLoadedGeologyState();
		} else {
			this->anchor = TileXYL(20, 20, MapLayer::Underground);
		}
		std::vector<TileIndex> starter;
		// Initial support is actual open geometry; no undeclared commissioning-pocket permission is inferred.
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) starter.push_back(TileAddWrap(this->anchor, x, y));
		}
		this->world.Open(starter);
		ExcavationJob job;
		job.company = _current_company; job.model_id = 0; job.commissioning_anchor = this->anchor;
		job.requested_tiles = {TileAddWrap(this->anchor, 2, 0)};
		// Explicit authorized floor contains both 3x3 envelopes; existing starter support outside it is never excavated.
		for (int y = -1; y <= 1; ++y) {
			for (int x = 0; x <= 3; ++x) job.authorized_tiles.push_back(TileAddWrap(this->anchor, x, y));
		}
		for (int step = 1; step <= 2; ++step) {
			ExcavationCutGeometry cut;
			cut.cutter_position = TileAddWrap(this->anchor, step, 0); cut.heading = Direction::SW;
			cut.support_tiles = {this->anchor};
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(cut.cutter_position, x, y));
			}
			job.route.push_back(std::move(cut));
		}
		const auto reserved = ReserveExcavationJob(job, true);
		REQUIRE(reserved.error == ExcavationError::None);
		this->id = reserved.id;
	}
	/** @return Exact live cursor/revision request; no test-owned duplicate progress cursor exists. */
	ExcavationCutRequest Request() const
	{
		const auto *job = GetExcavationJob(this->id);
		return {this->id, job->revision, job->cursor};
	}
	/** Accumulate exactly the quoted synchronized work without opening a cell or generating stock. */
	void Work()
	{
		const auto quote = PrepareExcavationCut(this->Request(), this->participant.Capture());
		REQUIRE(quote.error == ExcavationError::None);
		REQUIRE(AdvanceExcavationWork(this->id, quote.work_ticks, this->participant) == ExcavationError::None);
	}
};

/** Quote/no-progress are read-only, complete execution charges once, and replay cannot produce another receipt. */
TEST_CASE("Excavation complete cuts charge support and deliver independent spoil exactly once", "[excavation-cut]")
{
	CutFixtureRig rig;
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_current_company)->money;
	const auto quote = PrepareExcavationCut(rig.Request(), rig.participant.Capture());
	REQUIRE(quote.error == ExcavationError::None);
	CHECK(quote.new_cells == 3);
	CHECK(quote.earth_t == 9);
	CHECK(quote.rock_t == 18);
	CHECK(quote.support_units == 3);
	CHECK(quote.cost == 90);
	CHECK(quote.work_ticks == 222);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(CommitExcavationCut(rig.Request(), rig.participant).error == ExcavationError::NoProgress);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(rig.participant.commits == 0);
	rig.Work();
	const auto request = rig.Request();
	const auto committed = CommitExcavationCut(request, rig.participant);
	REQUIRE(committed.error == ExcavationError::None);
	CHECK(committed == quote);
	CHECK(rig.participant.earth == 9);
	CHECK(rig.participant.rock == 18);
	CHECK(rig.participant.supports == 61);
	CHECK(Company::Get(_current_company)->money == money - 90);
	CHECK(GetExcavationJob(rig.id)->cursor == 1);
	CHECK(GetExcavationJob(rig.id)->partial_work_ticks == 0);
	const auto after = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CommitExcavationCut(request, rig.participant).error == ExcavationError::StaleParticipant);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == after);
	CHECK(rig.participant.commits == 1);
	// Cancellation after a real coordinated cut preserves its paid/cargo receipt and completed cavity.
	CHECK(CancelExcavationJob(rig.id) == ExcavationError::None);
	CHECK(GetExcavationJob(rig.id)->completed_earth_t == 9);
	CHECK(GetExcavationJob(rig.id)->completed_rock_t == 18);
	CHECK(GetExcavationJob(rig.id)->completed_work_cost == 90);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(rig.anchor, 2, 0)) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(rig.anchor, 3, 0)) == ExcavationState::Solid);
	CHECK(rig.participant.earth == 9);
	CHECK(Company::Get(_current_company)->money == money - 90);
}

/** Full independent sinks, insufficient funds and participant refusal leave all geological/external quantities unchanged. */
TEST_CASE("Excavation failed preparations and external commits preserve the whole transaction", "[excavation-cut-refusal]")
{
	CutFixtureRig rig;
	rig.Work();
	ExcavationError expected = ExcavationError::None;
	SECTION("Rock capacity cannot borrow unused Earth space") { rig.participant.rock = 128; expected = ExcavationError::InsufficientCapacity; }
	SECTION("Last missing currency unit refuses the complete cut") { Company::Get(_current_company)->money = 89; expected = ExcavationError::InsufficientFunds; }
	SECTION("Last missing support unit refuses the complete cut") { rig.participant.supports = 2; expected = ExcavationError::MissingSupport; }
	SECTION("Stale external validation is read-only") { rig.participant.validate_allowed = false; expected = ExcavationError::StaleParticipant; }
	SECTION("An unchanged false external commit cannot leave geological progress") { rig.participant.commit_allowed = false; expected = ExcavationError::StaleParticipant; }
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto material = rig.participant.Capture();
	const auto result = CommitExcavationCut(rig.Request(), rig.participant);
	CHECK(result.error == expected);
	CHECK(result.earth_t == 0);
	CHECK(result.rock_t == 0);
	CHECK(result.cost == 0);
	CHECK(result.reserve_debits.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(rig.participant.Capture().earth_space_t == material.earth_space_t);
	CHECK(rig.participant.Capture().rock_space_t == material.rock_space_t);
	CHECK(rig.participant.Capture().available_funds == material.available_funds);
	CHECK(rig.participant.Capture().support_units == material.support_units);
	CHECK(rig.participant.commits == 0);
}

/** Resource loss clips to the actual last reserve tonne and never creates a second mineral cargo channel. */
TEST_CASE("Excavation loss ledger debits the last recoverable seam unit once", "[excavation-cut-reserve]")
{
	CutFixtureRig rig(true);
	const auto quote = PrepareExcavationCut(rig.Request(), rig.participant.Capture());
	REQUIRE(quote.error == ExcavationError::None);
	REQUIRE(quote.reserve_debits.size() == 1);
	CHECK(quote.reserve_debits[0].tonnes == 1);
	CHECK(quote.reserve_debits[0].expected_remaining == 1);
	CHECK(quote.reserve_debits[0].resource == CT_COAL);
	const auto id = quote.reserve_debits[0].deposit;
	CHECK(GetGeologyWorldState().deposits.Get(id)->remaining_reserve == 1);
	rig.Work();
	const auto request = rig.Request();
	const auto receipt = CommitExcavationCut(request, rig.participant);
	REQUIRE(receipt.error == ExcavationError::None);
	CHECK(receipt.reserve_debits == quote.reserve_debits);
	CHECK(GetGeologyWorldState().deposits.Get(id)->remaining_reserve == 0);
	CHECK(GetGeologyWorldState().deposits.Get(id)->claimant == CompanyID::Invalid());
	CHECK(rig.participant.earth == receipt.earth_t);
	CHECK(rig.participant.rock == receipt.rock_t);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CommitExcavationCut(request, rig.participant).error == ExcavationError::StaleParticipant);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
}

/** Real infrastructure construction/demolition cannot refill a cavity or replenish the last consumed mineral unit. */
TEST_CASE("Excavation demolition preserves cavities and finite seam depletion", "[excavation-demolition]")
{
	CutFixtureRig rig(true);
	rig.Work();
	const auto receipt = CommitExcavationCut(rig.Request(), rig.participant);
	REQUIRE(receipt.error == ExcavationError::None);
	REQUIRE(receipt.reserve_debits.size() == 1);
	const auto seam = receipt.reserve_debits[0].deposit;
	REQUIRE(CancelExcavationJob(rig.id) == ExcavationError::None);
	const auto tile = TileAddWrap(rig.anchor, 2, 0);
	ResetRoadTypes();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto earth = rig.participant.earth, rock = rig.participant.rock;
	REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, tile, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
	CHECK(GetTileType(tile) == TileType::Road);
	REQUIRE(CmdLandscapeClear(DoCommandFlag::Execute, tile).Succeeded());
	CHECK(GetTileType(tile) == TileType::Clear);
	CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().deposits.Get(seam)->remaining_reserve == 0);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.participant.earth == earth);
	CHECK(rig.participant.rock == rock);
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(CaptureGeologyState(), restored, true) == GeologyError::None);
	CHECK(restored.deposits.Claim(seam, _current_company) == GeologyError::Depleted);
	CHECK(restored.deposits.Get(seam)->remaining_reserve == 0);
}

/** Two actual companies compete for exclusive work; partial progress reload/cancel cannot replay an earlier ore/spoil receipt. */
TEST_CASE("Excavation competing ownership and resumed partial work conserve receipts", "[excavation-resume-conservation]")
{
	CutFixtureRig rig(true);
	auto competing = *GetExcavationJob(rig.id);
	competing.id = ExcavationJobID{UINT32_MAX};
	competing.state = ExcavationJobState::Proposed;
	competing.revision = 0;
	competing.pass_terms.reset();
	REQUIRE(Company::CanAllocateItem());
	std::unique_ptr<Company> rival{Company::Create()};
	const auto owner = _current_company;
	competing.company = rival->index;
	_current_company = rival->index;
	const auto rival_result = ReserveExcavationJob(competing, true);
	_current_company = owner;
	CHECK(rival_result.error == ExcavationError::Conflict);
	CHECK(GetGeologyWorldState().jobs.size() == 1);
	rig.Work();
	const auto first_request = rig.Request();
	const auto receipt = CommitExcavationCut(first_request, rig.participant);
	REQUIRE(receipt.error == ExcavationError::None);
	REQUIRE(receipt.reserve_debits.size() == 1);
	const auto seam = receipt.reserve_debits[0].deposit;
	REQUIRE(AdvanceExcavationWork(rig.id, 23, rig.participant) == ExcavationError::None);
	const auto snapshot = CaptureGeologyState();
	const auto digest = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(owner)->money;
	const auto earth = rig.participant.earth, rock = rig.participant.rock;
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(snapshot, restored, true) == GeologyError::None);
	StageLoadedGeologyState(std::move(restored));
	AcceptLoadedGeologyState();
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == digest);
	CHECK(GetExcavationJob(rig.id)->cursor == 1);
	CHECK(GetExcavationJob(rig.id)->partial_work_ticks == 23);
	CHECK(GetExcavationJob(rig.id)->pass_terms->fingerprint == snapshot.jobs[0].pass_terms->fingerprint);
	CHECK(CommitExcavationCut(first_request, rig.participant).error == ExcavationError::StaleParticipant);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == digest);
	REQUIRE(CancelExcavationJob(rig.id) == ExcavationError::None);
	CHECK(GetGeologyWorldState().job_claims.empty());
	CHECK(GetGeologyWorldState().deposits.Get(seam)->remaining_reserve == 0);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(rig.anchor, 2, 0)) == ExcavationState::Excavated);
	CHECK(GetGeologyWorldState().terrain.Get(TileAddWrap(rig.anchor, 3, 0)) == ExcavationState::Solid);
	CHECK(rig.participant.earth == earth);
	CHECK(rig.participant.rock == rock);
	CHECK(Company::Get(owner)->money == funds);
	REQUIRE(PrepareGeologyRestoration(CaptureGeologyState(), restored, true) == GeologyError::None);
	CHECK(restored.jobs[0].state == ExcavationJobState::Cancelled);
	CHECK(restored.deposits.Get(seam)->remaining_reserve == 0);
	CHECK(restored.jobs[0].completed_earth_t == earth);
}
