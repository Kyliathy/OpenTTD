/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_vehicle.h Canonical native orbital fleet and finite mining missions. */
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
	SpaceVehicle(VehicleID index);
	~SpaceVehicle() override;
	bool IsPrimaryVehicle() const override { return true; }
	bool Tick() override;
	void OnNewCalendarDay() override;
	void OnNewEconomyDay() override;
	void GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const override;
	void UpdateDeltaXY() override;
	void MarkDirty() override;
	ExpensesType GetExpenseType(bool) const override { return ExpensesType::Other; }
	Money GetRunningCost() const override;
	void UpdateSpacePosition();
};

EngineID GetSpaceProbeEngine();
uint32_t GetSpaceVehicleCount();
bool InitialiseSpaceEngineDefinition();
bool ValidateSpaceVehicles();
uint64_t ComputeSpaceVehicleDigest();
/** Explicit demo setup only: retire survey records, debit purchases, allocate ordinary Vehicle identities. */
bool PrepareNativeSpaceMiningDemo();
void ReleaseSpaceMiningSlot(SpaceVehicle &vehicle);
CommandCost AssignSpaceMiningMission(SpaceVehicle &vehicle, uint32_t body, bool execute);

#endif /* SPACE_VEHICLE_H */
