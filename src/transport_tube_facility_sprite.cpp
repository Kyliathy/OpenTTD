/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file transport_tube_facility_sprite.cpp Donor native facility animation registration/retirement, independent of packed sprite allocation.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CC.
 * Original rail ANITentries require this owner before resumed ticks; Merge51extends this same owner with typed A Ground family consumers.
 */
#include "stdafx.h"
#include "transport_tube_facility_sprite.h"
#include "station_map.h"
#include "animated_tile_func.h"
#include "viewport_func.h"
#include "timer/timer_game_tick.h"
#include "transport_tube_sprite.h"
#include "ground_sprite.h"
#include "station_base.h"
#include "station_supply.h"
#include "rail_map.h"
#include "train.h"
#include "vehicle_func.h"
#include "safeguards.h"

/** Resolve the original default geometry whose display clock belongs to this owner.
 * @param tile Existing world cell, independently revalidated after lifecycle changes.
 * @return Actual native depot or default platform; custom station renderers keep their own animation owner.
 */
static bool IsTubeFacilityArtTile(TileIndex tile)
{
	if (!IsValidTile(tile)) return false;
	const auto terminal = GetBuiltTubeTerminal(tile, CompanyID{GetTileOwner(tile)});
	if (!terminal.has_value()) return false;
	if (terminal->depot != DepotID::Invalid()) return true;
	// A matching axis bit cannot borrow a custom renderer's full geometry or
	// display lifecycle; only the donor's default platform layout participates.
	return !IsCustomStationSpecIndex(tile) && GetStationGfx(tile) < 8;
}

/** @copydoc RegisterTubeFacilityArtAnimation */
void RegisterTubeFacilityArtAnimation(TileIndex tile)
{
	if (IsTubeFacilityArtTile(tile)) AddAnimatedTile(tile, false);
}

/** @copydoc AnimateTubeFacilityArt */
bool AnimateTubeFacilityArt(TileIndex tile)
{
	if (!IsTubeFacilityArtTile(tile)) return false;
	if (TimerGameTick::counter % 8 == 0) MarkTileDirtyByTile(tile);
	return true;
}

/**
 * Resolve joined conventional freight membership through the original station's actual bounded cells.
 * @param station Sole native packet/order owner shared by any joined freight modes.
 * @return Whether actual conventional rail or truck berths make its tube platform an interchange.
 */
static bool IsTubeArtInterchange(const Station &station)
{
	if (station.truck_stops != nullptr) return true;
	// Native station rectangles can contain gaps and another station's cells.
	// Only exact installed membership supplies conventional interchange identity;
	// nearby independent berths and same-XY layers cannot share packet custody.
	for (TileIndex tile : station.train_station) {
		if (!IsRailStationTile(tile) || GetStationIndex(tile) != station.index) continue; // A rectangle gap or foreign cell is not this station's conventional berth.
		if (!IsTubeRailType(GetRailType(tile))) return true;
	}
	return false;
}

/**
 * Read facility warning/handling state without executing a repair, transfer or electrical opportunity.
 * @param terminal Actual retained native depot/platform identity and owner.
 * @return Authored ready1, working2 or blocked3; instant native construction has no unfinished saved phase0.
 */
static uint8_t GetTubeFacilityVisualState(const TubeTerminal &terminal)
{
	if (!IsTubeTileEnergized(terminal.tile)) return 3;
	bool blocked = false, working = false;
	// Inspect only the native spatial bucket at this actual cell. A parked
	// healthy capsule is ready; only real depot-service demand or the native
	// station loading phase supplies handling animation. No whole-pool scan runs.
	/** @param vehicle Actual spatially indexed part. @return Always false so remaining real occupants are also inspected. */
	HasVehicleOnTile(terminal.tile, [&](const Vehicle *vehicle) {
		if (vehicle->type != VehicleType::Train) return false;
		const Train *part = Train::From(vehicle);
		if (!GetTubeVehicleCapability(part->engine_type).has_value()) return false;
		const Train *head = part->First();
		const auto *binding = GetTubePowerVehicle(head->index);
		// A resident's original selected wait takes precedence over another
		// source lighting the network; energized casing promises no work grant.
		if (binding == nullptr || binding->wait != TubePowerWait::None) {
			blocked = true;
		} else if (head->IsInDepot() && GetTubePowerPhase(*head) == TubePowerPhase::DepotService) { // Only the native damaged-depot demand can request the working maintenance cue.
			working = true;
		} else if (head->current_order.IsType(OT_LOADING)) {
			working = true;
		}
		return false;
	});
	if (terminal.station != StationID::Invalid()) {
		const Station *station = Station::Get(terminal.station);
		// This is the same finite native queue bound, including reservations,
		// used by transfer admission. Full retained stock warns without deleting
		// excess or copying packets to an animation-specific inventory.
		for (const auto &goods : station->goods) {
			if (goods.TotalCount() >= EXACT_STATION_SUPPLY_CAPACITY) blocked = true;
		}
	}
	return blocked ? 3 : working ? 2 : 1;
}

/** @copydoc GetTubeFacilityArt */
std::optional<TubeFacilityArt> GetTubeFacilityArt(TileIndex tile, uint64_t visual_ticks)
{
	if (!IsValidTile(tile)) return std::nullopt;
	const auto terminal = GetBuiltTubeTerminal(tile, CompanyID{GetTileOwner(tile)});
	if (!terminal.has_value()) return std::nullopt;
	GroundSpriteFamily asset;
	uint8_t view = 0;
	uint16_t variant = 0;
	SpriteBounds bounds;
	if (terminal->depot != DepotID::Invalid()) {
		asset = GroundSpriteFamily::FacilityTubeService;
		const DiagDirection door = GetRailDepotDirection(tile);
		view = static_cast<uint8_t>(door); bounds = GetMultiLayerDepotFrontBounds(door);
	} else {
		// Packed platform variants describe default native graphics roles0..3
		// on the two undirected axes. A custom station owns its own layout;
		// neither a fallback axis nor a source canvas may impersonate that role.
		if (IsCustomStationSpecIndex(tile) || GetStationGfx(tile) >= 8) return std::nullopt;
		const Station *station = Station::Get(terminal->station);
		asset = IsTubeArtInterchange(*station) ? GroundSpriteFamily::FacilityTubeInterchange : GroundSpriteFamily::FacilityTubeTerminal;
		view = static_cast<uint8_t>(GetRailStationAxis(tile)); variant = GetStationGfx(tile) / 2;
		// The exact authored foreground contains only the low loading ledge
		// beyond the native centre chord (world y>=9 for X, x>=9 for Y).
		// Its sorting box must not become a tall canopy over an actual capsule;
		// matching opposite offsets retain the original(-31,-33) source anchor.
		bounds = view == 0 ? SpriteBounds{{0, 9, 0}, {16, 7, 4}, {0, -9, 0}} : SpriteBounds{{9, 0, 0}, {7, 16, 4}, {-9, 0, 0}};
	}
	const uint8_t state = GetTubeFacilityVisualState(*terminal);
	const uint8_t frame = GetGroundVisualFrame(asset, visual_ticks);
	const auto back = GetGroundSprite(asset, {state, frame, view, variant, 0});
	const auto front = GetGroundSprite(asset, {state, frame, view, variant, 1});
	if (!back.has_value() || !front.has_value()) return std::nullopt; // A missing complete packed pair must retain native fallback instead of drawing half a facility.
	return TubeFacilityArt{{*back, *front}, bounds};
}
