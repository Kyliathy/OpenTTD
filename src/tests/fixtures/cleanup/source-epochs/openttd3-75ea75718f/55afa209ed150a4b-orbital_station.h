/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_station.h Native construction and completed Core inventory.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#ifndef ORBITAL_STATION_H
#define ORBITAL_STATION_H
#include "cargopacket.h"
#include "cargotype.h"
#include "space_production.h"
#include "multilayer_utility.h"
#include "utility_power.h"
#include "multilayer_space_content.h"
#include "space_structure_type.h"
#include <array>
#include <set>

struct SpaceStructure;
struct SpaceStructureState;
struct SpaceVehicle;
/** Saved physical assembly progress, expressed in whole and fractional canonical work ticks.
 * The remainder preserves exact conversion from whole joules to Q16 work;
 * it owns no energy balance or grant. The shared utility ledger owns energy.
 */
struct OrbitalConstructionProgress {
	uint32_t work_ticks = 0; ///< Completed canonical work ticks.
	uint16_t work_fraction = 0; ///< Fraction of the next tick, in units of1/65536.
	uint16_t work_remainder = 0; ///< Division remainder below the canonical joules per work tick.
	/** @param other Progress to compare. @return Whether every physical work component agrees. */
	bool operator==(const OrbitalConstructionProgress &other) const = default;
};
/** Saved once-only physical work receipt, shared by native jobs and detached transport. */
struct OrbitalConstructionState : OrbitalConstructionProgress {
	uint64_t last_work_tick = 0; ///< Native simulation tick that committed this progress; zero for migrated untouched receipts.
	uint64_t last_work_interval = 0; ///< Original shared allocation epoch; zero for portable first-Core work.
};
/** A blueprint owns these packets exclusively. No completed Station is required.
 * Materials are consumed together only at the final work tick; partial work
 * retains all inputs. Cancellation exposes them for physical collection. */
struct OrbitalConstructionJob : OrbitalConstructionState {
	std::array<StationCargoList, 2> inputs{};
	uint16_t age_ticks = 0;
	std::array<uint16_t, 2> consumed{};
};
/** Saved service identity/progress; the craft's manifest remains the sole return-allowance owner. */
struct OrbitalServiceState {
	SpaceCraftRef craft; ///< One generation-qualified berth occupant, including a lost craft waiting for cancellation.
	uint16_t remaining_ticks = 0; ///< Zero denotes the canonical empty job.
	bool paused = false; ///< Explicit player pause retains every reserved part.
	uint64_t last_work_interval = 0; ///< Prevents a second work debit in the same shared allocation epoch.
	uint64_t services_at_start = 0; ///< Exact native manifest serial renewed by this job once.
};
/** One contained berth/job owns actual RFMT/ELEC until completion or explicit cancellation. */
struct OrbitalServiceJob : OrbitalServiceState {
	std::array<StationCargoList, 2> inputs{}; ///< Exclusive RFMT and ELEC containers, separate from general stock.
};
/** Detached service transport contains references to existing packets rather than replacement cargo. */
struct OrbitalServiceRecord : OrbitalServiceState {
	std::array<uint16_t, 2> quantities{};
	std::array<std::vector<std::pair<CargoPacketID, StationID>>, 2> inputs;
};
/** Actual module utility identities and completion receipt; energy remains solely in the shared ledger. */
struct OrbitalFacilityState {
	UtilityProducerID producer{UINT32_MAX}; ///< Original paid panel; older inventory-only cores require explicit commissioning.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Original bounded service consumer: one Core job or two explicitly connected Dock jobs.
	UtilityConsumerID assembly_consumer{UINT32_MAX}; ///< One bounded construction slot; shares the actual panel with the separate service berth.
	bool power_enabled = true; ///< Physical panel availability; disabling it publishes zero generation.
	uint64_t services_completed = 0; ///< Once-only local service completion receipt.
	SpaceStructureRef utility_source; ///< Explicit Dock connection to an original adjacent owned panel; absent for other module roles.
};
/** Completed module facilities, contained by their generation-qualified native structure.
 * Activation has no free cargo, vehicle or power grant. Inventory remains inert
 * until the saved countdown finishes; operational stock never reuses job inputs.
 */
struct OrbitalModule : OrbitalFacilityState {
	std::array<StationCargoList, NUM_CARGO> stock{}; ///< Sole physical owner of completed-module cargo.
	uint16_t activation_ticks = 0; ///< Remaining synchronized activation work, zero means ready.
	uint16_t age_ticks = 0; ///< Native cargo-aging remainder, independent of UI refresh.
	OrbitalServiceJob service; ///< First Core/Dock berth and its reserved physical inputs.
	OrbitalServiceJob dock_service; ///< Second contained Dock berth; other module roles retain its canonical empty state.
	/** @return Every contained physical service owner for packet/lifecycle audits; capacity remains role-specific. */
	std::array<OrbitalServiceJob *, 2> GetServiceJobs() { return {&this->service, &this->dock_service}; }
	/** @return Read-only contained service owners; querying this array grants no operational berth. */
	std::array<const OrbitalServiceJob *, 2> GetServiceJobs() const { return {&this->service, &this->dock_service}; }
};
/** Copied transport for a contained module; packet IDs refer to existing packets, never copies. */
struct OrbitalModuleRecord : OrbitalFacilityState {
	uint16_t activation_ticks = 0, age_ticks = 0;
	std::array<uint16_t, NUM_CARGO> quantities{};
	std::array<std::vector<std::pair<CargoPacketID, StationID>>, NUM_CARGO> stock;
	OrbitalServiceRecord service;
	OrbitalServiceRecord dock_service;
	/** @return Both detached service records, including the canonical empty second record for old/non-Dock modules. */
	std::array<OrbitalServiceRecord *, 2> GetServiceJobs() { return {&this->service, &this->dock_service}; }
	/** @return Read-only detached service owners for same-file preview and physical exclusivity checks. */
	std::array<const OrbitalServiceRecord *, 2> GetServiceJobs() const { return {&this->service, &this->dock_service}; }
};
/** Physical delivery, bounded by the remaining recipe.
 * @param site Authoritative blueprint receiving materials.
 * @param probe Actual builder at the precise approach with a matching order.
 * @param execute False validates without moving packets.
 * @return Whether the requested delivery can be committed.
 */
bool ReserveOrbitalConstructionInputs(SpaceStructure &site, SpaceVehicle &probe, bool execute);
/** Advance one working tick by the lowest eligible native builder ID.
 * @param probe Actual ordered builder; additional builders cannot multiply progress.
 * @return Whether progress and any final consumption were committed.
 */
bool CommitOrbitalConstructionStep(SpaceVehicle &probe);
/** Check exact physical work conservation without querying a utility account.
 * @param progress Saved physical work. @param required_ticks Canonical complete recipe duration.
 * @return Whole joules still useful for this recipe, or absence for inconsistent/out-of-range progress.
 */
std::optional<uint64_t> GetOrbitalConstructionRemainingEnergy(const OrbitalConstructionProgress &progress, uint32_t required_ticks);
/** Convert an admitted whole-joule grant into exact bounded physical work without mutation.
 * @param progress Current saved physical work. @param required_ticks Canonical complete duration.
 * @param energy_j Positive actual grant use, capped at one canonical work tick and remaining useful work.
 * @return Next physical progress, or absence for invalid/overpaid/repeated completion.
 */
std::optional<OrbitalConstructionProgress> PlanOrbitalConstructionProgress(const OrbitalConstructionProgress &progress, uint32_t required_ticks, uint16_t energy_j);
/** Admit one normal synchronized construction opportunity before the native vehicle loop; grants no work or energy. */
void PrepareOrbitalConstructionTick();
/** Clear transient tick admission during world teardown; saved native jobs retain the sole progress receipt. */
void ResetOrbitalConstructionTick();
/** Gather the single eligible builder per actual Core, registering only a zero-energy bounded consumer when necessary.
 * @param requests Shared allocator's current complete request set; no private allocation is performed.
 */
void AppendOrbitalConstructionRequests(std::vector<UtilityPowerRequest> &requests);
/** @param probe Actual builder. @return Whether this tick authorizes construction work/upkeep. */
bool IsSpaceConstructionWorkTick(const SpaceVehicle &probe);
/** @param site Native blueprint. @return Whether every canonical recipe input is physically present. */
bool HasOrbitalConstructionInputs(const SpaceStructure &site);
/** Resolve the deterministic physical Core supplying an adjacent module's construction dependency.
 * @param state Candidate or actual paid module geometry and owner.
 * @return Lowest-ID owned operational Core sharing an orthogonal footprint edge, or nullptr.
 */
SpaceStructure *GetOrbitalConstructionCore(const SpaceStructureState &state);
/** Age retained job/module packets and advance paid Core activation once per synchronized tick. */
void TickOrbitalConstructionInventory();
/** @param kind Canonical static recipe. @return Whether this producer delivers its activation functions. */
bool IsSupportedOrbitalModuleKind(SpaceBuildKind kind);
/** Pure physical edge relation shared by construction dependencies and explicit local utility connections.
 * @param a First paid static footprint. @param b Second paid static footprint.
 * @return Whether their canonical footprints share a nonempty orthogonal edge, without counting approach cells.
 */
bool AreOrbitalModuleFootprintsAdjacent(const SpaceStructureState &a, const SpaceStructureState &b);
/** @param kind Canonical module recipe. @return Bounded operational service job count; an inert frame grants none by itself. */
uint8_t GetOrbitalModuleServiceCapacity(SpaceBuildKind kind);
/** Definition-only physical stock bound; connected modules never enlarge another module's container.
 * @param kind Canonical static recipe. @return Per-label stock limit, or zero for a module without freight storage.
 */
uint16_t GetOrbitalModuleStockLimit(SpaceBuildKind kind);
/** @param site Native structure. @return Whether its paid activation and actual owned Core dependency are ready. */
bool IsOrbitalModuleOperational(const SpaceStructure &site);
/** Actual same-layer panel output; this query neither publishes generation nor credits energy.
 * @param site Native completed module. @return Available watts from its declared original panel, or zero when blocked.
 */
uint32_t GetOrbitalModulePanelRate(const SpaceStructure &site);
/** @param site Native structure. @param cargo Resolved physical cargo type. @return Operational capacity, or zero for an inert/invalid owner. */
uint16_t GetOrbitalModuleCargoCapacity(const SpaceStructure &site, CargoType cargo);
/** Add every actual construction and completed-module packet to an exclusivity audit.
 * @param owned Packet IDs already encountered in other native containers.
 * @return Whether all orbital site packets have a unique physical owner.
 */
bool InsertOrbitalPacketOwners(std::set<CargoPacketID> &owned);
#endif
