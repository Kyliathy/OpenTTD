/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm.h Train-owned cutter definitions and project authority. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, master AD-5 */

#ifndef TBM_H
#define TBM_H

#include "excavation.h"
#include "engine_type.h"

struct Train;

/** Stable controller identity; geology continues to allocate and own its separate job identity. */
using TBMProjectID = StrongType::Typedef<uint32_t, struct TBMProjectIDTag, StrongType::Compare>;

/** Saved successful work phases; an interruption preserves the phase being resumed. */
enum class TBMPhase : uint8_t { Commissioning, TravelToFace, Bore, AdvanceSupport, ReturnForExchange, WaitForEmpties, Resume, Completed, Cancelled };

/** Saved resumable interruptions, independent of ordinary train orders and reversal flags. */
enum class TBMBlockedReason : uint8_t { None, Stopped, Breakdown, Servicing, MissingProject, MissingCutter, MissingOrigin, MissingPortal, MissingExchange, InvalidConsist, LockedModel, MissingSupport, EarthFull, RockFull, InsufficientFunds, Obstruction, AllocationFailure };

/** Immutable physical roles; only complete spoil wagon units are exchangeable. */
enum class TBMConsistRole : uint8_t { Invalid, Cutter, PermanentSupport, EarthWagon, RockWagon };

/** Feature-owned physical tuning of a profile-owned cutter model; this is never an EngineID. */
struct TBMCutterModel {
	uint16_t model_id; ///< Resolve layers, introduction and facility permissions through LayerVehicleCapability.
	uint16_t travel_speed; ///< Conventional train speed units; work speed uses the separate synchronized rate.
	uint16_t power_hp; ///< Installed traction power in the train engine's horsepower units.
	uint16_t work_rate_numerator; ///< Geological work ticks per simulation tick, divided by work_rate_denominator.
	uint16_t work_rate_denominator; ///< Fixed-point divisor; retain its remainder in project authority.
	Money purchase_cost; ///< Complete permanent machinery price before exchangeable wagon purchase.
	Money active_cost_per_year; ///< Recurring boring charge, separate from the geological cut/support fee.
	Money idle_cost_per_year; ///< Nonzero recurring stopped/service charge.
	uint16_t earth_wagon_model; ///< Profile-owned dedicated Earth hopper design.
	uint16_t rock_wagon_model; ///< Profile-owned dedicated Rock hopper design.
};

/**
 * Canonical saved train controller; it owns assignment, fixed parts, heading and state-duration accounting.
 * The referenced ExcavationJob exclusively owns route, face, cursor, partial geological work and material totals.
 * Wagon custody remains in the actual train chain; no cargo or mutable route cache is stored here.
 */
struct TBMProject {
	TBMProjectID id{UINT32_MAX}; ///< Invalid until this controller allocates a project.
	ExcavationJobID job{UINT32_MAX}; ///< Actual geological route/progress owner; absence is a recoverable block.
	CompanyID company = CompanyID::Invalid(); ///< Actual owner, never a copied cash account.
	VehicleID cutter = VehicleID::Invalid(); ///< Front engine of the commissioned train.
	uint16_t model_id = UINT16_MAX; ///< Immutable logical cutter model, independently of engine-pool allocation.
	std::vector<VehicleID> permanent_parts; ///< Ordered cutter followed by its articulated support parts; wagons never enter this list.
	Direction heading = Direction::Invalid; ///< Cutting orientation; backing never changes this authority.
	StationID origin = StationID::Invalid(); ///< Required actual owned Surface railway station.
	TileIndex portal = INVALID_TILE; ///< Actual commissioning pair endpoint, resolved by the portal owner.
	TileIndex exchange_bay = INVALID_TILE; ///< Actual exchange facility, or source-side commissioning exchange until Plan 5.
	TBMPhase phase = TBMPhase::Commissioning; ///< Work phase retained while stopped, blocked or servicing.
	TBMBlockedReason blocked = TBMBlockedReason::None; ///< One precise saved interruption reason.
	uint64_t phase_ticks = 0; ///< One synchronized phase timer; movement substeps/rendering cannot advance it.
	uint16_t work_remainder = 0; ///< Fractional work-rate numerator, separate from geological partial work.
	uint64_t active_ticks = 0; ///< Saved duration awaiting recurring active-cost accounting.
	uint64_t idle_ticks = 0; ///< Saved duration awaiting recurring idle-cost accounting, including stopped ticks.
	uint64_t cost_remainder = 0; ///< Fractional recurring-money numerator carried across daily accounting.
	uint64_t revision = 0; ///< Controller stamp invalidating prepared physical participants.
};

/** Read-only projected progress; geological fields are queried from their owner rather than saved twice. */
struct TBMWorkState {
	TBMPhase phase = TBMPhase::Commissioning; ///< Actual controller phase.
	TBMBlockedReason blocked = TBMBlockedReason::MissingProject; ///< Explicit invalid-reference default.
	TileIndex face = INVALID_TILE; ///< Next accepted geological face, absent after completion.
	uint32_t pass_cursor = 0; ///< Geological job's sole cursor.
	uint64_t partial_work_ticks = 0; ///< Geological job's sole partial accumulator.
	Direction heading = Direction::Invalid; ///< Saved forward cutting orientation.
	uint64_t phase_ticks = 0; ///< Controller's synchronized timer.
};

/** Actual whole-wagon boundary available to the later exchange owner; these are views, never another cargo store. */
struct TBMConsistUnit {
	VehicleID vehicle = VehicleID::Invalid(); ///< Real unit head; its articulated cargo parts move with it.
	TBMConsistRole role = TBMConsistRole::Invalid; ///< Dedicated Earth or Rock only.
};

/** Complete immutable-layout validation result, without changing links, cargo, reservations or funds. */
struct TBMConsistLayout {
	TBMBlockedReason error = TBMBlockedReason::InvalidConsist; ///< Success is None; malformed chains refuse entirely.
	std::vector<TBMConsistUnit> exchangeable_units; ///< Stable chain order of complete detachable wagons.
};

/** @return Both immutable physical cutter designs; profile capabilities remain the sole layer/date/facility source. */
std::span<const TBMCutterModel> GetTBMCutterModels();

/**
 * Resolve one physical cutter definition without allocating an engine or vehicle.
 * @param model_id Profile-owned logical design identity.
 * @return Immutable tuning, or nullptr for any non-cutter/unknown model.
 */
const TBMCutterModel *GetTBMCutterModel(uint16_t model_id);

/**
 * Validate cutter availability against explicit saved policy and synchronized calendar.
 * @param model_id Logical design, never a runtime engine ID.
 * @param layer Actual operation layer.
 * @param policy Accepted saved profile/version/override policy.
 * @param date Game-calendar date; economy and viewport state grant nothing.
 * @return Whether this cutter can operate here on this date under the selected profile.
 */
bool IsTBMCutterAvailable(uint16_t model_id, MapLayer layer, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date);

/**
 * Rebuild native machinery/hopper definitions through the existing engine pool and EIDS mapping after content loading.
 * @param activated Loader-owned configuration with pinned package and activated cargo meanings.
 * @return True after all four definitions exist, false for missing content or unavailable capacity.
 */
bool InitialiseTBMEngineDefinitions(const GRFConfigList &activated);

/**
 * Resolve a native design through the existing persisted engine mapping.
 * @param model_id Profile-owned cutter/hopper model (0..3).
 * @return Actual EngineID, or Invalid when unavailable.
 */
EngineID GetTBMEngine(uint16_t model_id);

/**
 * Classify an actual engine without interpreting an original/foreign engine number as a logical model.
 * @param engine Actual engine-pool identity.
 * @return Profile model 0..3, or UINT16_MAX for other/missing definitions.
 */
uint16_t GetTBMEngineModel(EngineID engine);

/**
 * Identify native articulated machinery for construction and purchase refusal.
 * @param engine Actual engine-pool identity.
 * @return Whether the native mapping identifies a cutter design.
 */
bool IsTBMCutterEngine(EngineID engine);

/**
 * Verify permanent cutter/support links and expose real detachable spoil units.
 * @param front Actual front train engine.
 * @param project Canonical immutable part IDs and company/cutter assignment.
 * @return Complete layout or unchanged refusal; uses GetNextUnit/GetPrevUnit for whole wagon boundaries.
 */
TBMConsistLayout InspectTBMConsist(const Train &front, const TBMProject &project);

#endif /* TBM_H */
