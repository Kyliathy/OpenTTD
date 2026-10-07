/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file depot_file_fixture.h Immutable actual depot evidence and ordinary pre-assignment depot withdrawal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13C
 */

#ifndef TESTS_DEPOT_FILE_FIXTURE_H
#define TESTS_DEPOT_FILE_FIXTURE_H

#include "underground_depot_fixture.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>

/** Write immutable native saves after public owners report an actually reached depot scenario. */
class DepotFileRecorder {
	const char *prefix = std::getenv("OTTD_TBM_LOGISTICS_RECORD"); ///< Optional fresh basename; normal tests never overwrite inputs.
	std::filesystem::path directory = std::filesystem::current_path().parent_path() / "src/tests/fixtures/haulage"; ///< Existing owning feature directory.
public:
	/** Validate the optional recorder's repository-local immutable input namespace. */
	DepotFileRecorder()
	{
		REQUIRE(std::filesystem::is_directory(this->directory));
		if (this->prefix != nullptr) {
			REQUIRE(std::filesystem::path(this->prefix).filename() == std::filesystem::path(this->prefix));
			REQUIRE_FALSE(std::string_view(this->prefix).starts_with("r2"));
		}
	}

	/**
	 * Save the actual whole world without replacing cargo, physical parts, invoices or controller state.
	 * @param state First genuinely observed state name, appended to the optional fresh basename.
	 */
	void Save(std::string_view state) const
	{
		if (this->prefix == nullptr) return;
		const auto path = this->directory / (std::string(this->prefix) + "-" + std::string(state) + ".sav");
		REQUIRE_FALSE(std::filesystem::exists(path));
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto projects = ComputeTBMProjectDigest();
		const auto exchanges = ComputeTBMExchangeDigest();
		const auto receipts = ComputeSpoilReceiverDigest();
		const Money cash = Company::Get(_current_company)->money;
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(ComputeTBMProjectDigest() == projects);
		CHECK(ComputeTBMExchangeDigest() == exchanges);
		CHECK(ComputeSpoilReceiverDigest() == receipts);
		CHECK(Company::Get(_current_company)->money == cash);
	}
};

/**
 * Return an unassigned, physically empty hauler to a genuine ordinary depot before changing its declared capacity.
 * @param workflow Real paid cleared world; the first project/assignment has already retired and its cutter remains stopped.
 */
static void WithdrawEmptyDepotHauler(UndergroundDepotWorkflow &workflow)
{
	REQUIRE(GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index) == nullptr);
	workflow.Do<Commands::SendVehicleToDepot>(workflow.world.hauler->index, DepotCommandFlags{}, VehicleListIdentifier{});
	workflow.Do<Commands::StartStopVehicle>(workflow.world.hauler->index, true);
	uint frames = 0;
	// Native depot orders and ordinary physical movement withdraw the complete empty chain. No pose/order completion is manufactured.
	while (!workflow.world.hauler->IsStoppedInDepot() && frames < 12000) {
		workflow.world.Tick();
		REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
		++frames;
	}
	INFO("withdrawn hauler=" << DescribeTrain(workflow.world.hauler));
	REQUIRE(workflow.world.hauler->IsStoppedInDepot());
	for (const Train *part = workflow.world.hauler; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0); // Physical withdrawal retains every real empty part.
}

#endif /* TESTS_DEPOT_FILE_FIXTURE_H */
