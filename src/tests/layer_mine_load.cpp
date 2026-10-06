/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_mine_load.cpp Exact finite binding restoration and malformed state refusal. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "layer_window_fixture.h"
#include "mock_environment.h"
#include "../saveload/saveload_func.h"
#include "../saveload/saveload_filter.h"
#include "../engine_base.h"
#include "mine_production_fixture.h"
#include "../multilayer_state.h"
#include "../table/sprites.h"

#include "../safeguards.h"


/** Construction, rational work and depletion retain claims/output without re-running funding or reserve generation. */
TEST_CASE("Saved mine bindings restore actual material and work", "[layer-mine-load]")
{
	MineProductionRig rig;
	SECTION("Under construction") {
		ClrBit(Tile{rig.industry->location.tile}.m1(), 7); // Restore the real tile completion flag independently of its animation stage.
		SetIndustryConstructionStage(rig.industry->location.tile, 0);
		CHECK_FALSE(IsLayerIndustryComplete(*rig.industry));
	}
	SECTION("Partially depleted with rational work") {
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 37, true).amount == 2);
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 1, true).amount == 0);
	}
	SECTION("Exhausted with final cargo awaiting pickup") {
		rig.SetReserve(3);
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 3);
		REQUIRE(GetLayerMineState(rig.industry->index)->phase == LayerMinePhase::Depleted);
	}
	const auto saved = CaptureLayerMines();
	const auto digest = ComputeLayerMineDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto output = rig.industry->produced[0].waiting;
	const auto phase = GetLayerMineOperationalStatus(*rig.industry);
	ResetLayerIndustryState();
	StageLoadedLayerMines(saved);
	CHECK(GetLayerMineState(rig.industry->index) == nullptr);
	REQUIRE(AcceptLoadedLayerMines());
	CHECK(CaptureLayerMines() == saved);
	CHECK(ComputeLayerMineDigest() == digest);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.industry->produced[0].waiting == output);
	CHECK(GetLayerMineOperationalStatus(*rig.industry) == phase);
	CHECK(AcceptLoadedLayerMines());
	CHECK(CaptureLayerMines() == saved);
}

/** Restore refuses corrupt references/counters rather than repairing geology or forgetting physical cargo. */
TEST_CASE("Invalid saved mine bindings leave accepted owners unchanged", "[layer-mine-load]")
{
	MineProductionRig rig;
	REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 4);
	const auto original = CaptureLayerMines();
	auto rows = original;
	SECTION("Missing native row") { rows.clear(); }
	SECTION("Duplicate industry or seam") { rows.push_back(rows[0]); }
	SECTION("Missing industry") { rows[0].industry = IndustryID::Invalid(); }
	SECTION("Missing geology") { rows[0].deposit = GeologyDepositID{UINT32_MAX}; }
	SECTION("Wrong company") { rows[0].company = CompanyID::Invalid(); }
	SECTION("Unknown phase") { rows[0].phase = static_cast<LayerMinePhase>(255); }
	SECTION("False depletion") { rows[0].phase = LayerMinePhase::Depleted; }
	SECTION("Forgotten extraction") { rows[0].extracted = 0; }
	SECTION("Invented station supply") { rows[0].dispatched = 5; }
	SECTION("Forgotten revision") { rows[0].revision = 0; }
	SECTION("Unbounded work") { rows[0].work_ticks = 75; }
	SECTION("Unbounded fractional work") { rows[0].work_fraction = 4; }
	SECTION("Unbounded rate remainder") { rows[0].scale_fraction = 200; }
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	StageLoadedLayerMines(std::move(rows));
	CHECK_FALSE(AcceptLoadedLayerMines());
	CHECK(CaptureLayerMines() == original);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(rig.industry->produced[0].waiting == 4);
}

/** Save the actual ordinary world/industry/geology descriptors for independent disk and runtime inspection. */
TEST_CASE("Finite mine state writes its actual savegame descriptor family", "[layer-mine-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	MineProductionRig rig;
	std::string suffix;
	LayerSaveContent content(rig.ActivatedContent());
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START; // Bare naming-town fixtures need the ordinary saved generator identity for full game reload.
	SECTION("Construction") {
		ClrBit(Tile{rig.industry->location.tile}.m1(), 7);
		SetIndustryConstructionStage(rig.industry->location.tile, 0);
		suffix = "construction";
	}
	SECTION("Partial") {
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 37, true).amount == 2);
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 1, true).amount == 0);
		suffix = "partial";
	}
	SECTION("Depleted") {
		rig.SetReserve(3);
		REQUIRE(ProduceLayerMineCargo(*rig.industry, 74, true).amount == 3);
		suffix = "depleted";
	}
	const auto saved = CaptureLayerMines();
	const auto reserve = rig.Reserve();
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); // The real save owner toggles its busy cursor, even with no visible window.
	const auto path = std::filesystem::current_path().parent_path() / "docs/zz-reach2/upgrades/2026-10" / ("r2mlvs6-cr-mine-" + suffix + ".sav");
	REQUIRE(std::filesystem::is_directory(path.parent_path()));
	AutoRestoreBackup<GameMode> no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(std::filesystem::file_size(path) > 8);
	CHECK(CaptureLayerMines() == saved);
	CHECK(rig.Reserve() == reserve);
}
