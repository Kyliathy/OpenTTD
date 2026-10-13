/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_order_sl.cpp Bounded generation/order transport beside native VEHS; Plan8 Group6. */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../space_vehicle.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

struct SpaceOrderTransport { uint64_t next = 1; std::vector<SpaceOrderRecord> records; };
static std::optional<SpaceOrderTransport> _candidate;
void ResetSpaceOrderLoadStaging() { _candidate.reset(); }
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
	_candidate.reset(); return true;
}
void ValidateSpaceOrderLoadPreview()
{
	if (!_load_check_data.space_orders_present && !IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders)) SlErrorCorrupt("Missing Space order family");
	if (_load_check_data.space_orders_active && (_load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || _load_check_data.map_layers < 3)) SlErrorCorrupt("Space order preview requires selected Space geometry");
	if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && _load_check_data.native_space_vehicle_rows.size() != _load_check_data.space_order_preview_refs.size()) SlErrorCorrupt("Native Space preview identity count mismatch");
	for (const auto &r : _load_check_data.space_order_preview_rows) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply)) {
			const auto &native = _load_check_data.native_space_vehicle_rows;
			const auto found = std::find_if(native.begin(), native.end(), [&](const auto &v) { return v.vehicle == r.vehicle; });
			if (found == native.end() || !_load_check_data.companies.contains(found->owner.base())) SlErrorCorrupt("Missing owned native Space vehicle preview");
			if (found->position.x < 0 || found->position.y < 0 || found->position.x > int64_t{_load_check_data.map_size_x - 1} * SPACE_Q16 || found->position.y > int64_t{_load_check_data.map_size_y - 1} * SPACE_Q16) SlErrorCorrupt("Invalid native Space position preview");
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
		for (const auto &o : r.queue.entries) if (!IsSpaceOrderAllowed(model, o)) SlErrorCorrupt("Invalid Space order role preview");
		if (r.queue.work_ticks != 0 && r.queue.entries[r.queue.cursor].target.id >= _load_check_data.asteroid_body_count) SlErrorCorrupt("Missing partial survey target preview");
		if (!r.queue.entries.empty() && found != _load_check_data.space_production_rows.end() && found->state.placement != SpacePlacement::Orbital) SlErrorCorrupt("Dormant hull has active Space orders preview");
	}
}

struct SPORChunkHandler : ChunkHandler {
	SPORChunkHandler() : ChunkHandler("SPOR", ChunkType::Riff) {}
	static void Write(uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift)); }
	static uint64_t ReadInt(uint width) { uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value; }
	void Save() const override
	{
		const auto records = CaptureSpaceOrders();
		const auto next = GetNextSpaceVehicleGeneration();
		if (!ValidateSpaceOrderRecords(records, next)) SlErrorCorrupt("Invalid Space order ownership on save");
		size_t length = 9; for (const auto &r : records) length += 22 + 14 * r.queue.entries.size();
		SlSetLength(length); Write(next, 8); Write(records.size(), 1);
		for (const auto &r : records) {
			const auto &q = r.queue;
			Write(r.vehicle.base(), 4); Write(r.generation, 8); Write(q.cursor, 1); Write(q.started, 1); Write(to_underlying(q.blocked), 1);
			Write(q.timer_ticks, 4); Write(q.work_ticks, 2); Write(q.entries.size(), 1);
			for (const auto &o : q.entries) { Write(to_underlying(o.action), 1); Write(to_underlying(o.target.kind), 1); Write(o.target.id, 4); Write(o.target.generation, 8); }
		}
	}
	SpaceOrderTransport Read() const
	{
		const auto length = SlGetFieldLength();
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders) || length < 9 || length > 9 + MAX_SPACE_VEHICLES * (22 + 14 * MAX_SPACE_ORDERS)) SlErrorCorrupt("Invalid Space order length");
		SpaceOrderTransport data;
		data.next = ReadInt(8); const auto count = ReadInt(1);
		if (count > MAX_SPACE_VEHICLES) SlErrorCorrupt("Invalid Space vehicle identity count");
		size_t consumed = 9;
		for (size_t i = 0; i < count; ++i) {
			if (consumed + 22 > length) SlErrorCorrupt("Truncated Space order record");
			SpaceOrderRecord r; auto &q = r.queue;
			r.vehicle = VehicleID{static_cast<uint32_t>(ReadInt(4))}; r.generation = ReadInt(8);
			q.cursor = static_cast<uint8_t>(ReadInt(1)); const auto started = ReadInt(1);
			if (started > 1) SlErrorCorrupt("Invalid Space order boolean");
			q.started = started != 0; q.blocked = static_cast<SpaceOrderBlock>(ReadInt(1));
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && q.blocked >= SpaceOrderBlock::ReturnsPending) SlErrorCorrupt("Unexpected mining budget wait in old Space orders");
			q.timer_ticks = static_cast<uint32_t>(ReadInt(4)); q.work_ticks = static_cast<uint16_t>(ReadInt(2)); const auto orders = ReadInt(1);
			if (IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge) && q.work_ticks != 0) SlErrorCorrupt("Unexpected partial scan in old Space orders");
			consumed += 22 + 14 * orders;
			if (orders > MAX_SPACE_ORDERS || consumed > length) SlErrorCorrupt("Invalid Space order count");
			for (size_t j = 0; j < orders; ++j) {
				SpaceOrder o; o.action = static_cast<SpaceOrderAction>(ReadInt(1)); o.target.kind = static_cast<SpaceOrderTargetKind>(ReadInt(1));
				if (IsSavegameVersionBefore(SaveLoadVersion::SpaceOrbitalResupply) && o.target.kind == SpaceOrderTargetKind::Craft) SlErrorCorrupt("Unexpected craft target in old Space orders");
				o.target.id = static_cast<uint32_t>(ReadInt(4)); o.target.generation = ReadInt(8); q.entries.push_back(o);
			}
			data.records.push_back(std::move(r));
		}
		if (consumed != length || !ValidateSpaceOrderRecords(data.records, data.next)) SlErrorCorrupt("Invalid Space order transport");
		return data;
	}
	void Load() const override { if (_candidate) SlErrorCorrupt("Repeated Space order family"); _candidate = this->Read(); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!_candidate && !IsSavegameVersionBefore(SaveLoadVersion::SpaceContentAndOrders)) SlErrorCorrupt("Missing Space order family");
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (_candidate && (_candidate->next != 1 || !_candidate->records.empty()) && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry)) SlErrorCorrupt("Space orders require selected profile");
	}
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
