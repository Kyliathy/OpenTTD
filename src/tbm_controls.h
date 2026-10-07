/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_controls.h What a player can do at a rail portal pair and with its boring project, for the portal window
 * (R2MLVS3MP master 7.9). One query derives the offered controls and the next step from the commands' own checks, so
 * the window never offers a step the commands would refuse.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, tasks 16.1/16.3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, task 13.4
 */

#ifndef TBM_CONTROLS_H
#define TBM_CONTROLS_H

#include "tbm.h"
#include "tbm_exchange.h"

/** The next thing a player should do at a pair, in the order a project's whole life asks for them. */
enum class TBMPlayerStep : uint8_t {
	Commission, ///< Sealed with no project: commission a boring machine to dig the far side.
	StartMachine, ///< The project's machine stands stopped; start it.
	Working, ///< The machine and its hauler get on with the work; nothing to do.
	Resume, ///< The player paused the project.
	AssignHauler, ///< The machine needs empties and no hauler serves the project.
	FixHold, ///< The project holds for a reason only the player can remove, which the window shows.
	DeliverLast, ///< Finished; the hauler still carries the last load to the origin.
	WaitForHauler, ///< Finished; the delivered hauler is still leaving the bay.
	ReleaseHauler, ///< Finished and delivered: release the hauler.
	StopMachine, ///< Finished with no hauler: stop the machine so the project can retire.
	Retire, ///< Finished, stopped and empty in its bay: retire the project.
	SellMachine, ///< Retired: sell the machine still standing in the portal.
	ConvertTrack, ///< Released temporary track waits to become permanent.
	Connect, ///< Both approaches are dug and built: connect the pair for ordinary trains.
	Connected, ///< The pair is open to ordinary trains.
	End, ///< End marker.
};

/** What the portal window shows and offers for one pair; a derived view, never saved. */
struct TBMPairControls {
	TBMPlayerStep step = TBMPlayerStep::Commission; ///< The next thing to do.
	std::optional<TBMProject> project; ///< The company's project working through the pair.
	std::optional<TBMExchangeAssignment> hauler; ///< The project's first hauler, in accepted request order.
	uint32_t cuts_done = 0; ///< Committed cuts of the project's route.
	uint32_t cuts_total = 0; ///< Cuts of the whole route.
	TBMWorkState work; ///< Face, partial work and the single hold reason, projected by the controller.
	std::optional<GeologyHardness> hardness; ///< Actual next-face geology; absent when the route has no face.
	uint64_t face_work_ticks = 0; ///< Pure estimate of the current cut's geological work requirement.
	uint32_t pass_number = 0; ///< One-based next accepted pass, derived from frozen exchange boundaries.
	uint32_t pass_total = 0; ///< Number of accepted passes, never a second saved progress cursor.
	uint32_t pass_cuts_done = 0; ///< Committed cuts within the displayed pass.
	uint32_t pass_cuts_total = 0; ///< Accepted cut count within the displayed pass.
	uint64_t earth_loaded_t = 0; ///< Actual packets aboard attached Earth wagon parts.
	uint64_t earth_capacity_t = 0; ///< Actual capacity of attached Earth wagon parts.
	uint64_t rock_loaded_t = 0; ///< Actual packets aboard attached Rock wagon parts.
	uint64_t rock_capacity_t = 0; ///< Actual capacity of attached Rock wagon parts.
	Money active_cost_per_year = 0; ///< Model's recurring boring rate, separate from work/support charges.
	Money idle_cost_per_year = 0; ///< Model's nonzero stopped/service rate.
	uint64_t earth_t = 0; ///< Earth the project has dug.
	uint64_t rock_t = 0; ///< Rock the project has dug.
	uint8_t earth_hoppers = 0; ///< The cutter's Earth hoppers; a hauler brings as many empties.
	uint8_t rock_hoppers = 0; ///< The cutter's Rock hoppers.
	TBMLogisticsWait wait = TBMLogisticsWait::None; ///< Why the project's spoil logistics wait.
	bool running = false; ///< The project's cutter is started.
	bool pause = false; ///< The project accepts Pause now.
	bool resume = false; ///< The project accepts Resume now.
	bool cancel = false; ///< The project accepts Cancel now.
	bool retire = false; ///< The project accepts Retire now.
	bool assign = false; ///< The project takes a hauler now.
	bool release = false; ///< The hauler can be released without stranding it in the bay.
	VehicleID sellable = VehicleID::Invalid(); ///< An ordinary train standing withdrawn and stopped in the portal.
	std::vector<TileIndex> convertible; ///< Released temporary track of the company, ready for conversion.
	bool connect = false; ///< The pair can be connected for ordinary trains now.
};

/**
 * Collect what a company can do at a pair now and the next step it should take. The checks are the commands' own
 * estimates, run as that company, so every control offered is one the command accepts.
 * @param key Canonical pair key: the upper end, where a project assembles and keeps its bay.
 * @param company Company asking, normally the local company.
 * @return The controls; a pair without a project of the company only offers selling, converting and connecting.
 */
TBMPairControls GetTBMPairControls(TileIndex key, CompanyID company);

#endif /* TBM_CONTROLS_H */
