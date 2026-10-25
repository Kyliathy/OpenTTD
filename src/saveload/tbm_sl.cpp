/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_sl.cpp Bounded typed TBM project transport, with the temporary support registry. Rows are structurally
 * checked here and staged; they are validated against vehicles, geology, portal work and the map only after vehicles
 * are rebuilt at the end of AfterLoadGame. Orders are saved by their own chunks and are not part of this family.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 3 and 8
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group5; version394 preserves the value of old fractional work.
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../tbm.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"

#include "../safeguards.h"

/** Fixed scalar project row; permanent parts are a bounded inline list. */
struct TBMProjectWire {
	uint32_t id = UINT32_MAX; ///< Project identity.
	uint32_t job = UINT32_MAX; ///< Geological job, or invalid with MissingProject.
	uint8_t company = 255; ///< Actual company.
	uint32_t cutter = VehicleID::Invalid().base(); ///< Front engine.
	uint16_t model = UINT16_MAX; ///< Logical cutter model.
	uint8_t parts = 0; ///< Permanent part count, 2..MAX_TBM_PERMANENT_PARTS.
	std::array<uint32_t, MAX_TBM_PERMANENT_PARTS> part{}; ///< Ordered permanent parts; unused slots must stay invalid.
	uint8_t heading = 255; ///< Cutting direction.
	uint16_t origin = StationID::Invalid().base(); ///< Surface origin station, or invalid with MissingOrigin.
	uint32_t portal = UINT32_MAX; ///< Canonical pair key, or invalid with MissingPortal.
	uint32_t exchange = UINT32_MAX; ///< Exchange tile, or invalid with MissingExchange.
	uint8_t phase = 0; ///< Saved work phase.
	uint8_t blocked = 0; ///< Saved interruption.
	uint64_t phase_ticks = 0; ///< Synchronized phase timer.
	uint16_t work_remainder = 0; ///< Fractional work-rate numerator.
	uint64_t active_ticks = 0; ///< Active duration awaiting accounting.
	uint64_t idle_ticks = 0; ///< Idle duration awaiting accounting.
	uint64_t cost_remainder = 0; ///< Fractional recurring-money numerator.
	uint64_t revision = 0; ///< Controller stamp.
};

static const SaveLoad _tbm_project_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(TBMProjectWire, id)),
	SaveLoad::Variable<VarFileType::U32>("job", SLE_OBJECT_ADDRESS(TBMProjectWire, job)),
	SaveLoad::Variable<VarFileType::U8>("company", SLE_OBJECT_ADDRESS(TBMProjectWire, company)),
	SaveLoad::Variable<VarFileType::U32>("cutter", SLE_OBJECT_ADDRESS(TBMProjectWire, cutter)),
	SaveLoad::Variable<VarFileType::U16>("model", SLE_OBJECT_ADDRESS(TBMProjectWire, model)),
	SaveLoad::Variable<VarFileType::U8>("parts", SLE_OBJECT_ADDRESS(TBMProjectWire, parts)),
	SaveLoad::Variable<VarFileType::U32>("part0", SLE_OBJECT_ADDRESS(TBMProjectWire, part[0])),
	SaveLoad::Variable<VarFileType::U32>("part1", SLE_OBJECT_ADDRESS(TBMProjectWire, part[1])),
	SaveLoad::Variable<VarFileType::U32>("part2", SLE_OBJECT_ADDRESS(TBMProjectWire, part[2])),
	SaveLoad::Variable<VarFileType::U32>("part3", SLE_OBJECT_ADDRESS(TBMProjectWire, part[3])),
	SaveLoad::Variable<VarFileType::U8>("heading", SLE_OBJECT_ADDRESS(TBMProjectWire, heading)),
	SaveLoad::Variable<VarFileType::U16>("origin", SLE_OBJECT_ADDRESS(TBMProjectWire, origin)),
	SaveLoad::Variable<VarFileType::U32>("portal", SLE_OBJECT_ADDRESS(TBMProjectWire, portal)),
	SaveLoad::Variable<VarFileType::U32>("exchange", SLE_OBJECT_ADDRESS(TBMProjectWire, exchange)),
	SaveLoad::Variable<VarFileType::U8>("phase", SLE_OBJECT_ADDRESS(TBMProjectWire, phase)),
	SaveLoad::Variable<VarFileType::U8>("blocked", SLE_OBJECT_ADDRESS(TBMProjectWire, blocked)),
	SaveLoad::Variable<VarFileType::U64>("phase_ticks", SLE_OBJECT_ADDRESS(TBMProjectWire, phase_ticks)),
	SaveLoad::Variable<VarFileType::U16>("work_remainder", SLE_OBJECT_ADDRESS(TBMProjectWire, work_remainder)),
	SaveLoad::Variable<VarFileType::U64>("active_ticks", SLE_OBJECT_ADDRESS(TBMProjectWire, active_ticks)),
	SaveLoad::Variable<VarFileType::U64>("idle_ticks", SLE_OBJECT_ADDRESS(TBMProjectWire, idle_ticks)),
	SaveLoad::Variable<VarFileType::U64>("cost_remainder", SLE_OBJECT_ADDRESS(TBMProjectWire, cost_remainder)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(TBMProjectWire, revision)),
};
static_assert(MAX_TBM_PERMANENT_PARTS == 4, "the descriptor lists exactly four part slots");

/** Fixed temporary support row (Group 8.1). */
struct TBMSupportWire {
	uint32_t tile = UINT32_MAX; ///< Rock-layer tile.
	uint32_t project = UINT32_MAX; ///< Project that laid it.
	uint8_t owner = 255; ///< Track owner.
	uint8_t track = 0; ///< Laid pieces.
	uint8_t allowed = 0; ///< Allowed traffic classes.
	uint16_t max_speed = 0; ///< Speed limit.
	int64_t conversion_price = 0; ///< Conversion price.
};

static const SaveLoad _tbm_support_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("tile", SLE_OBJECT_ADDRESS(TBMSupportWire, tile)),
	SaveLoad::Variable<VarFileType::U32>("project", SLE_OBJECT_ADDRESS(TBMSupportWire, project)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(TBMSupportWire, owner)),
	SaveLoad::Variable<VarFileType::U8>("track", SLE_OBJECT_ADDRESS(TBMSupportWire, track)),
	SaveLoad::Variable<VarFileType::U8>("allowed", SLE_OBJECT_ADDRESS(TBMSupportWire, allowed)),
	SaveLoad::Variable<VarFileType::U16>("max_speed", SLE_OBJECT_ADDRESS(TBMSupportWire, max_speed)),
	SaveLoad::Variable<VarFileType::I64>("conversion_price", SLE_OBJECT_ADDRESS(TBMSupportWire, conversion_price)),
};

static std::array<bool, 3> _tbm_presence{}; ///< Unpublished header, project table and support table presence.
static uint32_t _tbm_expected = 0; ///< Exact bounded row count from the header.
static uint32_t _tbm_next = 0; ///< Saved allocation cursor from the header.
static uint32_t _tbm_support_expected = 0; ///< Exact bounded support row count from the header.
static std::vector<TBMProject> _tbm_rows; ///< Unpublished structurally checked rows; never a second live store.
static std::vector<TBMSupportTile> _tbm_support_rows; ///< Unpublished structurally checked support rows.

/** Clear unpublished project transport when the world initializes. */
void ResetTBMProjectLoadStaging()
{
	_tbm_presence.fill(false);
	_tbm_expected = 0;
	_tbm_next = 0;
	_tbm_support_expected = 0;
	_tbm_rows.clear();
	_tbm_support_rows.clear();
}

/**
 * Check a saved tile reference against the loaded or previewed map size.
 * @param tile Saved tile index.
 * @param preview Use the copied preview dimensions instead of the live map.
 * @return Whether the index is invalid-by-design or inside the map.
 */
static bool CheckTBMWireTile(uint32_t tile, bool preview)
{
	if (tile == UINT32_MAX) return true;
	const uint64_t size = preview ? uint64_t{_load_check_data.map_size_x} * _load_check_data.map_size_y * _load_check_data.map_layers : Map::Size();
	return tile < size;
}

/**
 * Reconcile header, table and feature flag for load or preview.
 * @param preview Use the copied preview counts.
 * @return Whether a complete family exists; legitimate absence in older saves returns false.
 */
static bool CheckTBMProjectFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.tbm_project_present : _tbm_presence;
	const size_t expected = preview ? _load_check_data.tbm_project_expected : _tbm_expected;
	const size_t count = preview ? _load_check_data.tbm_project_rows : _tbm_rows.size();
	const uint32_t next = preview ? _load_check_data.tbm_project_next : _tbm_next;
	const size_t support_expected = preview ? _load_check_data.tbm_support_expected : _tbm_support_expected;
	const size_t support_count = preview ? _load_check_data.tbm_support_rows : _tbm_support_rows.size();
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0;
	if (!presence[0]) {
		// New saves always write the header; only older versions may lack the family, and then nothing may claim it.
		if (!IsSavegameVersionBefore(SaveLoadVersion::TBMProjects) || presence[1] || presence[2] || required) SlErrorCorrupt("Missing TBM project header");
		return false;
	}
	// Both tables must match the header counts, and the feature flag must match whether any ID was ever allocated.
	if (!presence[1] || !presence[2] || expected != count || support_expected != support_count || required != (next != 0)) SlErrorCorrupt("Incomplete TBM project family");
	return true;
}

/** Check the copied project family of a save preview without touching live owners. */
void ValidateTBMProjectLoadPreview()
{
	CheckTBMProjectFamily(true);
}

/** Header: format revision, exact row counts and allocation cursor; it also stages the rows once all chunks exist. */
struct TBMHChunkHandler : ChunkHandler {
	/** Register the sixteen-byte header. */
	TBMHChunkHandler() : ChunkHandler("TBMH", ChunkType::Riff) {}
	/** Save the format revision, the live count, the allocation cursor and the support count. */
	void Save() const override
	{
		std::array<uint32_t, 4> header{1, static_cast<uint32_t>(CaptureTBMProjects().size()), GetNextTBMProjectID().base(), static_cast<uint32_t>(CaptureTBMSupport().size())};
		SlSetLength(16);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Record copied bounds for the preview instead of the loader. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMProjects) || SlGetFieldLength() != 16) SlErrorCorrupt("Invalid TBM project header version/length");
		std::array<uint32_t, 4> header;
		SlCopy<VarFileType::U32>(header);
		// Unknown formats, oversized stores, a cursor that cannot cover the rows and support without any project refuse first.
		if (header[0] != 1 || header[1] > MAX_TBM_PROJECTS || header[2] == UINT32_MAX || header[1] > header[2] || header[3] > MAX_TBM_SUPPORT_TILES || (header[3] != 0 && header[2] == 0)) SlErrorCorrupt("Invalid TBM project header bounds");
		auto &presence = preview ? _load_check_data.tbm_project_present : _tbm_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate TBM project header");
		presence[0] = true;
		(preview ? _load_check_data.tbm_project_expected : _tbm_expected) = header[1];
		(preview ? _load_check_data.tbm_project_next : _tbm_next) = header[2];
		(preview ? _load_check_data.tbm_support_expected : _tbm_support_expected) = header[3];
	}
	/** Stage header bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact length checks. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage the complete family once every chunk is read; live validation waits for rebuilt vehicles. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckTBMProjectFamily(false)) return;
		StageLoadedTBMProjects(_tbm_rows, TBMProjectID{_tbm_next});
		StageLoadedTBMSupport(_tbm_support_rows);
	}
};

/** Exact project rows, bounded and structurally checked before any live reference is resolved. */
struct TBMPChunkHandler : ChunkHandler {
	/** Register the project table. */
	TBMPChunkHandler() : ChunkHandler("TBMP", ChunkType::Table) {}
	/** Save each live project once in increasing ID order. */
	void Save() const override
	{
		SlTableHeader(_tbm_project_desc);
		uint32_t ordinal = 0;
		for (const auto &project : CaptureTBMProjects()) {
			TBMProjectWire wire;
			wire.id = project.id.base();
			wire.job = project.job.base();
			wire.company = project.company.base();
			wire.cutter = project.cutter.base();
			wire.model = project.model_id;
			wire.parts = static_cast<uint8_t>(project.permanent_parts.size());
			wire.part.fill(VehicleID::Invalid().base());
			for (size_t i = 0; i < project.permanent_parts.size() && i < wire.part.size(); ++i) wire.part[i] = project.permanent_parts[i].base();
			wire.heading = to_underlying(project.heading);
			wire.origin = project.origin.base();
			wire.portal = project.portal.base();
			wire.exchange = project.exchange_bay.base();
			wire.phase = to_underlying(project.phase);
			wire.blocked = to_underlying(project.blocked);
			wire.phase_ticks = project.phase_ticks;
			wire.work_remainder = project.work_remainder;
			wire.active_ticks = project.active_ticks;
			wire.idle_ticks = project.idle_ticks;
			wire.cost_remainder = project.cost_remainder;
			wire.revision = project.revision;
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _tbm_project_desc);
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMProjects)) SlErrorCorrupt("Unexpected TBM project rows");
		const auto description = SlTableHeader(_tbm_project_desc);
		if (description.size() != std::size(_tbm_project_desc)) SlErrorCorrupt("Invalid TBM project field count");
		// A missing field cannot default to an unassigned cutter, a zero timer or an invented reference.
		for (const auto &field : _tbm_project_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing TBM project field");
		}
		uint32_t count = 0, previous = UINT32_MAX;
		// Bounds, ordering, enums and part slots are checked before a row enters unpublished transport.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_TBM_PROJECTS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid TBM project ordinal/count");
			TBMProjectWire wire;
			SlObject(&wire, description);
			const TBMCutterModel *model = GetTBMCutterModel(wire.model);
			if (model == nullptr) SlErrorCorrupt("Invalid TBM cutter model");
			const bool old_rate = IsSavegameVersionBefore(SaveLoadVersion::TBMHalfCuttingRate);
			const uint16_t divisor = old_rate ? (wire.model == 0 ? 1 : 2) : model->work_rate_denominator;
			// Validate in the original units before migration, for preview and
			// full load alike. Old Deep 1/2 becomes 2/4 exactly; completed cuts,
			// integer partial work, cash and cargo are untouched. Current saves
			// already use the new divisor and must never be scaled a second time.
			if (wire.work_remainder >= divisor) SlErrorCorrupt("Invalid TBM work fraction");
			if (old_rate) wire.work_remainder *= 2;
			// Only B425+ or the merged writer owns AwaitFaceTransfer. A's same
			// numeric425-440 history keeps FollowOrders as its original phase ceiling.
			const auto max_phase = IsSavegameVersionBefore(SaveLoadVersion::TBMReturnParking) ? TBMPhase::Cancelled :
				IsSavegameVersionBefore(SaveLoadVersion::TBMSettledRelease) ? TBMPhase::Parked :
				IsSavegameVersionBefore(SaveLoadVersion::TBMHangarDepots) ? TBMPhase::ReleasePending :
				IsSavegameVersionBefore(SaveLoadVersion::TBMNativeOrderControl) ? TBMPhase::ParkedHangar :
				SlHasSaveCapability(SaveCapability::TBMFaceTransfer) ? TBMPhase::AwaitFaceTransfer : TBMPhase::FollowOrders;
			const auto max_block = IsSavegameVersionBefore(SaveLoadVersion::TBMHangarDepots) ? TBMBlockedReason::LogisticsStall :
				IsSavegameVersionBefore(SaveLoadVersion::TBMNativeOrderControl) ? TBMBlockedReason::OccupiedHangar : TBMBlockedReason::MissingOrders;
			// Identities increase strictly; company, phase, reason and heading are known values; tiles fit the map.
			if (wire.id == UINT32_MAX || (count != 0 && wire.id <= previous) || wire.company >= MAX_COMPANIES || wire.phase > to_underlying(max_phase) || wire.blocked > to_underlying(max_block) || wire.heading >= to_underlying(Direction::End) || wire.revision == 0 || !CheckTBMWireTile(wire.portal, preview) || !CheckTBMWireTile(wire.exchange, preview)) SlErrorCorrupt("Invalid TBM project fields");
			if (wire.parts < 2 || wire.parts > MAX_TBM_PERMANENT_PARTS) SlErrorCorrupt("Invalid TBM project part count");
			// Used slots name real vehicle IDs and unused slots stay invalid, so a hidden extra part cannot be smuggled in.
			for (size_t i = 0; i < wire.part.size(); ++i) {
				const bool used = i < wire.parts;
				if (used == (wire.part[i] == VehicleID::Invalid().base())) SlErrorCorrupt("Invalid TBM project part slot");
			}
			previous = wire.id;
			if (!preview) {
				TBMProject project;
				project.id = TBMProjectID{wire.id};
				project.job = ExcavationJobID{wire.job};
				project.company = CompanyID{wire.company};
				project.cutter = VehicleID{wire.cutter};
				project.model_id = wire.model;
				for (size_t i = 0; i < wire.parts; ++i) project.permanent_parts.push_back(VehicleID{wire.part[i]});
				project.heading = static_cast<Direction>(wire.heading);
				project.origin = StationID{wire.origin};
				project.portal = TileIndex{wire.portal};
				project.exchange_bay = TileIndex{wire.exchange};
				project.phase = static_cast<TBMPhase>(wire.phase);
				project.blocked = static_cast<TBMBlockedReason>(wire.blocked);
				project.phase_ticks = wire.phase_ticks;
				project.work_remainder = wire.work_remainder;
				project.active_ticks = wire.active_ticks;
				project.idle_ticks = wire.idle_ticks;
				project.cost_remainder = wire.cost_remainder;
				project.revision = wire.revision;
				_tbm_rows.push_back(std::move(project));
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.tbm_project_present : _tbm_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate TBM project rows");
		presence[1] = true;
		if (preview) _load_check_data.tbm_project_rows = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Temporary support rows (Group 8.1), structurally checked; the map check waits for AcceptLoadedTBMProjects. */
struct TBMSChunkHandler : ChunkHandler {
	/** Register the support table. */
	TBMSChunkHandler() : ChunkHandler("TBMS", ChunkType::Table) {}
	/** Save each support tile once in increasing tile order. */
	void Save() const override
	{
		SlTableHeader(_tbm_support_desc);
		uint32_t ordinal = 0;
		for (const auto &support : CaptureTBMSupport()) {
			TBMSupportWire wire;
			wire.tile = support.tile.base();
			wire.project = support.project.base();
			wire.owner = support.owner.base();
			wire.track = support.track.base();
			wire.allowed = support.allowed.base();
			wire.max_speed = support.max_speed;
			wire.conversion_price = support.conversion_price.base();
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _tbm_support_desc);
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMProjects)) SlErrorCorrupt("Unexpected TBM support rows");
		const auto description = SlTableHeader(_tbm_support_desc);
		if (description.size() != std::size(_tbm_support_desc)) SlErrorCorrupt("Invalid TBM support field count");
		// A missing field cannot default to an open class set, a zero speed or a free conversion.
		for (const auto &field : _tbm_support_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing TBM support field");
		}
		uint32_t count = 0, previous = UINT32_MAX;
		// Bounds, ordering and known bit sets are checked before a row enters unpublished transport.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_TBM_SUPPORT_TILES || index != static_cast<int>(count)) SlErrorCorrupt("Invalid TBM support ordinal/count");
			TBMSupportWire wire;
			SlObject(&wire, description);
			const TrackBits track{wire.track};
			const TBMSupportClasses allowed{wire.allowed};
			// Tiles increase strictly and fit the map; the owner, pieces, classes, speed and price are known values.
			if (wire.tile == UINT32_MAX || !CheckTBMWireTile(wire.tile, preview) || (count != 0 && wire.tile <= previous) || wire.project == UINT32_MAX || wire.owner >= MAX_COMPANIES || !track.Any() || !track.IsValid() || !allowed.Any() || !allowed.IsValid() || wire.max_speed == 0 || wire.conversion_price < 0) SlErrorCorrupt("Invalid TBM support fields");
			previous = wire.tile;
			if (!preview) {
				TBMSupportTile support;
				support.tile = TileIndex{wire.tile};
				support.project = TBMProjectID{wire.project};
				support.owner = CompanyID{wire.owner};
				support.track = track;
				support.allowed = allowed;
				support.max_speed = wire.max_speed;
				support.conversion_price = wire.conversion_price;
				_tbm_support_rows.push_back(support);
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.tbm_project_present : _tbm_presence;
		if (presence[2]) SlErrorCorrupt("Duplicate TBM support rows");
		presence[2] = true;
		if (preview) _load_check_data.tbm_support_rows = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const TBMHChunkHandler TBMH;
static const TBMPChunkHandler TBMP;
static const TBMSChunkHandler TBMS;
static const ChunkHandlerRef tbm_project_handlers[] = {TBMH, TBMP, TBMS};
extern const ChunkHandlerTable _tbm_project_chunk_handlers(tbm_project_handlers);
