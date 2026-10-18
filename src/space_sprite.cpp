/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_sprite.cpp Final Space art reads original mission, resource and passive-shell owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#include "stdafx.h"
#include "space_sprite.h"
#include "space_vehicle.h"
#include "space_pod.h"
#include "space_manifest.h"
#include "space_approach.h"
#include "space_departure.h"
#include "orbital_station.h"
#include "orbital_service.h"
#include "asteroid.h"
#include "station_base.h"
#include "openttd.h"
#include <cstdlib>
#include "safeguards.h"

/** @copydoc GetSpaceArtDirection */
Direction GetSpaceArtDirection(SpacePoint from, SpacePoint to, Direction fallback)
{
	const int64_t dx = int64_t{to.x} - from.x, dy = int64_t{to.y} - from.y;
	const int64_t x = (dy - dx) * 2, y = dy + dx;
	// Coincident targets cannot create a new heading or angular state; keep the existing native facing, with a canonical view only for an invalid legacy facing.
	if (x == 0 && y == 0) return fallback < Direction::End ? fallback : Direction::NE;
	if (std::abs(x) > 2 * std::abs(y)) return x > 0 ? Direction::E : Direction::W;
	if (std::abs(y) > 2 * std::abs(x)) return y > 0 ? Direction::S : Direction::N;
	if (y < 0) return x > 0 ? Direction::NE : Direction::NW;
	return x > 0 ? Direction::SE : Direction::SW;
}

/** @copydoc GetSpaceCraftArtDirection */
Direction GetSpaceCraftArtDirection(const SpaceVehicle &craft)
{
	const auto fallback = craft.direction < Direction::End ? craft.direction : Direction::NE;
	const auto &queue = craft.space_orders;
	const SpaceOrder *order = queue.cursor < queue.entries.size() ? &queue.entries[queue.cursor] : nullptr;
	const auto *body = GetMutableAsteroidBody(craft.mission.target);
	// A physically latched miner faces its original body, independent of body velocity, fixed cosmetic yaw or stopped work. No attachment offset is rotated by artwork.
	if (craft.mission.phase == SpaceMissionPhase::Mining && body != nullptr && body->mining_slot == craft.index) return GetSpaceArtDirection(craft.position, body->motion.position, fallback);
	// Active survey work uses its current typed body; the unrelated legacy mining target cannot supply a scanner's facing.
	if (order != nullptr && order->action == SpaceOrderAction::Survey && IsSpaceSurveyWorkTick(craft)) {
		const auto *surveyed = GetMutableAsteroidBody(OrbitalBodyID{order->target.id});
		if (surveyed != nullptr) return GetSpaceArtDirection(craft.position, surveyed->motion.position, fallback);
	}
	SpacePoint destination;
	if (queue.approach) {
		const auto block = GetSpaceApproachDestination(craft, destination);
		// A queued craft can travel to its actual saved staging point even before admission; a removed target supplies no substitute destination.
		if ((block == SpaceOrderBlock::None || block == SpaceOrderBlock::ApproachBusy) && destination != craft.position) return GetSpaceArtDirection(craft.position, destination, fallback);
	}
	// Typed navigation resolves the real target through its existing generation and ownership checks; art neither starts an action nor requests a berth.
	if (order != nullptr && ResolveSpaceNavigationTarget(craft, order->target, destination) == SpaceOrderBlock::None) return GetSpaceArtDirection(craft.position, destination, fallback);
	if (craft.mission.phase == SpaceMissionPhase::Return) {
		const auto departure = GetSpaceMiningDepartureDestination(craft, craft.home);
		if (departure) return GetSpaceArtDirection(craft.position, *departure, fallback);
	}
	// Retained direct mining missions may have no typed queue; the original saved attachment point remains their geometric destination.
	if (craft.mission.phase == SpaceMissionPhase::Approach && body != nullptr) {
		destination = {body->motion.position.x + craft.mission.attachment_offset.x, body->motion.position.y + craft.mission.attachment_offset.y};
		return GetSpaceArtDirection(craft.position, destination, fallback);
	}
	return fallback;
}

/** Select only a real existing service countdown, without creating an assignment or querying packet copies.
 * @param craft Original hull and its sole onboard/Earth manifest or contained job.
 * @param pose Receives service state and a phase derived from the original countdown.
 * @param state Published service-state index in this exact probe family's source matrix.
 * @return Whether an original positive service job supplies this visual state.
 */
static bool SelectServicePose(const SpaceVehicle &craft, SpaceArtPose &pose, uint8_t state)
{
	const auto *contained = GetOrbitalServiceJobState(craft);
	// A retained contained job remains visible while power is unavailable; unchanged countdown means a frozen phase, never invented paid work or a second service clock.
	if (contained != nullptr && contained->remaining_ticks != 0) {
		pose.state = state;
		pose.frame = contained->paused ? 0 : (contained->remaining_ticks / 8) % 2;
		return true;
	}
	// Only an original positive manifest countdown represents onboard/Earth service; a missing compartment cannot become a decorative service job.
	if (craft.manifest && craft.manifest->service_ticks != 0) {
		pose.state = state;
		pose.frame = craft.manifest->service_paused ? 0 : (craft.manifest->service_ticks / 8) % 2;
		return true;
	}
	return false;
}

/** Query actual typed action failure independently of completion.
 * @param craft Original native queue, whose completion sentinel is an ordinary idle outcome.
 * @return Whether an actual outstanding wait/refusal supplies blocked art.
 */
static bool HasArtOrderBlock(const SpaceVehicle &craft)
{
	const auto block = craft.space_orders.blocked;
	return block != SpaceOrderBlock::None && block != SpaceOrderBlock::Completed;
}

/** Gate authored exhaust through existing actual movement presentation fields.
 * @param craft Original hull; stopped flags override a retained prior cur_speed.
 * @return Phase on the existing native vehicle tick counter, or absence when the authored idle pose must replace exhaust entirely.
 */
static std::optional<uint8_t> TravelArtFrame(const SpaceVehicle &craft)
{
	// A stopped, arrived or unstarted hull may retain an earlier speed. Only an outstanding actual destination can animate exhaust, without starting the next instruction or advancing a new clock.
	if (craft.cur_speed == 0 || craft.vehstatus.Test(VehState::Stopped)) return std::nullopt;
	SpacePoint destination;
	const auto &queue = craft.space_orders;
	if (!queue.entries.empty()) {
		// An unfinished saved instruction and its original admission are required; a completed queue cannot borrow stale movement from the previous action.
		if (!queue.started || queue.cursor >= queue.entries.size()) return std::nullopt;
		if (queue.approach) {
			if (GetSpaceApproachDestination(craft, destination) != SpaceOrderBlock::None) return std::nullopt;
		} else if (ResolveSpaceNavigationTarget(craft, queue.entries[queue.cursor].target, destination) != SpaceOrderBlock::None) return std::nullopt;
	} else if (craft.mission.phase == SpaceMissionPhase::Return) {
		const auto departure = GetSpaceMiningDepartureDestination(craft, craft.home);
		if (!departure) return std::nullopt;
		destination = *departure;
	} else if (craft.mission.phase == SpaceMissionPhase::Approach) {
		const auto *body = GetMutableAsteroidBody(craft.mission.target);
		if (body == nullptr) return std::nullopt;
		destination = {body->motion.position.x + craft.mission.attachment_offset.x, body->motion.position.y + craft.mission.attachment_offset.y};
	} else return std::nullopt;
	if (destination == craft.position) return std::nullopt;
	return (craft.tick_counter / 8) % 2;
}

/** Read admitted original movement without adding a counter or changing existing probe selection.
 * @param craft Actual hull and generation-qualified current destination.
 * @return Existing movement phase, or absence outside normal unpaused movement.
 */
std::optional<uint8_t> GetSpaceCraftTravelArtFrame(const SpaceVehicle &craft)
{
	// New orbital art cannot advertise active movement during a global pause
	// or editor query; the original destination and counter still own the phase.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return std::nullopt;
	const auto model = GetSpaceVehicleModel(craft);
	// Retained16 and hauler17 move through original typed orders only; a stale
	// legacy mining phase/target cannot lend either role an admitted destination.
	if ((model == 16 || model == 17) && craft.space_orders.entries.empty()) return std::nullopt;
	return TravelArtFrame(craft);
}

/** Read an original native probe-service countdown for an appended craft family.
 * @param craft Actual hull; native service tuning must admit its model.
 * @return Original service phase, zero while globally paused, or absence without a real job.
 */
std::optional<uint8_t> GetSpaceCraftServiceArtFrame(const SpaceVehicle &craft)
{
	if (!GetSpaceServiceTuning(GetSpaceVehicleModel(craft))) return std::nullopt;
	SpaceArtPose pose;
	if (!SelectServicePose(craft, pose, 0)) return std::nullopt;
	// Pausing freezes the actual paid job and selects its canonical source phase;
	// no new service owner or countdown exists in this shared presentation query.
	return _game_mode != GameMode::Normal || _pause_mode.Any() ? uint8_t{0} : pose.frame;
}

/** @copydoc GetSpaceCraftArtPose */
std::optional<SpaceArtPose> GetSpaceCraftArtPose(const SpaceVehicle &craft, Direction direction)
{
	const auto model = GetSpaceVehicleModel(craft);
	SpaceArtPose pose;
	pose.direction = direction;
	const auto &queue = craft.space_orders;
	const SpaceOrder *order = queue.cursor < queue.entries.size() ? &queue.entries[queue.cursor] : nullptr;
	if (model == 15) {
		pose.asset = SpaceSpriteAsset::VehicleModel15;
		if (SelectServicePose(craft, pose, 6)) return pose;
		// Paid quota/pod/service waits and typed failures remain original owner disclosures; visual selection never clears them or grants a return allowance.
		if (HasArtOrderBlock(craft) || GetSpaceMiningBlock(craft) != SpaceOrderBlock::None) { pose.state = 7; return pose; }
		if (craft.mission.phase == SpaceMissionPhase::Return) {
			if (const auto frame = TravelArtFrame(craft)) { pose.state = 5; pose.frame = *frame; return pose; }
		}
		// An admitted full hold can persist for one native opportunity before departure. A zero-capacity or empty hull is never represented as full cargo.
		if (craft.cargo_cap != 0 && craft.cargo.TotalCount() >= craft.cargo_cap) { pose.state = 4; return pose; }
		if (craft.mission.phase == SpaceMissionPhase::Mining) {
			const auto *body = GetMutableAsteroidBody(craft.mission.target);
			// A retained phase alone cannot attach a hull to a missing body or somebody else's lease. Keep ordinary free art until the native mission owner resolves that physical loss.
			if (body == nullptr || body->mining_slot != craft.index) return pose;
			pose.state = IsSpaceMiningWorkTick(craft) ? 3 : 2;
			pose.frame = pose.state == 3 ? craft.mission.work_ticks % 2 : 0;
			return pose;
		}
		if (craft.mission.phase == SpaceMissionPhase::Approach) {
			if (const auto frame = TravelArtFrame(craft)) { pose.state = 1; pose.frame = *frame; }
		}
		return pose;
	}
	if (model == 19) {
		pose.asset = SpaceSpriteAsset::VehicleModel19;
		if (SelectServicePose(craft, pose, 4)) return pose;
		if (HasArtOrderBlock(craft)) { pose.state = 5; return pose; }
		if (IsSpaceSurveyWorkTick(craft)) { pose.state = 2; pose.frame = (queue.work_ticks / 8) % 2; return pose; }
		if (order != nullptr) {
			// A saved instruction can remain stopped or already reached; only real travel selects a plume-bearing source pose.
			if (const auto frame = TravelArtFrame(craft)) { pose.state = order->action == SpaceOrderAction::ReturnService ? 3 : 1; pose.frame = *frame; }
		}
		return pose;
	}
	if (model == 20) {
		pose.asset = SpaceSpriteAsset::VehicleModel20;
		if (SelectServicePose(craft, pose, 6)) return pose;
		if (HasArtOrderBlock(craft)) { pose.state = 7; return pose; }
		// The native construction owner checks its exact selected worker and same-opportunity committed receipt; an already spent grant cannot hide real work or lend it to a second builder.
		if (const auto work = GetSpaceConstructionVisualWork(craft)) { pose.state = 4; pose.frame = *work % 2; return pose; }
		// Actual retained freight supplies its symbolic load cue before travel; source art grants no Steel/Goods count and never fabricates a loaded bed for an empty refit.
		if (craft.cargo.TotalCount() != 0 && craft.cargo_type < NUM_CARGO) {
			const auto label = CargoSpec::Get(craft.cargo_type)->label;
			if (label == CargoLabel{"STEL"}) { pose.state = 2; return pose; }
			if (label == CargoLabel{"GOOD"}) { pose.state = 3; return pose; }
		}
		if (order != nullptr) {
			// Empty builders retain their folded idle hull while stopped or at the berth; the construction/service owners above exclusively select real work poses.
			if (const auto frame = TravelArtFrame(craft)) { pose.state = order->action == SpaceOrderAction::ReturnService ? 5 : 1; pose.frame = *frame; }
		}
		return pose;
	}
	return std::nullopt; // Other original fleet roles retain their separate Plan9/11 source and integration obligations.
}

/** @copydoc GetSpaceAsteroidArtPose */
std::optional<SpaceArtPose> GetSpaceAsteroidArtPose(OrbitalBodyID id, CompanyID observer)
{
	const auto *body = GetMutableAsteroidBody(id);
	// A missing body or invalid saved enum cannot resolve into another public source family; presentation never installs a replacement resource record.
	if (body == nullptr || body->resource_class > AsteroidClass::Rich || body->size > AsteroidSize::Large) return std::nullopt;
	SpaceArtPose pose;
	pose.asset = static_cast<SpaceSpriteAsset>(to_underlying(SpaceSpriteAsset::AsteroidNickel) + to_underlying(body->resource_class));
	pose.size = to_underlying(body->size);
	pose.direction = static_cast<Direction>(id.base() % 8); // Fixed original identity; velocity/reflection never rotates the body or its actual attachment slot.
	const auto quantities = GetAsteroidQuantityDisclosure(observer, id);
	// Only this company's real completed survey can reveal exhaustion. Unknown/foreign/spectator observers keep the ordinary body even at actual zero reserve.
	if (quantities && quantities->remaining == 0) { pose.state = 3; return pose; }
	const auto *miner = SpaceVehicle::GetIfValid(body->mining_slot);
	// The exact original lease and shared native work admission supply the mining phase; sprite selection cannot reserve a slot, progress work or disclose a hidden reserve.
	if (miner != nullptr && miner->mission.target == id && IsSpaceMiningWorkTick(*miner)) {
		pose.state = 2;
		pose.frame = miner->mission.work_ticks % 2;
	// A stale/reused vehicle slot or another target cannot lend attachment to this body, even though the honest attached artwork is a free-body alias.
	} else if (miner != nullptr && miner->mission.target == id && miner->mission.phase == SpaceMissionPhase::Mining) {
		pose.state = 1; // Honest attached body alias; the separate native probe is drawn only once.
	}
	return pose;
}

/** @copydoc GetSpacePodArtPose */
std::optional<SpaceArtPose> GetSpacePodArtPose(const SpacePod &pod)
{
	// Stored/carried shells are inside their real workshop/carrier. Drawing another free pod there would duplicate a paid physical asset in the scene.
	if (pod.state.custody != SpacePodCustody::Drifting && pod.state.custody != SpacePodCustody::Descent) return std::nullopt;
	SpaceArtPose pose;
	pose.asset = SpaceSpriteAsset::VehicleModel18;
	pose.direction = Direction::NE;
	if (!pod.descent) { pose.state = 3; return pose; } // The original drifting shell has no active descent work; folded blocked art grants no packet or recovery owner.
	const auto &job = *pod.descent;
	const auto *center = ResolveLaunchCenter(job.center);
	if (center != nullptr) {
		const auto point = GetLaunchCenterPlaneReference(center->launch_center->tile);
		if (point) pose.direction = GetSpaceArtDirection(job.release_point, *point, Direction::NE);
	}
	if (job.blocked != SpaceDescentBlock::None) { pose.state = 3; return pose; }
	// Recovered is the published final LIVE recovery-ready pose. Successful atomic Surface recovery deletes this exact shell, so art cannot leave a persistent recovered marker.
	if (job.remaining_ticks <= 1) { pose.state = 2; pose.frame = 1; return pose; }
	pose.state = 1;
	pose.frame = (job.remaining_ticks / 8) % 2;
	return pose;
}
