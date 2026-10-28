/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file fios.h Declarations for savegames operations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; detached same-file descent packet counts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD; copied ordinary stock joins the original Space packet owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C; independent copied processor metadata cannot inherit another file's paid work or presence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; copied hauler refits join this file's physical cargo dimensions and quantities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB; copied packet source purpose and original feeder credit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC; independent order/DATE/MAPS/normal-station reference evidence.
 */

#ifndef FIOS_H
#define FIOS_H

#include "company_base.h"
#include "station_type.h"
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
#include "geothermal_generator_wire.h"
#include "utility_power_conversion_wire.h"
#include "transport_tube_geothermal.h"
#include "geothermal_riser_wire.h"
#include "layer_portal_work_wire.h"
#include "excavation.h"
#include "launch_operations.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_recipe.h"
#include "surface_supply_order_wire.h"
#include "space_transition.h"
#include "asteroid.h"
#include "space_mining_state.h"


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
struct NativeSpaceVehiclePreview {
	VehicleID vehicle = VehicleID::Invalid();
	VehicleType type = VehicleType::Invalid;
	Owner owner = OWNER_NONE;
	SpacePoint position{};
	std::vector<uint32_t> cargo_packet_refs; ///< Same-file serialized native freight references; preview owns no packets and never consults live cargo.
	uint32_t mining_target = UINT32_MAX; ///< Same-file native body reference; never resolved against the open world's field.
	uint32_t extracted_units = 0; ///< Actual hull's lifetime finite extraction receipt.
	uint16_t mining_work_ticks = 0; ///< Native partial extraction, separate from service and typed survey work.
	uint8_t mining_phase = 0; ///< Serialized SpaceMissionPhase; role/version validation precedes its use.
	SpaceMiningContinuationState mining_continuation; ///< Same-file contained intent; copied transport grants no native lease or movement.
	bool cargo_complete = false; ///< Decoder-only proof that this row's actual common cargo fields were read; an omitted variant cannot impersonate an empty carrier.
	CargoType freight_cargo = INVALID_CARGO; ///< Original common cargo_type; interpreted only through this file's CDIM meanings.
	uint16_t freight_capacity = 0; ///< Original common cargo_cap; a reported refit cannot override the physical role/mass bound.
};

/** Detached same-file packet quantity/provenance/credit for independent custody checks; this snapshot allocates no native packet or inventory. */
struct NativeCargoPacketPreview {
	CargoPacketID packet = CargoPacketID::Invalid(); ///< Actual serialized native CAPA pool index, not a count-only replacement packet.
	uint16_t quantity = 0; ///< Original positive whole units; native containers retain the sole gameplay ownership.
	Source source{Source::Invalid, SourceType::Industry}; ///< Candidate file's original provenance/purpose; never inherited from the open world.
	Money feeder_share{0}; ///< Original virtual transfer credit; bought company supplies must retain zero.
	// Positional initializers in the asteroid-origin tests rely on the origin fields following feeder_share (m2o28 Group 4).
	TileIndex source_tile = INVALID_TILE; ///< Original same-file production tile; candidate checks never call the open world's tile lookup.
	StationID first_station = StationID::Invalid(); ///< Original CAPA source station, distinct from natural resource provenance.
	bool legacy_asteroid_origin = false; ///< Copied migration receipt set only after same-file legacy qualification; preserves genuine pre-CDIM fallback without lending it to modern sources.
	uint64_t company_supply_order = 0; ///< Original optional order link, widened exactly; allocation alone supplies no refund authority.
	uint8_t company_supply_shipped = 0; ///< Literal original departure byte, checked before any boolean interpretation.
};

/** Same-file physical container reference with its own saved cargo ordinal and station identity; no packet or live inventory is owned here. */
struct NativeCargoOwnerPreview {
	uint32_t reference = 0; ///< Actual native encoded packet index+1, zero is invalid.
	CargoType cargo = INVALID_CARGO; ///< Original owner slot, resolved only through this candidate's definition evidence.
	StationID station = StationID::Invalid(); ///< Actual goods queue, or Invalid for every carrier/job.
};

/** Same-file configured cargo dimensions; detached metadata cannot allocate cargo or replace a loaded CargoSpec. */
struct NativeCargoDimensionPreview {
	CargoType cargo = INVALID_CARGO; ///< Original configured cargo ordinal used by the saved physical owners.
	CargoLabel label; ///< Actual configured label, retaining arbitrary required-content mappings.
	uint8_t weight = 0; ///< Original CargoSpec unit mass in sixteenths of a tonne, including legitimate weightless non-freight cargo.
	/** @param other Independently captured configured row. @return Exact ordinal, label and unit-mass equality without runtime registration. */
	bool operator==(const NativeCargoDimensionPreview &other) const = default;
};

/** Copied native Station authority for independent launch-target validation; this record allocates no station or cargo inventory. */
struct NativeStationOwnerPreview {
	StationID station = StationID::Invalid(); ///< Actual same-file native Station index; waypoints are excluded.
	Owner owner = OWNER_NONE; ///< Saved native authority, never borrowed from a station in the open world.
	TileIndex tile = INVALID_TILE; ///< Original base.xy for same-file Surface-coordinate/layer joins; no live map substitutes it.
	StationFacilities facilities{}; ///< Original normal-station facility mask, independent of the waypoint discriminator.
};

/** Detached whole-file preview transport; copied identities and authority cannot mutate or supply owners from the current playable world. */
struct LoadCheckData {
	bool surface_supply_orders_present = false; ///< This exact file supplied the required current order family, including a legitimate empty book.
	bool legacy_a_supply_wire = false; ///< Original A440 RIFF transport requires the detached A adapter; canonical tagged previews retain their separate contract.
	SurfaceSupplyOrderTransport surface_supply_orders; ///< Detached complete commitments/history only; preview never activates refunds, orders or stock.
	std::array<bool, 2> legacy_b_supply_present{}; ///< Same-file original SSOH header and SSOR TABLE presence, including an empty paid-history book.
	LegacyBSupplyTransport legacy_b_supply; ///< Original bounded B scalars, awaiting same-file conversion and complete packet custody; never live rights.
	bool legacy_b_supply_packet_fields_present = false; ///< This copied CAPA actually supplied original order/shipped/purpose scalars; book staging alone cannot validate paid packet custody.
	bool native_map_dimensions_present = false; ///< Original MAPS row, never default/previous-file geometry.
	bool native_economy_date_present = false; ///< Original DATE economy_date field, not calendar/live time.
	TimerGameEconomy::Date native_economy_date{0}; ///< Same-file native economy day for exact current allowance/reference joins.
	bool native_cargo_dimensions_present = false; ///< This file supplied its own versioned configured-dimensions family.
	std::vector<NativeCargoDimensionPreview> native_cargo_dimension_rows; ///< Copied definition metadata, never borrowed from active-world CargoSpec.
	bool space_structures_present = false;
	uint64_t next_space_structure_generation = 1;
	SpaceStructureRetirementHistory space_structure_retirement_history; ///< Same-file spent receipts; preview never borrows active-world demolition history.
	std::vector<SpaceStructureRecord> space_structure_rows;
	uint64_t next_space_vehicle_generation = 1;
	std::vector<NativeSpaceVehiclePreview> native_space_vehicle_rows;
	std::vector<NativeStationOwnerPreview> native_station_owner_rows; ///< Same-file STNN normal-station authority for explicit reached service targets; r2mlvs10-asteroids-and-mining-probes.md Group8B.
	bool native_vehicle_cargo_present = false, native_station_cargo_present = false; ///< Complete same-file native family reads, including legitimate empty tables; never inherited from a previous preview.
	std::vector<uint32_t> native_ordinary_vehicle_cargo_refs; ///< Original ordinary carrier references encoded as native ID+1; copied metadata creates no hull, cargo or reservation.
	std::vector<uint32_t> native_station_cargo_refs; ///< Original normal-station stock references encoded as native ID+1, across all cargo and next-hop buckets.
	std::vector<NativeCargoOwnerPreview> native_cargo_owner_rows; ///< Complete ordinary station/vehicle references with original slot/queue evidence, separate from aggregate exclusivity views; also the same-file cargo meanings for copied asteroid-origin checks (m2o28 Group 4 folded the incoming reference-only rows in here).
	bool space_transitions_present = false;
	std::vector<SpaceTransitionRecord> space_transition_rows;
	bool space_pods_present = false;
	uint64_t next_space_pod_generation = 1;
	std::vector<SpacePodRecord> space_pod_rows;
	bool space_manifests_present = false;
	std::vector<SpaceManifestRecord> space_manifest_rows;
	bool space_production_present = false, launch_operations_present = false;
	std::vector<SpaceProductionRecord> space_production_rows;
	std::vector<LaunchOperationsRecord> launch_operations_rows;
	std::vector<SpaceCraftRef> space_order_preview_refs;
	std::vector<SpaceOrderRecord> space_order_preview_rows;
	bool asteroid_knowledge_present = false;
	uint32_t asteroid_body_count = 0;
	uint32_t asteroid_next_body_id = 0; ///< Same-file lifetime issuance, independent of live count; never borrowed from the open world.
	std::vector<AsteroidIdentityRecord> asteroid_body_rows; ///< Sorted same-file IDs/classes, not another resource registry; r2sff1-space-fixes-features-1.md Group14B.
	std::vector<std::pair<OrbitalBodyID, VehicleID>> asteroid_mining_leases; ///< Same-file ASTR leases for bidirectional VEHS joins; no native reservation is created. Group23.
	std::vector<AsteroidKnowledgeRecord> asteroid_knowledge_rows;
	std::vector<LaunchCenterRef> launch_center_preview_refs;
	uint64_t next_launch_center_generation = 1; ///< Same-file issuance including retired centers; continuation cannot borrow the open world's generation bound.
	std::vector<LaunchCenterRecord> launch_center_preview_rows;
	bool space_descents_present = false;
	uint64_t next_space_descent_id = 1;
	std::vector<SpaceDescentRecord> space_descent_rows;
	bool native_cargo_packets_present = false; ///<434 copied CAPA metadata is independent of all packets in the open world.
	std::vector<NativeCargoPacketPreview> native_cargo_packet_rows; ///< Bounded original index order; binary lookup checks actual references without allocating cargo.
	bool space_content_present = false, space_orders_present = false;
	bool space_orders_active = false;
	uint16_t space_content_version = 0;
	uint64_t space_content_signature = 0;
	bool asteroid_present = false, asteroid_active = false; ///< Independent bounded404 preview presence.
	bool launch_centers_present = false, launch_centers_active = false; ///< Independent bounded407 preview presence.
	bool launch_hubs_present = false, launch_hubs_active = false; ///< Independent bounded408 membership preview.
	bool surface_handling_present = false, surface_handling_active = false; ///< Independent bounded409 handling transport preview.
	std::vector<SurfaceHandlingRecord> surface_handling_rows; ///< Original unpublished handling packet owners; the native launch hub remains the sole gameplay inventory.
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
	std::array<bool, 2> utility_conversion_present{}; ///< Independent original B428 header/table presence.
	std::array<uint64_t, 4> utility_conversion_header{}; ///< Exact schema/source/origin/epoch transport.
	std::vector<UtilityHeatConversionWire> utility_conversion_rows; ///< Candidate conversion scalars; never live energy.
	std::array<bool, 2> tube_geothermal_present{}; ///< Independent original B433 source family presence.
	uint32_t tube_geothermal_expected = 0; ///< Bounded source history cardinality.
	TubeGeothermalPowerState tube_geothermal_state; ///< Candidate physical/shared references, without balances.
	std::array<bool, 2> geothermal_generator_present{}; ///< Independent original B427 machinery family presence.
	uint32_t geothermal_generator_expected = 0; ///< Bounded original construction cardinality.
	std::vector<GeothermalGeneratorWire> geothermal_generator_rows; ///< Exact copied construction/service receipts.
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
	bool orbital_recipes_present = false; ///< This exact file supplied its required versioned processing metadata, independent of the active world.
	std::vector<OrbitalRecipeRecord> orbital_recipe_rows; ///< Copied bounded metadata/private receipt fields; never a native paid-work owner.
	std::map<MultiLayerRecipeID, uint64_t> orbital_retired_recipe_batches; ///< Independent SPRC aggregate; keeping it separate prevents chunk-order-dependent SPST overwrites.
	uint64_t orbital_retired_discarded_paid_j = 0; ///< Same-file spent cancelled work, never electricity available to a preview.

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
