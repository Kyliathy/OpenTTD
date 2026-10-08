/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector.h Actual native collector construction and source readiness; Industry owns every delivered cargo unit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 */

#ifndef GEOTHERMAL_COLLECTOR_H
#define GEOTHERMAL_COLLECTOR_H

#include "geothermal.h"

/** Native inventory owner; the collector side record contains no private material or energy store. */
struct Industry;

/** Saved actual construction participant, referencing retained original planning geometry rather than issuing a thermal/electrical identity. */
struct GeothermalCollectorState {
	IndustryID industry = IndustryID::Invalid(); ///< Actual native machinery and accepted-stock owner.
	GeothermalSiteID site{UINT32_MAX}; ///< Retained, consumed cold blueprint with original vent/floor/endpoint.
	CompanyID owner = CompanyID::Invalid(); ///< Current actual construction/source controller, invalid on abandonment.
	uint16_t work_ticks = 0; ///< Paid synchronized construction work, bounded by the frozen296 ticks.
	bool materials_committed = false; ///< Receipt for one complete frozen bill consumed from native accepted stock; never a second stock balance.
	bool has_work_tick = false; ///< Explicit once-per-tick metering presence, including tick zero.
	uint64_t last_work_tick = 0; ///< Last actual construction tick; retries in that tick cannot earn additional work.
	/** @param other Candidate transport. @return Exact equality of every authoritative construction/control field. */
	bool operator==(const GeothermalCollectorState &other) const = default;
};

/** Complete cold construction quote; a successful estimate spends no capital, retires no claim and creates no native industry. */
struct GeothermalCollectorAdmission {
	GeothermalSiteID site{UINT32_MAX}; ///< Exact cold claim selected by the command.
	GeothermalSiteAdmission foundation; ///< Full revalidated empty excavated machinery floor, natural source and frozen terms.
	IndustryType type = IT_INVALID; ///< Exact canonical native collector definition.
};

/** @param site Actual active company-owned collector blueprint. @return Whole capital/floor/native-pool admission, or unchanged refusal; prepaid capital covers construction labour and requires no electricity. */
GeothermalCollectorAdmission PrepareGeothermalCollectorConstruction(GeothermalSiteID site);
/** @param industry Actual newly allocated zero-footprint native industry. @param site Exact still-active blueprint. @return Whether a fully revalidated binding atomically consumes that reservation before non-failing native tile creation. */
bool BindGeothermalCollectorIndustry(Industry &industry, GeothermalSiteID site);
/** @param industry Actual native industry identity. @return Borrowed construction/control state or nullptr; re-query after mutation/reset. */
const GeothermalCollectorState *GetGeothermalCollectorState(IndustryID industry);
/** @param vent Original natural source identity. @return Actual exclusive collector industry, including abandoned/unfinished machinery, or invalid; cold blueprints do not count. */
IndustryID FindGeothermalCollectorForVent(GeothermalVentID vent);
/** @param industry Actual construction facility. @param cargo Runtime construction-input meaning. @return Exact remaining bill/history room in native accepted stock, or zero when unavailable/complete/unbound. */
uint32_t GetGeothermalCollectorInputRoom(const Industry &industry, CargoType cargo);
/** @param industry Actual receiving native facility. @param cargo Runtime input meaning. @param offered Whole delivered packet units. @return Exactly accepted native stock/history units; excess stays with its carrier. */
uint32_t ReceiveGeothermalCollectorCargo(Industry &industry, CargoType cargo, uint32_t offered);
/** @param industry Actual constructed/constructing native facility. Credit at most one paid labour tick after all materials/actual references/date are available; shortage retains stock/work and creates no heat. */
void AdvanceGeothermalCollector(Industry &industry);
/** @param industry Actual native identity. @return Whether real completed machinery, the whole consumed material bill and current company/source/date agree; no heat offer is created by this query. */
bool IsGeothermalCollectorOperational(IndustryID industry);
/** @param industry Actual native facility. @return Whether demolition would destroy any delivered input or malformed output; consumed construction materials remain a historical receipt. */
bool GeothermalCollectorHasStoredCargo(const Industry &industry);
/** @param industry Deleted native identity. Retire only its construction/source binding; geology and the retained cold blueprint are preserved. */
void ReleaseGeothermalCollectorIndustry(IndustryID industry);
/** @param old_owner Removed/taken-over company. @param new_owner Actual successor or invalid on abandonment; native cargo/work/receipt stay with the same industry. */
void ChangeGeothermalCollectorOwnership(Owner old_owner, Owner new_owner);
/** Clear collector state during whole-world initialization, independently of geology, native stock and cold planning transport. */
void ResetGeothermalCollectors();

/** @return Copied actual construction/control transport in increasing native industry order; native stock/source capacity retain their owners. */
std::vector<GeothermalCollectorState> CaptureGeothermalCollectors();
/** @param rows Bounded parsed actual construction transport. @param world_refs Check actual native industry/cold blueprint/source/company/tiles/stock when true. @return Whole unchanged admission, refusing fabricated completion, duplicate source and missing native bindings. */
bool ValidateGeothermalCollectors(std::span<const GeothermalCollectorState> rows, bool world_refs = true);
/** @param rows Unpublished parsed construction transport; ordinary native map/industry/geology must resolve before gameplay publication. */
void StageLoadedGeothermalCollectors(std::vector<GeothermalCollectorState> rows);
/** @return Whether a complete construction family agrees with actual saved native owners before publication; genuine older absence grants no finished collector. */
bool AcceptLoadedGeothermalCollectors();
/** @return Exact field-wise actual construction/control digest, reading physical stock from Industry without creating a second inventory or energy balance. */
uint64_t ComputeGeothermalCollectorDigest();
/** Clear unpublished concrete389 construction header/table transport during world initialization. */
void ResetGeothermalCollectorLoadStaging();
/** Validate independent copied construction/cold-site/native-industry preview references without mutating the running world. */
void ValidateGeothermalCollectorLoadPreview();

#endif /* GEOTHERMAL_COLLECTOR_H */
