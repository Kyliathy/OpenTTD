/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal.h Public geothermal sites, shared Heat allocation, installed utilities and metered electrical export.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5 electrical export
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group7 public handoff
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19
 */

#ifndef GEOTHERMAL_H
#define GEOTHERMAL_H

#include "excavation.h"
#include "multilayer_bootstrap.h"
#include "direction_type.h"
#include "industry_type.h"
#include "newgrf_config.h"
#include "utility_power_transfer.h"
#include "geothermal_type.h"
#include "geothermal_heat.h"
#include "geothermal_utility.h"

/** Existing canonical industry admission record; geothermal adds explicit capability while native pools retain identity and cargo. */
struct LayerIndustryCapability;

static constexpr uint16_t GEOTHERMAL_COLLECTOR_LOCAL_ID = 0x8300; ///< Reserved native IIDS/TIDS collector family, distinct from mines8100 and Surface processors8200.

/** @param activated Exact currently admitted native package. @return Whether a complete1980 collector definition installs through IIDS/TIDS; no facility, materials, work or heat is created. */
bool InitialiseGeothermalCollectorDefinitions(const GRFConfigList &activated);
/** @return Existing canonical collector IndustryType, or IT_INVALID when absent/foreign/malformed; queries allocate no definitions. */
IndustryType GetGeothermalCollectorIndustryType();
/** @param type Actual canonical IndustryType. @return Whether its exact native identity/layout/input-only definition is owned by this producer, independent of names/artwork. */
bool IsGeothermalCollectorDefinition(IndustryType type);
/** @param type Actual canonical industry definition. @return Borrowed immutable collector Deep/date capability, or nullptr for absent/foreign/malformed definitions. */
const LayerIndustryCapability *GetGeothermalCollectorIndustryCapability(IndustryType type);

/** Explicit player-selected source/site; world addressing and the natural source owner remain authoritative. */
struct GeothermalSiteRequest {
	GeothermalVentID vent{UINT32_MAX}; ///< Explicit selected natural source, never a new consumer-local vent identity.
	CompanyID owner = CompanyID::Invalid(); ///< Actual company preparing/binding the future utility facility.
	MultiLayerGeothermalConstructionKind kind = MultiLayerGeothermalConstructionKind::Collector; ///< Frozen collector or first powerhouse family; portal risers have their later distinct admission.
	TileIndex tile = INVALID_TILE; ///< Actual Deep facility origin, independent of any selected camera.
	TileIndex endpoint = INVALID_TILE; ///< Explicit utility connection cell on this facility's boundary; an endpoint alone creates no link.
	DiagDirection access = DiagDirection::Invalid; ///< Required outward native access side, on excavated floor.
};

/** Pure complete site quote; pointers borrow their canonical owners and cannot authorize later construction after the world changes. */
struct GeothermalSiteAdmission {
	GeothermalError error = GeothermalError::Refused; ///< First unchanged refusal, or complete geometry permission.
	TileIndex failure_tile = INVALID_TILE; ///< Actual refused floor/access/endpoint where known.
	const GeothermalVentField *vent = nullptr; ///< Borrowed selected source, with no copied private heat capacity or grant.
	const MultiLayerGeothermalConstruction *construction = nullptr; ///< Immutable profile-owned material/capital/work terms.
	std::vector<TileIndex> footprint; ///< Complete validated native world facility floor, retained only as a proposal.
};

/**
 * Prepare complete cold site geometry without claiming the vent, completing construction or creating a heat/power offer.
 * @param request Explicit source, actual company, facility family, full-floor origin and boundary utility endpoint/access side.
 * @return Read-only natural/profile-backed proposal or unchanged refusal; a later binding/commissioning must revalidate every participant.
 */
GeothermalSiteAdmission PrepareGeothermalSite(const GeothermalSiteRequest &request);

/** Cold site identity issued only by this owner; it is never a thermal/electrical consumer ID. */
using GeothermalSiteID = StrongType::Typedef<uint32_t, struct GeothermalSiteIDTag, StrongType::Compare>;
static constexpr size_t MAX_GEOTHERMAL_SITE_CLAIMS = 256; ///< Bounded retained planning identities, including cancellation history.

/** Saved cold site claim only; actual delivered materials/work and commissioned heat/generator participants belong to their later construction owners. */
struct GeothermalSiteClaim {
	GeothermalSiteID id{UINT32_MAX}; ///< Owner-issued retained site identity, never recycled after cancellation.
	GeothermalSiteRequest site; ///< Explicit original natural source/company/geometry contract.
	bool active = true; ///< Active blueprint protects its floor; cancellation never erases the natural source or excavated cavity.
};

/** Whole cold claim result; even successful registration creates no finished collector, heat offer or electrical identity. */
struct GeothermalSiteResult {
	GeothermalError error = GeothermalError::Refused; ///< Complete unchanged refusal or one registered/planned identity.
	GeothermalSiteID id{UINT32_MAX}; ///< Actual/predicted cold claim only after complete admission.
};

/** @param request Complete cold site geometry. @param execute False validates/predicts its retained site identity without reservation. @return One cold planning claim or unchanged refusal; no material/work completion or energy is created. */
GeothermalSiteResult ClaimGeothermalSite(const GeothermalSiteRequest &request, bool execute = true);
/** @param id Actual retained cold claim. @param execute False checks cancellation only. @return Complete owner-authorized release or unchanged refusal, preserving natural capacity, terrain and any later independently owned inventory. */
GeothermalError CancelGeothermalSite(GeothermalSiteID id, bool execute = true);
/** @param id Actual retained cold identity. @return Borrowed claim including retired history, or nullptr; re-query after registry mutation/reset. */
const GeothermalSiteClaim *GetGeothermalSiteClaim(GeothermalSiteID id);
/** @param tile Actual world cell. @return Borrowed active cold floor claim or nullptr, with no cross-layer/proximity reservation. */
const GeothermalSiteClaim *GetGeothermalSiteClaimForTile(TileIndex tile);
/** @return Copied retained planning transport, excluding all heat, electrical and cargo balances. */
std::vector<GeothermalSiteClaim> CaptureGeothermalSiteClaims();
/** @param rows Retained planning transport. @param world_refs Revalidate actual geology/company/floor/date when true. @return Complete validation without repairing, claiming or constructing any site. */
GeothermalError ValidateGeothermalSiteClaims(std::span<const GeothermalSiteClaim> rows, bool world_refs = true);
/** @param rows Parsed bounded cold planning transport, unpublished until actual geology/profile acceptance. */
void StageLoadedGeothermalSiteClaims(std::vector<GeothermalSiteClaim> rows);
/** @return Whether staged transport resolves against actual source/floor owners before publication; old absence stays empty. */
bool AcceptLoadedGeothermalSiteClaims();
/** Clear cold claims and pending transport on world initialization; natural source and energy authorities are independent. */
void ResetGeothermalSiteClaims();
/** @return Exact field-wise retained planning digest, independent of pointers, camera, packets and electrical/thermal quantities. */
uint64_t ComputeGeothermalSiteDigest();
/** @param old_owner Company being removed or taken over. @param new_owner Actual receiving owner or INVALID_OWNER. Cancel cold plans before pool/tile identity reuse, retaining historical planning and the original natural source. */
void ChangeGeothermalSiteCompanyOwnership(Owner old_owner, Owner new_owner);
/** Reset unpublished concrete388 cold-site header/table transport on ordinary world initialization. */
void ResetGeothermalSiteLoadStaging();
/** Validate complete independent copied cold-site map/natural-source transport without changing live claims or energy. */
void ValidateGeothermalSiteLoadPreview();

/** Requested export of already allocated electricity; the feature holds references and a quantity bound, never another generation/reservation account. */
struct GeothermalPowerExportRequest {
	UtilityGrantID grant; ///< Exact existing source reservation and shared accounting epoch.
	UtilityConsumerID destination{UINT32_MAX}; ///< Existing same-company final electrical request; intermediate accounts need no invented demand.
	TileIndex source_port = INVALID_TILE; ///< Explicit installed input cell; an imported source must continue from its original arriving housing.
	uint64_t maximum_j = 0; ///< Positive upper bound in joules; actual original publication, final room and physical residuals determine the admitted whole quantity.
	std::vector<UtilityLinkID> excluded; ///< Optional unavailable original cells/paid edges for deterministic physical discovery; this hint creates no capacity or replacement identity.
};

/** Exact shared sealed whole-path proposal; naming the feature's action does not duplicate error, quantity, path, retry sequence or energy ownership. */
using GeothermalPowerExportProposal = UtilityPathTransferProposal;

/**
 * Discover one deterministic installed paid path and prepare its currently admissible whole export without mutation.
 * @param request Existing source grant, final requested destination, installed sending port, positive joule bound and optional unavailable physical references.
 * @return Exact shared sealed proposal for the maximum admitted quantity up to the bound on the selected path, or unchanged refusal. Caller durations are not accepted; the shared owner uses original publications. This preparation does not split an action across separately committed paths.
 */
GeothermalPowerExportProposal PrepareGeothermalPowerExport(const GeothermalPowerExportRequest &request);

/**
 * Revalidate and settle every original crossing and final availability together through the sole shared utility owner.
 * @param proposal Exact successful prepared feature action, retaining original source/epoch and complete physical references.
 * @return One atomic transfer/sequence receipt or unchanged failure. Modified, stale or repeated proposals cannot debit a source or credit a destination; no generation, intermediate grant, money or heat is created by export.
 */
UtilityMutationReceipt ExportGeothermalPower(const GeothermalPowerExportProposal &proposal);

#endif /* GEOTHERMAL_H */
