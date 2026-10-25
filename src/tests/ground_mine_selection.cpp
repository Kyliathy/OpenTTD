/**
 * @file ground_mine_selection.cpp Actual funded nine-cell mine owners select qualified original source parts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Floor and construction completion are explicit fixture staging. These cases
 * qualify native owner/renderer selection, never transported first-plant bootstrap.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../ground_mine_sprite.h"
#include "mine_fixture.h"
#include "../core/backup_type.hpp"
#include <array>

#include "ground_render_camera_fixture.h"
#include "../safeguards.h"

/** Real command-body fixture installs the current native definition and funds its actual nine machinery cells. */
class GroundMineRig : public MineAdmissionRig {
	decltype(_industile_mngr.mappings) tile_mappings = _industile_mngr.mappings; ///< Preserve caller TIDS ownership.
	AutoRestoreBackup<decltype(_economy.fluct)> fluct; ///< Keep native production outside recession for explicit status fixtures.
	AutoRestoreBackup<decltype(_settings_game.economy.industry_cargo_scale)> scale; ///< Preserve caller production scale.

public:
	Industry *industry = nullptr; ///< Borrowed actual funded industry; parent teardown owns deletion.

	/** Install current native geometry and fund it through the existing authoritative command body.
	 * @param index Existing public mine recipe index, including genuinely unselected refusal fixtures.
	 * @param completed Explicit fixture completion of all real machinery, independent of art or gameplay permission.
	 */
	explicit GroundMineRig(size_t index = 0, bool completed = true) : MineAdmissionRig(index), fluct(_economy.fluct, 1),
		scale(_settings_game.economy.industry_cargo_scale, uint16_t{100})
	{
		REQUIRE(InitialiseLayerMineDefinitions(this->ActivatedContent()));
		this->request.type = GetLayerMineIndustryType(GetMultiLayerMineRecipes()[index].resource);
		REQUIRE(this->request.type != IT_INVALID);
		std::vector<TileIndex> missing;
		// This disclosed fixture floor is a three-wide/five-long rectangle.
		// Actual admission independently requires nine machinery and four loading
		// cells; opening extra ordinary floor supplies no production or art authority.
		for (int y = 0; y < 5; ++y) {
			for (int x = 0; x < 3; ++x) {
				const auto tile = TileAddWrap(this->request.tile, x, y);
				REQUIRE(IsValidTile(tile));
				if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) missing.push_back(tile);
			}
		}
		if (!missing.empty()) this->Open(missing);
		this->PrepareFunding();
		const auto admission = ValidateMineDepositClaim(this->request);
		REQUIRE(admission.error == LayerIndustryError::None);
		REQUIRE(admission.machinery.size() == 9);
		REQUIRE(admission.loading.size() == 4);
		REQUIRE(this->Fund(DoCommandFlag::Execute).Succeeded());
		this->industry = *Industry::Iterate().begin();
		REQUIRE(this->industry->location.w == 3);
		REQUIRE(this->industry->location.h == 3);
		if (completed) this->Complete();
		_game_mode = GameMode::Normal;
		_pause_mode = {};
	}

	/** Restore actual tile-definition mappings before the existing shared world teardown. */
	~GroundMineRig() { _industile_mngr.mappings = std::move(this->tile_mappings); }

	/** Complete only owned native machinery through its actual map accessors; loading floor remains ordinary floor. */
	void Complete()
	{
		// Actual industry ownership determines completion; the fixture floor
		// rectangle never creates additional machinery or a second construction owner.
		for (TileIndex tile : this->industry->location) {
			if (this->industry->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
	}

	/** Restore the selected finite seam to an explicitly depleted fixture state through validated geology loading. */
	void DepleteForFixture()
	{
		auto snapshot = CaptureGeologyState();
		const auto found = std::ranges::find(snapshot.deposits, this->request.deposit, &GeologyDeposit::id);
		REQUIRE(found != snapshot.deposits.end());
		found->remaining_reserve = 0;
		this->RestoreGeology(snapshot);
	}
};

/** The real current seven-resource catalogue selects its exact nine cells and synchronized source parts without mutation. */
TEST_CASE("Selected mine sprites follow current funded native cells and pure synchronized phases", "[ground-mine-selection]")
{
	const size_t selected = GENERATE(0, 1, 2, 3, 4, 5, 6);
	static constexpr std::array<size_t, 7> recipes{0, 1, 2, 4, 5, 7, 8};
	static constexpr std::array families{GroundSpriteFamily::MineCOAL, GroundSpriteFamily::MineIORE, GroundSpriteFamily::MineCORE,
		GroundSpriteFamily::MineSALT, GroundSpriteFamily::MineLITH, GroundSpriteFamily::MineCOBT, GroundSpriteFamily::MineRARE};
	GroundMineRig rig(recipes[selected]);
	REQUIRE(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Operating);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto mine = ComputeLayerMineDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random, interactive = _interactive_random;
	const auto waiting = rig.industry->produced.front().waiting;
	// Native definition order/coordinates are independently qualified by the
	// paid-layout producer. Every source cell, part and phase must agree with
	// that actual physical layout, independent of the selector's coordinate formula.
	uint16_t cell = 0;
	for (const auto &piece : GetIndustrySpec(rig.industry->type)->layouts.front()) {
		const auto tile = TileAddWrap(rig.industry->location.tile, piece.ti.x, piece.ti.y);
		for (uint8_t phase = 0; phase < 4; ++phase) {
			for (uint8_t part = 0; part < 2; ++part) {
				const auto expected = GetGroundSprite(families[selected], {1, phase, 0, cell, part});
				CHECK(GetGroundMineSprite(tile, part != 0, phase * 8) == expected);
				CHECK(GetMultiLayerIndustrySprite(tile, part != 0, phase * 8) == expected);
			}
		}
		++cell;
	}
	CHECK(cell == 9);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerMineDigest() == mine);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(rig.industry->produced.front().waiting == waiting);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Local construction stages remain static until the entire actual machinery owner completes. */
TEST_CASE("Selected mine sprites retain per-cell construction and the whole mine completion gate", "[ground-mine-selection]")
{
	GroundMineRig rig(0, false);
	const auto tile = rig.industry->location.tile;
	// A camera clock never advances physical construction. All four native
	// stages, including a completed cell waiting for another cell, select the
	// same fixed stage pixels in both aligned source parents.
	for (uint8_t stage = 0; stage < 4; ++stage) {
		SetIndustryConstructionStage(tile, stage);
		CHECK(GetGroundMineSprite(tile, false, 127) == GetGroundSprite(GroundSpriteFamily::MineCOAL, {0, stage, 0, 0, 0}));
		CHECK(GetGroundMineSprite(tile, true, 8) == GetGroundSprite(GroundSpriteFamily::MineCOAL, {0, stage, 0, 0, 1}));
	}
	SetIndustryCompleted(tile);
	REQUIRE(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Construction);
	CHECK(GetGroundMineSprite(tile, false, 16) == GetGroundSprite(GroundSpriteFamily::MineCOAL, {0, 3}));
	rig.Complete();
	CHECK(GetGroundMineSprite(tile, false, 16) == GetGroundSprite(GroundSpriteFamily::MineCOAL, {1, 2}));
}

/** Pause/closure, native full output and finite depletion retain installed source geometry without earning work. */
TEST_CASE("Selected mine sprites follow blocked output and depleted native reserve", "[ground-mine-selection]")
{
	GroundMineRig rig;
	const auto tile = rig.industry->location.tile;
	const auto blocked = GetGroundSprite(GroundSpriteFamily::MineCOAL, {2, 0});
	_pause_mode.Set(PauseMode::Normal);
	CHECK(GetGroundMineSprite(tile, false, 24) == blocked);
	_pause_mode = {};
	rig.industry->prod_level = PRODLEVEL_CLOSURE;
	CHECK(GetGroundMineSprite(tile, false, 24) == blocked);
	rig.industry->prod_level = PRODLEVEL_DEFAULT;
	const auto *capability = GetLayerIndustryCapability(rig.industry->type);
	REQUIRE(capability != nullptr);
	// Fill through actual finite extraction and native work. The source test
	// never invents historical output or makes drawing debit/credit cargo.
	while (rig.industry->produced.front().waiting < capability->output_capacity) {
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount > 0);
	}
	REQUIRE(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::OutputFull);
	const auto full = rig.industry->produced.front().waiting;
	const auto digest = ComputeLayerMineDigest();
	CHECK(GetGroundMineSprite(tile, false, 24) == blocked);
	CHECK(rig.industry->produced.front().waiting == full);
	CHECK(ComputeLayerMineDigest() == digest);
	rig.DepleteForFixture();
	CHECK(GetGroundMineSprite(tile, false, 24) == GetGroundSprite(GroundSpriteFamily::MineCOAL, {3, 0}));
	CHECK(rig.industry->produced.front().waiting == full);
}

/** An already owned Deep mine keeps its blocked machinery view when its actual calendar owner refuses operation. */
TEST_CASE("Selected mine sprites retain native calendar unavailability without granting operation", "[ground-mine-selection]")
{
	GroundMineRig rig(5);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	REQUIRE(GetLayerMineOperationalStatus(*rig.industry) == LayerMineOperationalStatus::Unavailable);
	CHECK(GetGroundMineSprite(rig.industry->location.tile, false, 24) == GetGroundSprite(GroundSpriteFamily::MineLITH, {2, 0}));
}

/** Invalid/unbound/foreign geometry and Classic retain their previous renderer; source art creates no owner. */
TEST_CASE("Selected mine sprites refuse invalid unselected Classic and detached geometry", "[ground-mine-selection]")
{
	GroundMineRig rig;
	const auto tile = rig.industry->location.tile;
	CHECK_FALSE(GetGroundMineSprite(INVALID_TILE, false, 0));
	CHECK_FALSE(GetGroundMineSprite(TileAddWrap(tile, 0, 3), false, 0));
	{
		AutoRestoreBackup unsupported(rig.industry->location.w, uint16_t{2});
		CHECK_FALSE(GetGroundMineSprite(tile, false, 0));
	}
	const auto policy = GetCurrentMultiLayerProfileSelection();
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetGroundMineSprite(tile, false, 0));
	REQUIRE(RestoreMultiLayerProfileSelection(policy, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	ReleaseLayerMineIndustry(rig.industry->index);
	CHECK_FALSE(GetGroundMineSprite(tile, false, 0));
}

/** A real unselected native mineral retains its existing graphics despite sharing the machinery geometry. */
TEST_CASE("Selected mine sprites do not claim unpublished Bauxite source", "[ground-mine-selection]")
{
	GroundMineRig rig(3);
	CHECK_FALSE(GetGroundMineSprite(rig.industry->location.tile, false, 0));
}

/** Actual main/extra cameras and hidden-building presentation preserve the physical source owner and its live state. */
TEST_CASE("Merged mine selection retains its physical owner across actual cameras", "[merged-ground-camera]")
{
	PrepareGroundRenderCameraLanguage();
	GroundMineRig rig(0);
	/** @return Actual public consumer identities at the unchanged visual phase; no raster or earned work is claimed. */
	const auto query = [&] { const auto tile = rig.industry->location.tile; return std::vector<SpriteID>{GetMultiLayerIndustrySprite(tile, false, 24).value(), GetMultiLayerIndustrySprite(tile, true, 24).value()}; };
	CheckGroundRenderCameraPurity(query);
}
