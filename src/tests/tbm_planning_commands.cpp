/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_planning_commands.cpp Real project planning command identity, quote and atomicity boundaries. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_commission_fixture.h"
#include "../tbm_replanning.h"
#include "../tbm_area_preview.h"
#include "../command_func.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** A normally commissioned second project with real receivers, plus two additional straight advances to request. */
class PlanningCommandRig : public TBMCommissionRig {
public:
	TBMProjectID id{UINT32_MAX}; ///< Actual controller allocated by the existing commissioning command.
	TileIndex more_start = INVALID_TILE; ///< Additional floor starts at the accepted endpoint's next cell.
	TileIndex more_end = INVALID_TILE; ///< New final floor beyond the old cutter's 3x3 coverage.

	/** Commission through the existing owner without moving a train or editing its work phase. */
	PlanningCommandRig()
	{
		REQUIRE(this->Commission(DoCommandFlag::Execute).Succeeded());
		this->id = CaptureTBMProjects().back().id;
		const auto *job = GetExcavationJob(GetTBMProject(this->id)->job);
		REQUIRE(job != nullptr);
		const auto &last = job->route.back();
		const auto delta = TileIndexDiffCByDir(last.heading);
		this->more_start = TileAddWrap(last.cutter_position, delta.x, delta.y);
		this->more_end = TileAddWrap(last.cutter_position, 3 * delta.x, 3 * delta.y);
	}

	/** Pause only through the same typed command used by the real browser. */
	void Pause()
	{
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, this->id, TBMProjectAction::Pause).Succeeded());
	}
};

/** A reviewed extension retains the same actual machine, packets, project and geological identity through command dispatch. */
TEST_CASE("Paused TBM extension command preserves identity and refuses duplicate confirmation", "[tbm-planning-command]")
{
	PlanningCommandRig rig;
	const auto running = rig.rig.Digest();
	CHECK(PreviewTBMExtension(_current_company, rig.id, rig.more_start, rig.more_end).result.GetErrorMessage() == STR_ERROR_TBM_EXTEND_PAUSE);
	CHECK(rig.rig.Digest() == running);
	rig.Pause();
	const auto project = *GetTBMProject(rig.id);
	const auto job = *GetExcavationJob(project.job);
	const auto before = rig.rig.Digest();
	const auto projects = ComputeTBMProjectDigest();
	const auto quote = PreviewTBMExtension(_current_company, rig.id, rig.more_start, rig.more_end);
	INFO("extension refusal=" << quote.result.GetErrorMessage().base() << " geometry=" << uint{to_underlying(quote.plan.error)});
	REQUIRE(quote.result.Succeeded());
	CHECK(quote.cutter == project.cutter);
	CHECK(quote.job_revision == job.revision);
	CHECK(quote.result.GetCost() == 0);
	CHECK(Command<Commands::ExtendTBMProject>::Do({}, rig.id, rig.more_start, rig.more_end, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	CHECK(rig.rig.Digest() == before);
	CHECK(ComputeTBMProjectDigest() == projects);
	REQUIRE(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, rig.id, rig.more_start, rig.more_end, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto extended = *GetTBMProject(rig.id);
	CHECK(extended.cutter == project.cutter);
	CHECK(extended.job == project.job);
	CHECK(extended.permanent_parts == project.permanent_parts);
	CHECK(extended.phase == project.phase);
	CHECK(extended.blocked == TBMBlockedReason::Paused);
	CHECK(extended.revision == project.revision + 1);
	CHECK(GetExcavationJob(project.job)->route.size() == job.route.size() + quote.plan.route.size());
	CHECK(GetExcavationJob(project.job)->cursor == job.cursor);
	CHECK(Vehicle::GetNumItems() == before.vehicles);
	CHECK(Company::Get(_current_company)->money == before.money);
	const auto committed = rig.rig.Digest();
	const auto controllers = ComputeTBMProjectDigest();
	CHECK(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, rig.id, rig.more_start, rig.more_end, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Failed());
	CHECK(rig.rig.Digest() == committed);
	CHECK(ComputeTBMProjectDigest() == controllers);
}

/** Reviewed geometry and caller identity remain authoritative even when a valid project still exists. */
TEST_CASE("TBM work quote refuses changed fingerprint foreign actor and wrong layer atomically", "[tbm-planning-command]")
{
	PlanningCommandRig rig;
	rig.Pause();
	const auto quote = PreviewTBMExtension(_current_company, rig.id, rig.more_start, rig.more_end);
	REQUIRE(quote.result.Succeeded());
	const auto before = rig.rig.Digest();
	const auto projects = ComputeTBMProjectDigest();
	CHECK(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, rig.id, rig.more_start, rig.more_end,
		quote.job_revision, quote.fingerprint ^ uint64_t{1}, ExcavationPlanIntent::Area).GetErrorMessage() == STR_ERROR_TBM_EXTEND_STALE);
	CHECK(PreviewTBMExtension(_current_company, rig.id, MultiLayerMap::TileInLayer(rig.more_start, MapLayer::Surface), rig.more_end).result.Failed());
	CHECK(PreviewTBMExtension(CompanyID{1}, rig.id, rig.more_start, rig.more_end).result.Failed());
	CHECK(rig.rig.Digest() == before);
	CHECK(ComputeTBMProjectDigest() == projects);
}

/** A reviewed new-project quote cannot be replaced with a different shape or second purchase at confirmation. */
TEST_CASE("New TBM area confirmation requires its exact reviewed quote", "[tbm-planning-command]")
{
	TBMCommissionRig rig;
	const auto origin = rig.rig.fixture.origin->index;
	const auto quote = PreviewTBMCommissionArea(_current_company, rig.portal, 0, 2, 2, origin, rig.start, rig.end, false);
	REQUIRE(quote.purchase.Succeeded());
	const auto before = rig.rig.Digest();
	CHECK(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, rig.portal, 0, 2, 2, origin,
		rig.start, rig.end, quote.fingerprint ^ uint64_t{1}, ExcavationPlanIntent::Area).GetErrorMessage() == STR_ERROR_TBM_EXTEND_STALE);
	CHECK(rig.rig.Digest() == before);
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, rig.portal, 0, 2, 2, origin,
		rig.start, rig.end, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto committed = rig.rig.Digest();
	CHECK(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, rig.portal, 0, 2, 2, origin,
		rig.start, rig.end, quote.fingerprint, ExcavationPlanIntent::Area).Failed());
	CHECK(rig.rig.Digest() == committed);
}

/** A straight portal request reaches its exact centre endpoint, and changing its reviewed intent cannot buy work. */
TEST_CASE("Straight TBM commissioning binds intent and exact cutter endpoint", "[tbm-planning-intent]")
{
	TBMCommissionRig rig;
	const auto origin = rig.rig.fixture.origin->index;
	const auto before = rig.rig.Digest();
	const auto quote = PreviewTBMCommissionArea(_current_company, rig.portal, 0, 2, 2, origin, rig.rock, rig.end, false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Succeeded());
	REQUIRE(quote.area.plan.route.size() == 3);
	CHECK(quote.area.plan.end.position == rig.end);
	const auto heading = quote.area.plan.route.front().heading;
	// Every centre is a real forward advance on the chosen alignment; complete
	// body/tail clearance and prices still come from the canonical producer.
	for (const auto &cut : quote.area.plan.route) CHECK(cut.heading == heading);
	CHECK(rig.rig.Digest() == before);
	CHECK(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, rig.portal, 0, 2, 2, origin,
		rig.rock, rig.end, quote.fingerprint, ExcavationPlanIntent::Area).Failed());
	CHECK(PreviewTBMCommissionArea(_current_company, rig.portal, 0, 2, 2, origin, rig.rock,
		TileAddWrap(rig.end, 1, 1), false, ExcavationPlanIntent::StraightTunnel).purchase.Failed());
	CHECK(rig.rig.Digest() == before);
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, rig.portal, 0, 2, 2, origin,
		rig.rock, rig.end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const auto project = CaptureTBMProjects().back();
	CHECK(GetExcavationJob(project.job)->route == quote.area.plan.route);
}

/** Straight continuation preserves the old route and refuses sideways/backward requests without area-search detours. */
TEST_CASE("An existing TBM appends only the reviewed straight continuation", "[tbm-planning-intent]")
{
	PlanningCommandRig rig;
	rig.Pause();
	const auto project = *GetTBMProject(rig.id);
	const auto job = *GetExcavationJob(project.job);
	const auto start = job.route.back().cutter_position;
	const auto before = rig.rig.Digest();
	const auto quote = PreviewTBMExtension(_current_company, rig.id, start, rig.more_end, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.plan.route.size() == 3);
	CHECK(quote.plan.end.position == rig.more_end);
	for (const auto &cut : quote.plan.route) CHECK(cut.heading == job.route.back().heading); // No selected straight centre may introduce a coverage turn.
	CHECK(PreviewTBMExtension(_current_company, rig.id, rig.more_start, rig.more_end, ExcavationPlanIntent::StraightTunnel).result.Failed());
	CHECK(PreviewTBMExtension(_current_company, rig.id, start, job.commissioning_anchor, ExcavationPlanIntent::StraightTunnel).result.Failed());
	CHECK(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, rig.id, start, rig.more_end,
		quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Failed());
	CHECK(rig.rig.Digest() == before);
	CHECK(*GetTBMProject(rig.id) == project);
	REQUIRE(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, rig.id, start, rig.more_end,
		quote.job_revision, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const auto *accepted = GetExcavationJob(project.job);
	CHECK(std::equal(job.route.begin(), job.route.end(), accepted->route.begin()));
	CHECK(accepted->route.back().cutter_position == rig.more_end);
	CHECK(GetTBMProject(rig.id)->cutter == project.cutter);
}

/** The advertised straight working ring must include complete conservative diagonal sweeps in both rock layers. */
TEST_CASE("Straight tunnel outlines authorize all eight complete cutter sweeps", "[tbm-planning-intent]")
{
	TBMCommissionRig rig;
	const auto before = rig.rig.Digest();
	// All native headings use the same geometry owner. Checking both planes
	// prevents a diagonal corner or layer offset from escaping paid clearance.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (uint8_t value = 0; value < 8; ++value) {
			const Direction heading = static_cast<Direction>(value);
			const auto delta = TileIndexDiffCByDir(heading);
			const TileIndex start = TileXYL(30, 30, layer);
			const TileIndex end = TileAddWrap(start, 3 * delta.x, 3 * delta.y);
			const auto outline = MakeTBMTunnelOutline(start, heading, end);
			REQUIRE(outline.error == ExcavationError::None);
			REQUIRE(outline.requested_tiles.size() == 3);
			ExcavationPlanningPose pose{start, heading, std::vector<TileIndex>(8, start)};
			for (uint step_index = 0; step_index < 3; ++step_index) {
				const auto step = AdvanceExcavationPlanningPose(pose, heading);
				REQUIRE(step.error == ExcavationError::None);
				for (TileIndex tile : step.cut.swept_tiles) CHECK(std::ranges::binary_search(outline.authorized_tiles, tile));
				pose = step.next;
			}
			CHECK(pose.position == end);
		}
	}
	CHECK(rig.rig.Digest() == before);
}
