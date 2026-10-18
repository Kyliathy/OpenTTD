/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file orbital_sprite.cpp Orbital art reads original hull, cargo, work and service owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#include "stdafx.h"
#include "orbital_sprite.h"
#include "space_sprite.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_mining_tuning.h"
#include "asteroid.h"
#include "space_structure.h"
#include "orbital_manufacturing.h"
#include "orbital_recipe.h"
#include "orbital_service.h"
#include "company_func.h"
#include "tile_type.h"
#include "openttd.h"
#include "safeguards.h"

/** Read an actual outstanding native order refusal without treating completion as failure.
 * @param craft Original queue whose typed reason remains owned by its controller.
 * @return Whether that original reason selects a static blocked display.
 */
static bool HasOrbitalArtOrderBlock(const SpaceVehicle &craft)
{
	const auto block = craft.space_orders.blocked;
	return block != SpaceOrderBlock::None && block != SpaceOrderBlock::Completed;
}

/**
 * Select actual heavy21, retained16 or hauler17 art from original native owners.
 * @param craft Original native hull, packets, mission and typed queue; no inputs are granted.
 * @param direction Existing renderer view, already resolved from world geometry for map use.
 * @return Authored native pose, or absence for another role or invalid view.
 */
std::optional<OrbitalArtPose> GetOrbitalCraftArtPose(const SpaceVehicle &craft, Direction direction)
{
	if (direction >= Direction::End) return std::nullopt;
	const auto model = GetSpaceVehicleModel(craft);
	OrbitalArtPose pose;
	pose.direction = direction;
	if (model == SPACE_HEAVY_MINER_MODEL) {
		pose.asset = OrbitalSpriteAsset::VehicleModel21;
		if (const auto frame = GetSpaceCraftServiceArtFrame(craft)) { pose.state = 6; pose.frame = *frame; return pose; }
		// Heavy supply, return allowance, shell/module route and typed failures
		// stay native; displaying amber cannot clear a wait or grant another return.
		if (HasOrbitalArtOrderBlock(craft) || GetSpaceMiningBlock(craft) != SpaceOrderBlock::None) { pose.state = 7; return pose; }
		if (craft.mission.phase == SpaceMissionPhase::Return) {
			if (const auto frame = GetSpaceCraftTravelArtFrame(craft)) { pose.state = 5; pose.frame = *frame; return pose; }
		}
		// Original positive hold capacity and packets alone supply a full ore
		// cue; a stopped return retains the actual load without showing exhaust.
		if (craft.cargo_cap != 0 && craft.cargo.TotalCount() >= craft.cargo_cap) { pose.state = 4; return pose; }
		if (craft.mission.phase == SpaceMissionPhase::Mining) {
			const auto *body = GetMutableAsteroidBody(craft.mission.target);
			// A saved phase cannot borrow another hull's lease or fabricate a body.
			// Current reciprocal admission is mandatory before showing attached tools.
			if (body == nullptr || body->mining_slot != craft.index) return pose;
			pose.state = 2;
			// Only admitted original work during normal unpaused simulation selects
			// cutter phases. Existing mission parity never advances through drawing.
			if (_game_mode == GameMode::Normal && !_pause_mode.Any() && IsSpaceMiningWorkTick(craft)) {
				pose.state = 3; pose.frame = craft.mission.work_ticks % 2;
			}
			return pose;
		}
		if (craft.mission.phase == SpaceMissionPhase::Approach) {
			if (const auto frame = GetSpaceCraftTravelArtFrame(craft)) { pose.state = 1; pose.frame = *frame; }
		}
		return pose;
	}
	if (model == 16) {
		pose.asset = OrbitalSpriteAsset::VehicleModel16;
		if (HasOrbitalArtOrderBlock(craft)) { pose.state = 4; return pose; }
		// ReadyProbe admits20 only and native service tuning excludes16. Retained
		// construction/service-looking compartments grant neither work nor repair;
		// canonical idle represents those unreachable source states honestly.
		if (const auto frame = GetSpaceCraftTravelArtFrame(craft)) { pose.state = 1; pose.frame = *frame; }
		return pose;
	}
	if (model == 17) {
		pose.asset = OrbitalSpriteAsset::VehicleModel17;
		const auto count = craft.cargo.TotalCount();
		const auto capacity = GetSpaceFreightCapacity(model, craft.cargo_type);
		// The actual original hold remains visible even for an incompatible or
		// zero-capacity refit. Missing capacity cannot erase retained packets;
		// only a real positive native freight capacity admits the full-load cue.
		const uint8_t load = count == 0 ? 0 : capacity != 0 && count >= capacity ? 2 : 1;
		pose.state = load;
		if (HasOrbitalArtOrderBlock(craft)) { pose.state = 7 + load; return pose; }
		// Hauler17 has no native service producer. Real admitted movement adds
		// exhaust to the same original load tier; all stopped/waiting poses retain it.
		if (const auto frame = GetSpaceCraftTravelArtFrame(craft)) { pose.state = 3 + load; pose.frame = *frame; }
		return pose;
	}
	return std::nullopt;
}

/** Map actual native module kinds to their already delivered source family.
 * @param kind Original content-owned construction identity.
 * @return Source family, or absence for a mobile/nonmodule construction kind.
 */
static std::optional<OrbitalSpriteAsset> OrbitalModuleAsset(SpaceBuildKind kind)
{
	switch (kind) {
		case SpaceBuildKind::Core: return OrbitalSpriteAsset::ModuleNucleus;
		case SpaceBuildKind::Dock: return OrbitalSpriteAsset::ModuleDock;
		case SpaceBuildKind::Storage: return OrbitalSpriteAsset::ModuleStorage;
		case SpaceBuildKind::Power: return OrbitalSpriteAsset::ModulePower;
		case SpaceBuildKind::FuelStore: return OrbitalSpriteAsset::ModuleFuelStore;
		case SpaceBuildKind::Assembly: return OrbitalSpriteAsset::ModuleWorkshop;
		case SpaceBuildKind::Steelworks:
		case SpaceBuildKind::Goodsworks:
		case SpaceBuildKind::Refinery:
		case SpaceBuildKind::IceProcessor: return OrbitalSpriteAsset::ModuleProcessor;
		default: return std::nullopt;
	}
}

/** Select physical stages first, then original owned facility admission/receipts.
 * @param site Actual native site; no field or stock is copied into another authority.
 * @param observer Existing viewer; private queries never switch the current company.
 * @param tile Original row-major world cell.
 * @param part Back or genuine foreground source part.
 * @return Transient published pose, or absence for an invalid/collected site.
 */
std::optional<OrbitalArtPose> GetOrbitalModuleArtPose(const SpaceStructure &site, CompanyID observer, uint8_t tile, uint8_t part)
{
	const auto asset = OrbitalModuleAsset(site.state.kind);
	if (!asset) return std::nullopt;
	const auto &range = _orbital_sprite_ranges[static_cast<size_t>(*asset)];
	// Neither a bad source cell nor a terminal/collected blueprint may borrow a
	// neighbouring building's art or remain a usable physical facility.
	if (tile >= range.tiles || part >= range.parts || site.state.phase >= SpaceStructurePhase::End ||
		(site.state.phase == SpaceStructurePhase::Cancelled && site.job.inputs[0].TotalCount() == 0 && site.job.inputs[1].TotalCount() == 0)) return std::nullopt;
	OrbitalArtPose pose{*asset, 0, 0, Direction::N, tile, part};
	if (site.state.phase != SpaceStructurePhase::FrameComplete) {
		// Actual fractional paid work also erects partial hardware. Deliveries
		// alone, global time and a stopped builder cannot complete a paid frame.
		pose.state = site.job.work_ticks != 0 || site.job.work_fraction != 0 || site.job.work_remainder != 0 ? 1 : 0;
		return pose;
	}
	pose.state = 2;
	// FuelStore is a paid physical frame with no native activation owner. Its
	// source aliases complete honestly; a tank picture cannot commission storage.
	if (site.state.kind == SpaceBuildKind::FuelStore) return pose;
	if (!IsOrbitalModuleOperational(site)) { pose.state = 3; return pose; }
	if (site.state.kind == SpaceBuildKind::Core) {
		if (!site.module->power_enabled) pose.state = 3;
	} else if (site.state.kind == SpaceBuildKind::Power) {
		if (GetOrbitalModulePanelRate(site) == 0) pose.state = 3;
	} else if (site.state.kind == SpaceBuildKind::Dock) {
		if (IsOrbitalServiceFacilityBlocked(site)) pose.state = 3;
	}
	// Public physical stages remain visible to other viewers, while owner-only
	// stock/unit queries retain their native actor boundary without impersonation.
	if (observer != site.state.owner || _current_company != site.state.owner) return pose;
	if (site.state.kind == SpaceBuildKind::Assembly) {
		if (GetOrbitalManufactureWait(site) != OrbitalManufactureWait::None) pose.state = 3;
		else if (const auto work = GetOrbitalManufactureVisualWork(site)) pose.frame = *work % 2;
	} else if (IsOrbitalRecipeKind(site.state.kind)) {
		const auto wait = GetOrbitalRecipeWait(site);
		// Disabled is an intentional idle machine. Only native material, output,
		// power or admission failures select amber; no power requirement is invented
		// for genuinely unpowered refining/ice recipes.
		if (wait != OrbitalRecipeWait::None && wait != OrbitalRecipeWait::Disabled && wait != OrbitalRecipeWait::Work) pose.state = 3;
		else if (const auto frame = GetOrbitalRecipeVisualFrame(site)) pose.frame = *frame;
	}
	return pose;
}

/** Provide native sorting ownership without moving any approved source pixel.
 * @param site Actual structure and its content-defined physical footprint.
 * @param tile Source cell in native row-major order.
 * @param part Back or genuine near-jaw foreground.
 * @return Exact compensated bounds, or absence for undeclared source geometry.
 */
std::optional<SpriteBounds> GetOrbitalModuleArtBounds(const SpaceStructure &site, uint8_t tile, uint8_t part)
{
	const auto asset = OrbitalModuleAsset(site.state.kind);
	if (!asset) return std::nullopt;
	const auto *definition = GetSpaceConstructionDefinition(site.state.kind);
	// Cuts only describe the delivered two/three-cell fixed footprints. Invalid
	// geometry must not wrap a foreground into unrelated world coordinates.
	if (definition == nullptr || definition->width == 0 || tile >= definition->width * definition->height || part > 1) return std::nullopt;
	if (part == 0) return SpriteBounds{{0, 0, 0}, {16, 16, 8}, {0, 0, 0}};
	// Original craft bounds are eight world units high. The separated near jaw
	// sorts at the upper edge of the actual south approach's hull clearance;
	// Dock covers both existing half-tile berth centers, all other sites one.
	// Inverse offsets retain the original cell corner and GRF anchor exactly.
	const int8_t x = static_cast<int8_t>(4 - (tile % definition->width) * static_cast<int>(TILE_SIZE));
	const int8_t y = static_cast<int8_t>((definition->height - tile / definition->width) * static_cast<int>(TILE_SIZE) + 4);
	const uint8_t width = site.state.kind == SpaceBuildKind::Dock ? 16 : 8;
	return SpriteBounds{{x, y, 8}, {width, 8, 1}, {static_cast<int8_t>(-x), static_cast<int8_t>(-y), -8}};
}
