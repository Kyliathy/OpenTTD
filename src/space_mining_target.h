/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_target.h Read-only mining candidate ordering and original native admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group18.
 */
#ifndef SPACE_MINING_TARGET_H
#define SPACE_MINING_TARGET_H
#include "asteroid.h"
#include "cargo_type.h"
#include "space_departure.h"
struct SpaceVehicle;

/** Detached interception/departure evidence; it grants no pose, request, attachment or extraction work. */
struct SpaceMiningInterceptForecast {
	uint32_t arrival_ticks = 0; ///< Actual native pursuit opportunities, with the first using the supplied already-moved body position.
	SpaceMotion body_at_arrival; ///< Copied straight trajectory when native tolerance admits attachment.
	uint8_t travel_remainder = 0; ///< Original movement carry after arrival; copied departure uses this same fraction.
	SpacePoint departure_point; ///< One clear bounded local endpoint, subject to native occupancy revalidation at actual departure.
	uint32_t departure_ticks = 0; ///< Exact native local travel to that endpoint, leaving an additional release margin before hazard.
};
/** Forecast actual bounded native pursuit and a clear local departure without moving any owner.
 * @param position Original bounded craft point at its next pursuit opportunity.
 * @param remainder Original unsigned movement fraction.
 * @param model Actual canonical miner role, 15 or 21; both reuse the native mover.
 * @param body Original/copied finite target whose motion remains straight.
 * @param bounds Same physical plane as craft and body.
 * @param obstacles Original static footprints and berths, copied once by the caller.
 * @param allowed_start_berth Exact currently admitted source berth from the native approach owner, if any; only the first departure segment may leave that point.
 * @return Arrival/departure evidence, or refusal before an unsafe interception. Existing pursuit deadline bounds work; its minimum is not treated as required travel.
 */
std::optional<SpaceMiningInterceptForecast> ForecastSpaceMiningIntercept(SpacePoint position, uint8_t remainder, uint16_t model,
	const AsteroidBody &body, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles, std::optional<SpacePoint> allowed_start_berth = std::nullopt);

/** Rank finite matching resources using only explicit prevalidated field metadata.
 * @param bodies Original or copied live bodies; IDs need not equal vector slots.
 * @param position Actual bounded asking position in the same plane.
 * @param mineral Previously pinned stable mineral label, never a cargo ordinal inferred from a vanished body.
 * @return All finite matching IDs in exact distance/ID order. This temporary list grants no eligibility, lease or assignment and does not truncate to a shortlist.
 */
std::vector<OrbitalBodyID> RankMiningTargetCandidates(std::span<const AsteroidBody> bodies, SpacePoint position, CargoLabel mineral);
/** Select an immediately eligible original native resource without reserving it.
 * @param craft Actual asking miner; native estimate APIs accept its mutable reference but this query publishes no changes.
 * @param mineral Previously pinned meaning; disagreement with existing intent refuses.
 * @return Exact nearest compatible free and safely interceptable target, or absence preserving the hold/policy. Assignment rechecks every original gate; selector exposure remains gated on continuation/persistence integration.
 */
std::optional<OrbitalBodyID> FindNearestEligibleMiningTarget(SpaceVehicle &craft, CargoLabel mineral);
/** @return Bounded fallback delay of one native motion day (74 simulation opportunities); pause and fast-forward follow the ordinary tick driver. This is independent of asteroid replacement's 20-60 second interval. */
uint16_t GetSpaceMiningTargetRetryDelay();
/** Wake only existing matching full-load target waits after an original resource/claim change.
 * @param mineral Stable label of the body that entered, exited or released its claim, captured before removal.
 * This resets the existing mission's retry delay only. It performs no selection, movement, request, reserve change or cargo transfer. New body lifecycle producers call it after their atomic commit in Group28; lease release uses the existing native owner.
 */
void NotifySpaceMiningTargetChange(CargoLabel mineral);
#endif /* SPACE_MINING_TARGET_H */
