/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_heat.h Physical thermal request proposals backed by shared native utility identities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups2-4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group15 stable ConsumerID priority
 */

#ifndef GEOTHERMAL_HEAT_H
#define GEOTHERMAL_HEAT_H

#include "utility_power.h"

/** Transient generator or recipe demand; it owns no reservation, energy balance or runtime identity. */
struct GeothermalHeatRequest {
	CompanyID owner = CompanyID::Invalid(); ///< Actual physical consumer's controlling company.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Immutable Heat consumer issued by the common utility owner.
	TileIndex endpoint = INVALID_TILE; ///< Explicit excavated Deep port; connected-path eligibility is validated by the physical link owner.
	UtilityHeatUse use = UtilityHeatUse::DirectRecipe; ///< Generator and direct-processing roles compete in the same priority order.
	uint64_t needed_j = 0; ///< Feasible whole thermal requirement after the consumer checks inputs, outputs and auxiliary power.
};

/**
 * Validate a typed physical demand without registering, granting or consuming energy.
 * @param request Actual owner, shared consumer identity, Deep port, immutable benefit and feasible quantity.
 * @return Shared request or absence for invalid owner/domain/role/date/quantity/port.
 * Physical connectivity remains a separate admission step before allocation;
 * this proposal never turns endpoint proximity into source eligibility.
 * Both roles use the frozen stable ConsumerID priority, without an extra fair cursor.
 */
std::optional<UtilityPowerRequest> PrepareGeothermalHeatRequest(const GeothermalHeatRequest &request);

/** Actual whole consumer prepares feasible inputs/output/auxiliary work before reserving heat; no private utility or material balance lives here. */
class GeothermalHeatParticipant : public UtilityMutationParticipant {
public:
	/** Destroy through the public complete consumer boundary. */
	virtual ~GeothermalHeatParticipant() = default;
	/** @return The immutable quantity domain of every whole geothermal benefit. */
	UtilityEnergyDomain EnergyDomain() const final { return UtilityEnergyDomain::Heat; }
	/** @param request Exact native identity/port/role/quantity. @return Whether the actual full work/input/output/auxiliary transaction is feasible without mutation; a starved/full owner requests zero usable heat. */
	virtual bool ValidateDemand(const GeothermalHeatRequest &request) const = 0;
};

/** Transient complete demand admission; callbacks are actual live consumer owners and are never serialized as saved work. */
struct GeothermalHeatDemand {
	GeothermalHeatRequest request; ///< Exact physical request, with no energy inventory.
	GeothermalHeatParticipant *participant = nullptr; ///< Actual whole work/input/output/auxiliary owner, required before positive demand.
};

/** Saved physical consumer association; all control, role, requested/granted/consumed quantities remain with the shared utility/native facility owners. */
struct GeothermalHeatConsumerBinding {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Actual shared Heat identity; no feature-local allocator exists.
	TileIndex endpoint = INVALID_TILE; ///< Explicit Deep port selected when the physical consumer registers.
};

/** Saved exact path of one current shared source slice; quantities and original use/release remain exclusively in that native slice. */
struct GeothermalHeatRoute {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Actual reservation owner.
	UtilityProducerID producer{UINT32_MAX}; ///< Actual original thermal source.
	std::vector<UtilityLinkID> cells; ///< Installed cardinal path whose rate bounded that source slice, including both ports.
};

/** Explicit393 transport row; only issued references and path positions are serialized, never native energy amounts. */
struct GeothermalHeatRouteWire {
	uint8_t kind = 0; ///< Port0, source path1, ordered installed cell2.
	uint32_t consumer = UINT32_MAX; ///< Existing shared Heat consumer.
	uint32_t producer = UINT32_MAX; ///< Existing shared source for paths/cells; invalid for a port.
	uint32_t position = 0; ///< Zero for ports/path declarations, dense ordered ordinal for cells.
	uint32_t reference = UINT32_MAX; ///< Explicit world tile for a port, issued link for a cell, invalid for a path declaration.
};

/** Transient bounded physical proposal; only commit can publish native grants and their saved route references. */
struct GeothermalHeatAllocation {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Complete physical/native admission or unchanged refusal.
	uint64_t interval = 0; ///< Exact common accounting epoch.
	uint64_t sequence = 0; ///< Shared ledger revision observed during preparation.
	uint64_t topology_revision = 0; ///< Actual installed topology used by every path.
	std::vector<UtilityPowerRequest> requests; ///< Feasible complete demands; zero supply remains distinct from zero demand.
	std::vector<UtilityHeatRoutingShare> shares; ///< Checked transient exact native source proposal, never a second live quantity owner.
	std::vector<GeothermalHeatRoute> routes; ///< Exact proposed current path references, with no copied grant/use amounts.
};

/** @param owner Actual physical consumer company. @param endpoint Explicit owned installed Deep Heat port. @param use Immutable direct/generator benefit. @param capacity_j Bounded per-interval demand. @param execute False predicts without registering. @return Shared Heat consumer identity or unchanged refusal; no facility, stock or grant is fabricated. */
std::optional<UtilityConsumerID> RegisterGeothermalHeatConsumer(CompanyID owner, TileIndex endpoint, UtilityHeatUse use, uint64_t capacity_j, bool execute = true);
/** @param interval Exact open native epoch. @param demands Actual whole feasible consumer participants. @return Pure deterministic path/source-limited proposal; no source/consumer/quantity/time changes. */
GeothermalHeatAllocation PrepareVentHeatAllocation(uint64_t interval, std::span<const GeothermalHeatDemand> demands);
/** @param interval Exact open native epoch. @param demands Actual whole feasible consumer participants. @param execute False estimates complete routing/allocation. @return Complete once-only physical/native reservation or unchanged refusal, in frozen stable consumer/source order. */
UtilityPowerError AllocateVentHeat(uint64_t interval, std::span<const GeothermalHeatDemand> demands, bool execute = true);
/** @param grant Exact current native consumer/epoch. @param quantity_j Positive whole feasible use. @param participant Actual full work/input/output/auxiliary benefit. @return Atomic whole use and native source settlement or unchanged refusal; removed paths cannot authorize retained unused heat. */
UtilityMutationReceipt ConsumeGeothermalHeat(UtilityGrantID grant, uint64_t quantity_j, GeothermalHeatParticipant &participant);
/** @param proposal Exact prepared shared conversion. @param participant Actual complete generator service owner. @return Atomic conversion after revalidating original live physical routes; utility quantities retain their shared owner. */
UtilityHeatConversionReceipt ConvertGeothermalHeat(const UtilityHeatConversionProposal &proposal, GeothermalHeatParticipant &participant);
/** @return Bounded saved physical bindings in stable shared consumer order; no copied account quantities. */
std::vector<GeothermalHeatConsumerBinding> CaptureGeothermalHeatConsumers();
/** @return Exact current saved source-slice path references; source/grant/use/carry retain the native utility owner. */
std::vector<GeothermalHeatRoute> CaptureGeothermalHeatRoutes();
/** @return Borrowed exact current physical source paths, valid until allocation/load/rotation/reset; native topology cleanup may inspect them without fallible copies or a duplicate energy balance. */
std::span<const GeothermalHeatRoute> GetGeothermalHeatRoutes();
/** @return Epoch of authoritative route references, or zero before physical allocation; reading never rotates time or grants. */
uint64_t GetGeothermalHeatRouteInterval();
/** Clear physical bindings/routes during actual world reset, after shared quantities have been reset by their existing owner. */
void ResetGeothermalHeatState();
/** Expire only physical route references after the existing shared owner rotates/curtails its reservations; consumer ports and all native consumed history remain with their owners. */
void ClearGeothermalHeatRoutes();
/** @param bindings Unpublished saved physical ports. @param routes Exact saved current source path references. @param interval Route epoch. Stage without publishing any eligibility, grant or energy. */
void StageLoadedGeothermalHeatState(std::vector<GeothermalHeatConsumerBinding> bindings, std::vector<GeothermalHeatRoute> routes, uint64_t interval);
/** @return Whether complete staged physical references agree with actual shared quantities/native sources/installed geometry before atomic publication. */
bool AcceptLoadedGeothermalHeatState();
/** @param bindings Copied physical ports. @param routes Copied exact source paths. @param interval Copied route epoch. @param native This file's shared utility authority. @param width Copied world width. @param height Copied single-plane height. @param layers Copied plane count. @param source_ports Original constructed source ports from this file only. @return Whether physical references and source/path conservation agree without querying or changing the running world. */
bool ValidateGeothermalHeatRouteState(std::span<const GeothermalHeatConsumerBinding> bindings, std::span<const GeothermalHeatRoute> routes, uint64_t interval,
	const UtilityPowerState &native, uint32_t width, uint32_t height, uint8_t layers, const std::map<UtilityProducerID, TileIndex> &source_ports);
/** @return Field-wise physical reference digest; it reads no duplicate energy or material balance. */
uint64_t ComputeGeothermalHeatRouteDigest();
/** Clear unpublished concrete physical heat route transport during world reset. */
void ResetGeothermalHeatRouteLoadStaging();
/** Validate independent copied physical route/consumer/source/installed references without changing the running world. */
void ValidateGeothermalHeatRouteLoadPreview();

#endif /* GEOTHERMAL_HEAT_H */
