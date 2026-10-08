/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_cmd.cpp Building, drawing and removing a pair of layer portal tiles. @see docs/zz-reach2/upgrades/2026-10/r2mlm7-layer-portal-foundation.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md, docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "transport_tube.h"
#include "multilayer_sprite.h"
#include "layer_portal_map.h"
#include "excavation.h"
#include "timer/timer_game_calendar.h"
#include "command_func.h"
#include "landscape.h"
#include "landscape_cmd.h"
#include "clear_map.h"
#include "company_base.h"
#include "company_func.h"
#include "company_gui.h"
#include "road_cmd.h"
#include "road_func.h"
#include "road_internal.h"
#include "rail.h"
#include "signal_func.h"
#include "vehicle_func.h"
#include "roadveh.h"
#include "train.h"
#include "newgrf_sound.h"
#include "engine_base.h"
#include "sound_func.h"
#include "bridge_map.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "newgrf_railtype.h"
#include "newgrf_roadtype.h"
#include "map_layer_gui.h"
#include "viewport_func.h"
#include "tbm_exchange.h"

#include "table/sprites.h"
#include "table/strings.h"

#include "safeguards.h"

extern const DiagDirectionIndexArray<uint8_t> _tunnel_visibility_frame;

/**
 * Move a position that has crossed a shaft edge onto the pair tile's entry edge.
 * The layer offset carries the position into the other band. Stepping back one
 * tile along the heading undoes the crossing, so the vehicle sits on the pair.
 * @param portal End whose shaft was just crossed.
 * @param x World X, updated in place.
 * @param y World Y, updated in place.
 */
void TranslateIntoLayerPortalPair(TileIndex portal, int &x, int &y)
{
	assert(IsLayerPortalTile(portal));
	MapLayer from = GetTileLayer(portal);
	MapLayer to = GetLayerPortalTargetLayer(portal);
	//The stacked bands differ by whole planes of world Y.
	y += (static_cast<int>(to_underlying(to)) - static_cast<int>(to_underlying(from))) * static_cast<int>(Map::SizeY()) * static_cast<int>(TILE_SIZE);
	TileIndexDiffC step = TileIndexDiffCByDiagDir(GetLayerPortalShaftDirection(portal));
	x -= step.x * static_cast<int>(TILE_SIZE);
	y -= step.y * static_cast<int>(TILE_SIZE);
	assert(TileVirtXY(x, y) == GetLayerPortalPairTile(portal));
}

/**
 * Distance of a point from the door edge of a portal end, measured along the shaft heading.
 * @param shaft Shaft direction of the end.
 * @param x World X of the point.
 * @param y World Y of the point.
 * @return 0 at the door edge, TILE_SIZE - 1 at the shaft edge.
 */
static uint8_t GetLayerPortalDepth(DiagDirection shaft, int x, int y)
{
	uint8_t pos = static_cast<uint8_t>((DiagDirToAxis(shaft) == Axis::X ? x : y) & TILE_UNIT_MASK);
	//NE and NW head towards smaller coordinates, so their door edge is the high end of the tile.
	if (shaft == DiagDirection::NE || shaft == DiagDirection::NW) return static_cast<uint8_t>(TILE_SIZE - 1 - pos);
	return pos;
}

/**
 * Whether a vehicle at a point on a portal end is inside the shaft, so it is not drawn.
 * The point alone decides, so a turn or a reversal on the end keeps the state right.
 * @param shaft Shaft direction of the end.
 * @param x World X of the point.
 * @param y World Y of the point.
 * @return True from the tunnel visibility frame of that shaft to the shaft edge.
 */
bool IsHiddenInLayerPortal(DiagDirection shaft, int x, int y)
{
	return GetLayerPortalDepth(shaft, x, y) >= _tunnel_visibility_frame[shaft];
}

/** Far fronts cover solid roofs/walls over the whole footprint; near door frames retain their edge sorting. */
static constexpr DiagDirectionIndexArray<SpriteBounds> _layer_portal_front_bounds{{{
	{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 20}, {}},
	{{0, 15, 0}, {16, 1, 20}, {0, -15, 0}},
	{{15, 0, 0}, {1, 16, 20}, {-15, 0, 0}},
	{{0, 0, 0}, {TILE_SIZE, TILE_SIZE, 20}, {}},
}}};

/** Vanilla tunnel roof bounds with north-corner sprite offsets, as used by its catenary (r2mlm13 D-1). */
static constexpr DiagDirectionIndexArray<SpriteBounds> _road_tunnel_front_bounds{{{
	{{0, 1, BB_Z_SEPARATOR}, {TILE_SIZE, TILE_SIZE - 1, 1}, {0, -1, -BB_Z_SEPARATOR}},
	{{1, 0, BB_Z_SEPARATOR}, {TILE_SIZE - 1, TILE_SIZE, 1}, {-1, 0, -BB_Z_SEPARATOR}},
	{{0, 1, BB_Z_SEPARATOR}, {TILE_SIZE, TILE_SIZE - 1, 1}, {0, -1, -BB_Z_SEPARATOR}},
	{{1, 0, BB_Z_SEPARATOR}, {TILE_SIZE - 1, TILE_SIZE, 1}, {-1, 0, -BB_Z_SEPARATOR}},
}}};

/** Vanilla rear separator boxes keep tunnel roofs sorted apart from neighbouring vehicles. */
static constexpr DiagDirectionIndexArray<SpriteBounds> _road_tunnel_rear_sep{{{
	{{}, {TILE_SIZE, 1, TILE_HEIGHT}, {}},
	{{}, {1, TILE_SIZE, TILE_HEIGHT}, {}},
	{{}, {TILE_SIZE, 1, TILE_HEIGHT}, {}},
	{{}, {1, TILE_SIZE, TILE_HEIGHT}, {}},
}}};

/** Vanilla front separator boxes keep the mouth's roof sorted apart from its road approach. */
static constexpr DiagDirectionIndexArray<SpriteBounds> _road_tunnel_front_sep{{{
	{{0, TILE_SIZE - 1, 0}, {TILE_SIZE, 1, TILE_HEIGHT}, {}},
	{{TILE_SIZE - 1, 0, 0}, {1, TILE_SIZE, TILE_HEIGHT}, {}},
	{{0, TILE_SIZE - 1, 0}, {TILE_SIZE, 1, TILE_HEIGHT}, {}},
	{{TILE_SIZE - 1, 0, 0}, {1, TILE_SIZE, TILE_HEIGHT}, {}},
}}};

/**
 * Draw a portal end using generated back/front sprites.
 * Road tunnels cut both parts from one whole picture. Rail/elevator near fronts combine aligned frames with cut roof/wall coverage;
 * their far fronts reuse the whole back picture so vehicles cannot show through solid walls and roofs.
 * Rail and elevator road surfaces retain their type overlays; tunnel backs include the paved approach.
 * @param ti Tile to draw.
 */
static void DrawTile_LayerPortal(TileInfo *ti)
{
	//A whole back draws under vehicles; its mouth/front sorts over them with vanilla tunnel separator boxes.
	if (GetLayerPortalKind(ti->tile) == LayerPortalKind::RoadTunnel) {
		DiagDirection shaft = GetLayerPortalShaftDirection(ti->tile);
		SpriteID base = GetTileLayer(ti->tile) == MapLayer::Surface ? SPR_MAP_LAYER_ROAD_TUNNEL_SURFACE : SPR_MAP_LAYER_ROAD_TUNNEL_UNDERGROUND;
		SpriteID back = base + to_underlying(GetLayerPortalDoorDirection(ti->tile)) * 2;
		DrawGroundSprite(back, PAL_NONE);
		AddSortableSpriteToDraw(back + 1, PAL_NONE, *ti, _road_tunnel_front_bounds[shaft], false);
		AddSortableSpriteToDraw(SPR_EMPTY_BOUNDING_BOX, PAL_NONE, *ti, _road_tunnel_rear_sep[shaft]);
		AddSortableSpriteToDraw(SPR_EMPTY_BOUNDING_BOX, PAL_NONE, *ti, _road_tunnel_front_sep[shaft]);
		return;
	}
	DiagDirection door = GetLayerPortalDoorDirection(ti->tile);
	bool rail = GetLayerPortalTransportType(ti->tile) == TransportType::Rail;
	bool surface = GetTileLayer(ti->tile) == MapLayer::Surface;
	SpriteID base = rail ? (surface ? SPR_MAP_LAYER_RAIL_PORTAL_SURFACE : SPR_MAP_LAYER_RAIL_PORTAL_UNDERGROUND)
	                     : (surface ? SPR_MAP_LAYER_ROAD_ELEVATOR_SURFACE : SPR_MAP_LAYER_ROAD_ELEVATOR_UNDERGROUND);
	SpriteID back = base + to_underlying(door) * 2;
	SpriteID front = back + 1;
	// Read the pair owner's actual work phase without changing track admission,
	// reservations or mouth sorting. Classic and road housings retain their art.
	if (const auto authored = GetMultiLayerPortalSprite(ti->tile, false)) {
		back = *authored;
		front = *GetMultiLayerPortalSprite(ti->tile, true);
	}
	bool invisible = IsInvisibilitySet(TransparencyOption::Buildings);
	bool transparent = IsTransparencySet(TransparencyOption::Buildings);
	Axis axis = DiagDirToAxis(GetLayerPortalShaftDirection(ti->tile));
	DrawGroundSprite(SPR_FLAT_BARE_LAND, PAL_NONE);
	//Both building parts follow the existing building transparency controls.
	if (!invisible) {
		SpriteID image = back;
		if (transparent) SetBit(image, PALETTE_MODIFIER_TRANSPARENT);
		DrawGroundSprite(image, transparent ? PALETTE_TO_TRANSPARENT : PAL_NONE);
	}
	if (rail) {
		//The axis track uses the rail type's own sprites, including monorail and maglev.
		const RailTypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));
		SpriteID track = rti->UsesOverlay() ? GetCustomRailSprite(rti, ti->tile, RailSpriteType::Overlay) + to_underlying(axis)
		                                 : (axis == Axis::X ? rti->base_sprites.single_x : rti->base_sprites.single_y);
		DrawGroundSprite(track, rti->UsesOverlay() ? PAL_NONE : PALETTE_TO_BARE_LAND);
		//L-11: only a reserved rail end receives the reservation tint.
		if (_settings_client.gui.show_track_reservation && HasLayerPortalReservation(ti->tile)) {
			DrawGroundSprite(track, PALETTE_CRASH);
		}
	} else {
		//The road axis replaces the depot layout; road type overlays are still drawn with it.
		RoadType road_rt = GetRoadTypeRoad(ti->tile);
		const RoadTypeInfo *rti = GetRoadTypeInfo(road_rt);
		if (!rti->UsesOverlay()) DrawGroundSprite(SPR_ROAD_Y + to_underlying(axis), PALETTE_TO_BARE_LAND);
		DrawRoadOverlays(ti, PAL_NONE, rti, nullptr, to_underlying(axis), 0);
	}
	if (!invisible) {
		AddSortableSpriteToDraw(front, PAL_NONE, *ti, _layer_portal_front_bounds[door], transparent);
	}
}

/** @copydoc GetSlopePixelZProc */
static int GetSlopePixelZ_LayerPortal(TileIndex tile, uint, uint, bool)
{
	//A portal end is flat, so every point on it has the tile height.
	return TilePixelHeight(tile);
}

/** @copydoc GetTileDescProc */
static void GetTileDesc_LayerPortal(TileIndex tile, TileDesc &td)
{
	switch (GetLayerPortalKind(tile)) {
		case LayerPortalKind::RailTunnel: td.str = STR_LAI_LAYER_PORTAL_RAIL; break;
		case LayerPortalKind::RoadTunnel: td.str = STR_LAI_LAYER_PORTAL_ROAD; break;
		case LayerPortalKind::RoadElevator: td.str = STR_LAI_LAYER_PORTAL_ROAD_ELEVATOR; break;
	}
	td.dparam = GetMapLayerName(GetLayerPortalTargetLayer(tile)).base();
	td.owner[0] = GetTileOwner(tile);
	if (GetLayerPortalTransportType(tile) == TransportType::Rail) {
		const RailTypeInfo *rti = GetRailTypeInfo(GetRailType(tile));
		td.railtype = rti->strings.name;
		td.rail_speed = rti->max_speed;
	} else {
		const RoadTypeInfo *rti = GetRoadTypeInfo(GetRoadTypeRoad(tile));
		td.roadtype = rti->strings.name;
		td.road_speed = rti->max_speed / 2;
	}
}

/**
 * Open native status/connection controls without altering portal state.
 * @param tile Actual clicked housing.
 * @return True when native pair controls handled the click; Classic keeps its previous behavior.
 */
static bool ClickTile_LayerPortal(TileIndex tile)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return false;
	ShowLayerPortalWindow(tile);
	return true;
}

/** @copydoc TileLoopProc */
static void TileLoop_LayerPortal(TileIndex)
{
}

/** @copydoc GetTileTrackStatusProc */
static TrackStatus GetTileTrackStatus_LayerPortal(TileIndex tile, TransportType mode, RoadTramType sub_mode, DiagDirection side)
{
	if (CheckLayerPortalAvailability(tile) != LayerPortalWorkError::None) return {}; // Even a saved Connected Deep pair remains unavailable before the shared technology date.
	if (GetLayerPortalWorkState(tile) != LayerPortalWorkState::Connected) return {}; // A saved shaft can remain physically paired while ordinary paths/signals see no open network edge.
	TransportType transport = GetLayerPortalTransportType(tile);
	if (transport != mode) return {};
	//A road end answers only the road kind it carries. Elevators carry road and never tram, so a tram query finds nothing.
	if (transport == TransportType::Road && !HasTileRoadType(tile, sub_mode)) return {};
	//FD-4: on its own layer an end is reached only through its door. The shaft side answers nothing; a step through the shaft asks with no side.
	if (side != DiagDirection::Invalid && side != GetLayerPortalDoorDirection(tile)) return {};
	TrackdirBits trackdirs = TrackBitsToTrackdirBits(DiagDirToDiagTrack(GetLayerPortalShaftDirection(tile)));
	return {trackdirs, {}};
}

/**
 * Move the infrastructure pieces of a portal pair from one owner to another.
 * FD-6, the rule AfterLoadCompanyStats counts by: rail pieces belong to the tile owner, road pieces to the road owner,
 * which is always the tile owner. Each end counts GetLayerPortalPieceCount pieces, and the lower tile index counts the pair.
 * @param tile The end whose owner change is being processed.
 * @param pair The other end.
 * @param old_owner Owner that loses the pieces.
 * @param new_owner Owner that gains the pieces, or INVALID_OWNER when the pieces are dropped.
 */
static void TransferPortalInfrastructure(TileIndex tile, TileIndex pair, Owner old_owner, Owner new_owner)
{
	//The higher end is skipped, so the pair moves once.
	if (tile > pair) return;
	TransportType transport = GetLayerPortalTransportType(tile);
	uint pieces = GetLayerPortalPieceCount(transport) * 2;
	if (transport == TransportType::Rail) {
		RailType rt = GetRailType(tile);
		//An owner that is not a company has no counters to change.
		if (Company *from = Company::GetIfValid(old_owner); from != nullptr) {
			from->infrastructure.rail[rt] -= pieces;
			DirtyCompanyInfrastructureWindows(old_owner);
		}
		if (Company *to = Company::GetIfValid(new_owner); to != nullptr) {
			to->infrastructure.rail[rt] += pieces;
			DirtyCompanyInfrastructureWindows(new_owner);
		}
	} else {
		//UpdateCompanyRoadInfrastructure skips an owner that is not a company and dirties the window itself.
		RoadType rt = GetRoadTypeRoad(tile);
		UpdateCompanyRoadInfrastructure(rt, old_owner, -static_cast<int>(pieces));
		UpdateCompanyRoadInfrastructure(rt, new_owner, static_cast<int>(pieces));
	}
}

/** @copydoc ChangeTileOwnerProc */
static void ChangeTileOwner_LayerPortal(TileIndex tile, Owner old_owner, Owner new_owner)
{
	//H-2: this proc runs for every tile of the map, so an end that old_owner does not own is left alone.
	if (!IsTileOwner(tile, old_owner)) return;

	TileIndex pair = GetLayerPortalPairTile(tile);
	TransportType transport = GetLayerPortalTransportType(tile);
	if (GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) {
		// A merger moves the pair with its trains, stations, job and project (r2mlvs3 Group 11.4). Company removal needs a
		// safe empty pair: the TBM lifecycle owner has already released its assigned work, and anything else still bound
		// or occupied stays recoverable.
		if (new_owner == INVALID_OWNER && CheckLayerPortalLifecycle(tile) != LayerPortalWorkError::None) return;
		if (new_owner == INVALID_OWNER && transport == TransportType::Rail) {
			[[maybe_unused]] const auto removed = Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, tile);
			assert(removed.Succeeded());
			return;
		}
		const Owner receiver = new_owner == INVALID_OWNER ? OWNER_NONE : new_owner;
		UpdateSignalsInBuffer(); // Finish queued old-owner signals against old geometry before publishing the receiving company's pair and notifications.
		if (TransferLayerPortalWorkPair(tile, receiver) != LayerPortalWorkError::None) return;
		const auto lower = std::min(tile, pair), upper = std::max(tile, pair);
		TransferPortalInfrastructure(lower, upper, old_owner, receiver);
		// Native empty pairs transfer together; the later map-walk callback sees the already changed remote owner and skips a second accounting update.
		for (TileIndex end : {lower, upper}) {
			if (transport == TransportType::Road) { SetRoadOwner(end, RoadTramType::Road, receiver); SetRoadOwner(end, RoadTramType::Tram, OWNER_NONE); }
			SetTileOwner(end, receiver);
		}
		NotifyLayerPortalRoutingChanged(lower);
		return;
	}
	if (new_owner == INVALID_OWNER) {
		//A rail pair is removed together, by the lower end. ClearTile_LayerPortal subtracts its pieces, so nothing is transferred first.
		//The higher end of a rail pair is a Clear tile by the time the map walk reaches it, so its own proc never runs.
		if (transport == TransportType::Rail) {
			if (tile < pair) {
				[[maybe_unused]] CommandCost ret = Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, tile);
				assert(ret.Succeeded());
			}
			return;
		}
		//M-3: a road pair stays and becomes ownerless. The lower end drops the pieces of both ends.
		TransferPortalInfrastructure(tile, pair, old_owner, INVALID_OWNER);
		SetRoadOwner(tile, RoadTramType::Road, OWNER_NONE);
		SetRoadOwner(tile, RoadTramType::Tram, OWNER_NONE);
		SetTileOwner(tile, OWNER_NONE);
		return;
	}

	TransferPortalInfrastructure(tile, pair, old_owner, new_owner);
	//The road owners follow the tile owner, as MakeRoadLayerPortal set them.
	if (transport == TransportType::Road) {
		SetRoadOwner(tile, RoadTramType::Road, new_owner);
		SetRoadOwner(tile, RoadTramType::Tram, new_owner);
	}
	SetTileOwner(tile, new_owner);
}

/** @copydoc TerraformTileProc */
static CommandCost TerraformTile_LayerPortal(TileIndex, DoCommandFlags, int, Slope)
{
	return CommandCost(STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST);
}

/** @copydoc CheckBuildAboveProc */
static CommandCost CheckBuildAbove_LayerPortal(TileIndex, DoCommandFlags, Axis, int)
{
	return CommandCost(STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST);
}

/** Inner point of a magazine by exit side: where a depot keeps a train facing that way (rail_cmd.cpp, _fractcoords_enter). */
static constexpr DiagDirectionIndexArray<Coord2D<uint8_t>> _magazine_inner{{{
	{10, 8}, // NE
	{8, 4}, // SE
	{4, 8}, // SW
	{8, 10}, // NW
}}};

/** Unit step of a part leaving a magazine by exit side (rail_cmd.cpp, _deltacoord_leaveoffset). */
static constexpr DiagDirectionIndexArray<Coord2D<int8_t>> _magazine_leave_step{{{
	{-1, 0}, // NE
	{0, 1}, // SE
	{1, 0}, // SW
	{0, -1}, // NW
}}};

/**
 * Run the depot rules of a magazine for one part of a project train on the key of its Commissioning pair
 * (R2MLVS3MP-M decision 2). Leaving, the part ahead passing the leave point releases the next part, as at a depot
 * exit. Entering, a part that reaches the inner point stands aside in the depot state; the last one readies the train
 * to leave forwards and stops it, so its bay admission sees it whole.
 * @param part Part of the train moving on the tile.
 * @param tile Magazine tile.
 * @param x World X of the part's new position.
 * @param y World Y of the part's new position.
 * @return EnteredWormhole when the part entered the depot state; nothing when the ordinary end rules apply.
 */
static std::optional<VehicleEnterTileStates> RunLayerPortalMagazine(Train *part, TileIndex tile, int x, int y)
{
	const DiagDirection exit = GetLayerPortalMagazineExit(tile, *part->First());
	if (exit == DiagDirection::Invalid) return std::nullopt;
	const Coord2D<uint8_t> fract{static_cast<uint8_t>(x & TILE_UNIT_MASK), static_cast<uint8_t>(y & TILE_UNIT_MASK)};
	const Coord2D<uint8_t> inner = _magazine_inner[exit];
	if (part->GetMovingDirection() == DiagDirToDir(exit)) {
		// Leaving: the next part follows once this one is a whole part's length out, as at a depot exit.
		const int length = part->CalcNextVehicleOffset() + 1;
		const Coord2D<uint8_t> leave{static_cast<uint8_t>(inner.x + length * _magazine_leave_step[exit].x), static_cast<uint8_t>(inner.y + length * _magazine_leave_step[exit].y)};
		if (fract == leave) {
			if (Train *next = part->GetMovingNext(); next != nullptr && next->track == Track::Depot) {
				next->track = AxisToTrack(DiagDirToAxis(exit));
				// FD-3: the point decides visibility, so a part released inside the housing stays hidden.
				next->vehstatus.Set(VehState::Hidden, IsHiddenInLayerPortal(GetLayerPortalShaftDirection(tile), next->x_pos, next->y_pos));
			}
		}
		return std::nullopt;
	}
	if (part->GetMovingDirection() != DiagDirToDir(ReverseDiagDir(exit)) || fract != inner) return std::nullopt;
	// Entering: the part stands aside in the depot state, hidden like a train in a depot.
	part->track = Track::Depot;
	part->vehstatus.Set(VehState::Hidden);
	part->tile = tile;
	if (part->GetMovingNext() == nullptr) {
		Train *consist = part->First();
		// The train leaves forwards next time: one that backed in stops backing, one that drove in turns round.
		if (consist->vehicle_flags.Test(VehicleFlag::DrivingBackwards)) {
			consist->vehicle_flags.Reset(VehicleFlag::DrivingBackwards);
		} else {
			for (Train *u = consist; u != nullptr; u = u->Next()) u->direction = ReverseDir(u->direction);
		}
		consist->cur_speed = 0;
		consist->subspeed = 0;
		// The bay services the boring machine at every visit, as a depot would; it never goes to one.
		if (GetTBMProjectForCutter(consist->index) != nullptr) VehicleServiceInDepot(consist);
	}
	return VehicleEnterTileState::EnteredWormhole;
}

/** @copydoc VehicleEnterTileProc */
static VehicleEnterTileStates VehicleEnterTile_LayerPortal(Vehicle *v, TileIndex tile, int x, int y)
{
	if (CheckLayerPortalAvailability(tile) != LayerPortalWorkError::None) return VehicleEnterTileState::CannotEnter; // Real vehicle admission rechecks both Deep endpoints independently of route caches and force-proceed.
	if (GetLayerPortalWorkState(tile) != LayerPortalWorkState::Connected) {
		const auto *work = GetLayerPortalWorkRecord(tile);
		// Actual movement rechecks the entering part/project; cached paths, force-proceed and unrelated followers cannot bypass sealed authority.
		// Besides the cutter's parts, an assigned hauler of the project's Source bay may use the key (R2MLVS3MP-M).
		const bool admitted = work != nullptr && (CheckLayerPortalWorkAccess(tile, v->index, work->project) == LayerPortalWorkError::None || (v->type == VehicleType::Train && IsTBMHaulerAdmittedAtBay(*Train::From(v)->First(), tile)));
		if (!admitted) return VehicleEnterTileState::CannotEnter;
	}
	TransportType transport = GetLayerPortalTransportType(tile);
	//Each end accepts only its own transport. A road elevator refuses trams.
	if (transport == TransportType::Rail && v->type != VehicleType::Train) return VehicleEnterTileState::CannotEnter;
	if (transport == TransportType::Road) {
		if (v->type != VehicleType::Road) return VehicleEnterTileState::CannotEnter;
		if (RoadTypeIsTram(RoadVehicle::From(v)->roadtype)) return VehicleEnterTileState::CannotEnter;
	}

	// The key of a Commissioning pair is the depot-like magazine of the project's own trains (R2MLVS3MP-M decision 2).
	if (v->type == VehicleType::Train) {
		if (const auto magazine = RunLayerPortalMagazine(Train::From(v), tile, x, y); magazine.has_value()) return *magazine;
	}

	const DiagDirection shaft = GetLayerPortalShaftDirection(tile);
	const DiagDirection vdir = DirToDiagDir(v->GetMovingDirection());

	//FD-1: heading into the shaft, the vehicle came through the door at this end's height. Heading out, it came through the shaft, and the controller sets its new height right after this call.
	if (vdir == shaft && abs(GetSlopePixelZ(x, y, true) - v->z_pos) > 2) return VehicleEnterTileState::CannotEnter;

	//Steam engines play the tunnel sound one step past the door, once per train: only the moving front of a train heading into the shaft triggers it. The track stays the axis track, never a wormhole.
	if (transport == TransportType::Rail && vdir == shaft && v->IsMovingFront() && GetLayerPortalDepth(shaft, x, y) == 1) {
		Train *t = Train::From(v);
		//A NewGRF tunnel sound replaces the built-in one; without it only steam engines are heard, as in a tunnel.
		if (!PlayVehicleSound(t, VehicleSoundEvent::Tunnel) && RailVehInfo(t->engine_type)->engclass == EngineClass::Steam) {
			SndPlayVehicleFx(SND_05_TRAIN_THROUGH_TUNNEL, v);
		}
	}

	//FD-3: the point decides visibility. Only the change from shown to hidden plays the elevator sound.
	bool hidden = IsHiddenInLayerPortal(shaft, x, y);
	if (hidden && !v->vehstatus.Test(VehState::Hidden)) {
		//Road works is the mechanical stand-in for the elevator ride. Only the front of a road vehicle plays it, so an articulated bus is heard once.
		if (GetLayerPortalKind(tile) == LayerPortalKind::RoadElevator && v->IsFrontEngine()) SndPlayTileFx(SND_21_ROAD_WORKS, tile);
		v->vehstatus.Set(VehState::Hidden);
	} else if (!hidden && v->vehstatus.Test(VehState::Hidden)) {
		v->vehstatus.Reset(VehState::Hidden);
	}
	return {};
}

/**
 * Tell signals and the pathfinder that a rail end appeared or disappeared.
 * @param tile Rail portal end.
 * @param owner Owner used for the signal update.
 */
static void NotifyRailPortalChanged(TileIndex tile, Owner owner)
{
	DiagDirection shaft = GetLayerPortalShaftDirection(tile);
	AddSideToSignalBuffer(tile, shaft, owner);
	AddSideToSignalBuffer(tile, ReverseDiagDir(shaft), owner);
	YapfNotifyTrackLayoutChange(tile, DiagDirToDiagTrack(shaft));
}

/**
 * Notify both endpoints only after committed phase/geometry publication.
 * @param tile One actual endpoint of a valid reciprocal pair.
 */
void NotifyLayerPortalRoutingChanged(TileIndex tile)
{
	if (!GetLayerPortalPairKey(tile)) return;
	const auto pair = GetLayerPortalPairTile(tile);
	if (GetLayerPortalTransportType(tile) == TransportType::Rail) {
		NotifyRailPortalChanged(tile, GetTileOwner(tile));
		NotifyRailPortalChanged(pair, GetTileOwner(pair));
	}
	MarkTileDirtyByTile(tile); MarkTileDirtyByTile(pair);
}

/** @copydoc ClearTileProc */
static CommandCost ClearTile_LayerPortal(TileIndex tile, DoCommandFlags flags)
{
	if (flags.Test(DoCommandFlag::Auto)) return CommandCost(STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST);
	if (CheckLayerPortalLifecycle(tile) != LayerPortalWorkError::None) return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);

	TileIndex pair = GetLayerPortalPairTile(tile);
	TransportType transport = GetLayerPortalTransportType(tile);
	//FD-6: a road end follows the road removal rules, so anyone may remove an ownerless elevator. A rail end needs its owner.
	if (transport == TransportType::Road) {
		RoadBits axis_road = AxisToRoadBits(DiagDirToAxis(GetLayerPortalShaftDirection(tile)));
		CommandCost ownership = CheckAllowRemoveRoad(tile, axis_road, GetRoadOwner(tile, RoadTramType::Road), RoadTramType::Road, flags, false);
		if (ownership.Failed()) return ownership;
	} else {
		CommandCost ownership = CheckTileOwnership(tile);
		if (ownership.Failed()) return ownership;
	}

	CommandCost blocked = EnsureNoVehicleOnGround(tile);
	if (blocked.Failed()) return blocked;
	blocked = EnsureNoVehicleOnGround(pair);
	if (blocked.Failed()) return blocked;

	//M-2: only rail ends hold path reservations, and a train may have reserved through either end of the pair.
	//Removing the pair under a reserved path would leave that train driving onto track that is gone, so both ends block the removal.
	if (transport == TransportType::Rail && (HasLayerPortalReservation(tile) || HasLayerPortalReservation(pair))) {
		return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);
	}

	if (flags.Test(DoCommandFlag::Execute)) {
		Owner owner = GetTileOwner(tile);
		DiagDirection shaft = GetLayerPortalShaftDirection(tile);
		if (RetireLayerPortalWorkPair(tile) != LayerPortalWorkError::None) return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);
		//FD-6: rail pieces belong to the tile owner and road pieces to the road owner, the rule AfterLoadCompanyStats counts by.
		uint pieces = GetLayerPortalPieceCount(transport) * 2;
		if (transport == TransportType::Rail) {
			if (Company *c = Company::GetIfValid(owner); c != nullptr) {
				c->infrastructure.rail[GetRailType(tile)] -= pieces;
				DirtyCompanyInfrastructureWindows(owner);
			}
		} else {
			//UpdateCompanyRoadInfrastructure skips an owner that is not a company.
			UpdateCompanyRoadInfrastructure(GetRoadTypeRoad(tile), GetRoadOwner(tile, RoadTramType::Road), -static_cast<int>(pieces));
		}
		DoClearSquare(tile);
		DoClearSquare(pair);
		if (transport == TransportType::Rail) {
			//H-1: both ends are Clear now, so the saved shaft gives the doors. The pair's door faces the clicked end's shaft direction.
			Track track = DiagDirToDiagTrack(shaft);
			AddSideToSignalBuffer(tile, ReverseDiagDir(shaft), owner);
			AddSideToSignalBuffer(pair, shaft, owner);
			YapfNotifyTrackLayoutChange(tile, track);
			YapfNotifyTrackLayoutChange(pair, track);
		}
	}

	//One price for the pair, the same multiple used when the pair was built.
	return CommandCost(ExpensesType::Construction, _price[Price::ClearTunnel] * 4);
}

/**
 * Whether one end can hold a portal.
 * @param tile End to test.
 * @param shaft_dir Shaft direction of that end.
 * @return An empty cost, or the reason this end is unsuitable.
 */
static CommandCost CheckPortalEnd(TileIndex tile, DiagDirection shaft_dir)
{
	if (!IsValidTile(tile) || !IsInnerTile(tile)) return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	if (GetTileSlope(tile) != SLOPE_FLAT) return CommandCost(STR_ERROR_FLAT_LAND_REQUIRED);
	if (IsBridgeAbove(tile)) return CommandCost(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
	if (CommandCost layer = CheckLayerAllowsConstruction(tile, MapLayerConstruction::LayerPortal); layer.Failed()) return layer;
	//The door needs a neighbour that is still inside the map, so a road or track can arrive.
	TileIndex door_neighbour = TileAddByDiagDir(tile, ReverseDiagDir(shaft_dir));
	if (!IsInnerTile(door_neighbour)) return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	return CheckExcavationConstruction(std::array{tile, door_neighbour});
}

/** Actual two-end proposal; ordinary access floor and restricted target housing remain separate domains. */
struct NativePortalBuildProposal {
	CommandCost cost{ExpensesType::Construction}; ///< Complete readonly clearing/housing quote.
	TileIndex source = INVALID_TILE, target = INVALID_TILE; ///< Actual reciprocal physical endpoints.
	bool target_solid = false; ///< Scoped metadata housing on persistent solid rock, never general excavation.
	uint8_t clearing_count = 0; ///< Ordinary clear operations whose shared company budget must fit before either executes.
};

/**
 * Validate a native pair completely without clearing, reserving, charging or touching RNG.
 * @param tile Actual accessible source endpoint.
 * @param pair Actual destination endpoint on its supported plane.
 * @param shaft Source shaft direction; opposite end uses its reverse.
 * @param transport Actual rail/road family.
 * @param flags Caller command policy; Execute is removed for all quotes.
 * @return Complete readonly proposal with cost/refusal.
 */
static NativePortalBuildProposal PrepareNativePortalBuild(TileIndex tile, TileIndex pair, DiagDirection shaft, TransportType transport, DoCommandFlags flags)
{
	NativePortalBuildProposal result;
	result.source = tile; result.target = pair;
	if (!Company::IsValidID(_current_company)) { result.cost = CMD_ERROR; return result; }
	if ((GetTileLayer(tile) == MapLayer::DeepUnderground || GetTileLayer(pair) == MapLayer::DeepUnderground) && !CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) { result.cost = CommandCost(STR_ERROR_EXCAVATION_DEEP_LOCKED); return result; } // Both build directions use the same saved calendar authority before any geometry/clearing quote.
	// Native paired mutation supports prepared plain-ground end sites; existing infrastructure/objects must be explicitly cleared through their own safe commands first.
	for (TileIndex end : {tile, pair}) {
		if (!IsValidTile(end) || !IsInnerTile(end) || GetTileType(end) != TileType::Clear) { result.cost = CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL); return result; }
		if (GetTileSlope(end) != SLOPE_FLAT) { result.cost = CommandCost(STR_ERROR_FLAT_LAND_REQUIRED); return result; }
		if (IsBridgeAbove(end)) { result.cost = CommandCost(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST); return result; }
		if (!MultiLayerMap::GetRules(GetTileLayer(end)).allowed.Test(MapLayerConstruction::LayerPortal)) { result.cost = CommandCost(STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER); return result; }
		if (CommandCost vehicle = EnsureNoVehicleOnGround(end); vehicle.Failed()) { result.cost = vehicle; return result; }
	}
	// Only inner endpoints can derive door neighbours safely; edge clicks must refuse before address arithmetic.
	const auto source_step = TileIndexDiffCByDiagDir(ReverseDiagDir(shaft));
	const auto target_step = TileIndexDiffCByDiagDir(shaft);
	const auto source_door = TileAddWrap(tile, source_step.x, source_step.y);
	const auto target_door = TileAddWrap(pair, target_step.x, target_step.y);
	// Approaches must remain inner compatible floor without conflicting vehicles/owners; solid destination rock stays a sealed face.
	for (TileIndex door : {source_door, target_door}) {
		if (!IsValidTile(door) || !IsInnerTile(door)) { result.cost = CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL); return result; }
		const auto type = GetTileType(door);
		const bool compatible = type == TileType::Clear || (transport == TransportType::Rail && type == TileType::Railway) || (transport == TransportType::Road && type == TileType::Road);
		if (!compatible || (type != TileType::Clear && GetTileOwner(door) != _current_company)) { result.cost = CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL); return result; }
		if (CommandCost vehicle = EnsureNoVehicleOnGround(door); vehicle.Failed()) { result.cost = vehicle; return result; }
	}
	if (CommandCost access = CheckExcavationConstruction(std::array{tile, source_door}); access.Failed()) { result.cost = access; return result; }
	const auto &geology = GetGeologyWorldState();
	// Every target cell must be valid unclaimed geology; only solid Clear ground skips ordinary clearing and receives sealed housing metadata.
	for (TileIndex cell : {pair, target_door}) {
		if (geology.job_claims.contains(cell)) { result.cost = CommandCost(STR_ERROR_EXCAVATION_ACTIVE_CUT); return result; }
		const auto terrain = geology.terrain.Get(cell);
		if (terrain == ExcavationState::Excavating) { result.cost = CommandCost(STR_ERROR_EXCAVATION_ACTIVE_CUT); return result; }
		if (terrain == ExcavationState::Solid) {
			if (GetTileType(cell) != TileType::Clear) { result.cost = CommandCost(STR_ERROR_EXCAVATION_SOLID_ROCK); return result; }
			if (cell == pair) result.target_solid = true;
			continue;
		}
		if (CommandCost access = CheckExcavationConstruction(std::array{cell}); access.Failed()) { result.cost = access; return result; }
	}
	flags.Reset(DoCommandFlag::Execute);
	const auto source_clear = Command<Commands::LandscapeClear>::Do(flags, tile);
	if (source_clear.Failed()) { result.cost = source_clear; return result; }
	result.cost.AddCost(source_clear.GetCost()); ++result.clearing_count;
	if (!result.target_solid) {
		const auto target_clear = Command<Commands::LandscapeClear>::Do(flags, pair);
		if (target_clear.Failed()) { result.cost = target_clear; return result; }
		result.cost.AddCost(target_clear.GetCost()); ++result.clearing_count;
	}
	// A direct/API caller without Auto must fit the combined two-end clearing budget before either endpoint consumes its first allowance.
	if (!flags.Any({DoCommandFlag::Auto, DoCommandFlag::Bankrupt}) && (Company::Get(_current_company)->clear_limit >> 16) < result.clearing_count) { result.cost = CommandCost(STR_ERROR_CLEARING_LIMIT_REACHED); return result; }
	result.cost.AddCost(_price[Price::BuildTunnel] * 4);
	if (GetLayerPortalWorkPairCount() >= MAX_LAYER_PORTAL_WORK_PAIRS) result.cost = CommandCost(STR_ERROR_TOO_MANY_OBJECTS);
	return result;
}

/** Ephemeral public-map-field backup used only for rollback of the two prepared plain-ground endpoints. */
struct PortalBuildTileBackup {
	TileIndex tile; ///< Actual map address.
	std::array<uint8_t, 8> bytes; ///< Exact public type/height/m1/m3/m4/m5/m6/m7 fields.
	uint16_t m2, m8; ///< Exact public wider fields, never private tile-array copies.
	/** @param index Actual prepared endpoint to restore if coordinated publication fails. */
	explicit PortalBuildTileBackup(TileIndex index) : tile(index), bytes{Tile{index}.type(), Tile{index}.height(), Tile{index}.m1(), Tile{index}.m3(), Tile{index}.m4(), Tile{index}.m5(), Tile{index}.m6(), Tile{index}.m7()}, m2(Tile{index}.m2()), m8(Tile{index}.m8()) {}
	/** Restore only this actual endpoint's public fields; no geological, actor or inventory owner is copied. */
	void Restore() const noexcept
	{
		Tile t{tile};
		t.type() = bytes[0]; t.height() = bytes[1]; t.m1() = bytes[2]; t.m3() = bytes[3]; t.m4() = bytes[4]; t.m5() = bytes[5]; t.m6() = bytes[6]; t.m7() = bytes[7];
		t.m2() = m2; t.m8() = m8;
	}
};

/**
 * Execute a complete native housing proposal after preallocating actual pair-owner capacity.
 * @param proposal Complete readonly source/target/clearing proposal.
 * @param flags Actual estimate/execute policy.
 * @param transport Actual rail/road housing family.
 * @param shaft Actual source shaft direction.
 * @param railtype Actual validated rail type.
 * @param roadtype Actual validated road type.
 * @return Complete quote or unchanged preparation/commit refusal.
 */
static CommandCost BuildNativePortalPair(const NativePortalBuildProposal &proposal, DoCommandFlags flags, TransportType transport, DiagDirection shaft, RailType railtype, RoadType roadtype)
{
	if (proposal.cost.Failed()) return proposal.cost;
	if (!flags.Test(DoCommandFlag::Execute)) return proposal.cost;
	auto capacity = PrepareLayerPortalWorkPair(std::min(proposal.source, proposal.target), _current_company);
	if (!capacity.Ready()) return CommandCost(STR_ERROR_TOO_MANY_OBJECTS);
	const PortalBuildTileBackup source_backup(proposal.source), target_backup(proposal.target);
	const auto clearing_budget = Company::Get(_current_company)->clear_limit;
	const auto source_clear = Command<Commands::LandscapeClear>::Do(flags, proposal.source);
	if (source_clear.Failed()) return source_clear;
	if (!proposal.target_solid) {
		const auto target_clear = Command<Commands::LandscapeClear>::Do(flags, proposal.target);
		if (target_clear.Failed()) {
			source_backup.Restore(); target_backup.Restore();
			Company::Get(_current_company)->clear_limit = clearing_budget;
			return target_clear;
		}
	}
	if (transport == TransportType::Rail) {
		MakeRailLayerPortal(proposal.source, _current_company, shaft, GetTileLayer(proposal.target), railtype);
		MakeRailLayerPortal(proposal.target, _current_company, ReverseDiagDir(shaft), GetTileLayer(proposal.source), railtype);
	} else {
		MakeRoadLayerPortal(proposal.source, _current_company, shaft, GetTileLayer(proposal.target), roadtype);
		MakeRoadLayerPortal(proposal.target, _current_company, ReverseDiagDir(shaft), GetTileLayer(proposal.source), roadtype);
	}
	if (!capacity.Publish()) {
		source_backup.Restore(); target_backup.Restore();
		Company::Get(_current_company)->clear_limit = clearing_budget;
		return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	}
	// Infrastructure and notifications follow both geometry and owner publication; a failed preparation leaves neither accounting nor a half-pair.
	if (transport == TransportType::Rail) {
		Company::Get(_current_company)->infrastructure.rail[railtype] += GetLayerPortalPieceCount(transport) * 2;
		NotifyRailPortalChanged(proposal.source, _current_company);
		NotifyRailPortalChanged(proposal.target, _current_company);
	} else {
		UpdateCompanyRoadInfrastructure(roadtype, _current_company, static_cast<int>(GetLayerPortalPieceCount(transport) * 2));
	}
	DirtyCompanyInfrastructureWindows(_current_company);
	MarkTileDirtyByTile(proposal.source); MarkTileDirtyByTile(proposal.target);
	return proposal.cost;
}

/**
 * Build both ends of a layer portal, or change nothing.
 * The price is four times a tunnel, chosen because a portal replaces a short tunnel between two layers.
 * @param flags Command flags. Nothing is written unless Execute is set.
 * @param tile Clicked end.
 * @param transport_type Rail or Road.
 * @param target_layer Layer of the other end.
 * @param shaft_dir Heading down the shaft from the clicked end.
 * @param railtype Rail type for a rail portal.
 * @param roadtype Road type for a road portal. Tram types are refused.
 * @return The cost, or the error.
 */
CommandCost CmdBuildLayerPortal(DoCommandFlags flags, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype)
{
	// Tube guideways never become commissioning shafts or cross-layer capsule
	// routes; reject their semantic type before constructing either endpoint.
	if (transport_type == TransportType::Rail && IsTubeRailType(railtype)) return CMD_ERROR;
	if (transport_type == TransportType::Rail) {
		if (!ValParamRailType(railtype)) return CMD_ERROR;
	} else if (transport_type == TransportType::Road) {
		//An elevator carries road vehicles only: the type must exist for this company and must not be a tram type.
		if (!ValParamRoadType(roadtype) || !RoadTypeIsRoad(roadtype)) return CMD_ERROR;
	} else {
		return CMD_ERROR;
	}
	if (to_underlying(shaft_dir) > to_underlying(DiagDirection::NW)) return CMD_ERROR;
	//M-11: reject invalid targets and unsupported kind/Layer pairs before TileInLayer sees the target.
	if (!IsValidTile(tile)) return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	MapLayer from = GetTileLayer(tile);
	LayerPortalKind kind = transport_type == TransportType::Rail ? LayerPortalKind::RailTunnel
		: (from == MapLayer::DeepUnderground || target_layer == MapLayer::DeepUnderground ? LayerPortalKind::RoadElevator : LayerPortalKind::RoadTunnel);
	if (!MultiLayerMap::IsValidLayer(target_layer) || !MultiLayerMap::IsPortalPairAllowed(kind, from, target_layer)) {
		return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	}

	TileIndex pair = MultiLayerMap::TileInLayer(tile, target_layer);
	DiagDirection pair_shaft = ReverseDiagDir(shaft_dir);
	if (GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) {
		return BuildNativePortalPair(PrepareNativePortalBuild(tile, pair, shaft_dir, transport_type, flags), flags, transport_type, shaft_dir, railtype, roadtype);
	}
	if (CommandCost end = CheckPortalEnd(tile, shaft_dir); end.Failed()) return end;
	if (CommandCost end = CheckPortalEnd(pair, pair_shaft); end.Failed()) return end;

	CommandCost cost(ExpensesType::Construction);
	CommandCost cleared = Command<Commands::LandscapeClear>::Do(flags, tile);
	if (cleared.Failed()) return cleared;
	cost.AddCost(std::move(cleared));
	cleared = Command<Commands::LandscapeClear>::Do(flags, pair);
	if (cleared.Failed()) return cleared;
	cost.AddCost(std::move(cleared));
	cost.AddCost(_price[Price::BuildTunnel] * 4);

	if (flags.Test(DoCommandFlag::Execute)) {
		if (transport_type == TransportType::Rail) {
			MakeRailLayerPortal(tile, _current_company, shaft_dir, target_layer, railtype);
			MakeRailLayerPortal(pair, _current_company, pair_shaft, GetTileLayer(tile), railtype);
			Company::Get(_current_company)->infrastructure.rail[railtype] += GetLayerPortalPieceCount(transport_type) * 2;
		} else {
			MakeRoadLayerPortal(tile, _current_company, shaft_dir, target_layer, roadtype);
			MakeRoadLayerPortal(pair, _current_company, pair_shaft, GetTileLayer(tile), roadtype);
			UpdateCompanyRoadInfrastructure(roadtype, _current_company, static_cast<int>(GetLayerPortalPieceCount(transport_type) * 2));
		}
		DirtyCompanyInfrastructureWindows(_current_company);
		MarkTileDirtyByTile(tile);
		MarkTileDirtyByTile(pair);
		if (transport_type == TransportType::Rail) {
			NotifyRailPortalChanged(tile, _current_company);
			NotifyRailPortalChanged(pair, _current_company);
		}
	}
	return cost;
}

/**
 * Adapt the canonical pair-state transaction to the existing synchronized command kernel.
 * @param flags Actual estimate/execute policy.
 * @param tile Actual endpoint.
 * @param connected Requested ordinary connection state.
 * @return Zero-cost success or a concrete unchanged refusal.
 */
CommandCost CmdChangeLayerPortalConnection(DoCommandFlags flags, TileIndex tile, bool connected)
{
	const auto result = ChangeLayerPortalConnection(tile, _current_company, connected, flags.Test(DoCommandFlag::Execute));
	switch (result) {
		case LayerPortalWorkError::None: return CommandCost(ExpensesType::Construction);
		case LayerPortalWorkError::LockedDeep: return CommandCost(STR_ERROR_EXCAVATION_DEEP_LOCKED);
		case LayerPortalWorkError::MissingExcavation: return CommandCost(STR_ERROR_EXCAVATION_SOLID_ROCK);
		case LayerPortalWorkError::InvalidOwner: return CommandCost(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
		case LayerPortalWorkError::Conflict: return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);
		default: return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	}
}

/**
 * Play the construction sound at the clicked end.
 * @param cmd Command that finished.
 * @param result Result of the command.
 * @param tile Clicked end.
 * @param transport_type Unused transport argument.
 * @param target_layer Unused target argument.
 * @param shaft_dir Unused direction argument.
 * @param railtype Unused rail type argument.
 * @param roadtype Unused road type argument.
 */
/**
 * Finish visible portal construction by connecting existing road stubs, as ordinary road depots/stops do.
 * @param command Completed command identity.
 * @param result Actual portal construction receipt; refusals must not post follow-up work.
 * @param tile Actual clicked endpoint, on either layer.
 * @param transport Built transport family; rail approaches retain their ordinary track controls.
 * @param target Requested other layer, already validated by the command.
 * @param shaft Requested shaft direction, already present in the actual endpoint geometry.
 * @param railtype Requested rail type, unused for road-stub completion.
 * @param roadtype Command-captured road type; later toolbar selection cannot change this construction.
 */
void CcBuildLayerPortal([[maybe_unused]] Commands command, const CommandCost &result, TileIndex tile, TransportType transport,
	[[maybe_unused]] MapLayer target, [[maybe_unused]] DiagDirection shaft, [[maybe_unused]] RailType railtype, RoadType roadtype)
{
	if (result.Failed()) return;
	SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, tile);
	if (transport != TransportType::Road) return;
	// A newly placed structure supplies the missing outer bit to an existing road stub; it never builds through solid rock or connects the sealed pair.
	for (TileIndex end : {tile, GetLayerPortalPairTile(tile)}) {
		const TileIndex approach = GetLayerPortalDoorTile(end);
		// Only existing normal roads receive the same paid, command-validated completion as a depot entrance; empty floor remains a player construction task.
		if (!IsNormalRoadTile(approach) || !GetRoadBits(approach, GetRoadTramType(roadtype)).Any()) continue;
		Command<Commands::BuildRoad>::Post(approach, DiagDirToRoadBits(GetLayerPortalShaftDirection(end)), roadtype, {}, TownID::Invalid());
	}
}

/** TileTypeProcs for TileType::LayerPortal. */
extern const TileTypeProcs _tile_type_layer_portal_procs = {
	.draw_tile_proc = DrawTile_LayerPortal,
	.get_slope_pixel_z_proc = GetSlopePixelZ_LayerPortal,
	.clear_tile_proc = ClearTile_LayerPortal,
	.get_tile_desc_proc = GetTileDesc_LayerPortal,
	.get_tile_track_status_proc = GetTileTrackStatus_LayerPortal,
	.click_tile_proc = ClickTile_LayerPortal,
	.tile_loop_proc = TileLoop_LayerPortal,
	.change_tile_owner_proc = ChangeTileOwner_LayerPortal,
	.vehicle_enter_tile_proc = VehicleEnterTile_LayerPortal,
	.terraform_tile_proc = TerraformTile_LayerPortal,
	.check_build_above_proc = CheckBuildAbove_LayerPortal,
};
