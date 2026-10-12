/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file fios.h Declarations for savegames operations. @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md */

#ifndef FIOS_H
#define FIOS_H

#include "company_base.h"
#include "newgrf_config.h"
#include "gamelog.h"
#include "network/core/tcp_content_type.h"
#include "timer/timer_game_calendar.h"
#include "utility_power.h"
#include "utility_power_lineage.h"
#include "geothermal_heat.h"
#include "transport_tube.h"
#include "geothermal_site_wire.h"
#include "geothermal_collector_wire.h"
#include "geothermal_riser_wire.h"
#include "layer_portal_work_wire.h"
#include "excavation.h"


/** Special values for save-load window for the data parameter of #InvalidateWindowData. */
enum SaveLoadInvalidateWindowData : uint8_t {
	SLIWD_RESCAN_FILES,          ///< Rescan all files (when changed directory, ...)
	SLIWD_SELECTION_CHANGES,     ///< File selection has changed (user click, ...)
	SLIWD_FILTER_CHANGES,        ///< The filename filter has changed (via the editbox)
};

/** Outcome of a directory creation attempt. */
enum class DirectoryCreateResult : uint8_t {
	Success,            ///< Directory was created.
	AlreadyExists,      ///< A file or directory with that name already exists.
	PermissionDenied,   ///< The OS rejected the operation for permission reasons.
	OtherError,         ///< Any other filesystem error.
};

using CompanyPropertiesMap = std::map<uint, std::unique_ptr<CompanyProperties>>;

/** Copied native machinery identity/shape for independent dependent-family preview; no stock or Industry pool item is allocated. */
struct NativeIndustryPreview {
	uint16_t id = UINT16_MAX; ///< Actual serialized native pool ordinal.
	uint32_t tile = UINT32_MAX; ///< Native serialized anchor in this file's explicit map geometry.
	uint8_t width = 0, height = 0, type = UINT8_MAX; ///< Actual native floor and runtime definition, matched against the same file's IIDS mapping.
};

/**
 * Read-only save preview container; explicit metadata is transport, never live gameplay authority.
 */
struct LoadCheckData {
	bool asteroid_present = false, asteroid_active = false; ///< Independent bounded404 preview presence.
	bool launch_centers_present = false, launch_centers_active = false; ///< Independent bounded407 preview presence.
	bool launch_hubs_present = false, launch_hubs_active = false; ///< Independent bounded408 membership preview.
	uint64_t asteroid_seed = 0, orbital_seed = 0; ///< Candidate field and metadata roots; never live-world values.
	bool checkable = false; ///< True if the savegame could be checked by SL_LOAD_CHECK. (Old savegames are not checkable.)
	StringID error{}; ///< Error message from loading. INVALID_STRING_ID if no error.
	std::string error_msg{}; ///< Data to pass to string parameters when displaying #error.

	uint32_t map_size_x = 0;
	uint32_t map_size_y = 0;
	uint8_t map_layers = 1; ///< Layers stored in the file. Old files are one surface plane.
	bool native_tick_present = false; ///< Version400 preview copied its own synchronized clock rather than borrowing live work time.
	uint64_t native_tick_counter = 0; ///< Read-only saved native clock for exact construction tick-reference checks.
	uint16_t multilayer_schema_version = 0; ///< Zero denotes legacy chunk absence in this preview only.
	uint8_t gameplay_profile_id = 0; ///< Saved profile identity for exact preview content validation.
	uint16_t gameplay_profile_version = 0; ///< Saved immutable profile revision.
	uint32_t multilayer_feature_flags = 0; ///< Independent preview feature requirements, never live metadata.
	bool geology_header_present = false; ///< New-version feature transport presence.
	bool geology_deposits_present = false; ///< Bounded scalar table presence.
	bool geology_spans_present = false; ///< Compact shape table presence.
	uint16_t geology_generator_version = 0; ///< Zero denotes inactive legacy terrain.
	uint32_t geology_expected_deposits = 0; ///< Header cardinality, validated against rows.
	uint32_t geology_expected_spans = 0; ///< Header shape-row bound.
	uint32_t geology_deposit_rows = 0; ///< Actual independent preview count.
	uint32_t geology_span_rows = 0; ///< Actual independent preview span count.
	bool geology_vents_present = false; ///< Independent natural-source family presence.
	uint32_t geology_vent_rows = 0; ///< Bounded natural-source count; no heat grant is implied.
	std::vector<GeothermalVentField> geology_vent_fields; ///< Copied natural source identities/rectangles for independent dependent-family preview; never live rates or claims.
	std::array<bool, 4> excavation_present{}; ///< Independent header/job/route/cell preview presence.
	std::array<uint32_t, 4> excavation_header{}; ///< Saved next-ID/job/route/cell cardinalities, never live progress.
	std::array<uint32_t, 3> excavation_rows{}; ///< Actual bounded job/route/cell preview counts.
	bool excavation_pass_present = false; ///< Independent version-374 return/capacity transport presence.
	uint32_t excavation_pass_rows = 0; ///< Bounded copied pass count, never live input.
	std::array<bool, 2> portal_work_present{}; ///< Independent native-work header/table preview presence.
	uint32_t portal_work_expected = 0, portal_work_rows = 0; ///< Exact bounded copied row counts.
	std::vector<PortalWorkWire> portal_work_records; ///< Copied original portal company/stamps for dependent preview; no live topology or routing is consulted.
	std::array<bool, 3> tbm_project_present{}; ///< Independent TBM project header, project table and support table preview presence.
	uint32_t tbm_project_expected = 0, tbm_project_rows = 0, tbm_project_next = 0; ///< Exact copied project counts and allocation cursor.
	uint32_t tbm_support_expected = 0, tbm_support_rows = 0; ///< Exact copied temporary support counts.
	std::array<bool, 2> spoil_receiver_present{}; ///< Independent spoil receiver header and row table preview presence.
	uint32_t spoil_receiver_expected = 0, spoil_receiver_rows = 0; ///< Exact copied spoil receiver counts.
	std::array<bool, 2> layer_mine_present{}; ///< Independent mine header/table preview presence. See r2mlvs6-mines-and-resource-chains.md.
	uint32_t layer_mine_expected = 0, layer_mine_rows = 0; ///< Exact copied mine counts; no claims are activated by preview.
	std::array<bool, 3> layer_processor_present{}; ///< Independent native processor header/scalar/counter preview presence. See r2mlvs6-mines-and-resource-chains.md.
	std::array<uint32_t, 2> layer_processor_expected{}, layer_processor_rows{}; ///< Exact declared/read scalar and counter counts; no recipe is activated by preview.
	std::array<bool, 4> tbm_exchange_present{}; ///< Independent exchange header, assignment, set and set-wagon table preview presence.
	std::array<uint32_t, 5> tbm_exchange_header{}; ///< Copied assignment count and cursor, set count and cursor, and set-wagon row count.
	std::array<uint32_t, 3> tbm_exchange_rows{}; ///< Copied assignment, set and set-wagon row counts.
	std::array<bool, 2> utility_power_present{}; ///< Independent header/table preview; Plan 12 Group 7B.
	std::array<uint64_t, 14> utility_power_header{}; ///< Copied utility identity/phase/counts, committed local topology and403 shared slice cursor; no live geometry is borrowed.
	std::array<uint32_t, 5> utility_power_rows{}; ///< Copied source/consumer/connection/share/directed-link counts.
	UtilityPowerState utility_power_state; ///< Unpublished preview balances; never replaces live energy authority.
	std::array<bool, 2> utility_path_present{}; ///< Independent403 shared lineage header/table presence, even for an empty segment family.
	std::array<uint64_t, 3> utility_path_header{}; ///< Copied path revision, exact epoch and bounded reference count.
	std::vector<UtilityPowerPathWire> utility_path_rows; ///< Unpublished ordered segment references; original quantities remain in utility_power_state.
	std::array<bool, 2> tube_power_present{}; ///< Independent physical paid service header/table preview. See r2mlvs17-underground-transport-tubes.md Group5.
	std::array<uint32_t, 2> tube_power_expected{}; ///< Copied exact connection and path-cell counts.
	std::vector<TubePowerConnection> tube_power_rows; ///< Unpublished preview invoices; no energy or movement authority.
	std::array<bool, 2> tube_vehicle_present{}; ///< Independent387 capsule binding header/table preview.
	uint32_t tube_vehicle_expected = 0; ///< Copied exact retained binding count.
	uint64_t tube_vehicle_tick = UINT64_MAX; ///< Last complete global request tick; preview never renews allocation.
	std::vector<TubePowerVehicle> tube_vehicle_rows; ///< Copied physical associations and carry, never live movement permission.
	std::array<bool, 2> geothermal_site_present{}; ///< Independent388 cold-site header/table presence.
	uint32_t geothermal_site_expected = 0; ///< Copied exact retained planning identity count.
	std::vector<GeothermalSiteWire> geothermal_site_rows; ///< Copied local-coordinate blueprints; no conversion through the live map and no heat/power.
	std::array<bool, 2> geothermal_collector_present{}; ///< Independent389 actual construction header/table presence.
	uint32_t geothermal_collector_expected = 0; ///< Copied bounded actual native participant count.
	std::vector<GeothermalCollectorWire> geothermal_collector_rows; ///< Copied work/control/receipt transport; Industry remains the stock/machinery owner.
	std::array<bool, 2> geothermal_riser_present{}; ///< Independent400 native machinery header/table presence.
	uint32_t geothermal_riser_expected = 0; ///< Exact bounded paid machinery count.
	std::vector<GeothermalRiserWire> geothermal_riser_rows; ///< Copied original geometry/work/receipt/loss markers; native materials remain in INDY.
	std::array<bool, 2> geothermal_heat_route_present{}; ///< Independent393 physical header/table presence.
	std::array<uint32_t, 3> geothermal_heat_route_expected{}; ///< Exact copied port/path/cell cardinalities.
	uint64_t geothermal_heat_route_interval = 0; ///< Copied native grant epoch; preview never allocates.
	std::vector<GeothermalHeatRouteWire> geothermal_heat_route_rows; ///< Copied physical references only; no running-world geometry or quantities.
	std::vector<uint16_t> native_industry_preview_ids; ///< Copied current-format native INDY identities for independent actual construction references; no industry is allocated by preview.
	std::vector<NativeIndustryPreview> native_industry_preview_rows; ///< Copied400 native floor/type identity, without material buffers or gameplay publication.
	std::vector<uint8_t> native_riser_preview_types; ///< Same-file canonical riser IIDS mappings, never a live or duplicate content catalogue.

	TimerGameCalendar::Date current_date{};

	LandscapeType landscape{}; ///< Landscape type.
	TimerGameCalendar::Year starting_year{}; ///< Starting date.

	CompanyPropertiesMap companies{}; ///< Company information.

	GRFConfigList grfconfig{}; ///< NewGrf configuration from save.
	GRFListCompatibility grf_compatibility = GRFListCompatibility::NotFound; ///< Summary state of NewGrfs, whether missing files or only compatible found.

	Gamelog gamelog{}; ///< Gamelog actions

	/** Initialize independent preview fields without changing current world policy. */
	LoadCheckData() {}

	/**
	 * Check whether loading the game resulted in errors.
	 * @return true if errors were encountered.
	 */
	bool HasErrors()
	{
		// Only an actually checked file can supply a preview error; an untouched preview is not a failed load.
		return this->checkable && this->error != INVALID_STRING_ID;
	}

	/**
	 * Check whether the game uses any NewGrfs.
	 * @return true if NewGrfs are used.
	 */
	bool HasNewGrfs()
	{
		// Offer content actions only for a valid checked preview with a real saved configuration.
		return this->checkable && this->error == INVALID_STRING_ID && !this->grfconfig.empty();
	}

	void Clear();
};

extern LoadCheckData _load_check_data;

/** Deals with finding savegames */
struct FiosItem {
	FiosType type;
	int64_t mtime;
	EncodedString title;
	std::string name;
};

/** List of file information. */
class FileList : public std::vector<FiosItem> {
public:
	void BuildFileList(AbstractFileType abstract_filetype, SaveLoadOperation fop, bool show_dirs);
	const FiosItem *FindItem(std::string_view file);
};
bool FiosItemSorter(const FiosItem &a, const FiosItem &b);
bool FiosItemNameSorter(const FiosItem &a, const FiosItem &b);
bool FiosItemModificationDateSorter(const FiosItem &a, const FiosItem &b);

void ShowSaveLoadDialog(AbstractFileType abstract_filetype, SaveLoadOperation fop);

void FiosGetSavegameList(SaveLoadOperation fop, bool show_dirs, FileList &file_list);
void FiosGetScenarioList(SaveLoadOperation fop, bool show_dirs, FileList &file_list);
void FiosGetHeightmapList(SaveLoadOperation fop, bool show_dirs, FileList &file_list);
void FiosGetTownDataList(SaveLoadOperation fop, bool show_dirs, FileList &file_list);

bool FiosBrowseTo(const FiosItem *item);

std::string FiosGetCurrentPath();
std::optional<uint64_t> FiosGetDiskFreeSpace(const std::string &path);
std::string FiosMakeHeightmapName(std::string_view name);
std::string FiosMakeSavegameName(std::string_view name);
DirectoryCreateResult FiosCreateDirectory(std::string_view name);

std::tuple<FiosType, std::string> FiosGetSavegameListCallback(SaveLoadOperation fop, std::string_view file, std::string_view ext);
std::tuple<FiosType, std::string> FiosGetScenarioListCallback(SaveLoadOperation fop, std::string_view file, std::string_view ext);
std::tuple<FiosType, std::string> FiosGetHeightmapListCallback(SaveLoadOperation fop, std::string_view file, std::string_view ext);

void ScanScenarios();
std::optional<std::string_view> FindScenario(const ContentInfo &ci, bool md5sum);

/**
 * A savegame name automatically numbered.
 */
struct FiosNumberedSaveName {
	FiosNumberedSaveName(const std::string &prefix);
	std::string Filename();
	std::string Extension();
private:
	std::string prefix;
	int number;
};

#endif /* FIOS_H */
