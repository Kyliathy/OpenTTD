/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_state.h Contained original mining mission discriminators for native and copied transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups17/23.
 */
#ifndef SPACE_MINING_STATE_H
#define SPACE_MINING_STATE_H
#include "space_orders.h"
#include "cargo_type.h"

/** Saved native mission phases; a missing initial target remains Idle until an explicit assignment. */
enum class SpaceMissionPhase : uint8_t { Idle, Approach, Mining, Return, Holding };
/** Explicit continuation progress, independent of the ordinary mission's generic Holding phase. */
enum class MiningContinuationPhase : uint8_t { None, Detaching, SeekMineral, ReturnCargo, NeedService, Waiting, End };
/** Named safe waits preserve cargo and instruction when no destination or departure is currently admissible. */
enum class MiningContinuationWait : uint8_t { None, NoCompatibleAsteroid, NoReceiver, ServiceUnavailable, DepartureBlocked, End };
/** Saved intent contained by the original mission; targets and coordinates confer no berth, cargo or service rights. */
struct SpaceMiningContinuationState {
	MiningContinuationPhase phase = MiningContinuationPhase::None;
	CargoLabel mineral{}; ///< Pinned at first admitted configured assignment, including an empty hold.
	OrbitalBodyID previous_body = INVALID_ORBITAL_BODY; ///< Historical last body; never interpreted as a live vector offset.
	std::optional<SpaceOrderTarget> effective_target; ///< Current generation-qualified destination; original Mine instruction stays intact.
	std::optional<SpacePoint> departure_point; ///< Bounded safe-departure proposal owned by this mission.
	uint16_t retry_ticks = 0; ///< Bounded simulation delay; no wall clock or catch-up burst.
	MiningContinuationWait waiting = MiningContinuationWait::None;
	uint8_t route_cursor = UINT8_MAX; ///< Next original Visit/handling index while Mine remains held; no copied instructions or second queue.
	uint32_t travel_ticks = 0; ///< Frozen remaining travel allowance for the current effective Visit/receiver; actual service has its own original timer.
	/** @param other Mission-owned intent to compare. @return Complete canonical default/state equality for quotes and persistence. */
	bool operator==(const SpaceMiningContinuationState &other) const = default;
};
#endif /* SPACE_MINING_STATE_H */
