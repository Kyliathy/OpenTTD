/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_sl.cpp Checked explicit continuation options under original VEHS/SPOR owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group23.
 */
#include "../stdafx.h"
#include "space_mining_sl.h"
#include "../space_mining_target.h"
#include "../space_mining_continuation.h"
#include "../fios.h"
#include <set>
#include "../safeguards.h"

/** @copydoc EncodeSpaceMiningContinuation */
SpaceMiningContinuationWire EncodeSpaceMiningContinuation(const SpaceMiningContinuationState &state)
{
	SpaceMiningContinuationWire wire;
	wire.phase = to_underlying(state.phase); wire.previous_body = state.previous_body.base();
	// Cargo labels have stable byte significance; host wrapper layout and local
	// cargo ordinals cannot change a pinned mineral across native reload.
	for (uint index = 0; index < 4; ++index) wire.mineral = (wire.mineral << 8) | static_cast<uint8_t>(state.mineral[index]);
	if (state.effective_target) {
		const auto &target = *state.effective_target; wire.effective_present = 1;
		wire.effective_kind = to_underlying(target.kind); wire.effective_id = target.id; wire.effective_generation = target.generation;
	}
	if (state.departure_point) { wire.departure_present = 1; wire.departure_x = state.departure_point->x; wire.departure_y = state.departure_point->y; }
	wire.retry_ticks = state.retry_ticks; wire.waiting = to_underlying(state.waiting); wire.route_cursor = state.route_cursor; wire.travel_ticks = state.travel_ticks;
	return wire;
}

/** @copydoc DecodeSpaceMiningContinuation */
std::optional<SpaceMiningContinuationState> DecodeSpaceMiningContinuation(const SpaceMiningContinuationWire &wire)
{
	// Unknown phases/flags or unbounded retry/travel must refuse before they can
	// hide work or acquire a new destination during the first restored tick.
	if (wire.phase >= to_underlying(MiningContinuationPhase::End) || wire.waiting >= to_underlying(MiningContinuationWait::End) ||
		wire.effective_present > 1 || wire.departure_present > 1 || wire.retry_ticks > GetSpaceMiningTargetRetryDelay() || wire.travel_ticks > (1U << 24) ||
		(wire.route_cursor != UINT8_MAX && wire.route_cursor > MAX_SPACE_ORDERS)) return std::nullopt;
	SpaceMiningContinuationState state;
	state.phase = static_cast<MiningContinuationPhase>(wire.phase); state.previous_body = OrbitalBodyID{wire.previous_body};
	for (uint index = 0; index < state.mineral.size(); ++index) state.mineral[index] = static_cast<uint8_t>(wire.mineral >> ((3 - index) * 8));
	if (wire.effective_present != 0) {
		const auto kind = static_cast<SpaceOrderTargetKind>(wire.effective_kind);
		// Effective destinations belong only to natural mining or the original
		// return domains. Reusable pool targets always retain their incarnation.
		if (wire.effective_id == UINT32_MAX || (kind != SpaceOrderTargetKind::Body && kind != SpaceOrderTargetKind::Structure && kind != SpaceOrderTargetKind::LaunchCenter) ||
			(kind == SpaceOrderTargetKind::Body ? wire.effective_generation != 0 : wire.effective_generation == 0)) return std::nullopt;
		state.effective_target = SpaceOrderTarget{kind, wire.effective_id, wire.effective_generation};
	} else if (wire.effective_kind != 0 || wire.effective_id != UINT32_MAX || wire.effective_generation != 0) return std::nullopt; // Absent options cannot hide another target.
	if (wire.departure_present != 0) {
		state.departure_point = SpacePoint{wire.departure_x, wire.departure_y};
	} else if (wire.departure_x != 0 || wire.departure_y != 0) return std::nullopt; // Absent departure has no hidden physical destination.
	state.retry_ticks = wire.retry_ticks; state.waiting = static_cast<MiningContinuationWait>(wire.waiting);
	state.route_cursor = wire.route_cursor; state.travel_ticks = wire.travel_ticks;
	return state;
}

/** @copydoc ValidateSpaceMiningContinuationSchema */
bool ValidateSpaceMiningContinuationSchema(std::span<const std::string_view> fields, bool supported)
{
	static constexpr std::string_view keys[]{"phase", "mineral", "previous_body", "effective_present", "effective_kind", "effective_id", "effective_generation",
		"departure_present", "departure_x", "departure_y", "retry_ticks", "waiting", "route_cursor", "travel_ticks"};
	const bool modern = supported;
	std::array<uint8_t, std::size(keys)> counts{};
	// Require every explicit current field once. Genuine older tables cannot
	// smuggle future intent through the framework's unknown-field skip behavior.
	for (const auto key : fields) {
		if (!key.starts_with("mission.continuation.")) continue;
		if (!modern) return false;
		const auto suffix = key.substr(std::string_view{"mission.continuation."}.size());
		const auto found = std::ranges::find(keys, suffix);
		if (found != std::end(keys) && ++counts[found - std::begin(keys)] != 1) return false;
	}
	/** @param count Occurrences of one required scalar key. @return Whether the original table uniquely supplies it. */
	return !modern || std::ranges::all_of(counts, [](uint8_t count) { return count == 1; });
}

/** @copydoc ValidateSpaceMiningContinuationPreview */
bool ValidateSpaceMiningContinuationPreview(const LoadCheckData &data, bool legacy_departure_supported)
{
	// Transport bounds and identities must be established before any cursor or
	// same-file role/mineral metadata participates in an effective-target join.
	if (!ValidateSpaceOrderRecords(data.space_order_preview_rows, data.next_space_vehicle_generation) ||
		!ValidateAsteroidIdentityRecords(data.asteroid_body_rows, data.asteroid_next_body_id) ||
		data.native_space_vehicle_rows.size() != data.space_order_preview_rows.size() || data.asteroid_mining_leases.size() > MAX_SPACE_VEHICLES) return false;
	// Every original hull contributes one queue and its own candidate role,
	// manifest and configured cargo meaning; no live pool or CargoSpec is read.
	for (const auto &record : data.space_order_preview_rows) {
		const auto native = std::ranges::find(data.native_space_vehicle_rows, record.vehicle, &NativeSpaceVehiclePreview::vehicle);
		if (native == data.native_space_vehicle_rows.end()) return false;
		const auto product = std::ranges::find(data.space_production_rows, SpaceCraftRef{record.vehicle, record.generation}, &SpaceProductionRecord::craft);
		if (product == data.space_production_rows.end()) return false;
		const auto model = product->state.content_version == 0 ? SPACE_PROBE_MODEL : product->state.model;
		const auto manifest = std::ranges::find(data.space_manifest_rows, SpaceCraftRef{record.vehicle, record.generation}, &SpaceManifestRecord::craft);
		const bool pods = manifest != data.space_manifest_rows.end() && !manifest->pods.empty();
		const auto meaning = std::ranges::find(data.native_cargo_dimension_rows, native->freight_cargo, &NativeCargoDimensionPreview::cargo);
		const auto label = meaning == data.native_cargo_dimension_rows.end() ? std::nullopt : std::optional{meaning->label};
		const auto &intent = native->mining_continuation;
		// The unchanged scalar layout cannot opt an older443 source into the
		// new unqueued semantics. Genuine older empty queues remain inert only.
		if (!legacy_departure_supported && record.queue.entries.empty() && intent != SpaceMiningContinuationState{}) return false;
		/** @param row This candidate's configured cargo meaning. @return Whether the pinned immutable mineral is actually defined here. */
		const bool defined = std::ranges::any_of(data.native_cargo_dimension_rows, [&](const auto &row) { return row.label == intent.mineral; });
		const auto phase = static_cast<SpaceMissionPhase>(native->mining_phase);
		if (!ValidateSpaceMiningContinuation(intent, {record.queue, data.asteroid_body_rows, data.asteroid_next_body_id,
			GetSpacePlaneBounds(data.map_size_x, data.map_size_y), model, phase, OrbitalBodyID{native->mining_target}, native->mining_work_ticks,
			!native->cargo_packet_refs.empty(), defined, label, GetSpaceReturnRoute(model, record.queue, pods, product->state.home),
			data.next_space_structure_generation, data.next_launch_center_generation})) return false;
		// Native queue installation also requires unstarted typed actions to
		// remain Holding. They cannot inherit a separate active pursuit/lease.
		if (!record.queue.entries.empty() && !record.queue.started && phase != SpaceMissionPhase::Holding) return false;
		// Retained original body targets are history only after issuance; neither
		// an unstarted instruction nor a detached wait can claim a future body.
		for (const auto &order : record.queue.entries) {
			if (order.target.kind == SpaceOrderTargetKind::Body && order.target.id >= data.asteroid_next_body_id) return false;
		}
		// Active extraction must own this exact same-file ASTR lease. The reverse
		// pass below also rejects an orphaned or duplicate lease after detachment.
		if (phase == SpaceMissionPhase::Mining && std::ranges::find(data.asteroid_mining_leases,
			std::pair{OrbitalBodyID{native->mining_target}, record.vehicle}) == data.asteroid_mining_leases.end()) return false;
	}
	OrbitalBodyID previous = INVALID_ORBITAL_BODY; std::set<VehicleID> owners;
	// Stable original ASTR lease rows must each resolve back to the same one
	// active native miner. A copied hull cannot reserve two bodies after reload.
	for (const auto &[body, vehicle] : data.asteroid_mining_leases) {
		if ((previous != INVALID_ORBITAL_BODY && body <= previous) || !owners.insert(vehicle).second || FindAsteroidIdentity(data.asteroid_body_rows, body) == nullptr) return false;
		previous = body;
		const auto native = std::ranges::find(data.native_space_vehicle_rows, vehicle, &NativeSpaceVehiclePreview::vehicle);
		if (native == data.native_space_vehicle_rows.end() || native->mining_phase != to_underlying(SpaceMissionPhase::Mining) || native->mining_target != body.base()) return false;
	}
	return true;
}
