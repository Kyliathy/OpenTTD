/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_order_sl.cpp Bounded generation/order/approach transport beside native VEHS.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group23; per-Mine policy preserves original queue targets.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Groups6/11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C heavy native role/version admission.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 18; policy bytes and departure admission follow capabilities, and the writer emits its own row layout.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 21; a failed load's null pass revokes staged and installed state.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_vehicle.h"
#include "../space_mining_tuning.h"
#include "../space_approach.h"
#include "../space_structure.h"
#include "../space_transition.h"
#include "space_mining_sl.h"
#include "../timer/timer_game_tick.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include <bit>
#include "../safeguards.h"

/** Detached same-file queues; installation never allocates another hull or target. */
struct SpaceOrderTransport { uint64_t next = 1; std::vector<SpaceOrderRecord> records; };
static std::optional<SpaceOrderTransport> _candidate;
static constexpr size_t SPACE_APPROACH_REQUEST_BYTES = 1 + 4 + 8 + 8 + 1 + 4 + 4; ///< Target domain/ID/generation, priority, slot and two signed staging coordinates.
/** @copydoc ResetSpaceOrderLoadStaging */
void ResetSpaceOrderLoadStaging() { _candidate.reset(); }
/** @copydoc AcceptLoadedSpaceOrders */
bool AcceptLoadedSpaceOrders()
{
	if (!_candidate) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders)) return false;
		// Legacy native craft retain their paid body, mission, cargo, work and costs. The empty queue grants no new actions.
		SpaceOrderTransport legacy;
		for (const auto *v : SpaceVehicle::Iterate()) {
			const uint64_t generation = uint64_t{v->index.base()} + 1;
			legacy.records.push_back({v->index, generation, {}}); legacy.next = std::max(legacy.next, generation + 1);
		}
		return InstallLoadedSpaceOrders(legacy.records, legacy.next);
	}
	if (!InstallLoadedSpaceOrders(_candidate->records, _candidate->next)) return false;
	const bool heavy_runtime = !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerMissions);
	// Native reconstruction and copied preview enforce the same source boundary: a431 paid hull cannot arrive with future mining progress or future Mine instructions.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		const auto model = GetSpaceVehicleModel(*craft);
		// Source443's fields describe configured Mine intent only. A genuinely
		// older file cannot forge newly supported unqueued legacy departure.
		if (!SlHasSaveCapability(SaveCapability::SpaceLegacyDeparture) && craft->space_orders.entries.empty() && craft->mission.continuation != SpaceMiningContinuationState{}) return false;
		if (!IsSpaceMiningStateSupported(model, craft->mission.phase, craft->mission.work_ticks, craft->extracted_units, heavy_runtime)) return false;
		// Retained queue history is gameplay capability too;431 admitted no typed21 actions, so even an unstarted Visit must refuse in that commissioning-only source.
		if (model == SPACE_HEAVY_MINER_MODEL && !heavy_runtime && !craft->space_orders.entries.empty()) return false;
	}
	_candidate.reset(); return true;
}
/** @copydoc ValidateSpaceOrderLoadPreview */
void ValidateSpaceOrderLoadPreview()
{
	// Current VEHS intent, ASTR leases and SPOR history must agree using only
	// this candidate. Older files retain their inert migrated continuation.
	if (SlHasSaveCapability(SaveCapability::SpaceMiningContinuation) && !ValidateSpaceMiningContinuationPreview(_load_check_data,
		SlHasSaveCapability(SaveCapability::SpaceLegacyDeparture))) SlErrorCorrupt("Invalid same-file mining continuation or lease preview");
	if (!_load_check_data.space_orders_present && !IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders)) SlErrorCorrupt("Missing Space order family");
	if (_load_check_data.space_orders_active && (_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Space order preview requires selected Space geometry");
	if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && _load_check_data.native_space_vehicle_rows.size() != _load_check_data.space_order_preview_refs.size()) SlErrorCorrupt("Native Space preview identity count mismatch");
	if (!ValidateSpaceApproachRecords(_load_check_data.space_order_preview_rows, _load_check_data.native_tick_counter,
		_load_check_data.map_size_x, _load_check_data.map_size_y)) SlErrorCorrupt("Invalid Space approach clock or geometry preview");
	for (const auto &r : _load_check_data.space_order_preview_rows) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply)) {
			const auto &native = _load_check_data.native_space_vehicle_rows;
			const auto found = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == r.vehicle; });
			if (found == native.end() || !_load_check_data.companies.contains(found->owner.base())) SlErrorCorrupt("Missing owned native Space vehicle preview");
			if (found->position.x < 0 || found->position.y < 0 || found->position.x > int64_t{_load_check_data.map_size_x - 1} * SPACE_Q16 || found->position.y > int64_t{_load_check_data.map_size_y - 1} * SPACE_Q16) SlErrorCorrupt("Invalid native Space position preview");
			if (r.queue.approach) {
				const auto &a = *r.queue.approach;
				if (a.target.kind == SpaceOrderTargetKind::Body) {
					if (FindAsteroidIdentity(_load_check_data.asteroid_body_rows, OrbitalBodyID{a.target.id}) == nullptr) SlErrorCorrupt("Missing Space approach body preview");
				} else {
					const auto &sites = _load_check_data.space_structure_rows;
					const auto site = std::ranges::find_if(sites, [&](const auto &s) { return s.ref.structure.base() == a.target.id && s.ref.generation == a.target.generation; });
					// Resolve only copied same-file identities: a reused or foreign site cannot become an admitted preview berth.
					if (site == sites.end() || site->state.owner != found->owner || (site->state.phase == SpaceStructurePhase::Cancelled && site->quantities[0] == 0 && site->quantities[1] == 0)) SlErrorCorrupt("Missing owned Space approach site preview");
					// The second slot belongs only to explicitly activated Dock functionality; inert frames remain single construction approaches.
					if (a.slot == 1 && (site->state.kind != SpaceBuildKind::Dock || !site->module || site->module->activation_ticks != 0)) SlErrorCorrupt("Missing Dock approach capacity preview");
				}
			}
			for (const auto &o : r.queue.entries) if (o.target.kind == SpaceOrderTargetKind::Craft && r.queue.blocked != SpaceOrderBlock::TargetRemoved) {
				const SpaceCraftRef target{VehicleID{o.target.id}, o.target.generation};
				if (std::find(_load_check_data.space_order_preview_refs.begin(), _load_check_data.space_order_preview_refs.end(), target) == _load_check_data.space_order_preview_refs.end()) continue;
				const auto receiver = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == target.vehicle; });
				if (receiver == native.end() || receiver->owner != found->owner) SlErrorCorrupt("Foreign native Space visit preview");
			}
		}
		const auto found = std::find_if(_load_check_data.space_production_rows.begin(), _load_check_data.space_production_rows.end(), [&](const auto &p) { return p.craft == SpaceCraftRef{r.vehicle, r.generation}; });
		if (found == _load_check_data.space_production_rows.end() && !IsSavegameVersionBefore(SaveLoadVersion::SpaceLaunchAssembly)) SlErrorCorrupt("Missing Space order production identity preview");
		const uint16_t model = found == _load_check_data.space_production_rows.end() || found->state.content_version == 0 ? 15 : found->state.model;
		const bool heavy_runtime = !IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerMissions);
		// Same-file VEHS fields and same-file paid role jointly admit heavy work; preview never borrows an active hull or assumes an idle phase.
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply)) {
			/** @param row Copied same-file hull. @return Whether its exact native ID belongs to this queue. */
			const auto native = std::ranges::find_if(_load_check_data.native_space_vehicle_rows, [&](const auto &row) { return row.vehicle == r.vehicle; });
			if (native == _load_check_data.native_space_vehicle_rows.end()) SlErrorCorrupt("Missing mining hull preview");
			const auto phase = static_cast<SpaceMissionPhase>(native->mining_phase);
			if (!IsSpaceMiningStateSupported(model, phase, native->mining_work_ticks, native->extracted_units, heavy_runtime)) SlErrorCorrupt("Invalid native mining role or source version preview");
			// Active pursuit/extraction must name a body actually present in this candidate file, not a matching body in the open world.
			if ((phase == SpaceMissionPhase::Approach || phase == SpaceMissionPhase::Mining) &&
				FindAsteroidIdentity(_load_check_data.asteroid_body_rows, OrbitalBodyID{native->mining_target}) == nullptr) SlErrorCorrupt("Missing native mining body preview");
			// A started Mine owns this exact native target; copied queue progress cannot point extraction at a different natural resource.
			if (r.queue.started && r.queue.cursor < r.queue.entries.size() && r.queue.entries[r.queue.cursor].action == SpaceOrderAction::Mine) {
				const auto &order = r.queue.entries[r.queue.cursor];
				// Configured replacement mining already passed its effective-target
				// join above; legacy work still owns the exact original body.
				const bool configured = order.mining_policy != SpaceMiningPolicy::LegacyReturn || native->mining_continuation.phase != MiningContinuationPhase::None;
				if (!configured && native->mining_target != order.target.id) SlErrorCorrupt("Changed native mining target preview");
			}
		}
		// Source431 introduced commissioning only; any typed21 history, including Visit without extraction, would smuggle later runtime admission into that older file.
		if (model == SPACE_HEAVY_MINER_MODEL && !heavy_runtime && !r.queue.entries.empty()) SlErrorCorrupt("Future heavy orders in commissioning-only save");
		for (const auto &o : r.queue.entries) if (!IsSpaceOrderAllowed(model, o)) SlErrorCorrupt("Invalid Space order role preview");
		// Partial scan work belongs to an actual same-file body, regardless of
		// whether that ID is numerically above the remaining live population.
		if (r.queue.work_ticks != 0 && FindAsteroidIdentity(_load_check_data.asteroid_body_rows,
			OrbitalBodyID{r.queue.entries[r.queue.cursor].target.id}) == nullptr) SlErrorCorrupt("Missing partial survey target preview");
		if (found != _load_check_data.space_production_rows.end() && found->state.placement != SpacePlacement::Orbital) {
			// Dormant/native rocket orders remain empty. Only its exact same-file unresolved module flight may own a shared in-Space request while InTransit.
			if (!r.queue.entries.empty()) SlErrorCorrupt("Dormant hull has active Space orders preview");
			if (r.queue.approach) {
				const SpaceCraftRef reference{r.vehicle, r.generation};
				const auto &flights = _load_check_data.space_transition_rows;
				const auto flight = std::ranges::find_if(flights, [&](const auto &flight) { return flight.craft == reference; });
				// A copied request cannot change destination domain, fly during Surface preparation or borrow a completed/cancelled flight's admission.
				if (found->state.model != 14 || found->state.placement != SpacePlacement::InTransit || flight == flights.end() || !IsSpaceModuleFreightApproach(flight->state, r.queue.approach->target)) SlErrorCorrupt("Invalid native module flight approach preview");
			}
		}
	}
}

/** Version410 queue prefix plus version420 bounded optional shared approach request. */
struct SPORChunkHandler : ChunkHandler {
	/** Register the existing native queue family; no new parallel hull schema. */
	SPORChunkHandler() : ChunkHandler("SPOR", ChunkType::Riff) {}
	/** @param value Canonical integer. @param width Big-endian byte width. */
	static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
	/** @param width Big-endian byte width. @return Canonical decoded integer. */
	static uint64_t ReadInt(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
	/** Save every actual hull's queue and sole contained approach after native clock/ownership validation. */
	void Save() const override
	{
		const auto records = CaptureSpaceOrders();
		const auto next = GetNextSpaceVehicleGeneration();
		if (!ValidateSpaceOrderRecords(records, next) || !ValidateNativeSpaceApproaches()) SlErrorCorrupt("Invalid Space order ownership on save");
		// The writer's own row layout: merged 446 carries the policy byte, merged 441
		// does not (m2o28 Group 18). A 441 writer cannot hold a non-legacy policy, so it
		// refuses rather than write a file that silently drops the player's choice.
		const bool policies = SlHasSaveCapability(SaveCapability::SpaceMiningContinuation);
		const size_t order_bytes = policies ? 15 : 14;
		size_t length = 9;
		// Bound optional request bytes along with existing orders before declaring the exact family length.
		for (const auto &r : records) {
			length += 23 + order_bytes * r.queue.entries.size() + (r.queue.approach ? SPACE_APPROACH_REQUEST_BYTES : 0);
			for (const auto &o : r.queue.entries) {
				if (!policies && o.mining_policy != SpaceMiningPolicy::LegacyReturn) SlErrorCorrupt("Space mining policy needs the combined writer");
			}
		}
		SlSetLength(length); Write(next, 8); Write(records.size(), 1);
		// Native pool order provides stable row serialization; admission priority is explicitly saved rather than inferred from row order.
		for (const auto &r : records) {
			const auto &q = r.queue;
			Write(r.vehicle.base(), 4); Write(r.generation, 8); Write(q.cursor, 1); Write(q.started, 1); Write(to_underlying(q.blocked), 1);
			Write(q.timer_ticks, 4); Write(q.work_ticks, 2); Write(q.entries.size(), 1);
			for (const auto &o : q.entries) {
				Write(to_underlying(o.action), 1); Write(to_underlying(o.target.kind), 1); Write(o.target.id, 4); Write(o.target.generation, 8);
				if (policies) Write(to_underlying(o.mining_policy), 1);
			}
			Write(q.approach.has_value(), 1);
			if (q.approach) {
				const auto &a = *q.approach;
				Write(to_underlying(a.target.kind), 1); Write(a.target.id, 4); Write(a.target.generation, 8); Write(a.request_tick, 8); Write(a.slot, 1);
				Write(static_cast<uint32_t>(a.staging.x), 4); Write(static_cast<uint32_t>(a.staging.y), 4);
			}
		}
	}
	/** @return Bounded copied transport; old files migrate with no reservation or implicit berth admission. */
	SpaceOrderTransport Read() const
	{
		const auto length = SlGetFieldLength();
		const bool approaches = !IsSavegameVersionBefore(SaveLoadVersion::SpaceApproachReservations);
		const bool policies = SlHasSaveCapability(SaveCapability::SpaceMiningContinuation);
		const size_t order_bytes = policies ? 15 : 14;
		// Sources without the capability keep their original 14-byte order formula,
		// and their policy stays LegacyReturn. Policy bytes cannot cross the bounded
		// queue or optional approach owner.
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders) || length < 9 || length > 9 + MAX_SPACE_VEHICLES * (22 + order_bytes * MAX_SPACE_ORDERS + (approaches ? 1 + SPACE_APPROACH_REQUEST_BYTES : 0))) SlErrorCorrupt("Invalid Space order length");
		SpaceOrderTransport data;
		data.next = ReadInt(8); const auto count = ReadInt(1);
		if (count > MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid Space vehicle identity count");
		size_t consumed = 9;
		// Validate each variable-length row before reading its optional fields, preventing malformed queues from crossing chunk boundaries.
		for (size_t i = 0; i < count; ++i) {
			if (consumed + 22 > length) SlErrorCorrupt("Truncated Space order record");
			SpaceOrderRecord r; auto &q = r.queue;
			r.vehicle = VehicleID{static_cast<uint32_t>(ReadInt(4))}; r.generation = ReadInt(8);
			q.cursor = static_cast<uint8_t>(ReadInt(1)); const auto started = ReadInt(1);
			if (started > 1) SlErrorCorrupt("Invalid Space order boolean");
			q.started = started != 0; q.blocked = static_cast<SpaceOrderBlock>(ReadInt(1));
			if (!approaches && q.blocked >= SpaceOrderBlock::ApproachBusy) SlErrorCorrupt("Unexpected approach wait in old Space orders");
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceConstructionJobs) && q.blocked >= SpaceOrderBlock::MaterialsMissing) SlErrorCorrupt("Unexpected construction wait in old Space orders");
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && q.blocked >= SpaceOrderBlock::ReturnsPending) SlErrorCorrupt("Unexpected mining budget wait in old Space orders");
			q.timer_ticks = static_cast<uint32_t>(ReadInt(4)); q.work_ticks = static_cast<uint16_t>(ReadInt(2)); const auto orders = ReadInt(1);
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge) && q.work_ticks != 0) SlErrorCorrupt("Unexpected partial scan in old Space orders");
			consumed += 22 + order_bytes * orders;
			if (orders > MAX_SPACE_ORDERS || consumed > length) SlErrorCorrupt("Invalid Space order count");
			for (size_t j = 0; j < orders; ++j) {
				SpaceOrder o; o.action = static_cast<SpaceOrderAction>(ReadInt(1)); o.target.kind = static_cast<SpaceOrderTargetKind>(ReadInt(1));
				if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && o.target.kind == SpaceOrderTargetKind::Craft) SlErrorCorrupt("Unexpected craft target in old Space orders");
				o.target.id = static_cast<uint32_t>(ReadInt(4)); o.target.generation = ReadInt(8);
				if (policies) o.mining_policy = static_cast<SpaceMiningPolicy>(ReadInt(1));
				q.entries.push_back(o);
			}
			if (approaches) {
				if (++consumed > length) SlErrorCorrupt("Truncated Space approach presence");
				const auto present = ReadInt(1);
				if (present > 1) SlErrorCorrupt("Invalid Space approach presence");
				if (present != 0) {
					consumed += SPACE_APPROACH_REQUEST_BYTES;
					if (consumed > length) SlErrorCorrupt("Truncated Space approach request");
					SpaceApproachRequest a;
					a.target.kind = static_cast<SpaceOrderTargetKind>(ReadInt(1)); a.target.id = static_cast<uint32_t>(ReadInt(4)); a.target.generation = ReadInt(8);
					a.request_tick = ReadInt(8); a.slot = static_cast<uint8_t>(ReadInt(1));
					a.staging.x = std::bit_cast<int32_t>(static_cast<uint32_t>(ReadInt(4))); a.staging.y = std::bit_cast<int32_t>(static_cast<uint32_t>(ReadInt(4)));
					q.approach = a;
				}
			}
			data.records.push_back(std::move(r));
		}
		if (consumed != length || !ValidateSpaceOrderRecords(data.records, data.next)) SlErrorCorrupt("Invalid Space order transport");
		return data;
	}
	/** Decode detached requests without mutating native hulls, positions or target slots. */
	void Load() const override { if (_candidate) SlErrorCorrupt("Repeated Space order family"); _candidate = this->Read(); }
	/** Check required same-file family/profile presence before installation.
	 * A failed load's null pass drops the candidate queues and the generation issuer, so a late refusal
	 * cannot lend them to the next load (m2o28 Group 21).
	 */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) { ResetSpaceOrders(); return; }
		if (!_candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders)) SlErrorCorrupt("Missing Space order family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (_candidate && (_candidate->next != 1 || !_candidate->records.empty()) && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space orders require selected profile");
	}
	/** @param length Chunk length supplied by the loader; the bounded decoder validates its actual transport. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.space_orders_present) SlErrorCorrupt("Repeated Space order preview");
		const auto data = this->Read(); _load_check_data.space_orders_present = true; _load_check_data.space_orders_active = data.next != 1 || !data.records.empty();
		_load_check_data.next_space_vehicle_generation = data.next;
		for (const auto &r : data.records) _load_check_data.space_order_preview_refs.push_back({r.vehicle, r.generation});
		_load_check_data.space_order_preview_rows = data.records;
	}
};
static const SPORChunkHandler SPOR;
static const ChunkHandlerRef space_order_handlers[] = {SPOR};
extern const ChunkHandlerTable _space_order_chunk_handlers(space_order_handlers);
