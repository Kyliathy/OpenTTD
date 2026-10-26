/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_target.h Read-only return-domain and original native receiver admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 * These records allocate no receiver, packet inventory, shell, allowance, route queue or service job.
 */
#ifndef SPACE_RETURN_TARGET_H
#define SPACE_RETURN_TARGET_H
#include "space_orders.h"
#include "space_pod_type.h"
#include <optional>
struct SpaceVehicle;
struct LaunchCenterRef;

/** Reference to the original route's first handling instruction; retained Visit indices remain under that same queue owner. */
struct SpaceReturnRoute {
	SpaceOrderTargetKind domain = SpaceOrderTargetKind::LaunchCenter;
	std::optional<SpaceOrderTarget> specified; ///< Exact original directive/home, if present; LegacyReturn never substitutes another target.
	uint8_t visits_begin = 0; ///< First original queue position after the held Mine.
	uint8_t handling_index = 0; ///< First original Unload/ReturnService, or entries.size() for an implicit role/home return.
	SpaceOrderAction declared_action = SpaceOrderAction::Unload; ///< Selection does not complete or skip this real instruction.
	bool nearest = false; ///< Configured policies may search compatible owned members of domain; legacy remains exact.
	/** @param other Original route metadata to compare. @return Complete intent equality without granting physical navigation or handling. */
	bool operator==(const SpaceReturnRoute &other) const = default;
};
/** Resolve the permitted destination domain and preserve the original intervening Visit sequence.
 * @param craft Actual native mining role with its current Mine, or an unordered legacy return.
 * @return Original handling/home route, or absence when another work action intervenes or no real role supports an implicit return. Neither lookup nor nearest policy authorizes a foreign facility or performs a Visit/Unload/service action.
 */
std::optional<SpaceReturnRoute> GetSpaceReturnRoute(const SpaceVehicle &craft);
/** @param model Original native/candidate role. @param queue Original instructions. @param has_pods Whether this same hull retains real/copied shell references. @param home Original typed home identity. @return The same permitted route without consulting any live hull, receiver or cargo definition. */
std::optional<SpaceReturnRoute> GetSpaceReturnRoute(uint16_t model, const SpaceOrderQueue &queue, bool has_pods, LaunchCenterRef home);

/** Requested original handling producer; service is real work with its own retained parts/timer, never an unload side effect. */
enum class SpaceReturnPurpose : uint8_t { Cargo, Service, ServiceVisit, End }; ///< ServiceVisit reaches an existing service function when its allowance is still positive; it performs no unload or renewal.
/** Read-only destination/batch proposal; actual arrival revalidates the original native owners. */
struct SpaceReturnCandidate {
	SpaceOrderTarget target;
	SpacePoint anchor; ///< Original center anchor or canonical module approach, consistently used for distance ranking.
	SpaceReturnPurpose purpose = SpaceReturnPurpose::Cargo;
	uint16_t quantity = 0; ///< Actual next cargo batch; zero for service or an empty navigational return, never invented freight.
	SpacePodRef pod; ///< Actual uniquely carried shell for a positive Surface batch; unused for module/service/empty returns.
	/** @param other Read-only proposal to compare. @return Complete value equality without allocating or reserving any owner. */
	bool operator==(const SpaceReturnCandidate &other) const = default;
};
/** Query one destination through original native navigation admission.
 * @param craft Actual native miner; route, held freight, manifest and history remain unchanged.
 * @param target Exact candidate in the current original route's allowed domain.
 * @param purpose Cargo receiving or genuinely due native service.
 * @return Admitted original destination/batch, or absence for foreign/full/incompatible/busy/unavailable owners. This grants no arrival, berth, inventory, quota or payment.
 */
std::optional<SpaceReturnCandidate> GetSpaceReturnCandidateAdmission(const SpaceVehicle &craft, const SpaceOrderTarget &target, SpaceReturnPurpose purpose);
/** @param craft Actual asking miner. @param purpose Required original handling producer. @return Globally nearest eligible owned member of the permitted domain by exact squared distance and typed ID/generation; LegacyReturn retains its exact specified target. Selection changes no native owner. */
std::optional<SpaceReturnCandidate> FindNearestEligibleSpaceReturn(const SpaceVehicle &craft, SpaceReturnPurpose purpose);
/** Revalidate actual arrival and offer the original owner's real operation, without changing pose or forcing Holding.
 * @param craft Actual native miner; the queue controller owns physical navigation, intervening Visits and cursor progression.
 * @param target Previously selected exact destination; route/ownership/capacity are rechecked now.
 * @param purpose Real cargo return or due service start.
 * @return Whether an empty return actually arrived or the original positive transfer/service start succeeded. Service earns completion only through its existing normal work owner; this function advances no queue cursor.
 */
bool ExecuteSpaceReturnArrival(SpaceVehicle &craft, const SpaceOrderTarget &target, SpaceReturnPurpose purpose);
#endif /* SPACE_RETURN_TARGET_H */
