/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_departure.h Native pre-translation mining release under the original mission/lease owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group25B; normal-loop activation belongs to Group29.
 */
#ifndef SPACE_MINING_DEPARTURE_H
#define SPACE_MINING_DEPARTURE_H
#include "space_departure.h"
#include "space_production.h"

/** Physical reason this existing mining mission cannot safely follow its original target. */
enum class SpaceBodyDepartureReason : uint8_t { UnsafeAttachment, UnsafeInterception };
/** Read-only exact native participant and local proposal; no separate queue, lease or cargo owner. */
struct SpaceBodyDeparture {
	SpaceCraftRef craft;
	OrbitalBodyID body;
	SpaceBodyDepartureReason reason;
	SpaceDeparturePlan plan;
	/** @param other Pre-translation proposal to compare. @return Whether participant, reason and complete geometry/timing still agree. */
	bool operator==(const SpaceBodyDeparture &other) const = default;
};
/** @return Eligible original mining participants in stable VehicleID order, including stopped missions but excluding unrelated roles, detached waits and paid static service. */
std::vector<SpaceCraftRef> GatherSpaceMiningDepartureParticipants();
/** @param craft Exact current native participant. @return Due unsafe-step proposal, or absence while its original attachment/interception remains safe; changes no owner. */
std::optional<SpaceBodyDeparture> PlanSpaceBodyDeparture(SpaceCraftRef craft);
/** @param proposal Previously observed native participant, body, reason and local geometry. @return Whether unchanged current admission released only that hull's lease/partial work and saved its departure. Repeated or stale proposals refuse without movement, upkeep, cargo changes or implicit Start. */
bool BeginSpaceBodyDeparture(const SpaceBodyDeparture &proposal);
/** Publish due safety releases sequentially before any body motion/attachment transform.
 * @return Number of original hulls released; each proposal is planned after prior releases so contained departure points participate in occupancy.
 * This prepares the production owner; Group29 connects its sole normal-loop call.
 */
uint32_t ReleaseUnsafeSpaceMiningAttachments();
#endif /* SPACE_MINING_DEPARTURE_H */
