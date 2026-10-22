/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube.h Native guideway/capsule capability, explicit paid service and one metered front-tick boundary.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups1-8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md tasks10.1-10.2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47E physical end presentation
 */

#ifndef TRANSPORT_TUBE_H
#define TRANSPORT_TUBE_H

#include "rail_type.h"
#include "newgrf_config.h"
#include "engine_type.h"
#include "tile_type.h"
#include "command_type.h"
#include "direction_type.h"
#include "multilayer_content.h"
#include "utility_power.h"
#include "vehicle_type.h"
#include "station_type.h"
#include "depot_type.h"
#include "track_type.h"

/** Actual native consist owner; tube views borrow its vehicles without replacing their pool or packet authority. */
struct Train;

inline constexpr RailTypeLabel RAILTYPE_LABEL_TUBE{"UTUB"}; ///< Exact first-party semantic label; the canonical registry allocates its runtime slot.

/**
 * Allocate the native tube label through the existing railtype registry after exact package admission.
 * @param activated Actual active content configuration; missing/changed native package refuses before any registry write.
 * @return Actual native runtime railtype, or INVALID_RAILTYPE on collision, alternate-label claim or full registry.
 * Repeated initialization preserves the existing native identity. This foundation does not activate construction or engines.
 */
RailType InitialiseTubeRailType(const GRFConfigList &activated);

/** @return Exact canonical native tube railtype, or INVALID_RAILTYPE when absent; no runtime slot is cached or allocated by querying. */
RailType GetTubeRailType();

/**
 * Check actual canonical tube capability independently of compatibility/powered masks.
 * @param type Runtime railtype to inspect, including invalid/unallocated values.
 * @return Whether its exact UTUB registry row was admitted by the native producer; a label or alternate alone is insufficient.
 */
bool IsTubeRailType(RailType type);

/**
 * Describe physically unconnected mouths of one actual plain tube guideway tile.
 * @param tile Explicit world tile; invalid, ordinary and facility tiles return no end decorations.
 * @return Native NE/SE/SW/NW end flags derived from installed track endpoints and adjacent tube entry geometry.
 * Depot doors use their actual entry side. Signals, reservations, ownership, power and route/turn permissions
 * do not close a physical mouth here. This read-only presentation query grants no movement or construction.
 */
DiagDirectionIndexArray<bool> GetTubeGuidewayEnds(TileIndex tile);

/** Derived prototype capability; catalogue/EIDS/rail owners remain authoritative and this view is never saved. */
struct TubeVehicleCapability {
	EngineID engine = EngineID::Invalid(); ///< Actual persisted EIDS definition, never a logical model cast.
	const LayerVehicleCapability *catalogue = nullptr; ///< Borrowed immutable profile model, including dates/layers/exact refits.
	RailType guideway = INVALID_RAILTYPE; ///< Current canonical UTUB slot; invalid after registry retirement.
};

/**
 * Install the single Mole-70 prototype at its existing canonical EIDS identity.
 * @param activated Exact admitted content; the native tube registry must already exist.
 * @return Whether definition preflight and reconstruction succeed without collisions or missing cargo.
 * Loader/purchase-menu activation and electrical movement remain with later producer groups.
 */
bool InitialiseTubeEngineDefinitions(const GRFConfigList &activated);

/** @param model_id Prototype logical model9. @return Actual native EIDS engine, or Invalid for absent/foreign/unimplemented models. */
EngineID GetTubeEngine(uint16_t model_id);

/** @param engine Actual engine identity. @return Derived native capsule view, or no value for ordinary/foreign engines. */
std::optional<TubeVehicleCapability> GetTubeVehicleCapability(EngineID engine);

/**
 * Admit a whole actual train to its exclusive tube or conventional movement domain.
 * @param train Any physical part; the existing linked consist supplies every engine/cargo owner.
 * @param tile Actual destination world tile, independent of the active camera and compatibility masks.
 * @return Whether all parts share the destination domain and capsule layer/date rules; this never supplies energy.
 */
bool CanTubeTrainEnterTile(const Train &train, TileIndex tile);

/**
 * Validate a complete tube foundation/utility footprint through the existing geological owner.
 * @param footprint Explicit world floor and approach cells; all must occupy one permitted rock layer.
 * @return Pure construction permission or the existing profile/calendar/geological refusal; no tiles are excavated.
 */
CommandCost CheckTubeConstruction(std::span<const TileIndex> footprint);

/**
 * Validate tube station/depot geometry through the canonical rectangular construction helper.
 * @param tile Explicit world origin.
 * @param width Positive floor width, bounded by the ordinary owning constructor.
 * @param height Positive floor height, bounded by the ordinary owning constructor.
 * @param entrance Actual outward axis/door direction, or Invalid for no approach.
 * @param through Whether both opposing approaches are required.
 * @return Complete footprint permission; geometry/addressing stays with the existing owner.
 */
CommandCost CheckTubeConstructionArea(TileIndex tile, uint width, uint height,
	DiagDirection entrance = DiagDirection::Invalid, bool through = false);

/**
 * Validate actual train purchase/refit/clone location and cargo against the exclusive native domain.
 * @param engine Existing engine/EIDS definition.
 * @param tile Actual depot/refit world tile.
 * @param cargo Proposed exact cargo, or INVALID_CARGO for a location-only query.
 * @return Permission or unchanged refusal before any vehicle allocation/refit/charge.
 */
CommandCost CheckTubeVehicleAdmission(EngineID engine, TileIndex tile, CargoType cargo = INVALID_CARGO);

/**
 * Preflight every rail tile in a conversion before the ordinary command performs its first write.
 * @param tiles Complete selected world area, including nonrail cells used only for selection.
 * @param target Requested canonical railtype.
 * @param flags Estimate/execute policy passed unchanged to existing support guards.
 * @return Permission or complete refusal for domain, ground, paired portals, occupancy or reservations.
 */
CommandCost CheckTubeRailConversion(std::span<const TileIndex> tiles, RailType target, DoCommandFlags flags);

static constexpr size_t MAX_TUBE_POWER_CONNECTIONS = 256; ///< Bounded physical services, independently of ledger-issued source identities.
static constexpr size_t MAX_TUBE_POWER_PATH = 128; ///< One explicit excavated utility corridor between a portal and interchange.
static constexpr size_t MAX_TUBE_POWER_PATH_CELLS = 16384; ///< Whole-family save and topology work bound.

/** Specific paid service refusal; estimates and failed transactions change no energy, funds or physical owner. */
enum class TubePowerError : uint8_t { None, InvalidOwner, Unavailable, InvalidTopology, Limit, Funds, Ledger, Refused };

/** Saved physical service and invoice, keyed by the shared ledger's source identity; this record owns no electrical balance. */
struct TubePowerConnection {
	UtilityProducerID producer{UINT32_MAX}; ///< Ledger-issued external source; never a second generation ID allocator.
	CompanyID owner = CompanyID::Invalid(); ///< Company responsible for construction, lease and usage.
	TileIndex portal = INVALID_TILE; ///< Explicit Connected rock-layer endpoint; no coordinate-based cross-layer power.
	TileIndex interchange = INVALID_TILE; ///< Owned tube depot/station at the utility corridor's other end.
	uint64_t portal_revision = 0; ///< Original commissioned revision; rebuilding cannot renew an old connection.
	std::vector<TileIndex> utility_path; ///< Explicit adjacent excavated cells, beginning at the portal door and ending at the interchange.
	int32_t lease_day = INT32_MIN; ///< Last economy day actually invoiced; availability never depends on render time.
	uint16_t usage_remainder_j = 0; ///< Unpaid tariff numerator, always less than the canonical joules-per-Money denominator.
	uint64_t invoiced_money = 0; ///< Paid lease/usage history; energy history stays exclusively in utility_power.h.
	bool active = true; ///< Retired source retains its invoice and cannot silently bind another building.
};

/** Whole connection creation result; pure estimates predict only the ledger-issued source identity. */
struct TubePowerResult {
	TubePowerError error = TubePowerError::Refused; ///< Exact unchanged refusal or complete success.
	UtilityProducerID producer{UINT32_MAX}; ///< Allocated/predicted source only on complete admission.
};

/**
 * Connect a real commissioned portal to an owned excavated tube interchange and pay the first service-day lease.
 * @param portal Actual rock-layer Connected rail portal endpoint with accessible reciprocal approaches.
 * @param interchange Actual same-owner same-layer tube depot or terminal.
 * @param utility_path Explicit contiguous excavated utility corridor from the portal door to the interchange.
 * @param execute False estimates topology, ledger capacity and funds without publishing or paying.
 * @return Complete source identity or unchanged refusal; no geothermal source or Deep unlock is needed in Underground.
 */
TubePowerResult ConnectTubeExternalPower(TileIndex portal, TileIndex interchange, std::span<const TileIndex> utility_path, bool execute = true);

/**
 * Offer bounded external electrical service once in the ledger's current interval, paying a newly elapsed service day's lease.
 * @param producer Exact active physical connection's ledger source.
 * @param interval Current synchronized utility epoch.
 * @param economy_ticks Actual service opportunity, integrated by the common ledger in canonical 27-ms ticks.
 * @param execute False validates the publication and invoice without changing either owner.
 * @return Complete publication or unchanged refusal; unconsumed supply incurs no usage tariff.
 */
TubePowerError PublishTubeExternalPower(UtilityProducerID producer, uint64_t interval, uint32_t economy_ticks, bool execute = true);

/**
 * Settle actual granted use and its fractional tariff together with the complete movement/service participant.
 * @param producer Exact connected paid external source; the consumer must be explicitly and exclusively linked to it.
 * @param consumer Actual owned same-layer TubeWork reservation.
 * @param quantity_j Positive whole energy amount the actual participant will use.
 * @param participant Complete physical work owner; refusal preserves its state, funds and all source slices.
 * @return Common ledger receipt; successful replay is prevented by its saved operation sequence.
 */
UtilityMutationReceipt ConsumeTubeExternalPower(UtilityProducerID producer, UtilityConsumerID consumer,
	uint64_t quantity_j, UtilityMutationParticipant &participant);

/** @param producer Exact physical source to close. @param execute False estimates final fractional invoice/source retirement. @return Complete close or unchanged refusal. */
TubePowerError CloseTubeExternalPower(UtilityProducerID producer, bool execute = true);

/** @param producer Ledger-issued physical source. @return Borrowed connection or nullptr; queries never grant electricity. */
const TubePowerConnection *GetTubePowerConnection(UtilityProducerID producer);
/** @return Bounded field-wise copied physical service transport; electrical balances remain in the common ledger. */
std::vector<TubePowerConnection> CaptureTubePowerConnections();

/** Derived actual facility view; native stations/depots own IDs, packets, platform geometry and lifecycle. This view is never saved. */
struct TubeTerminal {
	TileIndex tile = INVALID_TILE; ///< Actual queried platform/depot cell, never a Surface coordinate projected into rock.
	CompanyID owner = CompanyID::Invalid(); ///< Current actual tile/facility company; callers must re-query after lifecycle changes.
	MapLayer layer = MapLayer::Surface; ///< Actual single rock plane, independent of the camera.
	RailType guideway = INVALID_RAILTYPE; ///< Admitted canonical UTUB identity, never a broad compatibility mask.
	StationID station = StationID::Invalid(); ///< Actual canonical packet owner for a loading platform; invalid for a service depot.
	DepotID depot = DepotID::Invalid(); ///< Actual canonical service facility; invalid for a loading platform.
	uint platform_length_tiles = 0; ///< Native contiguous loading length; zero for depots whose capacity is owned by native depot handling.
};

/** One physically reachable paid inlet; its saved source and invoices remain in the electrical/service owners. */
struct TubeInletReadiness {
	UtilityProducerID source{UINT32_MAX}; ///< Actual active explicit source, ready for a separate binding request rather than an implicit grant.
	bool lease_payable = false; ///< Existing lease is current or the actual company can pay this service day's required lease.
	bool usage_payable = false; ///< Actual company can cover the minimum rounded usage unit; full demand/invoice admission still revalidates funds.
};

/** Rebuildable physical readiness only; an empty inlet list has no power path, and every actual capsule still needs its own binding/grant. */
struct TubeNetworkReadiness {
	TubeTerminal terminal; ///< Actual facility view, retaining the sole native packet/service owner.
	std::vector<TubeInletReadiness> inlets; ///< Reachable valid paid sources in stable ledger identity order; no copied grant or joules.
};

/**
 * Inspect an actual owned tube loading platform or service depot without registering any facility or utility account.
 * @param tile Actual world cell of the native platform/depot.
 * @param company Explicit requesting owner; invalid/foreign owners receive no view.
 * @return Copied native identity/geometry, or absence for invalid, ordinary, locked, solid or retired references.
 */
std::optional<TubeTerminal> GetTubeTerminal(TileIndex tile, CompanyID company);

/**
 * Check model/cargo admission at an actual same-owner native tube facility.
 * @param tile Actual eligible tube platform/depot, resolved anew rather than trusting a cached TubeTerminal.
 * @param company Explicit facility/capsule company.
 * @param engine Actual admitted native capsule EIDS identity, never a logical model cast.
 * @param cargo Exact requested cargo or INVALID_CARGO for location/model-only admission.
 * @return Pure layer/date/domain/refit permission; this grants neither electrical work nor cargo transfer.
 */
bool CanTubeTerminalServeVehicle(TileIndex tile, CompanyID company, EngineID engine, CargoType cargo = INVALID_CARGO);

/**
 * Inspect physical reachable paid inlets and current minimum financial readiness for one actual facility.
 * @param tile Actual world platform/depot, never same-XY catchment or cross-layer reach.
 * @param company Explicit requesting owner.
 * @return Facility/inlet view, or absence for an invalid facility; no query publishes generation, registers a binding or issues an invoice.
 */
std::optional<TubeNetworkReadiness> GetTubeNetworkReadiness(TileIndex tile, CompanyID company);
/** Clear physical service and unpublished file transport on world initialization. */
void ResetTubePowerState();
/** @return Exact field-wise saved physical/invoice digest, independent of pointer layout and cameras. */
uint64_t ComputeTubePowerDigest();
/**
 * Validate physical service transport without repairing invoices, references or generation.
 * @param rows Copied physical transport.
 * @param profile Saved selection, independent of the new-world profile selector.
 * @param world_refs Resolve loaded companies, portals, geology and live ledger when true.
 * @param utility Optional independently parsed electrical transport for preview cross-family invoice/reference validation.
 * @return Structural or actual reference refusal without changing any owner.
 */
TubePowerError ValidateTubePowerConnections(std::span<const TubePowerConnection> rows, const MultiLayerProfileSelection &profile,
	bool world_refs = true, const UtilityPowerState *utility = nullptr);
/** @param rows Parsed bounded transport, unpublished until actual portal and utility acceptance. */
void StageLoadedTubePowerConnections(std::vector<TubePowerConnection> rows);
/** @return Whether staged physical services publish after actual referenced owners validate; old absence remains empty. */
bool AcceptLoadedTubePowerConnections();
/** Reset unpublished concrete tube-power transport at world initialization. */
void ResetTubePowerLoadStaging();
/** Validate independent copied tube-power preview without changing live utility accounts or physical connections. */
void ValidateTubePowerLoadPreview();

/** Native work phase; view state and powered compatibility cannot choose or advance it. */
enum class TubePowerPhase : uint8_t { Idle, Motion, DepotService };

/** Visible electrical wait, independent of native VehState::Stopped and physical path reservations. */
enum class TubePowerWait : uint8_t { None, NoConnection, Topology, Grant, Funds, Unavailable };

/** Pure checked demand proposal; only admitted actual work publishes its fractional carry. */
struct TubePowerDemand {
	uint64_t energy_j = 0; ///< Whole native electricity needed for this opportunity.
	uint16_t remainder_mj = 0; ///< Proposed integration carry,0..999; a refusal retains the prior saved value.
	uint64_t rate_w = 0; ///< Model/load/phase derived total for the complete actual consist.
};

/** Saved physical capsule-to-ledger binding; no cargo, grant or electricity balance is duplicated here. */
struct TubePowerVehicle {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Sole ledger-issued reservation identity, retained after retirement.
	UtilityProducerID source{UINT32_MAX}; ///< Explicit installed paid inlet, independent of coordinate/catchment proximity.
	VehicleID vehicle = VehicleID::Invalid(); ///< Actual same-owner capsule front; deletion retires before pool-ID reuse.
	uint16_t remainder_mj = 0; ///< Demand integration fraction after admitted powered ticks only.
	uint64_t prepared_tick = UINT64_MAX; ///< Last global synchronized request tick; initial sentinel requests nothing.
	uint64_t consumed_tick = UINT64_MAX; ///< Last admitted powered tick; two native substeps cannot consume twice.
	TubePowerPhase phase = TubePowerPhase::Idle; ///< Exact globally planned actual work phase, validated again before native commit.
	TubePowerWait wait = TubePowerWait::NoConnection; ///< Separate power wait; never stored as the player's Stopped flag.
	bool active = true; ///< Retained unbound identities cannot supply a reused capsule pool ID.
};

/**
 * Quote whole actual capsule demand with canonical model/load/phase and27-ms integration units.
 * @param train Actual complete native capsule consist; mixed/foreign parts and invalid load refuse.
 * @param phase Actual idle, propulsion or mandatory depot-service opportunity.
 * @param economy_ticks Synchronized duration within the shared utility bound.
 * @param remainder_mj Previously admitted saved0..999 integration numerator.
 * @return Exact pure demand/carry proposal or absence for invalid participants, phase or overflow.
 */
std::optional<TubePowerDemand> ComputeTubePowerDemand(const Train &train, TubePowerPhase phase,
	uint32_t economy_ticks = 1, uint16_t remainder_mj = 0);

/** @param train Actual front capsule to inspect. @return Native current opportunity; player stops never become an implicit movement request. */
TubePowerPhase GetTubePowerPhase(const Train &train);

/**
 * Bind a stopped actual capsule at the same owned tube interchange as its installed source.
 * @param vehicle Actual front engine; every part must be an eligible native capsule.
 * @param source Exact physically installed same-owner source at this real depot.
 * @param execute False validates/predicts the ledger consumer without allocation or binding.
 * @return Ledger-issued/predicted consumer, or absence for missing/foreign/duplicate identity or topology.
 */
std::optional<UtilityConsumerID> BindTubePowerVehicle(VehicleID vehicle, UtilityProducerID source, bool execute = true);

/**
 * Recover an actual held capsule through a newly installed connected service without renewing its old grant.
 * @param vehicle Existing owned native front, fully held at zero speed on its actual guideway.
 * @param source New explicitly paid same-company inlet whose physical circuit reaches every actual part.
 * @param execute False estimates the complete fresh consumer/binding without releasing or acquiring any identity.
 * @return Fresh ledger consumer, or unchanged refusal; prior source use/invoice stays historical and demand carry moves once.
 */
std::optional<UtilityConsumerID> ReconnectTubePowerVehicle(VehicleID vehicle, UtilityProducerID source, bool execute = true);

/** @param vehicle Actual front capsule. @return Borrowed immutable retained active binding, or nullptr; no grant is created by viewing it. */
const TubePowerVehicle *GetTubePowerVehicle(VehicleID vehicle);
/** @return Copied bounded saved capsule binding transport, excluding cargo and shared electrical quantities. */
std::vector<TubePowerVehicle> CaptureTubePowerVehicles();
/** @param rows Parsed physical capsule bindings. @param prepared_tick Saved global request tick; initial sentinel means no native allocation has run. */
void StageLoadedTubePowerVehicles(std::vector<TubePowerVehicle> rows, uint64_t prepared_tick);
/** @return Saved last complete global tube request tick; separate from the ledger's epoch identity. */
uint64_t GetTubePowerPreparedTick();
/**
 * Validate actual or independently copied capsule associations without repairing offers or carry.
 * @param rows Physical binding transport.
 * @param world_refs Resolve actual vehicles, source and ledger consumers when true.
 * @param utility Optional copied electrical family for independent preview reference validation.
 * @param connections Copied physical inlets paired with the preview electrical family; empty when actual owners resolve references.
 * @return Complete identity/fraction/referral validation without changing grants or work.
 */
TubePowerError ValidateTubePowerVehicles(std::span<const TubePowerVehicle> rows, bool world_refs = true,
	const UtilityPowerState *utility = nullptr, std::span<const TubePowerConnection> connections = {});
/** @return Whether staged bindings resolve after actual physical connections, utility and vehicles load; old absence remains empty. */
bool AcceptLoadedTubePowerVehicles();
/** Clear physical capsule bindings and transient execution authorization at world initialization. */
void ResetTubePowerVehicles();
/** @return Exact field-wise physical binding digest; quantities retain their ledger and vehicle owners. */
uint64_t ComputeTubePowerVehicleDigest();

/** Prepare all companies' collector offers and capsule opportunities once before the native vehicle loop, including collector-only worlds; the common ledger owns both domains' allocation/rotation and saved common tick. */
void PrepareTubePowerTick();
/** @param train Actual capsule front. @param participant Complete native movement/service callback. @return Whether powered work was admitted and settled once; unbound/unpowered work waits. */
bool AdvanceTubePowerTick(Train &train, UtilityMutationParticipant &participant);
/** @param train Actual capsule front. @return Whether its real native movement/service currently executes within the one paid whole-tick participant. */
bool IsTubePowerExecutionAuthorized(const Train &train);
/** @param vehicle Actual vehicle chain being deleted/replaced; retire bindings before its physical pool identity can be reused. */
void RetireTubePowerVehicle(VehicleID vehicle);
/** Reset unpublished387 physical capsule transport at world initialization. */
void ResetTubePowerVehicleLoadStaging();
/** Validate copied capsule family completeness without granting live movement. */
void ValidateTubePowerVehicleLoadPreview();
/** @return Whether an actual complete paid external transaction currently commits; ordinary depot/cache callbacks cannot authorize service. */
bool IsTubeExternalPowerCommitting();

/** @param source Exact installed paid inlet. @param train Actual whole native capsule. @return Whether its occupied tracks belong to the source's derived owned guideway component; queries grant no energy. */
bool IsTubePowerConnected(UtilityProducerID source, const Train &train);
/** @param source Exact active paid inlet. @param tile Actual owned same-layer guideway cell. @param required Actual occupied segments, or empty for a facility/any-segment query. @return Whether all requested segments are in its rebuildable component; queries grant no energy. */
bool IsTubePowerConnectedTile(UtilityProducerID source, TileIndex tile, TrackBits required = {});
/** @param tile Actual changed world cell, or INVALID_TILE for a full native capability/layout reset. Rebuild only affected derived components and expire their unused offers. */
void InvalidateTubePowerTopology(TileIndex tile);
/** Reset rebuildable physical component caches on world initialization/load; saved grants and invoices remain with their owners. */
void ResetTubePowerTopology();
/** @param portal Either physical portal endpoint being sealed/demolished. Retire attached service identities before their commissioned revision disappears. */
void InvalidateTubePowerForPortal(TileIndex portal);
/** @param tile Actual changed world tile. Retire an inlet whose depot/station/wire footprint was removed or changed to another transport domain. */
void InvalidateTubePowerForTile(TileIndex tile);
/** @param old_owner Company undergoing takeover/removal. @param new_owner Actual receiving company or INVALID_OWNER. Retire physical contracts before pool/tile identities change, retaining paid/consumed history. */
void ChangeTubePowerCompanyOwnership(Owner old_owner, Owner new_owner);

#endif /* TRANSPORT_TUBE_H */
