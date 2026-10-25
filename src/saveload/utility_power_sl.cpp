/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file utility_power_sl.cpp Bounded shared typed energy epochs, exact meters, domain roles, connectivity and grant-slice transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5 paid native electrical links
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group7B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md schema authority
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../utility_power.h"
#include "../utility_power_lineage.h"
#include "../utility_power_conversion_wire.h"
#include "../multilayer_state.h"
#include "../map_layer.h"
#include "../fios.h"

#include "../safeguards.h"

/** Explicit wire row kinds; no saved ordinal is reinterpreted between source, consumer, link and reservation. */
enum class UtilityWireKind : uint8_t { Source, Consumer, Connection, Share, Link };

/** Fixed scalar transport; documented union slots are interpreted only after the row kind is validated. */
struct UtilityWireRow {
	uint8_t type = 0; ///< Source/consumer/connection/share discriminator.
	uint32_t id = 0; ///< Source ID for Source; consumer ID for every other row.
	uint32_t reference = 0; ///< Producer ID for Connection/Share; zero otherwise.
	uint8_t owner = 0; ///< Actual company for source/consumer; zero for subordinate rows.
	uint8_t layer = 0; ///< Source/consumer explicit output layer; zero otherwise.
	uint8_t kind = 0; ///< Generation family or native consumer effect; zero otherwise.
	uint8_t active = 0; ///< Source liveness; version385 consumer liveness; zero for subordinate kinds.
	uint64_t epoch = 0; ///< Source publication/consumer allocation interval; zero otherwise.
	uint64_t a = 0; ///< Source rate / consumer capacity / share original allocated joules / legacy portal interval cap / declared local or paid-riser watt rate.
	uint64_t b = 0; ///< Source generated joules / consumer demand / share consumed joules.
	uint64_t c = 0; ///< Source reserved joules / consumer original grant / share released joules.
	uint64_t d = 0; ///< Consumer unmet demand; zero otherwise.
	uint32_t ticks = 0; ///< Source synchronized production duration; zero otherwise.
	uint16_t remainder = 0; ///< Source saved millijoule carry; zero otherwise.
	uint64_t lifetime_generated = 0; ///< Version382 source lifetime generation, never imported destination output.
	uint64_t lifetime_consumed = 0; ///< Version382 settled original-source use survives rotation and source loss.
	uint64_t transferred = 0; ///< Version383 original slice quantity moved to destination imports; zero otherwise.
	uint32_t imported_link = 0; ///< Version383 share incoming route or invalid sentinel; zero in other row kinds.
	uint32_t destination = 0; ///< Version383 directed link's consumer destination; zero otherwise.
	uint32_t source_endpoint = 0; ///< Version383 directed link's explicit source world tile; zero otherwise.
	uint32_t destination_endpoint = 0; ///< Version383 directed link's reciprocal destination world tile; zero otherwise.
	uint16_t town = 0; ///< Version384 consumer's actual town or invalid sentinel; zero in other kinds.
	uint8_t town_bonus = 0; ///< Version384 accepted current native benefit, bounded0..25.
	uint8_t town_applied = 0; ///< Version384 once-only actual city effect settlement.
	uint8_t town_suspended = 0; ///< Version384 custom-GS suppression for the remainder of this interval.
	uint8_t domain = 0; ///< Version390 source/consumer immutable quantity domain; subordinate/link rows keep zero and genuine older rows remain Electricity.
	uint16_t riser = 0; ///< Version401 actual paid native machinery ID for ElectricalRiser links only; reserved zero in every older/different row role.
	uint32_t share = 0; ///< Version403 shared slice identity, used only by reservation rows.
	uint32_t parent = 0; ///< Version403 exact preceding slice, or invalid for a root; reserved zero in other roles.
	uint16_t town_terminal = 0; ///<431 consumer-only original paid Surface station; other wire kinds must retain zero.
	uint32_t town_supply = 0; ///<431 consumer-only original Deep forwarding reservation; never energy.
	uint32_t town_source_port = 0; ///<431 consumer-only native powerhouse boundary tile.
	uint16_t generator = 0; ///<432 ElectricalFeed-only original native generator; every other wire role keeps zero.
};

static const SaveLoad _utility_row_desc[] = {
	SaveLoad::Variable<VarFileType::U8>("type", SLE_OBJECT_ADDRESS(UtilityWireRow, type)),
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(UtilityWireRow, id)),
	SaveLoad::Variable<VarFileType::U32>("reference", SLE_OBJECT_ADDRESS(UtilityWireRow, reference)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(UtilityWireRow, owner)),
	SaveLoad::Variable<VarFileType::U8>("layer", SLE_OBJECT_ADDRESS(UtilityWireRow, layer)),
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(UtilityWireRow, kind)),
	SaveLoad::Variable<VarFileType::U8>("active", SLE_OBJECT_ADDRESS(UtilityWireRow, active)),
	SaveLoad::Variable<VarFileType::U64>("epoch", SLE_OBJECT_ADDRESS(UtilityWireRow, epoch)),
	SaveLoad::Variable<VarFileType::U64>("a", SLE_OBJECT_ADDRESS(UtilityWireRow, a)),
	SaveLoad::Variable<VarFileType::U64>("b", SLE_OBJECT_ADDRESS(UtilityWireRow, b)),
	SaveLoad::Variable<VarFileType::U64>("c", SLE_OBJECT_ADDRESS(UtilityWireRow, c)),
	SaveLoad::Variable<VarFileType::U64>("d", SLE_OBJECT_ADDRESS(UtilityWireRow, d)),
	SaveLoad::Variable<VarFileType::U32>("ticks", SLE_OBJECT_ADDRESS(UtilityWireRow, ticks)),
	SaveLoad::Variable<VarFileType::U16>("remainder", SLE_OBJECT_ADDRESS(UtilityWireRow, remainder)),
	SaveLoad::Variable<VarFileType::U64>("lifetime_generated", SLE_OBJECT_ADDRESS(UtilityWireRow, lifetime_generated), SaveLoadVersion::UtilityMutations),
	SaveLoad::Variable<VarFileType::U64>("lifetime_consumed", SLE_OBJECT_ADDRESS(UtilityWireRow, lifetime_consumed), SaveLoadVersion::UtilityMutations),
	SaveLoad::Variable<VarFileType::U64>("transferred", SLE_OBJECT_ADDRESS(UtilityWireRow, transferred), SaveLoadVersion::UtilityTransfers),
	SaveLoad::Variable<VarFileType::U32>("imported_link", SLE_OBJECT_ADDRESS(UtilityWireRow, imported_link), SaveLoadVersion::UtilityTransfers),
	SaveLoad::Variable<VarFileType::U32>("destination", SLE_OBJECT_ADDRESS(UtilityWireRow, destination), SaveLoadVersion::UtilityTransfers),
	SaveLoad::Variable<VarFileType::U32>("source_endpoint", SLE_OBJECT_ADDRESS(UtilityWireRow, source_endpoint), SaveLoadVersion::UtilityTransfers),
	SaveLoad::Variable<VarFileType::U32>("destination_endpoint", SLE_OBJECT_ADDRESS(UtilityWireRow, destination_endpoint), SaveLoadVersion::UtilityTransfers),
	SaveLoad::Variable<VarFileType::U16>("town", SLE_OBJECT_ADDRESS(UtilityWireRow, town), SaveLoadVersion::UtilityConsumerEffects),
	SaveLoad::Variable<VarFileType::U8>("town_bonus", SLE_OBJECT_ADDRESS(UtilityWireRow, town_bonus), SaveLoadVersion::UtilityConsumerEffects),
	SaveLoad::Variable<VarFileType::U8>("town_applied", SLE_OBJECT_ADDRESS(UtilityWireRow, town_applied), SaveLoadVersion::UtilityConsumerEffects),
	SaveLoad::Variable<VarFileType::U8>("town_suspended", SLE_OBJECT_ADDRESS(UtilityWireRow, town_suspended), SaveLoadVersion::UtilityConsumerEffects),
	SaveLoad::Variable<VarFileType::U8>("domain", SLE_OBJECT_ADDRESS(UtilityWireRow, domain), SaveLoadVersion::UtilityHeatDomains),
	SaveLoad::Variable<VarFileType::U16>("riser", SLE_OBJECT_ADDRESS(UtilityWireRow, riser), SaveLoadVersion::PaidElectricalRiserLinks),
	SaveLoad::Variable<VarFileType::U32>("share", SLE_OBJECT_ADDRESS(UtilityWireRow, share), SaveLoadVersion::UtilityShareLineage),
	SaveLoad::Variable<VarFileType::U32>("parent", SLE_OBJECT_ADDRESS(UtilityWireRow, parent), SaveLoadVersion::UtilityShareLineage),
	SaveLoad::Variable<VarFileType::U16>("town_terminal", SLE_OBJECT_ADDRESS(UtilityWireRow, town_terminal)).WithCapability(SaveCapability::UtilityTownService),
	SaveLoad::Variable<VarFileType::U32>("town_supply", SLE_OBJECT_ADDRESS(UtilityWireRow, town_supply)).WithCapability(SaveCapability::UtilityTownService),
	SaveLoad::Variable<VarFileType::U32>("town_source_port", SLE_OBJECT_ADDRESS(UtilityWireRow, town_source_port)).WithCapability(SaveCapability::UtilityTownService),
	SaveLoad::Variable<VarFileType::U16>("generator", SLE_OBJECT_ADDRESS(UtilityWireRow, generator)).WithCapability(SaveCapability::UtilityLocalElectricalFeed),
};

static std::array<bool, 2> _utility_presence{}; ///< Unaccepted header/table presence, separate from live authority.
static std::array<uint64_t, 14> _utility_header{}; ///< Shared identity/phase/counts, local topology and403 exact slice cursor; generation never reconstructs installed cells.
static std::array<uint32_t, 5> _utility_counts{}; ///< Actual decoded counts by wire kind, including directed links.
static UtilityPowerState _utility_rows; ///< Structurally decoded transport, published only after actual company fixups.
static std::array<bool, 2> _utility_path_presence{}; ///< Unpublished403 reference-family header/table presence.
static std::array<uint64_t, 3> _utility_path_header{}; ///< Exact revision/epoch/reference count, independent of live resources.
static std::vector<UtilityPowerPathWire> _utility_path_rows; ///< Copied physical references only; no source balance is duplicated here.

/** Clear unpublished file transport when a world initializes, without creating any utility account. */
void ResetUtilityPowerLoadStaging()
{
	_utility_presence.fill(false);
	_utility_header.fill(0);
	_utility_counts.fill(0);
	_utility_rows = {};
	_utility_path_presence.fill(false); _utility_path_header.fill(0); _utility_path_rows.clear();
	ResetUtilityHeatConversionLoadStaging();
}

/**
 * Reconcile the entire family and metadata bit before staging or accepting preview.
 * @param preview True reads copied preview transport without touching live state.
 * @return Whether a complete family exists; older legitimate absence remains empty.
 */
static bool CheckUtilityPowerFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.utility_power_present : _utility_presence;
	const auto &header = preview ? _load_check_data.utility_power_header : _utility_header;
	const auto &counts = preview ? _load_check_data.utility_power_rows : _utility_counts;
	auto &rows = preview ? _load_check_data.utility_power_state : _utility_rows;
	const auto *metadata = preview ? nullptr : GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_UTILITY_POWER) != 0;
	if (!presence[0]) {
		// New files always carry both family chunks; older files may omit them
		// only when no metadata claims utility history or stray table exists.
		if (!IsSavegameVersionBefore(SaveLoadVersion::UtilityPower) || presence[1] || required) SlErrorCorrupt("Missing utility power header");
		return false;
	}
	if (!presence[1]) SlErrorCorrupt("Missing utility power table");
	// Exact header counts prevent missing/trailing records from silently
	// replacing authoritative source fractions or consumer reservations.
	for (size_t kind = 0; kind < counts.size(); ++kind) {
		if (counts[kind] != header[kind == 4 ? 11 : 5 + kind]) SlErrorCorrupt("Utility power row count mismatch");
	}
	const bool history = header[1] != 1 || header[2] != 0 || header[3] != 0 || header[4] != 0 || header[9] != 0 || header[10] != 0 || header[13] != 0;
	if (required != history) SlErrorCorrupt("Utility power metadata/history mismatch");
	/** @param source Copied native source. @return Whether this file retains a thermal producer identity. */
	const auto heat_source = [](const UtilityPowerProducer &source) { return source.meter.domain == UtilityEnergyDomain::Heat; };
	/** @param consumer Copied native consumer. @return Whether this file retains a thermal request/grant identity. */
	const auto heat_consumer = [](const UtilityPowerConsumer &consumer) { return consumer.meter.domain == UtilityEnergyDomain::Heat; };
	const bool heat_history = rows.heat_allocated || std::ranges::any_of(rows.producers, heat_source) || std::ranges::any_of(rows.consumers, heat_consumer);
	if (((flags & MULTILAYER_FEATURE_UTILITY_HEAT) != 0) != heat_history) SlErrorCorrupt("Utility heat metadata/history mismatch"); // Explicit saved domain/phase history cannot be borrowed from the running world or omitted from its marker.
	if (((flags & MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS) != 0) != (rows.local_link_revision != 0)) SlErrorCorrupt("Local utility metadata/history mismatch"); // Installed geometry history cannot be omitted or fabricated from an energy account.
	if (!IsSavegameVersionBefore(SaveLoadVersion::UtilityShareLineage)) {
		const auto &path_presence = preview ? _load_check_data.utility_path_present : _utility_path_presence;
		const auto &path_header = preview ? _load_check_data.utility_path_header : _utility_path_header;
		const auto &path_rows = preview ? _load_check_data.utility_path_rows : _utility_path_rows;
		// Every current file carries this exact reference family, including an
		// empty graph. Omitted paths cannot default into another usable import.
		if (!path_presence[0] || !path_presence[1] || path_header[1] != rows.interval || path_header[2] != path_rows.size()) SlErrorCorrupt("Missing or mismatched utility lineage family");
		std::map<uint32_t, UtilityPowerShare *> slices;
		// Reconcile only unpublished transport. Repeated family checks rebuild
		// segments from the same copied rows rather than appending them again.
		for (auto &consumer : rows.consumers) {
			for (auto &share : consumer.shares) {
				if (!slices.emplace(share.id.base(), &share).second) SlErrorCorrupt("Duplicate utility slice identity");
				share.segment.clear();
			}
		}
		for (const auto &path : path_rows) {
			const auto share = slices.find(path.share);
			if (share == slices.end() || path.ordinal != share->second->segment.size() || path.link >= rows.links.size()) SlErrorCorrupt("Invalid utility lineage reference/ordinal"); // Every path belongs to an actual retained slice and consecutive original installed references.
			share->second->segment.push_back(UtilityLinkID{path.link});
		}
	} else if (MigrateUtilityPowerLineage(rows) != UtilityPowerError::None) SlErrorCorrupt("Invalid older utility lineage");
	if (preview) {
		// Physical roles resolve in this candidate's native geometry. Neither a
		// coincident live tile nor a scalar division can supply missing endpoints.
		for (const auto &link : rows.links) {
			if (link.kind == UtilityLinkKind::ElectricalFeed) {
				const auto source = MultiLayerMap::DecodeStoredTile(link.source_endpoint, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
				const auto destination = MultiLayerMap::DecodeStoredTile(link.destination_endpoint, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
				// A local electrical feed joins two original Deep boundaries; wrong-plane references must never acquire usable power.
				if (!source.has_value() || !destination.has_value() || source->layer != MapLayer::DeepUnderground || destination->layer != MapLayer::DeepUnderground) SlErrorCorrupt("Invalid local electrical feed preview geometry");
			}
			if (link.kind != UtilityLinkKind::LocalCell) continue;
			const auto source = MultiLayerMap::DecodeStoredTile(link.source_endpoint, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
			// Installed cells need this file's declared layer and company; copying geometry cannot confer another owner's network rights.
			if (!source.has_value() || source->layer != link.local_layer ||
				(link.active && !_load_check_data.companies.contains(link.local_owner.base()))) SlErrorCorrupt("Invalid local utility preview geometry/owner");
		}
		// Town service keeps only a reference to the original Deep powerhouse;
		// the shared utility ledger remains the sole quantity and history owner.
		for (const auto &consumer : rows.consumers) {
			if (consumer.town_terminal == StationID::Invalid()) continue;
			const auto port = MultiLayerMap::DecodeStoredTile(consumer.town_source_port, _load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers);
			// An absent or foreign-plane boundary cannot dispatch a saved physical service.
			if (!port.has_value() || port->layer != MapLayer::DeepUnderground) SlErrorCorrupt("Invalid copied town service source port");
		}
		if (ValidateUtilityPowerLineage(rows, {_load_check_data.map_size_x, _load_check_data.map_size_y, _load_check_data.map_layers}) != UtilityPowerError::None) SlErrorCorrupt("Invalid copied utility lineage geometry");
	}

	MultiLayerProfileSelection selection;
	JoinLoadedUtilityHeatConversions(rows, preview); // Attach only this file's exact metadata/provenance before complete numerical validation; repeated checks replace copied extensions.
	selection.id = preview ? static_cast<MultiLayerGameplayProfileID>(_load_check_data.gameplay_profile_id) : metadata == nullptr ? MultiLayerGameplayProfileID::Classic : metadata->profile.id;
	if (ValidateUtilityPowerState(rows, selection, false, preview ? _load_check_data.map_layers : MultiLayerMap::GetLayerCount()) != UtilityPowerError::None) SlErrorCorrupt("Invalid utility power structural balances");
	return true;
}

/** Validate copied header/counts/balances for the save-selection preview without changing live account state. */
void ValidateUtilityPowerLoadPreview() { CheckUtilityPowerFamily(true); }

/** Fixed versioned utility family header; publication waits for complete records and real company/profile fixups. */
struct UPLHChunkHandler : ChunkHandler {
	/** Register the original electrical header and its explicitly versioned mutation/transfer extensions. */
	UPLHChunkHandler() : ChunkHandler("UPLH", ChunkType::Riff) {}
	/** Save current epoch/identity history and exact bounded row counts. */
	void Save() const override
	{
		const auto state = CaptureUtilityPowerState();
		uint64_t links = 0, shares = 0;
		// Count each saved connectivity/reservation row once; capacities are
		// validated by the owner before this bounded concrete transport.
		for (const auto &consumer : state.consumers) {
			links += consumer.linked_producers.size();
			shares += consumer.shares.size();
		}
		const uint64_t phases = (state.allocated ? 1U : 0U) | (state.heat_allocated ? 2U : 0U); // Independent once-only domains share a native epoch; grants never reconstruct allocation phase.
		std::array<uint64_t, 14> header{10, state.interval, state.next_producer, state.next_consumer, phases,
			state.producers.size(), state.consumers.size(), links, shares, state.operation_sequence, state.next_link, state.links.size(), state.local_link_revision, state.next_share};
		SlSetLength(112);
		SlCopy<VarFileType::U64>(header);
	}
	/** @param preview Select copied preview transport instead of unpublished full-load staging. */
	void Read(bool preview) const
	{
		const bool old = IsSavegameVersionBefore(SaveLoadVersion::UtilityMutations);
		const bool transfers = !IsSavegameVersionBefore(SaveLoadVersion::UtilityTransfers);
		const bool effects = !IsSavegameVersionBefore(SaveLoadVersion::UtilityConsumerEffects);
		const bool lifecycle = !IsSavegameVersionBefore(SaveLoadVersion::UtilityCompanyLifecycle);
		const bool thermal = !IsSavegameVersionBefore(SaveLoadVersion::UtilityHeatDomains);
		const bool local = !IsSavegameVersionBefore(SaveLoadVersion::LocalUtilityLinks);
		const bool paid = !IsSavegameVersionBefore(SaveLoadVersion::PaidElectricalRiserLinks);
		const bool relays = !IsSavegameVersionBefore(SaveLoadVersion::RockElectricalRelays);
		const bool lineage = !IsSavegameVersionBefore(SaveLoadVersion::UtilityShareLineage);
		const size_t length = lineage ? 112 : local ? 104 : transfers ? 96 : old ? 72 : 80;
		// Each explicit version owns its exact header; only version381 lacks a
		// saved mutation sequence because it had no actual-use operations.
		if (IsSavegameVersionBefore(SaveLoadVersion::UtilityPower) || SlGetFieldLength() != length) SlErrorCorrupt("Invalid utility power header version/length");
		std::array<uint64_t, 14> header{};
		auto fields = std::span(header).first(length / 8);
		SlCopy<VarFileType::U64>(fields);
		// Version, identity sentinels, epoch/phase and whole-store bounds refuse
		// before allocation, preserving source lineage and finite save memory.
		if (header[0] != (lineage ? 10 : relays ? 9 : paid ? 8 : local ? 7 : thermal ? 6 : lifecycle ? 5 : effects ? 4 : transfers ? 3 : old ? 1 : 2) || header[1] == 0 || header[1] == UINT64_MAX || header[2] >= UINT32_MAX || header[3] >= UINT32_MAX || header[4] > (thermal ? 3U : 1U) || header[9] == UINT64_MAX || header[10] >= UINT32_MAX || header[12] == UINT64_MAX || header[13] > MAX_UTILITY_POWER_SHARES ||
			header[5] > MAX_UTILITY_INTERVAL_ACCOUNTS || header[6] > MAX_UTILITY_INTERVAL_ACCOUNTS || header[5] > header[2] || header[6] > header[3] ||
			header[7] > MAX_UTILITY_POWER_CONNECTIONS || header[8] > MAX_UTILITY_POWER_SHARES || header[11] > MAX_UTILITY_INTERVAL_ACCOUNTS || header[11] > header[10]) SlErrorCorrupt("Invalid utility power header bounds");
		auto &presence = preview ? _load_check_data.utility_power_present : _utility_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate utility power header");
		presence[0] = true;
		(preview ? _load_check_data.utility_power_header : _utility_header) = header;
		auto &rows = preview ? _load_check_data.utility_power_state : _utility_rows;
		rows.interval = header[1];
		rows.next_producer = static_cast<uint32_t>(header[2]);
		rows.next_consumer = static_cast<uint32_t>(header[3]);
		rows.allocated = (header[4] & 1U) != 0;
		rows.heat_allocated = (header[4] & 2U) != 0;
		rows.operation_sequence = header[9];
		rows.next_link = static_cast<uint32_t>(header[10]);
		rows.local_link_revision = header[12];
		rows.next_share = static_cast<uint32_t>(header[13]);
	}
	/** Read unpublished full-load header. */
	void Load() const override { this->Read(false); }
	/** @param len Exact fixed reader checks the chunk boundary. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage only a complete structurally valid family; actual loaded company checks run during AfterLoadGame. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) { ResetUtilityPowerState(); return; } // A later native owner may refuse after numerical acceptance; Null cleanup must not retain partly published grants or path identities.
		if (CheckUtilityPowerFamily(false)) StageLoadedUtilityPowerState(_utility_rows);
	}
};

/** Fixed scalar source/consumer/link/share table, with explicit wire kinds and strict field/ordinal validation. */
struct UPLDChunkHandler : ChunkHandler {
	/** Register the bounded electrical record table. */
	UPLDChunkHandler() : ChunkHandler("UPLD", ChunkType::Table) {}
	/** Save stable source/consumer order, followed by sorted explicit links and original reservation slices. */
	void Save() const override
	{
		SlTableHeader(_utility_row_desc);
		uint32_t ordinal = 0;
		/** @param row Fully initialized typed transport; emit each bounded record exactly once. */
		const auto emit = [&ordinal](UtilityWireRow row) { SlSetArrayIndex(ordinal++); SlObject(&row, _utility_row_desc); };
		const auto state = CaptureUtilityPowerState();
		// Canonical producer fields preserve publication/carry and retirement;
		// consumer-only fields remain zero and cannot conceal generated energy.
		for (const auto &source : state.producers) {
			UtilityWireRow wire{0, source.meter.producer_id.base(), 0, source.owner.base(), to_underlying(source.meter.layer), to_underlying(source.kind), source.active,
				source.meter.accounting_interval, source.meter.generation_rate_w, source.meter.generation_energy_j, source.reserved_j, 0,
				source.meter.economy_ticks, source.meter.remainder_mj, source.generated_lifetime_j, source.consumed_lifetime_j};
			wire.domain = to_underlying(source.meter.domain); emit(wire);
		}
		// Original granted/unmet fields and exact slices are saved together;
		// queries later derive remaining balance without renewing reservations.
		for (const auto &consumer : state.consumers) {
			UtilityWireRow wire{1, consumer.meter.consumer_id.base(), 0, consumer.owner.base(), to_underlying(consumer.meter.layer), to_underlying(consumer.effect), consumer.active,
				consumer.meter.accounting_interval, consumer.capacity_j, consumer.meter.demand_energy_j, consumer.meter.granted_energy_j, consumer.meter.unmet_energy_j, 0, 0};
			wire.town = consumer.town.base(); wire.town_bonus = consumer.town_bonus_percent;
			wire.town_applied = consumer.town_effect_applied; wire.town_suspended = consumer.town_effect_suspended;
			wire.town_terminal = consumer.town_terminal.base(); wire.town_supply = consumer.town_supply.base(); wire.town_source_port = consumer.town_source_port.base();
			wire.domain = to_underlying(consumer.meter.domain);
			emit(wire);
		}
		// Each explicitly saved edge remains attached to its actual consumer;
		// rebuilding a display index cannot invent new physical connectivity.
		for (const auto &consumer : state.consumers) {
			for (const auto source : consumer.linked_producers) emit({2, consumer.meter.consumer_id.base(), source.base()});
		}
		// Every original source contribution keeps use/release fields, so retries
		// and topology loss cannot resurrect settled or expired electricity.
		for (const auto &consumer : state.consumers) {
			for (const auto &share : consumer.shares) {
				UtilityWireRow wire{3, consumer.meter.consumer_id.base(), share.producer.base(), 0, 0, 0, 0, 0, share.allocated_j, share.consumed_j, share.released_j};
				wire.transferred = share.transferred_j; wire.imported_link = share.imported_link.base();
				wire.share = share.id.base(); wire.parent = share.parent.base();
				emit(wire);
			}
		}
		// Explicit route endpoints/revision/capacity and actual flow are saved
		// after all reservation slices; no derived graph publishes an import.
		for (const auto &link : state.links) {
			if (link.kind == UtilityLinkKind::LocalCell) {
				UtilityWireRow wire{4, link.id.base(), 0, link.local_owner.base(), to_underlying(link.local_layer), 1, link.active, 0, link.local_capacity_w};
				wire.source_endpoint = wire.destination_endpoint = link.source_endpoint.base(); wire.domain = to_underlying(link.local_domain);
				emit(wire); continue; // Installed physical rate/identity has no source reservation, generated energy or metered flow.
			}
			const bool paid = link.kind == UtilityLinkKind::ElectricalRiser;
			UtilityWireRow wire{4, link.id.base(), link.source.base(), 0, 0, to_underlying(link.kind), link.active, link.portal_revision, paid ? link.riser_capacity_w : link.capacity_j, link.flow_j};
			if (link.kind == UtilityLinkKind::ElectricalFeed) wire.generator = link.generator.base(); // Only the explicit current role may retain native plant identity, including IndustryID zero.
			if (paid) wire.riser = link.riser.base(); // Declared paid role retains physical watts and its original native machinery; genuine older roles keep their original interval-joule encoding.
			wire.destination = link.destination.base(); wire.source_endpoint = link.source_endpoint.base(); wire.destination_endpoint = link.destination_endpoint.base();
			emit(wire);
		}
	}
	/** @param preview Decode copied preview transport without staging or publishing it. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::UtilityPower)) SlErrorCorrupt("Unexpected utility power records");
		const auto description = SlTableHeader(_utility_row_desc);
		const bool old = IsSavegameVersionBefore(SaveLoadVersion::UtilityMutations);
		const bool transfers = !IsSavegameVersionBefore(SaveLoadVersion::UtilityTransfers);
		const bool effects = !IsSavegameVersionBefore(SaveLoadVersion::UtilityConsumerEffects);
		const bool lifecycle = !IsSavegameVersionBefore(SaveLoadVersion::UtilityCompanyLifecycle);
		const bool local = !IsSavegameVersionBefore(SaveLoadVersion::LocalUtilityLinks);
		const bool paid = !IsSavegameVersionBefore(SaveLoadVersion::PaidElectricalRiserLinks);
		const bool lineage = !IsSavegameVersionBefore(SaveLoadVersion::UtilityShareLineage);
		const bool town_service = SlHasSaveCapability(SaveCapability::UtilityTownService);
		const bool local_feed = SlHasSaveCapability(SaveCapability::UtilityLocalElectricalFeed);
		if (description.size() != (local_feed ? 33 : town_service ? 32 : lineage ? 29 : paid ? 27 : !IsSavegameVersionBefore(SaveLoadVersion::UtilityHeatDomains) ? 26 : effects ? 25 : transfers ? 21 : old ? 14 : 16)) SlErrorCorrupt("Invalid utility power field count");
		if (!IsSavegameVersionBefore(SaveLoadVersion::UtilityHeatDomains)) {
			// Current typed native quantities require every named scalar. A
			// same-sized unknown replacement cannot default a Heat source into
			// Electricity or erase its original ID/owner/lifetime/carry/role.
			for (const auto &field : _utility_row_desc) {
				if (!SlIsObjectValidInSavegame(field)) continue; // Genuine older typed tables cannot default into a later paid machinery role or be required to carry its new column.
				if (std::ranges::count(description, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing typed utility field");
			}
		}
		// A missing scalar may not default to a free grant, a zero consumption
		// counter or a different owner/epoch during forward-compatible table read.
		for (const auto &field : _utility_row_desc) {
			// Version381 did not expose settlement totals; later files require
			// every new scalar rather than silently resetting consumed history.
			if (!SlIsObjectValidInSavegame(field)) continue;
			if (std::ranges::count(description, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing utility power field");
		}
		auto &rows = preview ? _load_check_data.utility_power_state : _utility_rows;
		auto &counts = preview ? _load_check_data.utility_power_rows : _utility_counts;
		uint32_t ordinal = 0;
		uint8_t prior_kind = 0;
		// Decode in bounded stable wire-kind order, then validate full identity,
		// origin, arithmetic and header counts before any live owner replacement.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			UtilityWireRow wire;
			SlObject(&wire, description);
			// Stable ordinals and increasing kinds prevent reordered or duplicate
			// identities from attaching grants to another saved account family.
			if (index != static_cast<int>(ordinal++) || wire.type > (transfers ? 4 : 3) || wire.type < prior_kind || wire.id == UINT32_MAX) SlErrorCorrupt("Invalid utility power ordinal/type");
			prior_kind = wire.type;
			const uint32_t bound = wire.type < 2 || wire.type == 4 ? MAX_UTILITY_INTERVAL_ACCOUNTS : wire.type == 2 ? MAX_UTILITY_POWER_CONNECTIONS : MAX_UTILITY_POWER_SHARES;
			if (++counts[wire.type] > bound) SlErrorCorrupt("Too many utility power rows");
			// Only shares carry transfer debits/import keys; only route rows carry
			// destination/endpoints. Other kinds cannot conceal extra flow fields.
			if (wire.type != 3 && (wire.transferred != 0 || wire.imported_link != 0)) SlErrorCorrupt("Invalid utility transfer fields");
			if (wire.type != 3 && (wire.share != 0 || wire.parent != 0)) SlErrorCorrupt("Invalid utility slice role fields"); // Only actual reservation rows can name a shared slice or parent; generation and native links cannot hide quantities here.
			if (wire.type != 4 && (wire.destination != 0 || wire.source_endpoint != 0 || wire.destination_endpoint != 0)) SlErrorCorrupt("Invalid utility route fields");
			if (wire.riser != 0 && !(paid && wire.type == 4 && wire.kind == 2)) SlErrorCorrupt("Invalid paid machinery role fields"); // Only the explicit paid electrical role can name a native riser, including actual IndustryID zero.
			// A genuine older table cannot acquire a later feed, and unrelated
			// rows cannot conceal another native machinery reference.
			if (wire.generator != 0 && !(local_feed && wire.type == 4 && wire.kind == 3)) SlErrorCorrupt("Invalid local feed machinery fields");
			// Only consumer rows carry a city offer; every serialized flag is an
			// actual boolean and cannot hide a second growth authority elsewhere.
			if ((wire.type != 1 && (wire.town != 0 || wire.town_bonus != 0 || wire.town_applied != 0 || wire.town_suspended != 0)) || wire.town_applied > 1 || wire.town_suspended > 1) SlErrorCorrupt("Invalid utility town fields");
			if (wire.type != 1 && (wire.town_terminal != 0 || wire.town_supply != 0 || wire.town_source_port != 0)) SlErrorCorrupt("Invalid utility town service role fields"); // A route, source or subordinate share cannot conceal another physical consumer.
			// Source/consumer records own typed quantities. A subordinate
			// connection, slice or electrical route cannot conceal another meter.
			if (wire.domain > to_underlying(UtilityEnergyDomain::Heat) || (wire.type > 1 && wire.domain != 0 && !(local && wire.type == 4 && wire.kind == 1))) SlErrorCorrupt("Invalid utility energy domain fields");
			if (wire.type == 0) {
				if (!SlHasSaveCapability(SaveCapability::UtilityHeatConversion) && wire.kind == to_underlying(UtilityGenerationKind::HeatConversion)) SlErrorCorrupt("Unexpected exact conversion in older utility format"); // Older rate-derived rows cannot acquire later coupled output semantics.
				// Producer rows cannot carry a foreign reference or consumer-only
				// payload, and liveness must be an actual boolean.
				if (wire.reference != 0 || wire.active > 1 || wire.d != 0) SlErrorCorrupt("Invalid utility source wire fields");
				UtilityPowerProducer source;
				source.owner = CompanyID{wire.owner}; source.kind = static_cast<UtilityGenerationKind>(wire.kind); source.active = wire.active != 0;
				source.meter.domain = static_cast<UtilityEnergyDomain>(wire.domain);
				source.meter.producer_id = UtilityProducerID{wire.id}; source.meter.layer = static_cast<MapLayer>(wire.layer);
				source.meter.accounting_interval = wire.epoch; source.meter.generation_rate_w = wire.a; source.meter.generation_energy_j = wire.b;
				source.reserved_j = wire.c; source.meter.economy_ticks = wire.ticks; source.meter.remainder_mj = wire.remainder;
				// The older format had no actual consumption. Migration retains
				// its exact current generation without manufacturing historical use.
				source.generated_lifetime_j = old ? wire.b : wire.lifetime_generated;
				source.consumed_lifetime_j = wire.lifetime_consumed;
				rows.producers.push_back(source);
			} else if (wire.type == 1) {
				// A consumer request never carries producer publication/fractions;
				// refusing these unused slots keeps generation at its sole owner.
				// Older formats reserved consumer liveness as zero; explicit385
				// persists retirement in that scalar without adding a second owner.
				if (wire.reference != 0 || wire.active > (lifecycle ? 1 : 0) || wire.ticks != 0 || wire.remainder != 0 || wire.lifetime_generated != 0 || wire.lifetime_consumed != 0) SlErrorCorrupt("Invalid utility consumer wire fields");
				UtilityPowerConsumer consumer;
				consumer.active = !lifecycle || wire.active != 0;
				consumer.owner = CompanyID{wire.owner}; consumer.effect = static_cast<UtilityConsumerEffect>(wire.kind); consumer.capacity_j = wire.a;
				consumer.meter.domain = static_cast<UtilityEnergyDomain>(wire.domain);
				consumer.meter.consumer_id = UtilityConsumerID{wire.id}; consumer.meter.layer = static_cast<MapLayer>(wire.layer);
				consumer.meter.accounting_interval = wire.epoch; consumer.meter.demand_energy_j = wire.b;
				consumer.meter.granted_energy_j = wire.c; consumer.meter.unmet_energy_j = wire.d;
				consumer.town = effects ? TownID{wire.town} : TownID::Invalid();
				consumer.town_bonus_percent = wire.town_bonus; consumer.town_effect_applied = wire.town_applied != 0;
				consumer.town_effect_suspended = wire.town_suspended != 0;
				consumer.town_terminal = town_service ? StationID{wire.town_terminal} : StationID::Invalid();
				consumer.town_supply = town_service ? UtilityConsumerID{wire.town_supply} : UtilityConsumerID{UINT32_MAX};
				consumer.town_source_port = town_service ? TileIndex{wire.town_source_port} : INVALID_TILE;
				rows.consumers.push_back(std::move(consumer));
			} else if (wire.type == 4) {
				if (local && wire.kind == 1) {
					// The appended local role has physical fields only. An older
					// portal row cannot default into heat or hide a generation meter.
					// Only explicit402 cells extend the physical plane policy. Older
					// Deep rows cannot silently become a relay or thermal crossing.
					const bool allowed_layer = IsSavegameVersionBefore(SaveLoadVersion::RockElectricalRelays) ? wire.layer == to_underlying(MapLayer::DeepUnderground) :
						IsMultiLayerLocalUtilityLayer(static_cast<UtilityEnergyDomain>(wire.domain), static_cast<MapLayer>(wire.layer));
					if (wire.reference != 0 || wire.destination != 0 || !allowed_layer || wire.active > 1 ||
						wire.epoch != 0 || wire.b != 0 || wire.c != 0 || wire.d != 0 || wire.ticks != 0 || wire.remainder != 0 || wire.lifetime_generated != 0 || wire.lifetime_consumed != 0) SlErrorCorrupt("Invalid local utility wire fields");
					UtilityPowerLink link; link.kind = UtilityLinkKind::LocalCell; link.id = UtilityLinkID{wire.id};
					link.local_owner = CompanyID{wire.owner}; link.local_domain = static_cast<UtilityEnergyDomain>(wire.domain); link.local_capacity_w = wire.a;
					link.local_layer = static_cast<MapLayer>(wire.layer);
					link.source_endpoint = TileIndex{wire.source_endpoint}; link.destination_endpoint = TileIndex{wire.destination_endpoint}; link.active = wire.active != 0;
					rows.links.push_back(link); continue;
				}
				// Directed route payload contains physical references and metering
				// only; producer/consumer publication or use cannot hide in it.
				const bool paid_row = paid && wire.kind == 2;
				const bool feed_row = local_feed && wire.kind == 3;
				if (wire.owner != 0 || wire.layer != 0 || (wire.kind != 0 && !paid_row && !feed_row) || wire.active > 1 || wire.c != 0 || wire.d != 0 || wire.ticks != 0 || wire.remainder != 0 || wire.lifetime_generated != 0 || wire.lifetime_consumed != 0) SlErrorCorrupt("Invalid utility link wire fields");
				UtilityPowerLink link;
				link.id = UtilityLinkID{wire.id}; link.source = UtilityConsumerID{wire.reference}; link.destination = UtilityConsumerID{wire.destination};
				link.source_endpoint = TileIndex{wire.source_endpoint}; link.destination_endpoint = TileIndex{wire.destination_endpoint};
				link.portal_revision = wire.epoch; link.flow_j = wire.b; link.active = wire.active != 0;
				if (paid_row) { link.kind = UtilityLinkKind::ElectricalRiser; link.riser = IndustryID{wire.riser}; link.riser_capacity_w = wire.a; }
				else if (feed_row) { link.kind = UtilityLinkKind::ElectricalFeed; link.generator = IndustryID{wire.generator}; link.capacity_j = wire.a; }
				else link.capacity_j = wire.a; // Preserve genuine older numerical portal caps; only declared401 paid roles interpret the scalar as an immutable physical rate.
				rows.links.push_back(link);
			} else {
				// Subordinate rows contain only their declared owner/source/slice
				// fields; unused payload cannot conceal another source or clock.
				if (wire.owner != 0 || wire.layer != 0 || wire.kind != 0 || wire.active != 0 || wire.epoch != 0 || wire.d != 0 || wire.ticks != 0 || wire.remainder != 0 || wire.lifetime_generated != 0 || wire.lifetime_consumed != 0 || wire.reference == UINT32_MAX) SlErrorCorrupt("Invalid utility subordinate wire fields");
				/** @param consumer Decoded owner. @return Its exact saved consumer identity. */
				const auto identity = [](const UtilityPowerConsumer &consumer) { return consumer.meter.consumer_id; };
				const auto consumer = std::ranges::find(rows.consumers, UtilityConsumerID{wire.id}, identity);
				if (consumer == rows.consumers.end()) SlErrorCorrupt("Missing utility row consumer");
				if (wire.type == 2) {
					// Connectivity is eligibility only: attaching a network edge
					// cannot smuggle generated, consumed or reserved quantities.
					if (wire.a != 0 || wire.b != 0 || wire.c != 0) SlErrorCorrupt("Invalid utility connection quantities");
					consumer->linked_producers.push_back(UtilityProducerID{wire.reference});
				} else {
					UtilityPowerShare share{UtilityProducerID{wire.reference}, wire.a, wire.b, wire.c, wire.transferred,
						UtilityLinkID{transfers ? wire.imported_link : UINT32_MAX}};
					if (lineage) { share.id = UtilityShareID{wire.share}; share.parent = UtilityShareID{wire.parent}; }
					consumer->shares.push_back(std::move(share));
				}
			}
		}
		auto &presence = preview ? _load_check_data.utility_power_present : _utility_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate utility power table");
		presence[1] = true;
	}
	/** Decode unpublished owner transport. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns exact field/ordinal/count validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Exact scalar path-row schema; decoded counts are bounded before references are attached to unpublished slices. */
static const SaveLoad _utility_path_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("share", SLE_OBJECT_ADDRESS(UtilityPowerPathWire, share)),
	SaveLoad::Variable<VarFileType::U32>("ordinal", SLE_OBJECT_ADDRESS(UtilityPowerPathWire, ordinal)),
	SaveLoad::Variable<VarFileType::U32>("link", SLE_OBJECT_ADDRESS(UtilityPowerPathWire, link)),
};

/** Mandatory bounded physical-reference header, including current files with no imported paths. */
struct UPPHChunkHandler : ChunkHandler {
	/** Register the shared utility path header; this family issues no energy identity. */
	UPPHChunkHandler() : ChunkHandler("UPPH", ChunkType::Riff) {}
	/** Save exact current epoch and reference count, with original quantities remaining solely in UPLD. */
	void Save() const override
	{
		const auto state = CaptureUtilityPowerState(); uint64_t count = 0;
		// Count only the newly traversed segment of each retained child. Full
		// ancestry reconstructs from exact parents without duplicate wire rows.
		for (const auto &consumer : state.consumers) {
			for (const auto &share : consumer.shares) count += share.segment.size();
		}
		std::array<uint64_t, 3> header{1, state.interval, count}; SlSetLength(24); SlCopy<VarFileType::U64>(header);
	}
	/** @param preview Select independent copied references rather than unpublished full-load staging. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::UtilityShareLineage) || SlGetFieldLength() != 24) SlErrorCorrupt("Invalid utility lineage header version/length"); // An earlier family cannot smuggle later physical lineage into its original one-hop interpretation.
		std::array<uint64_t, 3> header{}; SlCopy<VarFileType::U64>(header);
		if (header[0] != 1 || header[1] == 0 || header[1] == UINT64_MAX || header[2] > MAX_UTILITY_LINEAGE_LINKS) SlErrorCorrupt("Invalid utility lineage bounds"); // Exact epoch/count bounds prevent unbounded path allocation before any owner accepts references.
		auto &presence = preview ? _load_check_data.utility_path_present : _utility_path_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate utility lineage header");
		presence[0] = true; (preview ? _load_check_data.utility_path_header : _utility_path_header) = header;
	}
	/** Decode unpublished native bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader validates the exact header boundary. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Bounded ordered reference table; no row owns a source balance, grant quantity or physical installation. */
struct UPPDChunkHandler : ChunkHandler {
	/** Register the shared exact slice/segment/link table. */
	UPPDChunkHandler() : ChunkHandler("UPPD", ChunkType::Table) {}
	/** Save stable shared slice order and exact segment ordinals, independently of consumer presentation sorting. */
	void Save() const override
	{
		SlTableHeader(_utility_path_desc); const auto state = CaptureUtilityPowerState();
		std::map<UtilityShareID, const UtilityPowerShare *> shares;
		// Shared identities remain stable when source/parent presentation order
		// changes. Emit each retained segment once by that exact issued ID.
		for (const auto &consumer : state.consumers) {
			for (const auto &share : consumer.shares) shares.emplace(share.id, &share);
		}
		uint32_t ordinal = 0;
		for (const auto &[id, share] : shares) {
			for (uint32_t position = 0; position < share->segment.size(); ++position) {
				UtilityPowerPathWire row{id.base(), position, share->segment[position].base()}; SlSetArrayIndex(ordinal++); SlObject(&row, _utility_path_desc);
			}
		}
	}
	/** @param preview Decode copied preview rows rather than unpublished native staging. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::UtilityShareLineage)) SlErrorCorrupt("Unexpected utility lineage table");
		const auto description = SlTableHeader(_utility_path_desc);
		if (description.size() != std::size(_utility_path_desc)) SlErrorCorrupt("Invalid utility lineage field count");
		// All named fields are mandatory; an unknown same-sized replacement may
		// not default a path into a different parent or physical reference.
		for (const auto &field : _utility_path_desc) {
			if (std::ranges::count(description, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing utility lineage field");
		}
		auto &rows = preview ? _load_check_data.utility_path_rows : _utility_path_rows;
		uint32_t ordinal = 0; std::optional<std::pair<uint32_t, uint32_t>> previous;
		for (int index; (index = SlIterateArray()) != -1;) {
			UtilityPowerPathWire row{}; SlObject(&row, description);
			const std::pair key(row.share, row.ordinal);
			if (index != static_cast<int>(ordinal++) || rows.size() >= MAX_UTILITY_LINEAGE_LINKS || row.share >= MAX_UTILITY_POWER_SHARES ||
				row.ordinal >= MAX_UTILITY_INTERVAL_ACCOUNTS || row.link >= MAX_UTILITY_INTERVAL_ACCOUNTS || (previous.has_value() && key <= *previous)) SlErrorCorrupt("Invalid utility lineage ordinal/reference"); // Strict bounded ordering rejects duplicate or forged paths before attachment to a slice.
			rows.push_back(row); previous = key;
		}
		auto &presence = preview ? _load_check_data.utility_path_present : _utility_path_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate utility lineage table");
		presence[1] = true;
	}
	/** Decode unpublished native path references. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns exact bounded row/field validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const UPLHChunkHandler UPLH;
static const UPLDChunkHandler UPLD;
static const UPPHChunkHandler UPPH;
static const UPPDChunkHandler UPPD;
static const ChunkHandlerRef utility_power_handlers[] = {UPLH, UPLD, UPPH, UPPD};
extern const ChunkHandlerTable _utility_power_chunk_handlers(utility_power_handlers);
