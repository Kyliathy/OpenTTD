/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_vehicle.h Canonical native orbital fleet and finite mining missions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md; the paid carried hull remains the sole native unit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C; heavy mining uses the same mission and packet owners.
 */
#ifndef SPACE_VEHICLE_H
#define SPACE_VEHICLE_H
#include "vehicle_base.h"
#include "space.h"
#include "space_orders.h"
#include "space_production.h"

inline constexpr uint16_t SPACE_PROBE_MODEL = 15;
inline constexpr uint32_t MAX_SPACE_VEHICLES = 64;
struct SpaceManifest;
struct SpaceTransition;

/** Saved native mission phases; a missing initial target remains Idle until an explicit assignment. */
enum class SpaceMissionPhase : uint8_t { Idle, Approach, Mining, Return, Holding };
/** Body IDs are monotonic field identities; no station-order destination aliasing. */
struct SpaceMission {
	OrbitalBodyID target = INVALID_ORBITAL_BODY;
	SpacePoint attachment_offset{SPACE_Q16, -SPACE_Q16 / 2};
	SpaceMissionPhase phase = SpaceMissionPhase::Idle;
	uint32_t approach_ticks_left = 0;
	uint16_t work_ticks = 0;
	uint8_t travel_remainder = 0;
	uint8_t cost_remainder = 0;
};

/** One ordinary Vehicle pool owner of company identity, packets, costs and orbital mission. */
struct SpaceVehicle final : SpecializedVehicle<SpaceVehicle, VehicleType::Space> {
	SpacePoint position, home;
	SpaceMission mission;
	uint32_t extracted_units = 0;
	uint64_t generation = 0;
	SpaceOrderQueue space_orders;
	SpaceConstructionState construction;
	std::unique_ptr<SpaceManifest> manifest; ///< Real fuel/service packet compartments and carried native body references.
	std::unique_ptr<SpaceTransition> transition; ///< The same paid rocket owns its complete launch/recovery work and history.
	/** @param index Actual native Vehicle pool slot; this constructor grants no paid commissioning receipt. */
	SpaceVehicle(VehicleID index);
	/** Release only this hull's manifests, leases and commissioning references before native teardown. */
	~SpaceVehicle() override;
	/** @return True: each Space hull is one independent company fleet vehicle. */
	bool IsPrimaryVehicle() const override { return true; }
	/** @return Whether this hull survives one synchronized native mission/accounting tick. */
	bool Tick() override;
	/** Advance canonical calendar vehicle age. */
	void OnNewCalendarDay() override;
	/** Advance canonical economy age and depreciation. */
	void OnNewEconomyDay() override;
	/** @param direction Requested facing. @param image_type Native image context. @param result Sprite sequence to populate. */
	void GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const override;
	/** Refresh the native hull's spatial drawing bounds. */
	void UpdateDeltaXY() override;
	/** Invalidate this hull's viewport and fleet inspector after an owner mutation. */
	void MarkDirty() override;
	/** @param income Whether querying income; both directions use the Space operating account. @return Canonical expense category. */
	ExpensesType GetExpenseType([[maybe_unused]] bool income) const override { return ExpensesType::Other; }
	/** @return Annualized upkeep using the same work eligibility and shared budget as actual per-tick charges. */
	Money GetRunningCost() const override;
	/** Project this hull's actual layer-local point into the canonical native vehicle spatial index. */
	void UpdateSpacePosition();
};

/** @return Existing model15 EngineID; no hull, launch or parallel catalogue is allocated. */
EngineID GetSpaceProbeEngine();
/** @return Actual native Space hull count, including dormant paid hulls. */
uint32_t GetSpaceVehicleCount();
/** @return Whether the default miner and retained role engine definitions can be restored without commissioning. */
bool InitialiseSpaceEngineDefinition();
/** @return Whether all real fleet, packet, mission and paid producer dependencies are consistent. */
bool ValidateSpaceVehicles();
/** @return Deterministic digest of actual native fleet owners for continuation verification. */
uint64_t ComputeSpaceVehicleDigest();
/** Explicit demo setup only: retire survey records, debit purchases, allocate ordinary Vehicle identities. */
bool PrepareNativeSpaceMiningDemo();
/** @param vehicle Actual hull relinquishing only its own extraction lease and work. */
void ReleaseSpaceMiningSlot(SpaceVehicle &vehicle);
/** @param position Actual start point. @param approach Current authorized attachment point. @return Frozen upward-rounded model15 pursuit allowance in synchronized ticks. */
uint32_t GetSpaceMiningApproachTicks(SpacePoint position, SpacePoint approach);
/** @param vehicle Actual deployed miner. @param body Stable body ID, or the explicit recall sentinel.
 * @param execute False estimates without mutation. @return Admission or refusal preserving cargo and funds.
 */
CommandCost AssignSpaceMiningMission(SpaceVehicle &vehicle, uint32_t body, bool execute);

/** Validate role/work and source-version admission without consulting any active-world owner.
 * @param model Actual native or same-file paid model.
 * @param phase Saved native mining phase, including its bounded idle/holding states.
 * @param work_ticks Saved partial extraction work; service/queue work has a separate existing owner.
 * @param extracted_units This hull's lifetime finite-resource extraction receipt.
 * @param heavy_runtime Whether the source schema includes heavy mining, rather than commissioning alone.
 * @return Whether the role can own these mining fields; body, lease and packet joins validate separately.
 */
bool IsSpaceMiningStateSupported(uint16_t model, SpaceMissionPhase phase, uint16_t work_ticks, uint32_t extracted_units, bool heavy_runtime);

#endif /* SPACE_VEHICLE_H */
