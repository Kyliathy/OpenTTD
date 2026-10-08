/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_sl.cpp Bounded geology transport with independent preview and staged references. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 4 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../excavation_state.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../multilayer_utility.h"

#include "../safeguards.h"

static constexpr uint32_t GEOLOGY_HEADER_BYTES = 29; ///< Two U16, six U32 and one checked policy byte.

/** Bounded deposit scalar transport; compact spans live in GSPN rows, not an unbounded vector field. */
struct GeologyDepositWire {
	uint32_t id = UINT32_MAX; ///< Stable typed-ID payload.
	uint8_t layer = 0; ///< Exact saved layer meaning.
	CargoLabel resource{}; ///< Stable four-byte cargo meaning.
	uint8_t hardness = 0; ///< Checked hardness enum.
	uint8_t claimant = 255; ///< CompanyID invalid sentinel or actual company slot.
	uint64_t initial = 0; ///< Exact original reserve.
	uint64_t remaining = 0; ///< Sole finite balance.
	uint64_t surveyed = 0; ///< Saved information permissions.
	uint8_t spans = 0; ///< 1..64 compact shape rows expected for this record.
};

/** One compact span reference; no pointer, vector capacity or spatial bucket is serialized. */
struct GeologySpanWire {
	uint32_t deposit = UINT32_MAX; ///< Exact existing deposit identity.
	uint8_t ordinal = 0; ///< Canonical ordinal, checked against the owner's expected span count.
	uint16_t y = 0; ///< Plane row.
	uint16_t first_x = 0; ///< Inclusive first column.
	uint16_t last_x = 0; ///< Inclusive last column.
};

/** Bounded natural-source transport; kind 0 is one cursor/count header, kind 1 is an actual thermal field. */
struct GeothermalVentWire {
	uint8_t kind = 0; ///< Header versus actual natural field, not a freight/industry record.
	uint16_t version = 1; ///< Independent vent generator/wire revision.
	uint32_t next_id = 0; ///< Header's monotonic cursor only.
	uint32_t count = 0; ///< Header's actual field count.
	uint32_t id = UINT32_MAX; ///< Stable natural field identity.
	uint8_t layer = 3; ///< Immutable Deep storage meaning.
	uint16_t x = 0, y = 0; ///< Layer-local footprint origin.
	uint8_t width = 0, height = 0; ///< Bounded rectangular field shape.
	uint64_t capacity = 0; ///< Shared physical thermal rate, never generated freight.
};

static const SaveLoad _deposit_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(GeologyDepositWire, id)),
	SaveLoad::Variable<VarFileType::U8>("layer", SLE_OBJECT_ADDRESS(GeologyDepositWire, layer)),
	SaveLoad::Variable<VarFileType::Label>("resource", SLE_OBJECT_ADDRESS(GeologyDepositWire, resource)),
	SaveLoad::Variable<VarFileType::U8>("hardness", SLE_OBJECT_ADDRESS(GeologyDepositWire, hardness)),
	SaveLoad::Variable<VarFileType::U8>("claimant", SLE_OBJECT_ADDRESS(GeologyDepositWire, claimant)),
	SaveLoad::Variable<VarFileType::U64>("initial", SLE_OBJECT_ADDRESS(GeologyDepositWire, initial)),
	SaveLoad::Variable<VarFileType::U64>("remaining", SLE_OBJECT_ADDRESS(GeologyDepositWire, remaining)),
	SaveLoad::Variable<VarFileType::U64>("surveyed", SLE_OBJECT_ADDRESS(GeologyDepositWire, surveyed)),
	SaveLoad::Variable<VarFileType::U8>("spans", SLE_OBJECT_ADDRESS(GeologyDepositWire, spans)),
};
static const SaveLoad _span_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("deposit", SLE_OBJECT_ADDRESS(GeologySpanWire, deposit)),
	SaveLoad::Variable<VarFileType::U8>("ordinal", SLE_OBJECT_ADDRESS(GeologySpanWire, ordinal)),
	SaveLoad::Variable<VarFileType::U16>("y", SLE_OBJECT_ADDRESS(GeologySpanWire, y)),
	SaveLoad::Variable<VarFileType::U16>("first_x", SLE_OBJECT_ADDRESS(GeologySpanWire, first_x)),
	SaveLoad::Variable<VarFileType::U16>("last_x", SLE_OBJECT_ADDRESS(GeologySpanWire, last_x)),
};
static const SaveLoad _vent_desc[] = {
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(GeothermalVentWire, kind)),
	SaveLoad::Variable<VarFileType::U16>("version", SLE_OBJECT_ADDRESS(GeothermalVentWire, version)),
	SaveLoad::Variable<VarFileType::U32>("next_id", SLE_OBJECT_ADDRESS(GeothermalVentWire, next_id)),
	SaveLoad::Variable<VarFileType::U32>("count", SLE_OBJECT_ADDRESS(GeothermalVentWire, count)),
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(GeothermalVentWire, id)),
	SaveLoad::Variable<VarFileType::U8>("layer", SLE_OBJECT_ADDRESS(GeothermalVentWire, layer)),
	SaveLoad::Variable<VarFileType::U16>("x", SLE_OBJECT_ADDRESS(GeothermalVentWire, x)),
	SaveLoad::Variable<VarFileType::U16>("y", SLE_OBJECT_ADDRESS(GeothermalVentWire, y)),
	SaveLoad::Variable<VarFileType::U8>("width", SLE_OBJECT_ADDRESS(GeothermalVentWire, width)),
	SaveLoad::Variable<VarFileType::U8>("height", SLE_OBJECT_ADDRESS(GeothermalVentWire, height)),
	SaveLoad::Variable<VarFileType::U64>("capacity", SLE_OBJECT_ADDRESS(GeothermalVentWire, capacity)),
};

static GeologyStateSnapshot _geology_load; ///< Unaccepted descriptor transport; never live terrain or profile policy.
static uint32_t _expected_deposits = 0; ///< Header bound, checked before reserve allocations.
static uint32_t _expected_spans = 0; ///< Whole shape-row bound.
static std::vector<uint8_t> _deposit_span_counts; ///< Transport cardinality, not shape authority.
static std::vector<GeologySpanWire> _loaded_spans; ///< Unresolved typed references, checked after all chunks exist.
static bool _saw_header = false, _saw_deposits = false, _saw_spans = false; ///< Duplicate/missing chunk detection.
static bool _saw_vents = false; ///< Independent optional-old/required-new thermal family presence.

/** Clear loader transport at InitializeGame; preview uses only its own LoadCheckData fields. */
void ResetGeologyLoadStaging()
{
	_geology_load = {};
	_expected_deposits = _expected_spans = 0;
	_deposit_span_counts.clear();
	_loaded_spans.clear();
	_saw_header = _saw_deposits = _saw_spans = false;
	_saw_vents = false;
	ResetExcavationLoadStaging();
}

/**
 * Read an exact mandatory field table; missing fields cannot silently reset finite authority.
 * @param required Current owner descriptor, frozen by the feature wire version.
 * @return Actual table after exact required-name validation.
 */
static std::vector<SaveLoad> ReadGeologyTable(const SaveLoadTable &required)
{
	if (IsSavegameVersionBefore(SaveLoadVersion::GeologyExcavation)) SlErrorCorrupt("Unexpected geology chunk in old save version");
	auto actual = SlTableHeader(required);
	if (actual.size() != required.size()) SlErrorCorrupt("Invalid geology field count");
	// Every saved authority field is mandatory; a duplicated/unknown header cannot replace a missing reserve/identity field.
	for (const auto &field : required) {
		if (std::ranges::find(actual, field.name, &SaveLoad::name) == actual.end()) SlErrorCorrupt("Missing mandatory geology field");
	}
	return actual;
}

/** Fixed-size header and exact packed payload; no shape or state allocation occurs before length/cap checks. */
struct GEOSChunkHandler : ChunkHandler {
	/** Register the uniquely versioned geological terrain chunk. */
	GEOSChunkHandler() : ChunkHandler("GEOS", ChunkType::Riff) {}
	/** Save copied geometry and one owner payload, including explicit inactive legacy terrain. */
	void Save() const override
	{
		auto snapshot = CaptureGeologyState();
		uint32_t spans = 0;
		// Shape count is bounded by the owner cap; it validates later compact rows without serializing cache addresses.
		for (const auto &deposit : snapshot.deposits) spans += static_cast<uint32_t>(deposit.spans.size());
		std::array<uint16_t, 2> versions = {1, snapshot.generator_version};
		std::array<uint32_t, 6> header = {snapshot.width, snapshot.height, snapshot.next_deposit_id, static_cast<uint32_t>(snapshot.packed.size()), static_cast<uint32_t>(snapshot.deposits.size()), spans};
		SlSetLength(GEOLOGY_HEADER_BYTES + snapshot.packed.size());
		SlCopy<VarFileType::U16>(versions);
		SlCopy<VarFileType::U32>(header);
		SlWriteByte(snapshot.freeform_edges ? 1 : 0);
		// Inactive Classic/legacy transport has no buffer; SlCopy requires a non-null pointer even for zero-length copies.
		if (!snapshot.packed.empty()) SlCopy<VarFileType::U8>(snapshot.packed);
	}

	/**
	 * Read terrain transport for full load or nonmutating preview with exact geometry/length validation.
	 * @param preview Use copied preview dimensions instead of the current live Map.
	 * @param length Actual RIFF payload length, checked before any dynamic allocation.
	 * @return Bounded header/payload snapshot; deposits are supplied by their own table.
	 */
	GeologyStateSnapshot Read(bool preview, size_t length) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeologyExcavation)) SlErrorCorrupt("Unexpected geology chunk in old save version");
		if (length < GEOLOGY_HEADER_BYTES) SlErrorCorrupt("Truncated geology header");
		std::array<uint16_t, 2> versions{};
		std::array<uint32_t, 6> header{};
		SlCopy<VarFileType::U16>(versions);
		SlCopy<VarFileType::U32>(header);
		const uint8_t flags = SlReadByte();
		if (versions[0] != 1) SlErrorCorrupt("Unsupported geology wire version");
		if (versions[1] > GEOLOGY_GENERATOR_VERSION) SlErrorCorrupt("Unsupported geology generator version");
		if (flags > 1) SlErrorCorrupt("Invalid geology fence policy");
		const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX();
		const uint32_t height = preview ? _load_check_data.map_size_y : Map::SizeY();
		// Payload geometry must match the actual MAPS owner; multiplication occurs only after supported bounds are known.
		if (header[0] != width || header[1] != height || !CheckGeologyGenerationSize(width, height)) SlErrorCorrupt("Invalid geology dimensions");
		const uint32_t expected_bytes = versions[1] == 0 ? 0 : width * height / 2;
		// Exact length/caps prevent truncated payloads or attacker-controlled vector allocations before metadata validation.
		if (header[3] != expected_bytes || length != GEOLOGY_HEADER_BYTES + expected_bytes || header[4] > MAX_GEOLOGY_DEPOSITS || header[5] > MAX_GEOLOGY_DEPOSITS * MAX_GEOLOGY_SPANS) SlErrorCorrupt("Invalid geology payload bounds");
		if (versions[1] == 0) {
			// Inactive legacy terrain has no hidden reserve, records or reused identity cursor.
			if (header[2] != 0 || header[4] != 0 || header[5] != 0) SlErrorCorrupt("Invalid inactive geology header");
		}
		GeologyStateSnapshot snapshot;
		snapshot.generator_version = versions[1];
		snapshot.width = width;
		snapshot.height = height;
		snapshot.next_deposit_id = header[2];
		snapshot.freeform_edges = flags != 0;
		snapshot.packed.resize(expected_bytes);
		// Empty inactive terrain is a valid header-only feature, not a null buffer passed into the raw copier.
		if (!snapshot.packed.empty()) SlCopy<VarFileType::U8>(snapshot.packed);
		// Reserved terrain patterns refuse in preview too; preview must never publish a candidate into live storage.
		for (uint8_t byte : snapshot.packed) {
			if ((byte & (byte >> 1) & 0x55) != 0) SlErrorCorrupt("Reserved geology terrain encoding");
		}
		if (preview) {
			_load_check_data.geology_generator_version = versions[1];
			_load_check_data.geology_expected_deposits = header[4];
			_load_check_data.geology_expected_spans = header[5];
		} else {
			_expected_deposits = header[4];
			_expected_spans = header[5];
		}
		return snapshot;
	}

	/** Stage exact terrain transport; profile/feature publication happens only after pointer/reference checks. */
	void Load() const override
	{
		if (_saw_header) SlErrorCorrupt("Duplicate geology header");
		_geology_load = this->Read(false, SlGetFieldLength());
		_saw_header = true;
	}
	/**
	 * Read copied terrain preview without changing current map/profile/seed/staging.
	 * @param len Exact RIFF length provided by the preview dispatcher.
	 */
	void LoadCheck(size_t len) const override
	{
		if (_load_check_data.geology_header_present) SlErrorCorrupt("Duplicate geology preview header");
		this->Read(true, len);
		_load_check_data.geology_header_present = true;
	}
	/** Resolve all span/company references after every chunk has loaded; never infer absent data as newly solid ground. */
	void FixPointers() const override
	{
		// Failure cleanup also invokes every handler with the current version; it must not validate or publish half-read geology.
		if (!SlIsFixingPointers()) return;
		if (!_saw_header) {
			// Every registered fixup runs on old files too; only old-version absence is legitimate migration.
			if (!IsSavegameVersionBefore(SaveLoadVersion::GeologyExcavation) || _saw_deposits || _saw_spans) SlErrorCorrupt("Missing required geology header");
			return;
		}
		// All three bounded descriptor families must agree before resolving a single span or claim.
		if (!_saw_deposits || !_saw_spans || _geology_load.deposits.size() != _expected_deposits || _loaded_spans.size() != _expected_spans) SlErrorCorrupt("Missing or inconsistent geology records");
		// Reference fixup is idempotent: rebuild shapes from untouched wire rows instead of appending a second copy.
		for (auto &deposit : _geology_load.deposits) deposit.spans.clear();
		// Span IDs resolve only against actual authoritative records; no missing deposit is manufactured during fixup.
		for (const auto &wire : _loaded_spans) {
			auto record = std::ranges::lower_bound(_geology_load.deposits, GeologyDepositID{wire.deposit}, {}, &GeologyDeposit::id);
			if (record == _geology_load.deposits.end()) SlErrorCorrupt("Invalid geology span reference");
			if (record->id != GeologyDepositID{wire.deposit}) SlErrorCorrupt("Invalid geology span reference");
			if (record->spans.size() != wire.ordinal) SlErrorCorrupt("Invalid geology span ordinal");
			record->spans.push_back({wire.y, wire.first_x, wire.last_x});
		}
		// Expected per-record shape sizes are mandatory, even when another record has surplus rows.
		for (size_t i = 0; i < _geology_load.deposits.size(); ++i) {
			if (_geology_load.deposits[i].spans.size() != _deposit_span_counts[i]) SlErrorCorrupt("Incomplete geology shape");
		}
		const auto *metadata = GetPendingMultiLayerStateSchema();
		if (metadata == nullptr) SlErrorCorrupt("Missing geology profile metadata");
		const bool required = (metadata->feature_flags & MULTILAYER_FEATURE_GEOLOGY) != 0;
		if (required != (_geology_load.generator_version != 0)) SlErrorCorrupt("Geology feature metadata mismatch");
		const bool vents_required = (metadata->feature_flags & MULTILAYER_FEATURE_VENTS) != 0;
		// Older geology-only files preserve absence; an explicit vent feature must have its real source descriptor.
		if (vents_required && !_saw_vents) SlErrorCorrupt("Missing required geothermal fields");
		if (vents_required != !_geology_load.vents.empty()) SlErrorCorrupt("Geothermal field metadata mismatch");
		GeologyWorldState prepared;
		CompleteExcavationLoad(_geology_load);
		const auto error = PrepareGeologyRestoration(_geology_load, prepared);
		if (error != GeologyError::None) SlErrorCorrupt(fmt::format("Invalid geology authority ({})", to_underlying(error)));
		StageLoadedGeologyState(std::move(prepared));
	}
};

/** Deposit scalar table, with bounded row counts and span cardinality before any shape allocation. */
struct GDEPChunkHandler : ChunkHandler {
	/** Register the sole geological reserve descriptor. */
	GDEPChunkHandler() : ChunkHandler("GDEP", ChunkType::Table) {}
	/** Save canonical finite records; stable IDs are fields, while dense table ordinals are transport only. */
	void Save() const override
	{
		SlTableHeader(_deposit_desc);
		uint32_t ordinal = 0;
		// Stable record traversal preserves identity/reserve/visibility without serializing bucket order.
		for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
			GeologyDepositWire wire{deposit.id.base(), to_underlying(deposit.layer), deposit.resource, to_underlying(deposit.hardness), deposit.claimant.base(), deposit.initial_reserve, deposit.remaining_reserve, deposit.surveyed_companies, static_cast<uint8_t>(deposit.spans.size())};
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _deposit_desc);
		}
	}
	/**
	 * Read bounded finite scalar rows for load or preview, preserving one reserve authority.
	 * @param preview Use independent preview counters and never append to live/load candidate records.
	 */
	void Read(bool preview) const
	{
		const auto description = ReadGeologyTable(_deposit_desc);
		uint32_t count = 0;
		// Validate scalar bounds before storing a row; actual shapes/company references resolve only after all chunks exist.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_GEOLOGY_DEPOSITS) SlErrorCorrupt("Too many geology deposits");
			if (index != static_cast<int>(count)) SlErrorCorrupt("Invalid geology table ordinal");
			GeologyDepositWire wire;
			SlObject(&wire, description);
			// Checked IDs/enums/finite balances/cardinality reject partial/default authority even during preview.
			if (wire.id == UINT32_MAX || wire.spans == 0 || wire.spans > MAX_GEOLOGY_SPANS || wire.initial == 0 || wire.remaining > wire.initial || wire.hardness > to_underlying(GeologyHardness::Basalt)) SlErrorCorrupt("Invalid geology deposit fields");
			if (preview) {
				++count;
				continue;
			}
			GeologyDeposit deposit;
			deposit.id = GeologyDepositID{wire.id};
			deposit.layer = static_cast<MapLayer>(wire.layer);
			deposit.resource = wire.resource;
			deposit.hardness = static_cast<GeologyHardness>(wire.hardness);
			deposit.claimant = CompanyID{wire.claimant};
			deposit.initial_reserve = wire.initial;
			deposit.remaining_reserve = wire.remaining;
			deposit.surveyed_companies = wire.surveyed;
			// Canonical increasing IDs make later reference resolution deterministic and preserve deleted-ID holes.
			if (!_geology_load.deposits.empty() && _geology_load.deposits.back().id >= deposit.id) SlErrorCorrupt("Duplicate or unordered geology identity");
			_geology_load.deposits.push_back(std::move(deposit));
			_deposit_span_counts.push_back(wire.spans);
			++count;
		}
		if (preview) _load_check_data.geology_deposit_rows = count;
	}
	/** Stage finite scalar records; they cannot become a live mine or reserve until complete validation. */
	void Load() const override
	{
		if (_saw_deposits) SlErrorCorrupt("Duplicate geology deposit chunk");
		this->Read(false);
		_saw_deposits = true;
	}
	/** @param len Unused table payload length; bounded row parsing owns the transport. */
	void LoadCheck([[maybe_unused]] size_t len) const override
	{
		if (_load_check_data.geology_deposits_present) SlErrorCorrupt("Duplicate geology preview deposits");
		this->Read(true);
		_load_check_data.geology_deposits_present = true;
	}
};

/** Compact shape rows, resolved against actual stable deposit IDs during one owner fixup. */
struct GSPNChunkHandler : ChunkHandler {
	/** Register the sole geological span descriptor. */
	GSPNChunkHandler() : ChunkHandler("GSPN", ChunkType::Table) {}
	/** Save only actual bounded spans, without padding every deposit to the maximum shape size. */
	void Save() const override
	{
		SlTableHeader(_span_desc);
		uint32_t row = 0;
		// Deposit IDs and span ordinals retain canonical shape ownership independently of transport row numbering.
		for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
			for (uint8_t ordinal = 0; ordinal < deposit.spans.size(); ++ordinal) {
				const auto &span = deposit.spans[ordinal];
				GeologySpanWire wire{deposit.id.base(), ordinal, span.y, span.first_x, span.last_x};
				SlSetArrayIndex(row++);
				SlObject(&wire, _span_desc);
			}
		}
	}
	/**
	 * Parse bounded rows into unaccepted transport or independent preview counts.
	 * @param preview Never stage/mutate current geological authority for a file-picker preview.
	 */
	void Read(bool preview) const
	{
		const auto description = ReadGeologyTable(_span_desc);
		uint32_t count = 0;
		// The whole sparse shape family has a fixed row cap; indices/pointers cannot hide unbounded allocations.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_GEOLOGY_DEPOSITS * MAX_GEOLOGY_SPANS) SlErrorCorrupt("Too many geology spans");
			if (index != static_cast<int>(count)) SlErrorCorrupt("Invalid geology span table ordinal");
			GeologySpanWire wire;
			SlObject(&wire, description);
			// Span ordering and bounded width must be structurally meaningful before any deposit references are resolved.
			if (wire.deposit == UINT32_MAX || wire.ordinal >= MAX_GEOLOGY_SPANS || wire.first_x > wire.last_x || wire.last_x - wire.first_x >= 64) SlErrorCorrupt("Invalid geology span fields");
			if (!preview) _loaded_spans.push_back(wire);
			++count;
		}
		if (preview) _load_check_data.geology_span_rows = count;
	}
	/** Stage compact references; the header's fixup resolves them after all families are read. */
	void Load() const override
	{
		if (_saw_spans) SlErrorCorrupt("Duplicate geology span chunk");
		this->Read(false);
		_saw_spans = true;
	}
	/** @param len Unused table payload length; bounded row parsing owns the transport. */
	void LoadCheck([[maybe_unused]] size_t len) const override
	{
		if (_load_check_data.geology_spans_present) SlErrorCorrupt("Duplicate geology preview spans");
		this->Read(true);
		_load_check_data.geology_spans_present = true;
	}
};

/** Natural thermal family with one bounded cursor header and compact field rows; no heat allocation lives here. */
struct GVNTChunkHandler : ChunkHandler {
	/** Register the independent natural thermal descriptor. */
	GVNTChunkHandler() : ChunkHandler("GVNT", ChunkType::Table) {}
	/** Save natural source identity/capacity independently of mineral reserves and utility meters. */
	void Save() const override
	{
		SlTableHeader(_vent_desc);
		const auto &state = GetGeologyWorldState();
		GeothermalVentWire header;
		header.next_id = state.next_vent_id;
		header.count = static_cast<uint32_t>(state.vents.size());
		SlSetArrayIndex(0);
		SlObject(&header, _vent_desc);
		uint32_t row = 1;
		// Actual increasing IDs/rectangles/rates are authority; a shared natural source is not saved once per consumer.
		for (const auto &field : state.vents) {
			GeothermalVentWire wire;
			wire.kind = 1;
			wire.version = field.generation_version;
			wire.id = field.id.base();
			wire.layer = to_underlying(field.layer);
			wire.x = field.x; wire.y = field.y;
			wire.width = field.width; wire.height = field.height;
			wire.capacity = field.thermal_capacity_w;
			SlSetArrayIndex(row++);
			SlObject(&wire, _vent_desc);
		}
	}
	/**
	 * Read a bounded natural family into descriptor staging or independent preview counters.
	 * @param preview Never alter live source records, seed roots or the current accepted profile.
	 */
	void Read(bool preview) const
	{
		const auto description = ReadGeologyTable(_vent_desc);
		if (SlIterateArray() != 0) SlErrorCorrupt("Missing geothermal field header");
		GeothermalVentWire header;
		SlObject(&header, description);
		// One exact header bounds all allocations; a field row cannot impersonate a cursor or redefine the source version.
		if (header.kind != 0 || header.version != 1 || header.count > MAX_GEOTHERMAL_VENT_FIELDS) SlErrorCorrupt("Invalid geothermal field header");
		uint32_t count = 0;
		const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX();
		const uint32_t height = preview ? _load_check_data.map_size_y : Map::SizeY();
		// Every actual field must have a unique increasing natural ID and a fully bounded Deep rectangle/rate.
		uint32_t minimum_id = 0; // Actual and copied families share canonical identities; a duplicate cannot lend a dependent claim phantom capacity.
		for (int row = SlIterateArray(); row != -1; row = SlIterateArray()) {
			if (count >= header.count) SlErrorCorrupt("Too many geothermal fields");
			if (row != static_cast<int>(count + 1)) SlErrorCorrupt("Invalid geothermal table ordinal");
			GeothermalVentWire wire;
			SlObject(&wire, description);
			// Thermal authority is a natural source at one Deep location, never an extra cargo or an unbounded plant bonus.
			if (wire.kind != 1 || wire.version != 1 || wire.id >= header.next_id || wire.layer != to_underlying(MapLayer::DeepUnderground) || wire.capacity == 0 || wire.capacity > MAX_UTILITY_RATE_W) SlErrorCorrupt("Invalid geothermal source fields");
			if (wire.id < minimum_id) SlErrorCorrupt("Invalid geothermal source order");
			minimum_id = wire.id + 1;
			// Exact rectangle bounds are checked before any lookup/collector can treat the field as usable ground.
			if (wire.width == 0 || wire.height == 0 || wire.width > 16 || wire.height > 16 || wire.x + static_cast<uint32_t>(wire.width) - 1 >= width - 1 || wire.y + static_cast<uint32_t>(wire.height) - 1 >= height - 1) SlErrorCorrupt("Invalid geothermal source footprint");
			GeothermalVentField field;
			field.id = GeothermalVentID{wire.id}; field.generation_version = wire.version;
			field.layer = static_cast<MapLayer>(wire.layer); field.x = wire.x; field.y = wire.y;
			field.width = wire.width; field.height = wire.height; field.thermal_capacity_w = wire.capacity;
			// Dependent preview validates its explicit natural references against
			// this file's copied fields, never the running world's source registry.
			if (preview) _load_check_data.geology_vent_fields.push_back(field);
			else _geology_load.vents.push_back(field);
			++count;
		}
		if (count != header.count) SlErrorCorrupt("Incomplete geothermal field family");
		if (preview) {
			_load_check_data.geology_vent_rows = count;
		} else {
			_geology_load.next_vent_id = header.next_id;
		}
	}
	/** Stage natural fields for the geological owner's combined shape/reference validation. */
	void Load() const override
	{
		if (_saw_vents) SlErrorCorrupt("Duplicate geothermal field chunk");
		this->Read(false);
		_saw_vents = true;
	}
	/** @param len Unused table payload length; bounded row parser owns it. */
	void LoadCheck([[maybe_unused]] size_t len) const override
	{
		if (_load_check_data.geology_vents_present) SlErrorCorrupt("Duplicate geothermal preview fields");
		this->Read(true);
		_load_check_data.geology_vents_present = true;
	}
};

static const GEOSChunkHandler GEOS;
static const GDEPChunkHandler GDEP;
static const GSPNChunkHandler GSPN;
static const GVNTChunkHandler GVNT;
static const ChunkHandlerRef geology_chunk_handlers[] = {GEOS, GDEP, GSPN, GVNT};
extern const ChunkHandlerTable _geology_chunk_handlers(geology_chunk_handlers);
