/**
 * @file ground_processor_selection.cpp Current command-funded processor owners select exact native source cells and states.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Construction, incoming stocks and corrupt/full boundary slots are explicitly
 * staged native fixtures; no earned transport or actual viewport is claimed.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../ground_processor_sprite.h"
#include "../layer_processor_presentation.h"
#include "processor_fixture.h"
#include "../core/backup_type.hpp"
#include <array>

#include "ground_render_camera_fixture.h"
#include "../safeguards.h"

/** Actual native command creates incomplete sixteen-cell machinery for source construction checks. */
class GroundProcessorConstructionRig : public MineAdmissionRig {
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Restore caller native TIDS identities.

public:
	Industry *industry = nullptr; ///< Borrowed actual command-created facility, destroyed by parent world teardown.

	/** Fund the real current facility without completing machinery or fabricating a processing inventory.
	 * @param primary Existing native primary recipe identifying one selected physical Surface family.
	 */
	explicit GroundProcessorConstructionRig(MultiLayerRecipeID primary) : MineAdmissionRig()
	{
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		this->PrepareFunding();
		const auto type = GetLayerProcessorIndustryType(primary);
		REQUIRE(type != IT_INVALID);
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, TileXYL(30, 30, MapLayer::Surface), type, 0, true, 123).Succeeded());
		this->industry = *Industry::Iterate().begin();
		REQUIRE(this->industry->location.w == 4);
		REQUIRE(this->industry->location.h == 4);
		_game_mode = GameMode::Normal;
		_pause_mode = {};
	}

	/** Restore native tile-definition mappings before the existing fixture teardown. */
	~GroundProcessorConstructionRig() { _industile_mngr.mappings = std::move(this->tile_mappings); }
};

/** Four actual native definitions retain all sixteen source cells, both parts and complete synchronized working phases. */
TEST_CASE("Selected processor sprites follow all native source cells and input availability without mutation", "[ground-processor-selection]")
{
	const size_t selected = GENERATE(0, 1, 2, 3);
	static constexpr std::array primaries{MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"}, MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}};
	static constexpr std::array families{GroundSpriteFamily::FacilityMetalRefinery, GroundSpriteFamily::FacilityChemicalWorks,
		GroundSpriteFamily::FacilityBatteryWorks, GroundSpriteFamily::FacilityElectronicsWorks};
	ProcessorRig rig(primaries[selected]);
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	REQUIRE(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::Idle);
	CHECK(GetGroundProcessorSprite(rig.industry->location.tile, false, 24) == GetGroundSprite(families[selected], {1, 0}));
	// Deliver each actual recipe role through its sole incoming-stock owner.
	// Source selection then observes potential native work without earning ticks
	// or consuming even one role, independently of the initial empty work bank.
	for (const auto &role : record.recipe.inputs) {
		REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	}
	REQUIRE(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::Working);
	REQUIRE(GetLayerProcessorState(rig.industry->index)->work_ticks == 0);
	const auto state = *GetLayerProcessorState(rig.industry->index);
	const auto digest = ComputeLayerProcessorDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random, interactive = _interactive_random;
	uint16_t cell = 0;
	// Definition order and physical cells come from the independent current
	// funded-layout producer. Exhaust every real cell/part/phase, including the
	// actual industry renderer's existing public dispatch rather than an unused helper.
	for (const auto &piece : GetIndustrySpec(rig.industry->type)->layouts.front()) {
		const auto tile = TileAddWrap(rig.industry->location.tile, piece.ti.x, piece.ti.y);
		for (uint8_t phase = 0; phase < 4; ++phase) {
			for (uint8_t part = 0; part < 2; ++part) {
				const auto expected = GetGroundSprite(families[selected], {2, phase, 0, cell, part});
				CHECK(GetGroundProcessorSprite(tile, part != 0, phase * 8) == expected);
				CHECK(GetMultiLayerIndustrySprite(tile, part != 0, phase * 8) == expected);
			}
		}
		++cell;
	}
	CHECK(cell == 16);
	CHECK(*GetLayerProcessorState(rig.industry->index) == state);
	CHECK(ComputeLayerProcessorDigest() == digest);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Real construction controls all source stages and waits for every physical machinery cell before ready state. */
TEST_CASE("Selected processor sprites preserve local construction and global machinery readiness", "[ground-processor-selection]")
{
	GroundProcessorConstructionRig rig(MultiLayerRecipeID{"CFMT"});
	const auto tile = rig.industry->location.tile;
	// Time/view changes cannot progress paid construction. Each authentic
	// native stage chooses a fixed original source model in both parents.
	for (uint8_t stage = 0; stage < 4; ++stage) {
		SetIndustryConstructionStage(tile, stage);
		CHECK(GetGroundProcessorSprite(tile, false, 127) == GetGroundSprite(GroundSpriteFamily::FacilityMetalRefinery, {0, stage, 0, 0, 0}));
		CHECK(GetGroundProcessorSprite(tile, true, 8) == GetGroundSprite(GroundSpriteFamily::FacilityMetalRefinery, {0, stage, 0, 0, 1}));
	}
	SetIndustryCompleted(tile);
	REQUIRE(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::Construction);
	CHECK(GetGroundProcessorSprite(tile, false, 16) == GetGroundSprite(GroundSpriteFamily::FacilityMetalRefinery, {0, 3}));
	// Completion belongs only to this fixture's actual machinery. It changes
	// readiness, while absent native inputs keep the final plant idle/ready.
	for (TileIndex cell : rig.industry->location) SetIndustryCompleted(cell);
	CHECK(GetGroundProcessorSprite(tile, false, 24) == GetGroundSprite(GroundSpriteFamily::FacilityMetalRefinery, {1, 0}));
}

/** Banked work and incomplete input sets cannot animate completed battery machinery or silently commit its first batch. */
TEST_CASE("Selected processor sprites require every actual input independently of banked work", "[ground-processor-selection]")
{
	ProcessorRig rig(MultiLayerRecipeID{"BATT"});
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	const auto tile = rig.industry->location.tile;
	AdvanceLayerProcessor(*rig.industry, record.recipe.work_economy_ticks);
	REQUIRE(GetLayerProcessorState(rig.industry->index)->work_ticks > 0);
	CHECK(GetGroundProcessorSprite(tile, false, 8) == GetGroundSprite(GroundSpriteFamily::FacilityBatteryWorks, {1, 0}));
	REQUIRE(record.recipe.inputs.size() > 1);
	const auto &first = record.recipe.inputs.front();
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(first.label), first.quantity) == first.quantity);
	CHECK(GetGroundProcessorSprite(tile, false, 8) == GetGroundSprite(GroundSpriteFamily::FacilityBatteryWorks, {1, 0}));
	// Supply remaining actual roles through their native input owner. A new
	// working presentation is not an atomic production commit or stock debit.
	for (size_t index = 1; index < record.recipe.inputs.size(); ++index) {
		const auto &role = record.recipe.inputs[index];
		REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	}
	const auto before = *GetLayerProcessorState(rig.industry->index);
	CHECK(GetGroundProcessorSprite(tile, false, 8) == GetGroundSprite(GroundSpriteFamily::FacilityBatteryWorks, {2, 1}));
	CHECK(*GetLayerProcessorState(rig.industry->index) == before);
	CHECK(before.batches.at(record.recipe.id) == 0);
	CHECK(rig.industry->produced.front().waiting == 0);
}

/** Real output/history capacity and intentionally invalid native slots retain blocked housing without repairing stock. */
TEST_CASE("Selected processor sprites respect actual output history and invalid native inventory boundaries", "[ground-processor-selection]")
{
	ProcessorRig rig;
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	const auto tile = rig.industry->location.tile;
	for (const auto &role : record.recipe.inputs) {
		REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	}
	auto &output = rig.Output(record.recipe.outputs[0].label);
	const auto blocked = GetGroundSprite(GroundSpriteFamily::FacilityMetalRefinery, {3, 0});
	// These are explicit native boundary fixtures, not earned cargo. The
	// source query may neither lose staged output nor invent production room.
	output.waiting = record.output_capacity;
	const auto digest = ComputeLayerProcessorDigest();
	CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
	CHECK(ComputeLayerProcessorDigest() == digest);
	output.waiting = 0;
	output.history[THIS_MONTH].production = UINT16_MAX;
	CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
	CHECK(output.history[THIS_MONTH].production == UINT16_MAX);
	output.history[THIS_MONTH].production = 0;
	{
		AutoRestoreBackup invalid(rig.industry->accepted.front().cargo, INVALID_CARGO);
		REQUIRE(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::InvalidState);
		CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
		CHECK(rig.industry->accepted.front().cargo == INVALID_CARGO);
	}
}

/** Native pause, closure and the actual recipe calendar hold the same physical installed source without enabling work. */
TEST_CASE("Selected processor sprites retain native pause closure and date guards", "[ground-processor-selection]")
{
	ProcessorRig rig(MultiLayerRecipeID{"BATT"});
	const auto tile = rig.industry->location.tile;
	const auto blocked = GetGroundSprite(GroundSpriteFamily::FacilityBatteryWorks, {3, 0});
	_pause_mode.Set(PauseMode::Normal);
	CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
	_pause_mode = {};
	rig.industry->prod_level = PRODLEVEL_CLOSURE;
	CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
	rig.industry->prod_level = PRODLEVEL_DEFAULT;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	REQUIRE(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::Unavailable);
	CHECK(GetGroundProcessorSprite(tile, false, 24) == blocked);
}

/** Invalid/unbound geometry and Classic keep their native fallback rather than borrowing this source family. */
TEST_CASE("Selected processor sprites refuse invalid Classic unbound and unsupported physical owners", "[ground-processor-selection]")
{
	ProcessorRig rig;
	const auto tile = rig.industry->location.tile;
	CHECK_FALSE(GetGroundProcessorSprite(INVALID_TILE, false, 0));
	CHECK_FALSE(GetGroundProcessorSprite(TileAddWrap(tile, 4, 0), false, 0));
	{
		AutoRestoreBackup unsupported(rig.industry->location.w, uint16_t{3});
		CHECK_FALSE(GetGroundProcessorSprite(tile, false, 0));
	}
	const auto policy = GetCurrentMultiLayerProfileSelection();
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetGroundProcessorSprite(tile, false, 0));
	REQUIRE(RestoreMultiLayerProfileSelection(policy, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	ReleaseLayerProcessorIndustry(rig.industry->index);
	CHECK_FALSE(GetGroundProcessorSprite(tile, false, 0));
}

/** Actual main/extra cameras and hidden-building presentation preserve the physical source owner and its live state. */
TEST_CASE("Merged processor selection retains each actual campus across cameras", "[merged-ground-camera]")
{
	PrepareGroundRenderCameraLanguage();
	const auto primary = GENERATE(MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"}, MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}); ProcessorRig rig(primary);
	/** @return Actual public consumer identities at the unchanged visual phase; no raster or earned work is claimed. */
	const auto query = [&] { const auto tile = rig.industry->location.tile; return std::vector<SpriteID>{GetMultiLayerIndustrySprite(tile, false, 24).value(), GetMultiLayerIndustrySprite(tile, true, 24).value()}; };
	CheckGroundRenderCameraPurity(query);
}
