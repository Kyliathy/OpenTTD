/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file movement_fixture.h Shared helpers for tests in which real trains drive themselves: the pathfinder settings at
 * their defaults, a train built in a depot through the real owners, a readable description of a train for failed
 * drives, and the check that every part stands on the tile its position names.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-m-movement-design.md
 */

#ifndef TESTS_MOVEMENT_FIXTURE_H
#define TESTS_MOVEMENT_FIXTURE_H

#include "../settings_internal.h"
#include "../settings_type.h"
#include "../train.h"
#include "../map_func.h"
#include "../core/format.hpp"
#include "../train_cmd.h"
#include "../command_func.h"
#include "../vehicle_func.h"
#include "../engine_base.h"
#include "../company_base.h"
#include "../group.h"

/**
 * The pathfinder settings at their real defaults for one test. Unit tests otherwise start with every game setting at
 * zero, and a moving train divides by the path back-off interval and needs YAPF's search node budget.
 */
class PathfinderDefaults {
	PathfinderSettings saved = _settings_game.pf; ///< The caller's values, restored at the end.
public:
	/** Reset every saved "pf." setting to its default, as a new game has it. */
	PathfinderDefaults()
	{
		// Only real pathfinder settings are reset; every other game setting keeps the fixture's value.
		for (const auto &desc : GetSaveLoadSettingTable()) {
			const SettingDesc *sd = GetSettingDesc(desc);
			if (!sd->flags.Test(SettingFlag::NotInSave) && sd->GetName().starts_with("pf.")) sd->ResetToDefault(&_settings_game);
		}
	}

	/** Restore the caller's pathfinder settings. */
	~PathfinderDefaults() { _settings_game.pf = this->saved; }
};

/**
 * Build a locomotive and wagons in a depot through the real construction and coupling owners, with the unit number and
 * statistics bookkeeping of CmdBuildVehicle.
 * @param depot Depot to build in.
 * @param locomotive Engine of the front.
 * @param wagon Engine of every wagon.
 * @param wagons Number of wagons behind the locomotive.
 * @return The stopped front engine.
 */
inline Train *BuildTrainInDepot(TileIndex depot, const Engine *locomotive, const Engine *wagon, uint wagons)
{
	Vehicle *built = nullptr;
	REQUIRE(Vehicle::CanAllocateItem(1));
	REQUIRE(CmdBuildRailVehicle(DoCommandFlag::Execute, depot, locomotive, &built).Succeeded());
	Train *head = Train::From(built);
	head->unitnumber = Company::Get(_current_company)->freeunits[VehicleType::Train].UseID(GetFreeUnitNumber(VehicleType::Train));
	GroupStatistics::CountEngine(head, 1);
	GroupStatistics::CountVehicle(head, 1);
	// Each wagon is built free in the depot and coupled at the end of the train through the ordinary move.
	for (uint index = 0; index < wagons; ++index) {
		built = nullptr;
		REQUIRE(Vehicle::CanAllocateItem(1));
		REQUIRE(CmdBuildRailVehicle(DoCommandFlag::Execute, depot, wagon, &built).Succeeded());
		GroupStatistics::CountEngine(built, 1);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, built->index, head->Last()->index, false).Succeeded());
	}
	REQUIRE(head->IsStoppedInDepot());
	return head;
}

/**
 * @param train Front engine.
 * @return Its speed and facing, then every part's tile, track, position, direction and visibility, for a failed drive.
 */
inline std::string DescribeTrain(const Train *train)
{
	std::string text = fmt::format("speed {}{}", train->cur_speed, train->vehicle_flags.Test(VehicleFlag::DrivingBackwards) ? " backwards" : "");
	text += train->vehstatus.Test(VehState::Stopped) ? " stopped" : "";
	// One line per part, so a drive that stops short shows where the whole train stands.
	for (const Train *part = train; part != nullptr; part = part->Next()) {
		text += fmt::format("\n  tile {} track {} at {},{},{} dir {}{}", part->tile.base(), part->track.base(), part->x_pos, part->y_pos, part->z_pos,
				to_underlying(part->direction), part->vehstatus.Test(VehState::Hidden) ? " hidden" : "");
	}
	return text;
}

/**
 * @param train Front engine.
 * @return Whether every part on track stands on the tile its position lies in. A part inside a depot or a portal
 *         magazine keeps the depot state's tile, and a part in a tunnel or on a bridge is in a wormhole.
 */
inline bool IsTrainOnItsTiles(const Train *train)
{
	for (const Train *part = train; part != nullptr; part = part->Next()) {
		if (part->track == Track::Depot || part->track == Track::Wormhole) continue;
		if (TileVirtXY(part->x_pos, part->y_pos) != part->tile) return false;
	}
	return true;
}

#endif /* TESTS_MOVEMENT_FIXTURE_H */
