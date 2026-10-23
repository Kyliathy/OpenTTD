/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_sl.cpp Bounded sparse job and frozen-pass transport; geology publishes after combined validation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 10B/14 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../excavation_state.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

static constexpr uint32_t MAX_JOB_ROUTE_ROWS = MAX_EXCAVATION_JOBS * MAX_EXCAVATION_ROUTE; ///< Bounded segment transport.
static constexpr uint32_t MAX_JOB_TILE_ROWS = MAX_EXCAVATION_JOBS * MAX_EXCAVATION_JOB_TILES * 2 + 262144; ///< Floor and aggregate envelope references.

/** Fixed scalar job row; vectors and typed references are reconstructed only after all bounded families exist. */
struct ExcavationJobWire {
	uint32_t id = UINT32_MAX; ///< Stable actual identity.
	uint8_t company = 255; ///< Actual company or explicit paused/closed orphan.
	uint32_t cutter = UINT32_MAX, origin = UINT32_MAX; ///< Optional real owner references; invalid denotes unbound geometry.
	uint16_t model = UINT16_MAX; ///< Immutable logical TBM model.
	uint32_t anchor = UINT32_MAX; ///< Explicit world tile, never camera-local coordinates.
	uint8_t state = 0, reason = 0; ///< Saved bounded phase/recoverable pause.
	uint32_t cursor = 0; ///< Sole next segment.
	uint64_t partial = 0, revision = 0; ///< Synchronized work and transaction stamp.
	uint64_t earth = 0, rock = 0; ///< Historical committed statistics, never unloadable inventory.
	int64_t cost = 0; ///< Historical paid work, never a second cash account.
	uint16_t requested = 0, authorized = 0, routes = 0; ///< Exact bounded child cardinalities.
};

/** One advancing pose with exact child envelope cardinalities. */
struct ExcavationRouteWire {
	uint32_t job = UINT32_MAX; ///< Actual job identity, not dense transport ordinal.
	uint16_t ordinal = 0; ///< Saved segment index.
	uint32_t position = UINT32_MAX; ///< Explicit world pose.
	uint8_t heading = 255, swept = 0, support = 0; ///< Eight native headings and bounded envelope counts.
};

/** One explicit ordered cell; kind 0/1 is requested/authorized floor, kind 2/3 swept/support envelope. */
struct ExcavationTileWire {
	uint32_t job = UINT32_MAX; ///< Typed parent identity.
	uint8_t kind = 0; ///< Fixed child family meaning.
	uint16_t route = 0, ordinal = 0; ///< Zero route for floors; segment index for envelopes; canonical child index.
	uint32_t tile = UINT32_MAX; ///< Explicit saved world tile.
};

static const SaveLoad _job_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(ExcavationJobWire, id)),
	SaveLoad::Variable<VarFileType::U8>("company", SLE_OBJECT_ADDRESS(ExcavationJobWire, company)),
	SaveLoad::Variable<VarFileType::U32>("cutter", SLE_OBJECT_ADDRESS(ExcavationJobWire, cutter)),
	SaveLoad::Variable<VarFileType::U32>("origin", SLE_OBJECT_ADDRESS(ExcavationJobWire, origin)),
	SaveLoad::Variable<VarFileType::U16>("model", SLE_OBJECT_ADDRESS(ExcavationJobWire, model)),
	SaveLoad::Variable<VarFileType::U32>("anchor", SLE_OBJECT_ADDRESS(ExcavationJobWire, anchor)),
	SaveLoad::Variable<VarFileType::U8>("state", SLE_OBJECT_ADDRESS(ExcavationJobWire, state)),
	SaveLoad::Variable<VarFileType::U8>("reason", SLE_OBJECT_ADDRESS(ExcavationJobWire, reason)),
	SaveLoad::Variable<VarFileType::U32>("cursor", SLE_OBJECT_ADDRESS(ExcavationJobWire, cursor)),
	SaveLoad::Variable<VarFileType::U64>("partial", SLE_OBJECT_ADDRESS(ExcavationJobWire, partial)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(ExcavationJobWire, revision)),
	SaveLoad::Variable<VarFileType::U64>("earth", SLE_OBJECT_ADDRESS(ExcavationJobWire, earth)),
	SaveLoad::Variable<VarFileType::U64>("rock", SLE_OBJECT_ADDRESS(ExcavationJobWire, rock)),
	SaveLoad::Variable<VarFileType::I64>("cost", SLE_OBJECT_ADDRESS(ExcavationJobWire, cost)),
	SaveLoad::Variable<VarFileType::U16>("requested", SLE_OBJECT_ADDRESS(ExcavationJobWire, requested)),
	SaveLoad::Variable<VarFileType::U16>("authorized", SLE_OBJECT_ADDRESS(ExcavationJobWire, authorized)),
	SaveLoad::Variable<VarFileType::U16>("routes", SLE_OBJECT_ADDRESS(ExcavationJobWire, routes)),
};
static const SaveLoad _route_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("job", SLE_OBJECT_ADDRESS(ExcavationRouteWire, job)),
	SaveLoad::Variable<VarFileType::U16>("ordinal", SLE_OBJECT_ADDRESS(ExcavationRouteWire, ordinal)),
	SaveLoad::Variable<VarFileType::U32>("position", SLE_OBJECT_ADDRESS(ExcavationRouteWire, position)),
	SaveLoad::Variable<VarFileType::U8>("heading", SLE_OBJECT_ADDRESS(ExcavationRouteWire, heading)),
	SaveLoad::Variable<VarFileType::U8>("swept", SLE_OBJECT_ADDRESS(ExcavationRouteWire, swept)),
	SaveLoad::Variable<VarFileType::U8>("support", SLE_OBJECT_ADDRESS(ExcavationRouteWire, support)),
};
static const SaveLoad _job_tile_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("job", SLE_OBJECT_ADDRESS(ExcavationTileWire, job)),
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(ExcavationTileWire, kind)),
	SaveLoad::Variable<VarFileType::U16>("route", SLE_OBJECT_ADDRESS(ExcavationTileWire, route)),
	SaveLoad::Variable<VarFileType::U16>("ordinal", SLE_OBJECT_ADDRESS(ExcavationTileWire, ordinal)),
	SaveLoad::Variable<VarFileType::U32>("tile", SLE_OBJECT_ADDRESS(ExcavationTileWire, tile)),
};

static std::array<bool, 4> _job_presence{}; ///< Loader transport presence, never actual runtime records.
static std::array<uint32_t, 4> _job_header{}; ///< Next ID plus job/route/cell bounds from EXJS.
static std::vector<ExcavationJobWire> _job_rows; ///< Bounded unpublished scalars.
static std::vector<ExcavationRouteWire> _route_rows; ///< Bounded unresolved poses.
static std::vector<ExcavationTileWire> _job_tile_rows; ///< Bounded unresolved children; claims are rebuilt from authority.
static bool _pass_presence = false; ///< Unpublished pass-family presence.
static std::map<ExcavationJobID, ExcavationPassTerms> _pass_rows; ///< Bounded frozen input, attached only to real staged parents.

/** Reset unpublished wire transport when the actual world initializes. */
void ResetExcavationLoadStaging()
{
	_job_presence.fill(false);
	_job_header.fill(0);
	_job_rows.clear();
	_route_rows.clear();
	_job_tile_rows.clear();
	_pass_presence = false;
	_pass_rows.clear();
}

/**
 * Read a mandatory exact job field table without silently supplying default work/ownership.
 * @param required Frozen version-1 descriptor.
 * @return Actual complete descriptor or ordinary corruption error.
 */
static std::vector<SaveLoad> ReadExcavationTable(const SaveLoadTable &required)
{
	if (IsSavegameVersionBefore(SaveLoadVersion::ExcavationJobs)) SlErrorCorrupt("Unexpected excavation jobs in old version");
	auto actual = SlTableHeader(required);
	if (actual.size() != required.size()) SlErrorCorrupt("Invalid excavation field count");
	// Missing fields cannot become unclaimed terrain, zero paid work or a fabricated controller reference.
	for (const auto &field : required) {
		if (std::ranges::find(actual, field.name, &SaveLoad::name) == actual.end()) SlErrorCorrupt("Missing excavation field");
	}
	return actual;
}

/**
 * Check serialized world-tile address bounds using the actual map or independent copied preview geometry.
 * @param tile Wire world index; no plane offset is used to define gameplay permission.
 * @param preview Read copied save geometry rather than current world storage.
 * @return Whether the address belongs to a bounded rock-plane transport region.
 */
static bool CheckExcavationWireTile(uint32_t tile, bool preview)
{
	const uint32_t width = preview ? _load_check_data.map_size_x : Map::SizeX();
	const uint32_t height = preview ? _load_check_data.map_size_y : Map::SizeY();
	const uint32_t layers = preview ? _load_check_data.map_layers : Map::LayerCount();
	// Validate dimensions before multiplication/division; malformed preview data cannot overflow or address live terrain.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || layers < 4 || layers > MAX_MAP_LAYERS) return false;
	const uint32_t plane = width * height;
	if (tile >= static_cast<uint64_t>(plane) * layers) return false;
	const uint32_t saved_layer = tile / plane;
	return saved_layer == to_underlying(MapLayer::Underground) || saved_layer == to_underlying(MapLayer::DeepUnderground);
}

/** Fixed five-U32 transport header bounds all sparse children before combined publication. */
struct EXJSChunkHandler : ChunkHandler {
	/** Register the independent job family header. */
	EXJSChunkHandler() : ChunkHandler("EXJS", ChunkType::Riff) {}
	/** Save next-ID/counts without serializing lookup cache addresses. */
	void Save() const override
	{
		const auto &state = GetGeologyWorldState();
		std::array<uint32_t, 5> header = {1, state.next_job_id, static_cast<uint32_t>(state.jobs.size()), 0, 0};
		// Count canonical floors/envelopes once; child tables preserve the same traversal and exact cardinalities.
		for (const auto &job : state.jobs) {
			header[3] += static_cast<uint32_t>(job.route.size());
			header[4] += static_cast<uint32_t>(job.requested_tiles.size() + job.authorized_tiles.size());
			for (const auto &cut : job.route) header[4] += static_cast<uint32_t>(cut.swept_tiles.size() + cut.support_tiles.size());
		}
		SlSetLength(20);
		SlCopy<VarFileType::U32>(header);
	}
	/**
	 * Read exact bounded scalar counts for load or preview.
	 * @param preview Select the independent preview container.
	 */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::ExcavationJobs)) SlErrorCorrupt("Unexpected excavation header");
		if (SlGetFieldLength() != 20) SlErrorCorrupt("Invalid excavation header length");
		std::array<uint32_t, 5> header;
		SlCopy<VarFileType::U32>(header);
		// Reject oversized/unknown families before any child table allocation or geometry interpretation.
		if (header[0] != 1 || header[2] > MAX_EXCAVATION_JOBS || header[3] > MAX_JOB_ROUTE_ROWS || header[4] > MAX_JOB_TILE_ROWS) SlErrorCorrupt("Invalid excavation header bounds");
		auto &presence = preview ? _load_check_data.excavation_present : _job_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate excavation header");
		presence[0] = true;
		auto &counts = preview ? _load_check_data.excavation_header : _job_header;
		std::copy(header.begin() + 1, header.end(), counts.begin());
	}
	/** Stage only bounded copied counts. */
	void Load() const override { this->Read(false); }
	/** @param len Length already checked by the fixed header reader. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Canonical sparse job scalar table, with exact route/floor child counts. */
struct XJOBChunkHandler : ChunkHandler {
	/** Register actual job scalar transport. */
	XJOBChunkHandler() : ChunkHandler("XJOB", ChunkType::Table) {}
	/** Save scalar authority and optional real references in stable job-ID order. */
	void Save() const override
	{
		SlTableHeader(_job_desc);
		uint32_t ordinal = 0;
		// Dense table indices are transport only; explicit job IDs preserve retired holes.
		for (const auto &job : GetGeologyWorldState().jobs) {
			ExcavationJobWire wire{job.id.base(), job.company.base(), job.cutter.base(), job.origin.base(), job.model_id, job.commissioning_anchor.base(), to_underlying(job.state), to_underlying(job.pause_reason), job.cursor, job.partial_work_ticks, job.revision, job.completed_earth_t, job.completed_rock_t, job.completed_work_cost.base(), static_cast<uint16_t>(job.requested_tiles.size()), static_cast<uint16_t>(job.authorized_tiles.size()), static_cast<uint16_t>(job.route.size())};
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _job_desc);
		}
	}
	/** @param preview Validate copied scalar rows without creating live/staged preview jobs. */
	void Read(bool preview) const
	{
		const auto description = ReadExcavationTable(_job_desc);
		uint32_t count = 0, previous = UINT32_MAX;
		// Check scalar identity/progress and child counts before allocating any child envelope.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_EXCAVATION_JOBS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid excavation job ordinal/count");
			ExcavationJobWire wire;
			SlObject(&wire, description);
			// A saved proposed/unknown/duplicate identity, missing floor, wrapped stamp or negative paid total cannot become accepted work.
			if (wire.id == UINT32_MAX || (count != 0 && wire.id <= previous) || (wire.company != 255 && wire.company >= MAX_COMPANIES) || wire.state == 0 || wire.state > to_underlying(ExcavationJobState::Cancelled) || wire.reason > to_underlying(ExcavationError::Limit) || wire.requested == 0 || wire.authorized == 0 || wire.requested > MAX_EXCAVATION_JOB_TILES || wire.authorized > MAX_EXCAVATION_JOB_TILES || wire.routes == 0 || wire.routes > MAX_EXCAVATION_ROUTE || wire.cursor > wire.routes || wire.partial > MAX_EXCAVATION_WORK_TICKS || wire.revision == 0 || wire.revision == UINT64_MAX || wire.cost < 0 || !CheckExcavationWireTile(wire.anchor, preview)) SlErrorCorrupt("Invalid excavation job fields");
			previous = wire.id;
			if (wire.origin > StationID::Invalid().base()) SlErrorCorrupt("Invalid excavation station reference width");
			if (!preview) _job_rows.push_back(wire);
			++count;
		}
		auto &presence = preview ? _load_check_data.excavation_present : _job_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate excavation job table");
		presence[1] = true;
		if (preview) _load_check_data.excavation_rows[0] = count;
	}
	/** Stage unpublished scalar authority. */
	void Load() const override { this->Read(false); }
	/** @param len Table row reader owns length validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Saved advancing poses; routes are reconstructed through exact typed parent/ordinal references. */
struct XJRTChunkHandler : ChunkHandler {
	/** Register bounded route transport. */
	XJRTChunkHandler() : ChunkHandler("XJRT", ChunkType::Table) {}
	/** Save accepted forward poses and envelope sizes. */
	void Save() const override
	{
		SlTableHeader(_route_desc);
		uint32_t ordinal = 0;
		// Accepted route order defines the sole saved cursor meaning; no pathfinder cache is saved.
		for (const auto &job : GetGeologyWorldState().jobs) {
			for (uint16_t i = 0; i < job.route.size(); ++i) {
				const auto &cut = job.route[i];
				ExcavationRouteWire wire{job.id.base(), i, cut.cutter_position.base(), to_underlying(cut.heading), static_cast<uint8_t>(cut.swept_tiles.size()), static_cast<uint8_t>(cut.support_tiles.size())};
				SlSetArrayIndex(ordinal++);
				SlObject(&wire, _route_desc);
			}
		}
	}
	/** @param preview Check bounded pose fields independently of current gameplay/terrain. */
	void Read(bool preview) const
	{
		const auto description = ReadExcavationTable(_route_desc);
		uint32_t count = 0;
		// Envelope sizes are checked before later typed-reference assembly allocates their vectors.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_JOB_ROUTE_ROWS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid excavation route ordinal/count");
			ExcavationRouteWire wire;
			SlObject(&wire, description);
			if (wire.job == UINT32_MAX || wire.ordinal >= MAX_EXCAVATION_ROUTE || wire.heading >= 8 || wire.swept == 0 || wire.swept > 64 || wire.support > 64 || !CheckExcavationWireTile(wire.position, preview)) SlErrorCorrupt("Invalid excavation route fields"); // Every pose/envelope remains bounded even during preview.
			if (!preview) _route_rows.push_back(wire);
			++count;
		}
		auto &presence = preview ? _load_check_data.excavation_present : _job_presence;
		if (presence[2]) SlErrorCorrupt("Duplicate excavation routes");
		presence[2] = true;
		if (preview) _load_check_data.excavation_rows[1] = count;
	}
	/** Stage unresolved poses. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns actual row lengths. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Explicit canonical floors and envelopes; cached exclusive tile owners are never serialized. */
struct XJTLChunkHandler : ChunkHandler {
	/** Register compact tile references. */
	XJTLChunkHandler() : ChunkHandler("XJTL", ChunkType::Table) {}
	/**
	 * Save one canonical child family in exact ordinal order.
	 * @param job Actual stable parent identity.
	 * @param kind Fixed requested/authorized/swept/support meaning.
	 * @param route Actual segment index, or zero for floors.
	 * @param tiles Actual canonical child cells.
	 * @param ordinal Dense transport index, advanced per emitted reference.
	 */
	static void WriteTiles(ExcavationJobID job, uint8_t kind, uint16_t route, std::span<const TileIndex> tiles, uint32_t &ordinal)
	{
		// Stable child order prevents duplicated/omitted cells from changing the meaning of a loaded authorization.
		for (uint16_t i = 0; i < tiles.size(); ++i) {
			ExcavationTileWire wire{job.base(), kind, route, i, tiles[i].base()};
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _job_tile_desc);
		}
	}
	/** Save actual owner geometry, independent of claim-cache insertion order. */
	void Save() const override
	{
		SlTableHeader(_job_tile_desc);
		uint32_t ordinal = 0;
		// Floors precede route envelopes for each stable parent; all references use the real saved identity.
		for (const auto &job : GetGeologyWorldState().jobs) {
			WriteTiles(job.id, 0, 0, job.requested_tiles, ordinal);
			WriteTiles(job.id, 1, 0, job.authorized_tiles, ordinal);
			for (uint16_t i = 0; i < job.route.size(); ++i) {
				WriteTiles(job.id, 2, i, job.route[i].swept_tiles, ordinal);
				WriteTiles(job.id, 3, i, job.route[i].support_tiles, ordinal);
			}
		}
	}
	/** @param preview Validate only copied bounded wire references; actual parent/terrain claims resolve during full load. */
	void Read(bool preview) const
	{
		const auto description = ReadExcavationTable(_job_tile_desc);
		uint32_t count = 0;
		// Total rows and per-family ordinals are bounded before any job can acquire reconstructed floor permission.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_JOB_TILE_ROWS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid excavation cell ordinal/count");
			ExcavationTileWire wire;
			SlObject(&wire, description);
			if (wire.job == UINT32_MAX || wire.kind > 3 || wire.route >= MAX_EXCAVATION_ROUTE || wire.ordinal >= (wire.kind < 2 ? MAX_EXCAVATION_JOB_TILES : 64) || (wire.kind < 2 && wire.route != 0) || !CheckExcavationWireTile(wire.tile, preview)) SlErrorCorrupt("Invalid excavation cell fields"); // Scalar transport cannot request a huge, cross-domain or missing envelope.
			if (!preview) _job_tile_rows.push_back(wire);
			++count;
		}
		auto &presence = preview ? _load_check_data.excavation_present : _job_presence;
		if (presence[3]) SlErrorCorrupt("Duplicate excavation cells");
		presence[3] = true;
		if (preview) _load_check_data.excavation_rows[2] = count;
	}
	/** Stage unresolved explicit child cells. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns actual row lengths. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Version-374 frozen return/capacity transport with explicitly bounded variable children. */
struct XPASChunkHandler : ChunkHandler {
	/** Register the complete-prefix pass family. */
	XPASChunkHandler() : ChunkHandler("XPAS", ChunkType::Riff) {}
	/** Save actual accepted limits/returns, excluding mutable cargo and reconstructed claims. */
	void Save() const override
	{
		uint32_t count = 0, length = 8;
		// Size the exact bounded family before writing its immutable input records.
		for (const auto &job : GetGeologyWorldState().jobs) {
			if (!job.pass_terms) continue;
			++count;
			length += 32 + static_cast<uint32_t>(job.pass_terms->exchange_path.size() * 4 + job.pass_terms->exchange_after.size() * 2);
		}
		SlSetLength(length);
		std::array<uint32_t, 2> header{1, count};
		SlCopy<VarFileType::U32>(header);
		// Persist static limits and explicit return references in stable actual job-ID order.
		for (const auto &job : GetGeologyWorldState().jobs) {
			if (!job.pass_terms) continue;
			const auto &pass = *job.pass_terms;
			std::array<uint32_t, 1> id{job.id.base()};
			std::array<uint64_t, 3> values{pass.earth_capacity_t, pass.rock_capacity_t, pass.fingerprint};
			std::array<uint16_t, 2> sizes{static_cast<uint16_t>(pass.exchange_path.size()), static_cast<uint16_t>(pass.exchange_after.size())};
			SlCopy<VarFileType::U32>(id); SlCopy<VarFileType::U64>(values); SlCopy<VarFileType::U16>(sizes);
			for (TileIndex tile : pass.exchange_path) {
				std::array<uint32_t, 1> cell{tile.base()};
				SlCopy<VarFileType::U32>(cell);
			}
			auto returns = pass.exchange_after;
			SlCopy<VarFileType::U16>(returns);
		}
	}
	/**
	 * Bound scalar lengths/counts before allocating or interpreting a frozen input.
	 * @param preview Validate independent copied transport without touching live or staged job owners.
	 */
	void Read(bool preview) const
	{
		const size_t length = SlGetFieldLength();
		// The new family must fit the hard job/path/route envelope before any variable record is read.
		if (IsSavegameVersionBefore(SaveLoadVersion::ExcavationPasses) || length < 8 || length > 8 + MAX_EXCAVATION_JOBS * (32 + 64 * 4 + MAX_EXCAVATION_ROUTE * 2)) SlErrorCorrupt("Invalid excavation pass length/version");
		auto &present = preview ? _load_check_data.excavation_pass_present : _pass_presence;
		if (present) SlErrorCorrupt("Duplicate excavation pass family");
		std::array<uint32_t, 2> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_EXCAVATION_JOBS) SlErrorCorrupt("Invalid excavation pass header"); // Unknown/oversized input cannot authorize a job.
		size_t consumed = 8;
		uint32_t previous = UINT32_MAX;
		// Every parent and variable envelope is checked before allocation; preview never supplies a fabricated live job.
		for (uint32_t i = 0; i < header[1]; ++i) {
			if (length - consumed < 32) SlErrorCorrupt("Truncated excavation pass");
			std::array<uint32_t, 1> id;
			std::array<uint64_t, 3> values;
			std::array<uint16_t, 2> sizes;
			SlCopy<VarFileType::U32>(id); SlCopy<VarFileType::U64>(values); SlCopy<VarFileType::U16>(sizes);
			consumed += 32;
			const size_t children = sizes[0] * 4 + sizes[1] * 2;
			// Exact nonempty references and monotonic identities forbid duplicate/oversized or partially omitted backing input.
			if (id[0] == UINT32_MAX || (i != 0 && id[0] <= previous) || sizes[0] == 0 || sizes[0] > 64 || sizes[1] == 0 || sizes[1] > MAX_EXCAVATION_ROUTE || children > length - consumed) SlErrorCorrupt("Invalid excavation pass fields");
			previous = id[0];
			ExcavationPassTerms terms;
			terms.earth_capacity_t = values[0]; terms.rock_capacity_t = values[1]; terms.fingerprint = values[2];
			for (uint16_t j = 0; j < sizes[0]; ++j) {
				std::array<uint32_t, 1> cell;
				SlCopy<VarFileType::U32>(cell);
				if (!CheckExcavationWireTile(cell[0], preview)) SlErrorCorrupt("Invalid excavation pass tile");
				terms.exchange_path.push_back(TileIndex{cell[0]});
			}
			terms.exchange_after.resize(sizes[1]);
			SlCopy<VarFileType::U16>(terms.exchange_after);
			uint16_t prior_prefix = 0;
			for (uint16_t prefix : terms.exchange_after) {
				if (prefix <= prior_prefix || prefix > MAX_EXCAVATION_ROUTE) SlErrorCorrupt("Invalid excavation return prefix"); // Empty/repeated exchanges cannot conceal load limits.
				prior_prefix = prefix;
			}
			consumed += children;
			if (!preview) _pass_rows.emplace(ExcavationJobID{id[0]}, std::move(terms));
		}
		if (consumed != length) SlErrorCorrupt("Surplus excavation pass bytes");
		present = true;
		if (preview) _load_check_data.excavation_pass_rows = header[1];
	}
	/** Stage only bounded unpublished static terms. */
	void Load() const override { this->Read(false); }
	/** @param len The reader checks exact RIFF scalar/child length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/**
 * Check pass family absence/count against independent metadata without granting input to a missing job.
 * @param preview Select copied preview counts rather than unpublished full-load rows.
 */
static void ValidateExcavationPassFamily(bool preview)
{
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_EXCAVATION_PASSES) != 0;
	const bool present = preview ? _load_check_data.excavation_pass_present : _pass_presence;
	const size_t rows = preview ? _load_check_data.excavation_pass_rows : _pass_rows.size();
	// New saves always carry an explicit family, even empty; old absence preserves unvalidated historical work only.
	if ((!present && (!IsSavegameVersionBefore(SaveLoadVersion::ExcavationPasses) || required)) || required != (rows != 0)) SlErrorCorrupt("Incomplete excavation pass family");
}

/**
 * Resolve bounded child references only after all families are available; nothing here publishes live state.
 * @param snapshot Staged geological transport receiving complete canonical jobs and the monotonic cursor.
 */
void CompleteExcavationLoad(GeologyStateSnapshot &snapshot)
{
	ValidateExcavationPassFamily(false);
	const auto *metadata = GetPendingMultiLayerStateSchema();
	if (metadata == nullptr) SlErrorCorrupt("Missing excavation metadata");
	const bool required = (metadata->feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) != 0;
	if (!_job_presence[0]) {
		// Only predecessor files may omit this complete family; stray children or an explicit feature never become empty legacy work.
		if (!IsSavegameVersionBefore(SaveLoadVersion::ExcavationJobs) || required || _job_presence[1] || _job_presence[2] || _job_presence[3]) SlErrorCorrupt("Missing excavation job header");
		snapshot.jobs.clear();
		snapshot.next_job_id = 0;
		return;
	}
	// Every exact cardinality and presence must agree before resolving any identity into a job record.
	if (!_job_presence[1] || !_job_presence[2] || !_job_presence[3] || _job_rows.size() != _job_header[1] || _route_rows.size() != _job_header[2] || _job_tile_rows.size() != _job_header[3] || required != (_job_header[0] != 0)) SlErrorCorrupt("Incomplete excavation family");
	snapshot.jobs.clear();
	snapshot.next_job_id = _job_header[0];
	// Reconstruct scalars idempotently; route/floor vectors always start empty before exact children are appended.
	for (const auto &wire : _job_rows) {
		if (wire.id >= snapshot.next_job_id) SlErrorCorrupt("Invalid excavation allocator cursor");
		ExcavationJob job;
		job.id = ExcavationJobID{wire.id}; job.company = CompanyID{wire.company};
		job.cutter = VehicleID{wire.cutter}; job.origin = StationID{static_cast<StationID::BaseType>(wire.origin)};
		job.model_id = wire.model; job.commissioning_anchor = TileIndex{wire.anchor};
		job.state = static_cast<ExcavationJobState>(wire.state); job.pause_reason = static_cast<ExcavationError>(wire.reason);
		job.cursor = wire.cursor; job.partial_work_ticks = wire.partial; job.revision = wire.revision;
		job.completed_earth_t = wire.earth; job.completed_rock_t = wire.rock; job.completed_work_cost = wire.cost;
		snapshot.jobs.push_back(std::move(job));
	}
	// Resolve actual stable-ID parents and exact segment ordinals, never creating a missing record for a wire reference.
	for (const auto &wire : _route_rows) {
		auto it = std::ranges::lower_bound(snapshot.jobs, ExcavationJobID{wire.job}, {}, &ExcavationJob::id);
		if (it == snapshot.jobs.end() || it->id != ExcavationJobID{wire.job} || it->route.size() != wire.ordinal) SlErrorCorrupt("Invalid excavation route reference");
		it->route.push_back({TileIndex{wire.position}, static_cast<Direction>(wire.heading), {}, {}});
	}
	// Child vectors enforce exact ordinals per actual parent/family/segment before geometry and exclusive claims are validated.
	for (const auto &wire : _job_tile_rows) {
		auto it = std::ranges::lower_bound(snapshot.jobs, ExcavationJobID{wire.job}, {}, &ExcavationJob::id);
		if (it == snapshot.jobs.end() || it->id != ExcavationJobID{wire.job}) SlErrorCorrupt("Invalid excavation cell parent");
		std::vector<TileIndex> *tiles;
		if (wire.kind < 2) {
			tiles = wire.kind == 0 ? &it->requested_tiles : &it->authorized_tiles;
		} else {
			if (wire.route >= it->route.size()) SlErrorCorrupt("Invalid excavation cell segment");
			tiles = wire.kind == 2 ? &it->route[wire.route].swept_tiles : &it->route[wire.route].support_tiles;
		}
		if (tiles->size() != wire.ordinal) SlErrorCorrupt("Invalid excavation cell ordinal");
		tiles->push_back(TileIndex{wire.tile});
	}
	// Scalars' exact per-record counts cannot be satisfied by surplus references attached to another job.
	for (size_t i = 0; i < snapshot.jobs.size(); ++i) {
		const auto &job = snapshot.jobs[i];
		const auto &wire = _job_rows[i];
		if (job.requested_tiles.size() != wire.requested || job.authorized_tiles.size() != wire.authorized || job.route.size() != wire.routes) SlErrorCorrupt("Incomplete excavation geometry");
	}
	for (const auto &wire : _route_rows) {
		const auto it = std::ranges::lower_bound(snapshot.jobs, ExcavationJobID{wire.job}, {}, &ExcavationJob::id);
		const auto &cut = it->route[wire.ordinal];
		if (cut.swept_tiles.size() != wire.swept || cut.support_tiles.size() != wire.support) SlErrorCorrupt("Incomplete excavation envelopes"); // Exact saved support cannot disappear while its paid work remains accepted.
	}
	// Attach immutable input only after every typed parent and complete canonical route exists.
	for (const auto &[id, terms] : _pass_rows) {
		auto it = std::ranges::lower_bound(snapshot.jobs, id, {}, &ExcavationJob::id);
		if (it == snapshot.jobs.end() || it->id != id) SlErrorCorrupt("Invalid excavation pass parent"); // Orphan input cannot manufacture missing progress/claims.
		it->pass_terms = terms;
		if (ValidateExcavationPassTerms(*it, terms).error != ExcavationError::None) SlErrorCorrupt("Invalid excavation pass proof");
	}
}

/** Validate independent preview completeness without staging/publishing a candidate into actual geology. */
void ValidateExcavationLoadPreview()
{
	ValidateExcavationPassFamily(true);
	const auto &presence = _load_check_data.excavation_present;
	const auto &header = _load_check_data.excavation_header;
	const auto &rows = _load_check_data.excavation_rows;
	const bool required = (_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) != 0;
	if (!presence[0]) {
		// Older absence remains an empty job owner; neither an explicit bit nor orphan child tables may fabricate that migration.
		if (!IsSavegameVersionBefore(SaveLoadVersion::ExcavationJobs) || required || presence[1] || presence[2] || presence[3]) SlErrorCorrupt("Missing excavation preview header");
		return;
	}
	if (!presence[1] || !presence[2] || !presence[3] || rows[0] != header[1] || rows[1] != header[2] || rows[2] != header[3] || required != (header[0] != 0)) SlErrorCorrupt("Incomplete excavation preview family"); // Preview reports loadability only for complete bounded transport; real refs resolve at full load.
}

static const EXJSChunkHandler EXJS;
static const XJOBChunkHandler XJOB;
static const XJRTChunkHandler XJRT;
static const XJTLChunkHandler XJTL;
static const XPASChunkHandler XPAS;
static const ChunkHandlerRef excavation_chunk_handlers[] = {EXJS, XJOB, XJRT, XJTL, XPAS};
extern const ChunkHandlerTable _excavation_chunk_handlers(excavation_chunk_handlers);
