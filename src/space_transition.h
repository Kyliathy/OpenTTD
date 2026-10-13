/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_transition.h Native saved rocket preparation, layer transition and recovery. */
#ifndef SPACE_TRANSITION_H
#define SPACE_TRANSITION_H
#include "space_production.h"
#include "space_orders.h"
#include "map_layer_type.h"

struct LaunchOperationsRecord;
enum class SpaceFlightPhase : uint8_t { GroundLoading, Preparing, Ascent, OrbitalDelivery, ReturnRecovery, Turnaround, Holding, End };
enum class SpaceFlightBlock : uint8_t { None, TargetLost, HomeLost, PadBusy, ReceiverBusy, ReceiverFull, Approach, End };
/** Owned by the same native rocket Vehicle throughout a round trip. */
struct SpaceTransition {
	SpaceFlightPhase phase = SpaceFlightPhase::GroundLoading;
	SpaceFlightBlock blocked = SpaceFlightBlock::None;
	MapLayer layer = MapLayer::Surface;
	LaunchCenterRef source, recovery;
	SpaceOrderTarget destination;
	SpacePoint departure{};
	uint16_t remaining_ticks = 0;
	uint8_t pad_cost_remainder = 0;
	bool outbound_burned = false, recovery_burned = false, deployed = false;
	bool resupplied = false; ///< Separate414 receipt; an existing probe is not deployed again.
	uint64_t missions_started = 0, missions_completed = 0, missions_canceled = 0;
	bool operator==(const SpaceTransition &) const = default;
};
struct SpaceTransitionRecord { SpaceCraftRef craft; SpaceTransition state; };
bool IsSpaceFlightActive(const SpaceVehicle &craft);
MapLayer GetSpaceVehicleLayer(const SpaceVehicle &craft);
bool IsSpaceCraftAwaitingSurfaceRecovery(const SpaceVehicle &craft);
void TickSpaceTransitions();
void ForgetSpaceTransitionCenter(SpaceVehicle &craft, LaunchCenterRef center);
std::vector<SpaceTransitionRecord> CaptureSpaceTransitions();
bool ValidateSpaceTransitionRecords(std::span<const SpaceTransitionRecord> records, uint32_t width = 4096, uint32_t height = 4096);
bool ValidateSpaceTransitionLinks(std::span<const SpaceTransitionRecord> flight, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations);
bool ValidateSpaceTransitions();
bool InstallLoadedSpaceTransitions(std::span<const SpaceTransitionRecord> records);
uint64_t ComputeSpaceTransitionDigest();
void ResetSpaceTransitionLoadStaging();
bool AcceptLoadedSpaceTransitions();
void ValidateSpaceTransitionLoadPreview();
#endif
