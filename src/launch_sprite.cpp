/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_sprite.cpp Cargo-rocket presentation reads original phase, burn and geometry owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 * Source states and cadence are frozen in cargo-rocket-state-model-v1.json;
 * these queries never start work, request an approach, consume fuel or reveal a hull.
 */
#include "stdafx.h"
#include "launch_sprite.h"
#include "space_vehicle.h"
#include "space_transition.h"
#include "space_sprite.h"
#include "space_approach.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "safeguards.h"

/** @copydoc GetLaunchRocketArtDirection */
Direction GetLaunchRocketArtDirection(const SpaceVehicle &craft)
{
	const auto fallback = craft.direction < Direction::End ? craft.direction : Direction::NE;
	if (GetSpaceVehicleModel(craft) != 14) return fallback;
	if (GetSpaceVehicleLayer(craft) == MapLayer::Surface) {
		const auto *center = ResolveLaunchCenter(craft.construction.home);
		// Only the current owned home supplies physical pad orientation. A stale
		// station generation cannot turn an unrelated facility into this hull's home.
		if (center != nullptr && center->owner == craft.owner && center->launch_center->rotation < 4) {
			return static_cast<Direction>(to_underlying(Direction::NE) + 2 * center->launch_center->rotation);
		}
		return fallback;
	}
	if (!craft.transition) return fallback;
	const auto &flight = *craft.transition;
	if (flight.phase == SpaceFlightPhase::ReturnRecovery) {
		// Recovery interpolation uses this original departure and home even after
		// home loss. Retained burn/movement receipts do not create a new destination.
		return GetSpaceArtDirection(flight.departure, craft.home, fallback);
	}
	if (flight.phase == SpaceFlightPhase::OrbitalDelivery) {
		SpacePoint destination;
		if (craft.space_orders.approach) {
			const auto block = GetSpaceApproachDestination(craft, destination);
			// Both admitted travel and actual safe staging have native geometry;
			// artwork never acquires the receiver's shared reservation.
			if (block == SpaceOrderBlock::None || block == SpaceOrderBlock::ApproachBusy) return GetSpaceArtDirection(craft.position, destination, fallback);
		}
		if (ResolveSpaceNavigationTarget(craft, flight.destination, destination) == SpaceOrderBlock::None) return GetSpaceArtDirection(craft.position, destination, fallback);
	}
	return fallback;
}

/** @copydoc GetLaunchRocketArtPose */
std::optional<LaunchArtPose> GetLaunchRocketArtPose(const SpaceVehicle &craft, Direction direction)
{
	// This source belongs only to the original paid rocket. Neither another
	// role nor an invalid heading may borrow a neighbouring launch sprite.
	if (GetSpaceVehicleModel(craft) != 14 || direction >= Direction::End) return std::nullopt;
	LaunchArtPose pose; pose.direction = to_underlying(direction);
	if (!craft.transition) return pose;
	const auto &flight = *craft.transition;
	// Unknown saved enums are missing coverage, not an invented ready hull.
	if (flight.phase >= SpaceFlightPhase::End || flight.blocked >= SpaceFlightBlock::End) return std::nullopt;
	if (flight.phase == SpaceFlightPhase::GroundLoading) return pose;
	const auto flight_ticks = GetMultiLayerSpaceBudget().flight_ticks;
	if (flight.phase == SpaceFlightPhase::Ascent) {
		// Funded ascent continues under native target/home loss. Only its actual
		// outbound receipt and positive bounded countdown permit engine pixels.
		const bool funded = flight.outbound_burned && !flight.recovery_burned && flight.remaining_ticks > 0 && flight.remaining_ticks <= flight_ticks;
		pose.state = funded ? 2 : 5;
	} else if (flight.phase == SpaceFlightPhase::ReturnRecovery) {
		// The existing recovery leg still moves with a lost home. Both once-only
		// burns are required; an unfunded or exhausted leg cannot display thrust.
		const bool funded = flight.outbound_burned && flight.recovery_burned && flight.remaining_ticks > 0 && flight.remaining_ticks <= flight_ticks;
		pose.state = funded ? 4 : 5;
	} else if (flight.phase == SpaceFlightPhase::Holding || (flight.blocked != SpaceFlightBlock::None && flight.blocked != SpaceFlightBlock::Approach)) {
		// Real holding/receiver refusals keep a static sealed body. Approach is
		// native admitted movement and retains the orbital source state.
		pose.state = 5;
	} else {
		switch (flight.phase) {
			case SpaceFlightPhase::Preparing: pose.state = 1; break;
			case SpaceFlightPhase::OrbitalDelivery: pose.state = 3; break;
			case SpaceFlightPhase::Turnaround: pose.state = 6; break;
			default: return std::nullopt;
		}
	}
	// Only genuine active engine parts animate. Static states stay on frame0;
	// repeated drawing cannot earn a native opportunity or allocate another timer.
	if (pose.state == 2 || pose.state == 4) pose.frame = static_cast<uint8_t>(((flight_ticks - flight.remaining_ticks) / 8) % 2);
	return pose;
}
