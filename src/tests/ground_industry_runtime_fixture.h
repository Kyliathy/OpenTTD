/**
 * @file ground_industry_runtime_fixture.h Explicit current industry scene construction inside a real loaded application.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * These command-body fixtures stage floor and capital; they prove native
 * geometry/art, never earned excavation, procurement or transported bootstrap.
 */

#ifndef TESTS_GROUND_INDUSTRY_RUNTIME_FIXTURE_H
#define TESTS_GROUND_INDUSTRY_RUNTIME_FIXTURE_H

#include "../excavation_state.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../map_func.h"
#include "../openttd.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"

/**
 * Open only the disclosed fixture floor through the existing validated geology load boundary.
 * @param tiles Actual complete machinery/access footprint, selected without changing deposit claims or reserves.
 */
static void OpenGroundIndustryFixtureFloor(std::span<const TileIndex> tiles)
{
	auto snapshot = CaptureGeologyState();
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	// This is explicit art-fixture floor, not a second excavation command or
	// free gameplay progress. The original finite deposits/claims remain exact.
	for (const TileIndex tile : tiles) REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
	snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate));
	AcceptLoadedGeologyState();
}

/**
 * Fund one real selected mine through native seam/site admission in the loaded world.
 * @param resource Actual shared native cargo meaning selected for this art scope.
 * @return Command-created actual industry ID, or invalid if no complete unoccupied native site can be admitted.
 */
static IndustryID BuildGroundMineFixture(CargoLabel resource)
{
	const IndustryType type = GetLayerMineIndustryType(resource);
	REQUIRE(type != IT_INVALID);
	const auto deposits = CaptureGeologyState().deposits;
	// Search actual finite native seams and complete machinery/access footprints.
	// No new deposit, quantity, local industry identity or replacement claim is
	// manufactured just to make the selected model drawable.
	for (const auto &deposit : deposits) {
		// Only a live unclaimed seam of the actual requested native resource can
		// authorize this fixture's ordinary mine command.
		if (deposit.resource != resource || deposit.claimant != CompanyID::Invalid() || deposit.remaining_reserve == 0) continue;
		for (const auto &span : deposit.spans) {
			for (uint x = span.first_x; x <= span.last_x; ++x) {
				const auto anchor = TileXYL(x, span.y, deposit.layer);
				std::vector<TileIndex> floor;
				bool clear = true;
				// The current native producer owns nine machinery cells and four
				// loading cells. Expose its complete3x5 enclosing fixture rectangle
				// only where it cannot overwrite any existing facility or map fence.
				for (uint y = 0; y < 5; ++y) {
					for (uint dx = 0; dx < 3; ++dx) {
						const auto tile = TileAddWrap(anchor, dx, y);
						// World wrapping and occupied cells must never relocate source
						// machinery or destroy the retained collector/other facilities.
						if (!IsValidTile(tile) || GetTileLayer(tile) != deposit.layer || TileX(tile) != x + dx ||
							TilePlaneY(tile) != span.y + y || !IsTileType(tile, TileType::Clear)) clear = false;
						floor.push_back(tile);
					}
				}
				if (!clear) continue;
				OpenGroundIndustryFixtureFloor(floor);
				const LayerMineRequest request{anchor, type, 0, deposit.id, _current_company};
				if (ValidateMineDepositClaim(request).error != LayerIndustryError::None) continue;
				const auto cost = CmdBuildLayerMine(DoCommandFlag::Execute, anchor, type, 0, deposit.id, 123);
				if (cost.Failed()) continue;
				REQUIRE(IsTileType(anchor, TileType::Industry));
				return GetIndustryIndex(anchor);
			}
		}
	}
	return IndustryID::Invalid();
}

/**
 * Construct all eleven current selected industries using actual rebuilt native definitions.
 * @return Borrowed command-created industry identities; their sole native world remains the actual application owner.
 */
static std::vector<IndustryID> BuildGroundIndustryFixtures()
{
	std::vector<IndustryID> result;
	// Editor mode suppresses fixture opening news; native company/date/layer/
	// seam/footprint/claim admission and actual definition identities still run.
	// Return to the real normal mode before reading any operating presentation.
	AutoRestoreBackup mode(_game_mode, GameMode::Editor);
	const std::array resources{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"SALT"},
		CargoLabel{"LITH"}, CargoLabel{"COBT"}, CargoLabel{"RARE"}};
	// This is the explicit selected art list, not a new cargo capability table;
	// every actual identity and physical reserve is resolved through its owner.
	for (const auto resource : resources) {
		INFO(std::string(resource.begin(), resource.end()));
		const auto industry = BuildGroundMineFixture(resource);
		REQUIRE(industry != IndustryID::Invalid());
		REQUIRE(Industry::Get(industry)->location.w == 3);
		REQUIRE(Industry::Get(industry)->location.h == 3);
		result.push_back(industry);
	}
	const std::array primaries{MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"}, MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}};
	// Separate Surface sites avoid overlap among the four immutable facilities;
	// native command placement, input unions and recipe state remain authoritative.
	for (size_t index = 0; index < primaries.size(); ++index) {
		const auto type = GetLayerProcessorIndustryType(primaries[index]);
		REQUIRE(type != IT_INVALID);
		const auto tile = TileXYL(8 + static_cast<uint>(index) * 12, 38, MapLayer::Surface);
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, tile, type, 0, true, 123).Succeeded());
		const auto industry = GetIndustryIndex(tile);
		REQUIRE(Industry::Get(industry)->location.w == 4);
		REQUIRE(Industry::Get(industry)->location.h == 4);
		REQUIRE(GetLayerProcessorState(industry) != nullptr);
		result.push_back(industry);
	}
	return result;
}

#endif /* TESTS_GROUND_INDUSTRY_RUNTIME_FIXTURE_H */
