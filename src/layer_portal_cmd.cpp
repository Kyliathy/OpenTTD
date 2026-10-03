/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_cmd.cpp Building, drawing and removing a pair of layer portal tiles. @see docs/zz-reach2/upgrades/2026-10/r2mlm7-layer-portal-foundation.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "command_func.h"
#include "landscape.h"
#include "landscape_cmd.h"
#include "clear_map.h"
#include "company_base.h"
#include "company_func.h"
#include "company_gui.h"
#include "road_cmd.h"
#include "road_func.h"
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

#include "table/track_land.h"
/** One sprite of a road tile layout. table/road_land.h needs it; the layout is the one in road_cmd.cpp. */
struct DrawRoadTileStruct {
	uint16_t image; ///< Sprite of this part of the layout.
	uint8_t subcoord_x; ///< X offset of the sprite within the tile.
	uint8_t subcoord_y; ///< Y offset of the sprite within the tile.
};

#include "table/road_land.h"
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
 * Draw a portal end with a depot stand-in until the layer art exists.
 * Rail art is swapped by r2mlm9. Road art is swapped by r2mlm8 Group 7.
 * @param ti Tile to draw.
 */
static void DrawTile_LayerPortal(TileInfo *ti)
{
	DiagDirection door = GetLayerPortalDoorDirection(ti->tile);
	PaletteID palette = GetCompanyPalette(GetTileOwner(ti->tile));
	if (GetLayerPortalTransportType(ti->tile) == TransportType::Rail) {
		//Barren ground and the axis track, then the rail depot building with its opening on the mouth.
		const RailTypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));
		DrawGroundSprite(SPR_FLAT_BARE_LAND, PAL_NONE);
		SpriteID track = (DiagDirToAxis(GetLayerPortalShaftDirection(ti->tile)) == Axis::X) ? rti->base_sprites.single_x : rti->base_sprites.single_y;
		DrawGroundSprite(track, PALETTE_TO_BARE_LAND);
		const DrawTileSprites *dts = &_depot_gfx_table[door];
		DrawRailTileSeq(ti, dts, TransparencyOption::Buildings, rti->GetRailtypeSpriteOffset(), 0, palette);
	} else {
		//The road depot layout faces the door. Road type overlays are drawn with it.
		DrawGroundSprite(SPR_FLAT_BARE_LAND, PAL_NONE);
		const DrawTileSprites *dts = &_road_depot[door];
		RoadType road_rt = GetRoadTypeRoad(ti->tile);
		const RoadTypeInfo *rti = GetRoadTypeInfo(road_rt);
		int relocation = GetCustomRoadSprite(rti, ti->tile, RoadSpriteType::Depot);
		if (relocation != 0) relocation -= SPR_ROAD_DEPOT;
		DrawRailTileSeq(ti, dts, TransparencyOption::Buildings, relocation, 0, palette);
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
	td.str = GetLayerPortalTransportType(tile) == TransportType::Rail ? STR_LAI_LAYER_PORTAL_RAIL : STR_LAI_LAYER_PORTAL_ROAD;
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

/** @copydoc TileLoopProc */
static void TileLoop_LayerPortal(TileIndex)
{
}

/** @copydoc GetTileTrackStatusProc */
static TrackStatus GetTileTrackStatus_LayerPortal(TileIndex tile, TransportType mode, RoadTramType sub_mode, DiagDirection side)
{
	TransportType transport = GetLayerPortalTransportType(tile);
	if (transport != mode) return {};
	if (transport == TransportType::Road && !HasTileRoadType(tile, sub_mode)) return {};
	//The axis is open in both directions. A query from a perpendicular side finds nothing.
	if (side != DiagDirection::Invalid && side != GetLayerPortalDoorDirection(tile) && side != GetLayerPortalShaftDirection(tile)) return {};
	TrackdirBits trackdirs = TrackBitsToTrackdirBits(DiagDirToDiagTrack(GetLayerPortalShaftDirection(tile)));
	return {trackdirs, {}};
}

/**
 * Move infrastructure for both ends from one owner to another.
 * Only the lower tile index does this, so the pair is not counted twice.
 * @param tile The end whose owner change is being processed.
 * @param pair The other end.
 * @param old_owner Previous owner.
 * @param new_owner New owner, or INVALID_OWNER when the company is gone.
 */
static void TransferPortalInfrastructure(TileIndex tile, TileIndex pair, Owner old_owner, Owner new_owner)
{
	if (tile > pair) return;
	TransportType transport = GetLayerPortalTransportType(tile);
	uint pieces = LAYER_PORTAL_TRACKBIT_FACTOR(transport) * 2;
	if (!Company::IsValidID(old_owner)) return;
	if (transport == TransportType::Rail) {
		RailType rt = GetRailType(tile);
		Company::Get(old_owner)->infrastructure.rail[rt] -= pieces;
		if (Company::IsValidID(new_owner)) Company::Get(new_owner)->infrastructure.rail[rt] += pieces;
	} else {
		RoadType rt = GetRoadTypeRoad(tile);
		UpdateCompanyRoadInfrastructure(rt, old_owner, -static_cast<int>(pieces));
		if (Company::IsValidID(new_owner)) UpdateCompanyRoadInfrastructure(rt, new_owner, static_cast<int>(pieces));
	}
	DirtyCompanyInfrastructureWindows(old_owner);
	if (Company::IsValidID(new_owner)) DirtyCompanyInfrastructureWindows(new_owner);
}

/** @copydoc ChangeTileOwnerProc */
static void ChangeTileOwner_LayerPortal(TileIndex tile, Owner old_owner, Owner new_owner)
{
	TileIndex pair = GetLayerPortalPairTile(tile);
	TransferPortalInfrastructure(tile, pair, old_owner, new_owner);

	if (GetLayerPortalTransportType(tile) == TransportType::Road && GetRoadOwner(tile, RoadTramType::Road) == old_owner) {
		SetRoadOwner(tile, RoadTramType::Road, new_owner == INVALID_OWNER ? OWNER_NONE : new_owner);
	}
	if (!IsTileOwner(tile, old_owner)) return;

	//A rail pair is removed together. A road pair keeps both ends and drops the owner.
	if (new_owner == INVALID_OWNER) {
		if (GetLayerPortalTransportType(tile) == TransportType::Rail) {
			if (tile < pair) {
				[[maybe_unused]] CommandCost ret = Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, tile);
				assert(ret.Succeeded());
			}
		} else {
			SetTileOwner(tile, OWNER_NONE);
		}
		return;
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

/** @copydoc VehicleEnterTileProc */
static VehicleEnterTileStates VehicleEnterTile_LayerPortal(Vehicle *v, TileIndex tile, int x, int y)
{
	int z = GetSlopePixelZ(x, y, true) - v->z_pos;
	if (abs(z) > 2) return VehicleEnterTileState::CannotEnter;

	TransportType transport = GetLayerPortalTransportType(tile);
	//Each end accepts only its own transport. A road elevator refuses trams.
	if (transport == TransportType::Rail && v->type != VehicleType::Train) return VehicleEnterTileState::CannotEnter;
	if (transport == TransportType::Road) {
		if (v->type != VehicleType::Road) return VehicleEnterTileState::CannotEnter;
		if (RoadTypeIsTram(RoadVehicle::From(v)->roadtype)) return VehicleEnterTileState::CannotEnter;
	}

	const DiagDirection shaft = GetLayerPortalShaftDirection(tile);
	const DiagDirection vdir = DirToDiagDir(v->GetMovingDirection());
	uint8_t pos = (DiagDirToAxis(vdir) == Axis::X ? x : y) & TILE_UNIT_MASK;
	uint8_t frame = (vdir == DiagDirection::NE || vdir == DiagDirection::NW) ? TILE_SIZE - 1 - pos : pos;

	//Steam engines play the tunnel sound as they enter the mouth. The track stays the axis track, never a wormhole.
	if (transport == TransportType::Rail && v->type == VehicleType::Train && vdir == shaft && v->IsMovingFront() && frame == 1) {
		Train *t = Train::From(v);
		if (!PlayVehicleSound(t, VehicleSoundEvent::Tunnel) && RailVehInfo(t->engine_type)->engclass == EngineClass::Steam) {
			SndPlayVehicleFx(SND_05_TRAIN_THROUGH_TUNNEL, v);
		}
	}

	//Heading down the shaft: hide at the door frame. Heading out: show again past that frame.
	if (vdir == shaft && frame == _tunnel_visibility_frame[shaft]) {
		//Road works is the mechanical stand-in for the elevator ride.
		if (transport == TransportType::Road && v->IsFrontEngine()) SndPlayTileFx(SND_21_ROAD_WORKS, tile);
		v->vehstatus.Set(VehState::Hidden);
		return {};
	}
	if (vdir == ReverseDiagDir(shaft) && frame == TILE_SIZE - _tunnel_visibility_frame[shaft] && z == 0) {
		v->vehstatus.Reset(VehState::Hidden);
		return {};
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

/** @copydoc ClearTileProc */
static CommandCost ClearTile_LayerPortal(TileIndex tile, DoCommandFlags flags)
{
	if (flags.Test(DoCommandFlag::Auto)) return CommandCost(STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST);

	TileIndex pair = GetLayerPortalPairTile(tile);
	CommandCost ownership = CheckTileOwnership(tile);
	if (ownership.Failed()) return ownership;

	CommandCost blocked = EnsureNoVehicleOnGround(tile);
	if (blocked.Failed()) return blocked;
	blocked = EnsureNoVehicleOnGround(pair);
	if (blocked.Failed()) return blocked;

	//M-2: a train may have reserved through either end, so both ends are checked.
	if (GetLayerPortalTransportType(tile) == TransportType::Rail && (HasLayerPortalReservation(tile) || HasLayerPortalReservation(pair))) {
		return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);
	}

	if (flags.Test(DoCommandFlag::Execute)) {
		Owner owner = GetTileOwner(tile);
		TransportType transport = GetLayerPortalTransportType(tile);
		RailType railtype = transport == TransportType::Rail ? GetRailType(tile) : INVALID_RAILTYPE;
		RoadType roadtype = transport == TransportType::Road ? GetRoadTypeRoad(tile) : INVALID_ROADTYPE;
		DiagDirection shaft = GetLayerPortalShaftDirection(tile);
		if (Company::IsValidID(owner)) {
			if (transport == TransportType::Rail) {
				Company::Get(owner)->infrastructure.rail[railtype] -= LAYER_PORTAL_TRACKBIT_FACTOR(transport) * 2;
			} else {
				UpdateCompanyRoadInfrastructure(roadtype, owner, -static_cast<int>(LAYER_PORTAL_TRACKBIT_FACTOR(transport) * 2));
			}
			DirtyCompanyInfrastructureWindows(owner);
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
	if (GetTileSlope(tile) != SLOPE_FLAT) return CommandCost(STR_ERROR_LAYER_PORTAL_NEEDS_FLAT_LAND);
	if (IsBridgeAbove(tile)) return CommandCost(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
	if (CommandCost layer = CheckLayerAllowsConstruction(tile, MapLayerConstruction::LayerPortal); layer.Failed()) return layer;
	//The door needs a neighbour that is still inside the map, so a road or track can arrive.
	TileIndex door_neighbour = TileAddByDiagDir(tile, ReverseDiagDir(shaft_dir));
	if (!IsInnerTile(door_neighbour)) return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	return CommandCost();
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
	if (transport_type == TransportType::Rail) {
		if (!ValParamRailType(railtype)) return CMD_ERROR;
	} else if (transport_type == TransportType::Road) {
		//An elevator carries road vehicles only: the type must exist for this company and must not be a tram type.
		if (!ValParamRoadType(roadtype) || !RoadTypeIsRoad(roadtype)) return CMD_ERROR;
	} else {
		return CMD_ERROR;
	}
	if (to_underlying(shaft_dir) > to_underlying(DiagDirection::NW)) return CMD_ERROR;
	//Version 1 pairs Surface with Underground only. M-11: a layer without a pair (Space) is refused here, so TileInLayer never receives MapLayer::Invalid.
	if (!IsValidTile(tile)) return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	MapLayer expected_target = MultiLayerMap::GetPortalTargetLayer(GetTileLayer(tile));
	if (expected_target == MapLayer::Invalid || target_layer != expected_target) {
		return CommandCost(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL);
	}

	TileIndex pair = MultiLayerMap::TileInLayer(tile, target_layer);
	DiagDirection pair_shaft = ReverseDiagDir(shaft_dir);
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
			Company::Get(_current_company)->infrastructure.rail[railtype] += LAYER_PORTAL_TRACKBIT_FACTOR(transport_type) * 2;
		} else {
			MakeRoadLayerPortal(tile, _current_company, shaft_dir, target_layer, roadtype);
			MakeRoadLayerPortal(pair, _current_company, pair_shaft, GetTileLayer(tile), roadtype);
			UpdateCompanyRoadInfrastructure(roadtype, _current_company, static_cast<int>(LAYER_PORTAL_TRACKBIT_FACTOR(transport_type) * 2));
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
void CcBuildLayerPortal(Commands, const CommandCost &result, TileIndex tile, TransportType, MapLayer, DiagDirection, RailType, RoadType)
{
	if (result.Succeeded()) SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, tile);
}

/** TileTypeProcs for TileType::LayerPortal. */
extern const TileTypeProcs _tile_type_layer_portal_procs = {
	.draw_tile_proc = DrawTile_LayerPortal,
	.get_slope_pixel_z_proc = GetSlopePixelZ_LayerPortal,
	.clear_tile_proc = ClearTile_LayerPortal,
	.get_tile_desc_proc = GetTileDesc_LayerPortal,
	.get_tile_track_status_proc = GetTileTrackStatus_LayerPortal,
	.tile_loop_proc = TileLoop_LayerPortal,
	.change_tile_owner_proc = ChangeTileOwner_LayerPortal,
	.vehicle_enter_tile_proc = VehicleEnterTile_LayerPortal,
	.terraform_tile_proc = TerraformTile_LayerPortal,
	.check_build_above_proc = CheckBuildAbove_LayerPortal,
};
