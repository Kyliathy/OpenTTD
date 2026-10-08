/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file fios.h Declarations for savegames operations. @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md */

#ifndef FIOS_H
#define FIOS_H

#include "company_base.h"
#include "newgrf_config.h"
#include "gamelog.h"
#include "network/core/tcp_content_type.h"
#include "timer/timer_game_calendar.h"
#include "utility_power.h"


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

/**
 * Read-only save preview container; explicit metadata is transport, never live gameplay authority.
 */
struct LoadCheckData {
	bool checkable = false; ///< True if the savegame could be checked by SL_LOAD_CHECK. (Old savegames are not checkable.)
	StringID error{}; ///< Error message from loading. INVALID_STRING_ID if no error.
	std::string error_msg{}; ///< Data to pass to string parameters when displaying #error.

	uint32_t map_size_x = 0;
	uint32_t map_size_y = 0;
	uint8_t map_layers = 1; ///< Layers stored in the file. Old files are one surface plane.
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
	std::array<bool, 4> excavation_present{}; ///< Independent header/job/route/cell preview presence.
	std::array<uint32_t, 4> excavation_header{}; ///< Saved next-ID/job/route/cell cardinalities, never live progress.
	std::array<uint32_t, 3> excavation_rows{}; ///< Actual bounded job/route/cell preview counts.
	bool excavation_pass_present = false; ///< Independent version-374 return/capacity transport presence.
	uint32_t excavation_pass_rows = 0; ///< Bounded copied pass count, never live input.
	std::array<bool, 2> portal_work_present{}; ///< Independent native-work header/table preview presence.
	uint32_t portal_work_expected = 0, portal_work_rows = 0; ///< Exact bounded copied row counts.
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
	std::array<uint64_t, 12> utility_power_header{}; ///< Copied version, epoch, cursors, phase, counts, mutation sequence and directed link allocator/count.
	std::array<uint32_t, 5> utility_power_rows{}; ///< Copied source/consumer/connection/share/directed-link counts.
	UtilityPowerState utility_power_state; ///< Unpublished preview balances; never replaces live energy authority.
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
