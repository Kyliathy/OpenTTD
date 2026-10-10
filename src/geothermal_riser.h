/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser.h Physical admission, paid native machinery and original-portal lifecycle for electrical risers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups5B-5
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#ifndef GEOTHERMAL_RISER_H
#define GEOTHERMAL_RISER_H

#include "geothermal.h"
#include "multilayer_utility.h"

/** Native machinery and sole delivered-stock/history owner, defined by the shared industry module. */
struct Industry;

static constexpr uint16_t GEOTHERMAL_RISER_LOCAL_ID = 0x8310; ///< Native IIDS/TIDS family, separate from collector tiles8300..8308; no private runtime ID pool.

/** @param activated Exact admitted native package. @return Whether the complete native riser definition is installed without publishing machinery or electrical flow. */
bool InitialiseGeothermalRiserDefinitions(const GRFConfigList &activated);
/** @return Existing exact native riser IndustryType, or IT_INVALID when absent/malformed; no ID is allocated by this query. */
IndustryType GetGeothermalRiserIndustryType();
/** @param type Native IndustryType. @return Whether the exact package/layout/input-only machinery definition belongs to this riser family. */
bool IsGeothermalRiserDefinition(IndustryType type);
/** @param type Native IndustryType. @return Borrowed rock-layer/1980 capability for the exact riser family, or nullptr. */
const LayerIndustryCapability *GetGeothermalRiserIndustryCapability(IndustryType type);

/** Explicit rock-side machinery floor and its physical connection to one real commissioned portal. */
struct GeothermalRiserRequest {
	CompanyID owner = CompanyID::Invalid(); ///< Company funding both-endpoint access and the complete machinery.
	TileIndex portal = INVALID_TILE; ///< Actual rock-side sending endpoint; its reciprocal housing supplies the receiving layer.
	TileIndex tile = INVALID_TILE; ///< Top-left cell of the complete frozen2x2 machinery footprint.
	TileIndex port = INVALID_TILE; ///< Boundary machinery cell cardinally adjoining the sending portal housing.
	DiagDirection access = DiagDirection::Invalid; ///< Outward construction/loading approach, separately excavated.
	/** @param other Candidate original construction geometry. @return Exact comparison of the native owner and physical site. */
	bool operator==(const GeothermalRiserRequest &other) const = default;
};

/** Pure construction admission; a successful quote provides no machinery, materials, electricity or link identity. */
struct GeothermalRiserAdmission {
	GeothermalError error = GeothermalError::Refused; ///< First complete unchanged refusal, or None.
	TileIndex failure_tile = INVALID_TILE; ///< Actual rejected housing/floor where known.
	TileIndex destination = INVALID_TILE; ///< Native reciprocal endpoint, never a computed plane offset.
	uint64_t portal_revision = 0; ///< Exact Connected incarnation to revalidate before actual funding or export.
	const MultiLayerGeothermalConstruction *construction = nullptr; ///< Borrowed frozen capital/material/work terms.
	std::vector<TileIndex> footprint; ///< Complete excavated machinery floor, without modifying the portal's through-route.
};

/**
 * Quote complete riser geometry against both actual housings, calendar, excavation and native occupancy.
 * @param request Explicit company, sending portal, machinery floor, connection port and construction access.
 * @return Pure whole-site permission/refusal; later paid construction must revalidate it and supply real delivered materials.
 */
GeothermalRiserAdmission PrepareGeothermalRiser(const GeothermalRiserRequest &request);

/** Actual paid machinery/control association; Industry exclusively owns delivered stock and accepted history. */
struct GeothermalRiserState {
	IndustryID industry = IndustryID::Invalid(); ///< Actual native machinery identity; never a vent or private facility allocator.
	GeothermalRiserRequest site; ///< Original floor, attachment and current funding/control company.
	TileIndex destination = INVALID_TILE; ///< Original reciprocal receiving housing, retained even after physical loss.
	uint64_t portal_revision = 0; ///< Accepted original physical pair stamp, advanced only by its explicit connection/merger events.
	bool portal_retired = false; ///< Permanent original-housing loss/abandonment; coordinate/revision reuse cannot revive this paid attachment.
	uint16_t work_ticks = 0; ///< Prepaid synchronized labour, bounded by the frozen296 ticks.
	bool materials_committed = false; ///< Receipt for the complete native stock debit, without another material balance.
	bool has_work_tick = false; ///< Explicit once-only tick presence, including simulation tick zero.
	uint64_t last_work_tick = 0; ///< Last credited native tick; retry and reload cannot earn duplicate labour.
	/** @param other Candidate actual machinery transport. @return Equality of every authoritative construction/control field. */
	bool operator==(const GeothermalRiserState &other) const = default;
};

/** Full paid construction preflight; successful estimates create neither native machinery nor electrical capacity. */
struct GeothermalRiserConstructionAdmission {
	GeothermalRiserAdmission foundation; ///< Revalidated original two-endpoint geometry and complete empty floor.
	IndustryType type = IT_INVALID; ///< Exact native input-only riser definition.
};

/** @param request Actual company, portal and complete machinery/loading site. @return Whole native pool/capital/geometry admission without spending or allocating anything. */
GeothermalRiserConstructionAdmission PrepareGeothermalRiserConstruction(const GeothermalRiserRequest &request);
/** @param industry Actual allocated zero-footprint native industry. @param request Revalidated original site. @return Atomic construction binding before non-failing native floor creation; refusal leaves no machinery. */
bool BindGeothermalRiserIndustry(Industry &industry, const GeothermalRiserRequest &request);
/** @param industry Actual native machinery identity. @return Borrowed paid construction/control state, or nullptr; re-query after mutation. */
const GeothermalRiserState *GetGeothermalRiserState(IndustryID industry);
/** @param portal Actual sending housing. @return Existing unfinished/completed/abandoned machinery, or invalid; one physical sender cannot multiply riser capacity. */
IndustryID FindGeothermalRiserForPortal(TileIndex portal);
/** @param industry Actual native input-stock owner. @param cargo Runtime construction meaning. @return Remaining whole bill/history room, or zero when unavailable or committed. */
uint32_t GetGeothermalRiserInputRoom(const Industry &industry, CargoType cargo);
/** @param industry Actual receiving machinery. @param cargo Runtime construction meaning. @param offered Whole delivered packet units. @return Exact native stock/history receipt; excess stays with the carrier. */
uint32_t ReceiveGeothermalRiserCargo(Industry &industry, CargoType cargo, uint32_t offered);
/** @param industry Actual machinery. Advance at most one prepaid labour tick after the full bill and original portal/date/company agree; no electricity is needed or generated. */
void AdvanceGeothermalRiser(Industry &industry);
/** @param industry Actual native identity. @param require_normal_game Require the running game for public operation; full native load may inspect physical readiness before choosing gameplay/editor mode. @return Whether the full paid material/work boundary and original live portal agree; querying creates no transfer link or energy. */
bool IsGeothermalRiserOperational(IndustryID industry, bool require_normal_game = true);
/**
 * Bind a completed paid riser to actual same-company electrical participants through the sole shared link owner.
 * @param industry Original paid native machinery identity, never a numerical portal substitute.
 * @param source Actual source-side electrical reservation on the machinery's sending plane.
 * @param destination Actual receiving reservation on the reciprocal portal's plane.
 * @param execute False validates/predicts the whole binding without spending, allocating or moving energy.
 * @return Shared nonreused paid electrical edge, or unchanged refusal; registration alone grants no electricity.
 */
std::optional<UtilityLinkID> RegisterGeothermalRiserLink(IndustryID industry, UtilityConsumerID source, UtilityConsumerID destination, bool execute = true);
/** @param industry Actual machinery. @return Whether demolition would destroy delivered native input or malformed output; consumed supplies remain historical receipts. */
bool GeothermalRiserHasStoredCargo(const Industry &industry);
/** @param industry Deleted native identity. Release only its machinery binding; portal, geological floor and settled utility history retain their owners. */
void ReleaseGeothermalRiserIndustry(IndustryID industry);
/**
 * Preserve paid machinery through a committed transition of the same actual physical pair.
 * @param portal Existing endpoint whose native connection/company owner just advanced its record.
 * @param previous_owner Actual pretransition housing company, before native map-owner publication.
 * @param previous_revision Exact preceding stamp; stale/replaced/retired associations remain unavailable.
 */
void RevalidateGeothermalRiserPortal(TileIndex portal, CompanyID previous_owner, uint64_t previous_revision);
/** @param portal Actual endpoint being demolished after complete lifecycle admission. Permanently retire both directional machinery attachments without deleting stock/work or touching settled energy. */
void RetireGeothermalRiserPortal(TileIndex portal);
/** @param old_owner Removed/taken-over company. @param new_owner Native successor or invalid abandonment. Preserve actual stock/work and original portal incarnation. */
void ChangeGeothermalRiserOwnership(Owner old_owner, Owner new_owner);
/** Clear actual riser bindings during whole-world initialization; never transport them into another native industry pool. */
void ResetGeothermalRisers();
/** @return Copied authoritative machinery/control rows in native ID order; native stock/history retain Industry ownership. */
std::vector<GeothermalRiserState> CaptureGeothermalRisers();
/** @param rows Candidate complete construction family. @param world_refs Validate actual native stock/floor/company/portal/clock when true. @return Whole unchanged admission, with no reconstructed materials, work or electrical link. */
bool ValidateGeothermalRisers(std::span<const GeothermalRiserState> rows, bool world_refs = true);
/** @param rows Parsed unpublished construction references, accepted only after native map/industry/profile fixup. */
void StageLoadedGeothermalRisers(std::vector<GeothermalRiserState> rows);
/** @return Whole actual construction acceptance, or unchanged refusal; genuine older absence grants no machinery. */
bool AcceptLoadedGeothermalRisers();
/** @return Explicit field-wise construction/control/native-stock digest, excluding pointers, padding, rendering and all energy balances. */
uint64_t ComputeGeothermalRiserDigest();
/** Clear unpublished native riser header/table transport during whole-world initialization or failed-load cleanup. */
void ResetGeothermalRiserLoadStaging();
/** Validate independent copied riser/portal/industry references without reading or changing the running world. */
void ValidateGeothermalRiserLoadPreview();

#endif /* GEOTHERMAL_RISER_H */
