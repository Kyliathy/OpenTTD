/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_generation.cpp Versioned guaranteed geology with a private deterministic stream. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 3 */

#include "stdafx.h"
#include "excavation.h"
#include "multilayer_recipe.h"
#include "tile_map.h"

#include "safeguards.h"

/** Version-1 SplitMix64 stream; unsigned wrap is specified and neither global Randomizer is touched. */
class GeologySeedStream {
	uint64_t state; ///< Transient stream cursor, reconstructed only during new-world generation.
public:
	/** @param seed Independent feature/substream seed, never a Surface RNG draw. */
	explicit GeologySeedStream(uint64_t seed) : state(seed) {}
	/** @return Next versioned 64-bit value, using deliberate unsigned modular arithmetic. */
	uint64_t Next()
	{
		uint64_t z = (this->state += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return z ^ (z >> 31);
	}
	/** @param limit Positive exclusive bound. @return Stable bounded coordinate, with no floating-point dependence. */
	uint32_t Below(uint32_t limit) { assert(limit != 0); return static_cast<uint32_t>(this->Next() % limit); }
};

/** Transient reserved mining/loading/turn envelope; generation does not excavate or construct any of its cells. */
struct GeologyGenerationEnvelope {
	MapLayer layer; ///< Rock layer owning this workspace.
	uint16_t x; ///< Inclusive minimum plane column.
	uint16_t y; ///< Inclusive minimum plane row.
};

/**
 * Refuse geometry that cannot guarantee all frozen resources plus a distinct usable vent envelope.
 * @param width Plane width.
 * @param height Plane height.
 * @return Supported bounded map size.
 */
bool CheckGeologyGenerationSize(uint32_t width, uint32_t height)
{
	return width >= MIN_MAP_SIZE && height >= MIN_MAP_SIZE && width <= MAX_MAP_SIZE && height <= MAX_MAP_SIZE;
}

/**
 * Derive a stable root from synchronized input with explicit version-1 domain separation.
 * @param world_seed Original world-generation seed.
 * @return Independent geology root; the local stream does not alter global generation state.
 */
uint64_t DeriveGeologySeed(uint32_t world_seed)
{
	GeologySeedStream stream(0x47454f4c4f475931ULL ^ world_seed); // "GEOLOGY1" domain, frozen with generator version 1.
	return stream.Next();
}

/**
 * Check reserved work envelopes in a fixed bounded traversal without confusing touching edges with overlap.
 * @param proposed Candidate 16x16 seam/work envelope.
 * @param reserved Earlier mineral envelopes plus the reserved Deep vent workspace.
 * @return Whether every cell is playable and the envelope has no conflicting workspace.
 */
static bool IsGenerationEnvelopeFree(const GeologyGenerationEnvelope &proposed, std::span<const GeologyGenerationEnvelope> reserved)
{
	// The full work area, including loading/approach space, must stay inside the void fence.
	if (proposed.x + 15U >= Map::MaxX() || proposed.y + 15U >= Map::MaxY()) return false;
	// Only a few dozen reserved regions are examined; mandatory approach/vent space cannot be consumed by optional ore.
	for (const auto &existing : reserved) {
		if (existing.layer != proposed.layer) continue;
		// Same-layer rectangles intersect only when both axes overlap; merely adjoining envelopes remain legal.
		if (proposed.x < existing.x + 16 && existing.x < proposed.x + 16 && proposed.y < existing.y + 16 && existing.y < proposed.y + 16) return false;
	}
	// Both fence policies are verified over all workspace cells, not just the ore endpoints.
	for (uint y = proposed.y; y < proposed.y + 16U; ++y) {
		for (uint x = proposed.x; x < proposed.x + 16U; ++x) {
			if (!IsValidTile(TileXYL(x, y, proposed.layer))) return false;
		}
	}
	return true;
}

/**
 * Create a compact seam within reserved working/approach space; all cells remain solid.
 * @param id Next stable geological identity.
 * @param resource Frozen cargo meaning.
 * @param envelope Authorized workspace, including spare construction/turn space.
 * @param stream Local deterministic generator.
 * @return Canonicalizable 32-cell seam with exactly 1,000 t initial reserve per cell.
 */
static GeologyDeposit MakeGeneratedDeposit(GeologyDepositID id, CargoLabel resource, const GeologyGenerationEnvelope &envelope, GeologySeedStream &stream)
{
	GeologyDeposit deposit;
	deposit.id = id;
	deposit.layer = envelope.layer;
	deposit.resource = resource;
	deposit.hardness = envelope.layer == MapLayer::DeepUnderground ? GeologyHardness::Hard : GeologyHardness::Soft;
	deposit.initial_reserve = deposit.remaining_reserve = 32000;
	const uint16_t first_x = envelope.x + 2 + stream.Below(4);
	const uint16_t first_y = envelope.y + 2 + stream.Below(4);
	// Four connected spans leave a contiguous machinery/loading approach and generous legal turn space inside the envelope.
	for (uint16_t y = first_y; y < first_y + 4; ++y) deposit.spans.push_back({y, first_x, static_cast<uint16_t>(first_x + 7)});
	return deposit;
}

/**
 * Build guaranteed resources first, followed by bounded optional placements and deterministic fallbacks.
 * @param geology_seed Independent versioned root.
 * @param hardness_version Regional rule; zero is explicit historical generation, never the normal new-game default.
 * @return Complete geological proposal or explicit refusal, without changing tiles/RNG/live authority.
 */
GeologyGenerationResult PrepareGeologyGeneration(uint64_t geology_seed, uint16_t hardness_version)
{
	GeologyGenerationResult result;
	if (hardness_version > GEOLOGY_HARDNESS_VERSION) { result.error = GeologyError::InvalidShape; return result; }
	// Geology requires both rock planes and enough usable space before allocating/publishing any mandatory record.
	if (!CheckGeologyGenerationSize(Map::SizeX(), Map::SizeY()) || !MultiLayerMap::IsValidLayer(MapLayer::DeepUnderground)) {
		result.error = GeologyError::UnsupportedMap;
		return result;
	}
	std::vector<GeologyGenerationEnvelope> reserved;
	GeologySeedStream stream(geology_seed);
	const std::array<std::pair<uint16_t, uint16_t>, 5> mandatory_positions = {{{4, 4}, {24, 4}, {44, 4}, {4, 28}, {24, 28}}};
	std::array<uint32_t, 2> placed{};
	// Resource traversal follows the frozen catalogue, not runtime CargoSpec slots or unordered containers.
	for (const auto &mine : GetMultiLayerMineRecipes()) {
		const uint32_t rock_plane = mine.layer == MapLayer::DeepUnderground ? 1 : 0;
		if (placed[rock_plane] >= mandatory_positions.size()) { result.error = GeologyError::InvalidResource; return result; }
		const auto [x, y] = mandatory_positions[placed[rock_plane]++];
		const GeologyGenerationEnvelope envelope{mine.layer, x, y};
		if (!IsGenerationEnvelopeFree(envelope, reserved)) { result.error = GeologyError::UnsupportedMap; return result; }
		const auto error = result.state.deposits.Insert(MakeGeneratedDeposit(result.state.deposits.NextID(), mine.resource, envelope, stream));
		if (error != GeologyError::None) { result.error = error; return result; }
		reserved.push_back(envelope);
	}
	// Exactly five distinct resources per rock layer are mandatory; spare vent space cannot substitute for any of them.
	if (placed[0] != 5 || placed[1] != 5) { result.error = GeologyError::InvalidResource; return result; }
	const GeologyGenerationEnvelope vent_space{MapLayer::DeepUnderground, 44, 28};
	if (!IsGenerationEnvelopeFree(vent_space, reserved)) { result.error = GeologyError::UnsupportedMap; return result; }
	reserved.push_back(vent_space);
	// A separate versioned vent stream leaves mandatory and optional mineral positions unchanged when natural heat is added.
	GeologySeedStream vent_stream(geology_seed ^ 0x56454e545f563031ULL); // "VENT_V01" domain.
	GeothermalVentField vent;
	vent.id = GeothermalVentID{0};
	vent.x = vent_space.x + 4 + vent_stream.Below(4);
	vent.y = vent_space.y + 4 + vent_stream.Below(4);
	vent.thermal_capacity_w = 1000000;
	result.state.vents.push_back(vent);
	result.state.next_vent_id = 1;
	const auto vent_error = ValidateGeothermalVentFields(result.state.vents, result.state.next_vent_id);
	if (vent_error != GeologyError::None) { result.error = vent_error; return result; }
	const uint32_t optional_per_layer = std::min<uint32_t>(32, Map::PlaneSize() / 16384);
	// Optional density is capped; each attempt has eight random candidates and at most 256 fixed fallback cells.
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (uint32_t i = 0; i < optional_per_layer; ++i) {
			std::optional<GeologyGenerationEnvelope> chosen;
			for (uint32_t retry = 0; retry < 8; ++retry) {
				const GeologyGenerationEnvelope candidate{layer, static_cast<uint16_t>(2 + stream.Below(Map::SizeX() - 19)), static_cast<uint16_t>(2 + stream.Below(Map::SizeY() - 19))};
				if (IsGenerationEnvelopeFree(candidate, reserved)) { chosen = candidate; break; }
			}
			// Stable row-major fallback is bounded even on maximum maps; failure merely omits an optional seam.
			for (uint32_t cell = 0; !chosen && cell < 256; ++cell) {
				const uint32_t columns = (Map::SizeX() - 4) / 20;
				const GeologyGenerationEnvelope candidate{layer, static_cast<uint16_t>(4 + (cell % columns) * 20), static_cast<uint16_t>(4 + (cell / columns) * 20)};
				if (candidate.y + 15U >= Map::MaxY()) break;
				if (IsGenerationEnvelopeFree(candidate, reserved)) chosen = candidate;
			}
			if (!chosen) continue;
			const auto recipes = GetMultiLayerMineRecipes();
			const auto &mine = recipes[(layer == MapLayer::DeepUnderground ? 5 : 0) + i % 5];
			const auto error = result.state.deposits.Insert(MakeGeneratedDeposit(result.state.deposits.NextID(), mine.resource, *chosen, stream));
			if (error != GeologyError::None) { result.error = error; return result; }
			reserved.push_back(*chosen);
		}
	}
	if (!result.state.terrain.Reset(Map::PlaneSize())) { result.error = GeologyError::UnsupportedMap; return result; }
	result.state.generator_version = GEOLOGY_GENERATOR_VERSION;
	result.state.hardness_version = hardness_version;
	result.state.hardness_seed = hardness_version == 0 ? 0 : DeriveGeologyHardnessSeed(geology_seed);
	return result;
}
