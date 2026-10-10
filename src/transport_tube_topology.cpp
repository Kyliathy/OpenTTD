/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_topology.cpp Rebuildable owned guideway circuits, independent of grants, signals, cameras and routine accounting intervals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47E physical end presentation
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "train.h"
#include "rail_map.h"
#include "depot_map.h"
#include "pathfinder/follow_track.hpp"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "tile_cmd.h"
#include <map>
#include <set>

#include "safeguards.h"

/** @copydoc GetTubeGuidewayEnds */
DiagDirectionIndexArray<bool> GetTubeGuidewayEnds(TileIndex tile)
{
	DiagDirectionIndexArray<bool> ends{};
	// Only an actual plain canonical tube tile owns these end decorations.
	// Facilities supply their own housing, and invalid/ordinary cells must not
	// acquire a painted tube mouth or a new gameplay connection from this view.
	if (!IsValidTile(tile) || !IsPlainRailTile(tile) || !IsTubeRailType(GetRailType(tile))) return ends;
	// Both directions of every installed native piece describe its physical
	// mouths. Reversal and multiple pieces sharing an edge add no second end.
	for (Trackdir direction : GetTileTrackStatus(tile, TransportType::Rail, RoadTramType::Invalid).trackdirs) {
		ends[TrackdirToExitdir(direction)] = true;
	}
	// Check only the four immediate same-layer neighbours. Native track status
	// with an explicit entry side distinguishes a real depot door from its back
	// wall; no grant, vehicle route, reservation, signal or camera is consulted.
	for (uint index = 0; index < 4; ++index) {
		const auto port = static_cast<DiagDirection>(index);
		if (!ends[port]) continue;
		const auto delta = TileIndexDiffCByDiagDir(port);
		const TileIndex neighbour = TileAddWrap(tile, delta.x, delta.y);
		// A missing tile, different rock plane or conventional guideway cannot
		// provide a matching physical tube mouth merely through adjacency.
		if (!IsValidTile(neighbour) || GetTileLayer(neighbour) != GetTileLayer(tile) ||
				!IsTubeRailType(GetTileRailType(neighbour))) continue;
		const auto entry = ReverseDiagDir(port);
		for (Trackdir direction : GetTileTrackStatus(neighbour, TransportType::Rail, RoadTramType::Invalid, entry).trackdirs) {
			if (TrackdirToExitdir(direction) != entry) continue;
			ends[port] = false;
			break;
		}
	}
	return ends;
}

static constexpr size_t MAX_TUBE_CIRCUIT_NODES = 65536; ///< Bounded native trackdir work per installed service; oversized circuits refuse whole readiness.

/** Derived source component only; native tracks, utility slices and invoices retain their own authoritative owners. */
struct TubePowerCircuit {
	CompanyID owner = CompanyID::Invalid(); ///< Actual service's company; foreign guideways never become eligible.
	MapLayer layer = MapLayer::Surface; ///< One physical rock plane, independent of the active camera.
	std::map<TileIndex, TrackBits> tracks; ///< Actual reachable segments, distinguishing disconnected pieces sharing coordinates.
	bool dirty = true; ///< Only eligible physical layout/capability changes require a rebuild.
	bool complete = false; ///< An oversized/interrupted derivation provides no partial powered component.
};

static std::map<UtilityProducerID, TubePowerCircuit> _tube_circuits; ///< Rebuildable sparse components; never another saved grant/network authority.

/** @param tile Candidate actual world address. @param circuit Derived company/layer. @return Whether an actual canonical owned tube segment may participate. */
static bool IsCircuitFloor(TileIndex tile, const TubePowerCircuit &circuit)
{
	// A wire follows actual owned layer-local guideways. Foreign, conventional,
	// cross-layer and portal/bridge/tunnel cells cannot borrow electrical reach.
	return IsValidTile(tile) && GetTileLayer(tile) == circuit.layer && GetTileOwner(tile) == circuit.owner.base() &&
		!IsTileType(tile, TileType::TunnelBridge) && IsTubeRailType(GetTileRailType(tile));
}

/** @param connection Exact installed physical inlet. @param circuit Rebuildable result, published ready only after bounded traversal finishes. */
static void RebuildTubeCircuit(const TubePowerConnection &connection, TubePowerCircuit &circuit)
{
	circuit.tracks.clear(); circuit.owner = connection.owner; circuit.layer = GetTileLayer(connection.interchange);
	circuit.dirty = false; circuit.complete = false;
	if (!IsCircuitFloor(connection.interchange, circuit)) return;
	using Node = std::pair<TileIndex, Trackdir>;
	std::vector<Node> queue;
	std::set<Node> visited;
	const auto directions = GetTileTrackStatus(connection.interchange, TransportType::Rail, RoadTramType::Invalid).trackdirs;
	for (const auto direction : directions) queue.emplace_back(connection.interchange, direction);
	CFollowTrackRailNo90 follower(connection.owner, RailTypes{GetTubeRailType()});
	// Native track following supplies actual connected junction/turn geometry.
	// No reservation or signal colour controls electricity, and no plane scan,
	// camera state or old copied DTO can introduce another powered route.
	for (size_t index = 0; index < queue.size(); ++index) {
		const auto [tile, direction] = queue[index];
		if (!visited.insert({tile, direction}).second) continue;
		if (visited.size() > MAX_TUBE_CIRCUIT_NODES) { circuit.tracks.clear(); return; }
		circuit.tracks[tile].Set(TrackdirToTrack(direction));
		if (!follower.Follow(tile, direction)) continue;
		// The semantic domain refuses both actual shaft travel and a follower's
		// bridge/tunnel shortcut, even if a broad native rail mask admits it.
		if (follower.is_portal || follower.is_bridge || follower.is_tunnel || !IsCircuitFloor(follower.new_tile, circuit)) continue;
		// A native station follower skips to its platform end; retain every
		// actual intermediate platform segment so an occupying capsule does
		// not lose power merely because its physical owner is on that platform.
		if (follower.tiles_skipped != 0) {
			TileIndex platform = follower.new_tile;
			for (int step = 0; step < follower.tiles_skipped; ++step) {
				platform = TileAddByDiagDir(platform, ReverseDiagDir(follower.exitdir));
				if (!IsCircuitFloor(platform, circuit)) { circuit.tracks.clear(); return; }
				circuit.tracks[platform] |= TrackdirBitsToTrackBits(follower.new_td_bits);
			}
		}
		for (const auto next : follower.new_td_bits) queue.emplace_back(follower.new_tile, next);
		if (queue.size() > MAX_TUBE_CIRCUIT_NODES * 8) { circuit.tracks.clear(); return; } // Repeated junction branches cannot create unbounded temporary work before deduplication.
	}
	circuit.complete = true;
}

/** @copydoc IsTubePowerConnected */
bool IsTubePowerConnected(UtilityProducerID source, const Train &train)
{
	const auto *connection = GetTubePowerConnection(source);
	// Explicit source identity and company precede the derived physical lookup;
	// a retired source or adjacent unconnected rail cannot power this capsule.
	if (connection == nullptr || !connection->active || connection->owner != train.owner || !CanTubeTrainEnterTile(train, train.tile)) return false;
	// Every self-propelled actual part needs its own occupied powered segment;
	// coupling cannot drag a disconnected or foreign trailing capsule for free.
	for (const Train *part = train.First(); part != nullptr; part = part->Next()) {
		const auto occupied = part->track == Track::Depot ? TrackBits{} : part->track;
		if (!IsTubePowerConnectedTile(source, part->tile, occupied)) return false;
	}
	return true;
}

/** @copydoc IsTubePowerConnectedTile */
bool IsTubePowerConnectedTile(UtilityProducerID source, TileIndex tile, TrackBits required)
{
	const auto *connection = GetTubePowerConnection(source);
	// Real source liveness and floor addressing precede any derived lookup;
	// an invalid cell or retired identity cannot revive electrical reach.
	if (connection == nullptr || !connection->active || !IsValidTile(tile)) return false;
	auto &circuit = _tube_circuits[source];
	if (circuit.dirty) RebuildTubeCircuit(*connection, circuit);
	if (!circuit.complete) return false;
	if (!IsCircuitFloor(tile, circuit)) return false;
	const auto found = circuit.tracks.find(tile);
	if (found == circuit.tracks.end()) return false;
	return (found->second & required) == required;
}

/** @param consumer Actual affected reservation. Release only its current unused slices through the sole ledger. */
static void ExpireCircuitOffer(UtilityConsumerID consumer)
{
	const auto *record = GetUtilityPowerConsumer(consumer);
	if (record == nullptr) return;
	AutoRestoreBackup actor(_current_company, record->owner);
	const auto grant = GetUtilityAllocation(consumer);
	if (!grant.has_value()) return;
	if (grant->granted_energy_j == 0) return;
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {consumer, grant->accounting_interval}, grant->granted_energy_j);
	CommitUtilityMutation(release);
}

/** @copydoc InvalidateTubePowerTopology */
void InvalidateTubePowerTopology(TileIndex tile)
{
	std::set<UtilityProducerID> affected;
	// A changed segment affects its former component and adjacent additions.
	// Unrelated Surface/ordinary layouts do not cause per-interval route churn;
	// full native reset explicitly invalidates every derived source component.
	for (auto &[source, circuit] : _tube_circuits) {
		bool changed = tile == INVALID_TILE;
		if (!changed) {
			if (GetTileLayer(tile) != circuit.layer) continue;
			changed = circuit.tracks.contains(tile);
			for (const auto direction : EnumRange(DiagDirection::End)) changed |= circuit.tracks.contains(TileAddByDiagDir(tile, direction));
		}
		if (!changed) continue;
		circuit.dirty = true; affected.insert(source);
	}
	// Revoke only unused offers for physically affected services. Already used
	// source energy and the completed fee survive topology loss without refund.
	for (const auto &binding : CaptureTubePowerVehicles()) {
		if (binding.active && affected.contains(binding.source)) ExpireCircuitOffer(binding.consumer);
	}
	InvalidateTubePowerForTile(tile);
}

/** @copydoc ResetTubePowerTopology */
void ResetTubePowerTopology() { _tube_circuits.clear(); }
