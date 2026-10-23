/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator.h Native funded powerhouse construction, operating references and service; Industry alone owns material and shared utilities alone own energy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8DC/8E.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef GEOTHERMAL_GENERATOR_H
#define GEOTHERMAL_GENERATOR_H

#include "geothermal.h"
#include "geothermal_generator_geometry.h"
#include "geothermal_heat.h"

/** Existing native machinery/accepted-stock/history owner, never a generator-local inventory. */
struct Industry;

static constexpr uint16_t GEOTHERMAL_POWERHOUSE_LOCAL_ID = 0x8320; ///< Canonical IIDS/TIDS8320..832F; separate from collector8300..8308 and riser8310..8313.
static constexpr uint16_t GEOTHERMAL_MAGMA_LOCAL_ID = 0x8330; ///< Appended2000 IIDS/TIDS8330..833F, preserving every previous native mapping.

/** Actual paid construction/service history and original site/utility references; this record contains neither material stock nor generated energy. */
struct GeothermalGeneratorState {
	IndustryID industry = IndustryID::Invalid(); ///< Exact native machinery incarnation.
	GeothermalSiteID site{UINT32_MAX}; ///< Retained consumed original cold Powerhouse blueprint.
	CompanyID owner = CompanyID::Invalid(); ///< Actual company controller, invalid after abandonment.
	GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap; ///< Canonical dated content model.
	uint16_t work_ticks = 0; ///< Native prepaid labour, bounded by its canonical dated construction bill.
	bool materials_committed = false; ///< Historical whole-bill consumption receipt; no second material balance.
	bool has_work_tick = false; ///< Presence of actual once-per-tick work, including tick zero.
	uint64_t last_work_tick = 0; ///< Last actual construction opportunity; retries cannot bank extra labour.
	UtilityConsumerID heat_consumer{UINT32_MAX}; ///< Original installed-port HeatGenerator identity; a pending output registration may retain this identity alone.
	UtilityProducerID electrical_producer{UINT32_MAX}; ///< Original shared exact-conversion source; its meter alone owns energy, conversion numerator and thermal history.
	uint8_t service_remainder = 0; ///< Sole unpaid service numerator0..73, in Money/74; only committed active time accrues it.
	bool has_operating_tick = false; ///< Presence of one successfully committed native operating opportunity, including tick zero.
	uint64_t last_operating_tick = 0; ///< Last committed synchronized opportunity; duplicate calls cannot charge or generate twice.
	uint64_t active_service_ticks = 0; ///< Committed service-duration history, used to validate billing carry without storing another energy balance.
	/** @param other Candidate native transport. @return Exact equality of every saved construction/reference field. */
	bool operator==(const GeothermalGeneratorState &other) const = default;
};

/** Pure complete original-site funding admission; it allocates no industry, stock, work or utility identity. */
struct GeothermalGeneratorAdmission {
	GeothermalSiteID site{UINT32_MAX}; ///< Original active cold blueprint selected by the command.
	GeothermalSiteAdmission foundation; ///< Complete current native floor/company/date/source admission.
	IndustryType type = IT_INVALID; ///< Exact canonical registered native definition.
	GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap; ///< Original dated content model.
};

/** @param activated Exact admitted native package. @return Whether the complete1980 and2000 native IIDS/TIDS families installs without funding machinery. */
bool InitialiseGeothermalGeneratorDefinitions(const GRFConfigList &activated);
/** @param model Canonical dated generator content. @return Existing exact native type, or IT_INVALID for absent/unknown/malformed definitions; queries allocate nothing. */
IndustryType GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel model);
/** @param type Actual native definition. @return Whether the exact package/cells/inputs belong to this generator owner. */
bool IsGeothermalGeneratorDefinition(IndustryType type);
/** @param type Actual native definition. @return Borrowed immutable Deep/date capability, or nullptr for absent/foreign definitions. */
const LayerIndustryCapability *GetGeothermalGeneratorIndustryCapability(IndustryType type);
/** @param site Original active cold blueprint. @return Complete capital/floor/native-pool admission, or unchanged refusal before funding. */
GeothermalGeneratorAdmission PrepareGeothermalGeneratorConstruction(GeothermalSiteID site);
/** @param industry Real newly allocated empty native candidate. @param site Original still-active blueprint. @return Whether exact native construction binding consumes only that planning reservation before non-failing tile creation. */
bool BindGeothermalGeneratorIndustry(Industry &industry, GeothermalSiteID site);
/** @param industry Exact native machinery identity. @return Borrowed construction state, or nullptr; re-query after mutation/reset. */
const GeothermalGeneratorState *GetGeothermalGeneratorState(IndustryID industry);
/** @param industry Native machinery and stock owner. @param cargo Actual active cargo slot. @return Exact whole remaining construction bill space, or zero without current admission. */
uint32_t GetGeothermalGeneratorInputRoom(const Industry &industry, CargoType cargo);
/** @param industry Native machinery/stock owner. @param cargo Existing cargo meaning. @param offered Actual offered packet units. @return Exact accepted native stock/history receipt; excess remains with the caller. */
uint32_t ReceiveGeothermalGeneratorCargo(Industry &industry, CargoType cargo, uint32_t offered);
/** @param industry Actual native machinery. Advance one synchronized prepaid construction opportunity after complete stock admission; this function generates no heat/electricity. */
void AdvanceGeothermalGenerator(Industry &industry);
/** @param industry Exact native machinery identity. @return Whether native structure, whole-bill receipt, work, company and date permit subsequent operating admission; readiness alone creates no energy. */
bool IsGeothermalGeneratorConstructionComplete(IndustryID industry);
/** @param industry Original funded machinery. @return Declared boundary port for both separate utility domains, or INVALID_TILE; querying never installs a cell or registers an account. */
TileIndex GetGeothermalGeneratorPort(IndustryID industry);
/** @param industry Actual completed owned machinery. @return Whether both installed domain cells retain its declared port and controller; this pure query creates no identity. */
bool HasGeothermalGeneratorInstalledPorts(IndustryID industry);
/** @param industry Actual native machinery. @param economy_ticks Explicit synchronized duration being inspected. @return Whether its original owner can reserve the exact whole service invoice and nonzero fractional remainder; reading spends no money or work. */
bool HasGeothermalGeneratorServiceFunds(IndustryID industry, uint32_t economy_ticks);
/** @param industry Original completed machinery. @param economy_ticks Actual shared publication duration. @return Pure feasible physical demand, or absence without source/path/output/service admission; no accounts or time are acquired. */
std::optional<GeothermalHeatRequest> PrepareGeothermalGeneratorDemand(IndustryID industry, uint32_t economy_ticks);
/** @param industry Actual original machinery. @param heat_j Positive exact remaining allocated Heat. @param economy_ticks Actual common synchronized duration. @return Whole native service and physically routed shared conversion, or unchanged refusal. */
UtilityHeatConversionReceipt ConvertGeothermalGeneratorHeat(IndustryID industry, uint64_t heat_j, uint32_t economy_ticks);
/** @param interval Exact open shared epoch. @param economy_ticks Actual synchronized duration. @param other_demands Existing physical consumers' feasible whole demands; their actual owners retain consume/benefit authority. Acquire missing native references, allocate one shared physical Heat budget and commit complete native generation before electrical allocation. */
void RunGeothermalGenerators(uint64_t interval, uint32_t economy_ticks, std::span<const GeothermalHeatDemand> other_demands = {});
/** @return Whether retained generator bindings require the common driver even during temporary source/path interruptions. */
bool HasGeothermalGenerators();
/** @param state Detached native facility references/service. @param utility This candidate's complete shared owner. @return Whether original roles, immutable model bounds and retained control agree without querying live energy or geometry. */
bool ValidateGeothermalGeneratorOperatingReferences(const GeothermalGeneratorState &state, const UtilityPowerState &utility);
/** @return Whether every retained original generator input resolves its declared saved physical port after both native owners have accepted; no topology repair or energy mutation occurs. */
bool ValidateGeothermalGeneratorPhysicalReferences();
/** @param industry Actual native candidate or machinery being destroyed. Retire only its binding; never refund consumed bill or allocate replacement rights. */
void ReleaseGeothermalGeneratorIndustry(IndustryID industry);
/** @param industry Existing native machinery. @return Whether retained real input/output stock protects removal, including staged construction before binding acceptance. */
bool GeothermalGeneratorHasStoredCargo(const Industry &industry);
/** @param old_owner Original current controller. @param new_owner Surviving takeover company or invalid bankruptcy owner. Transfer/retire actual control while preserving original work/material history. */
void ChangeGeothermalGeneratorOwnership(Owner old_owner, Owner new_owner);
/** @return Stable native-industry-ordered construction/reference transport, with no material inventory. */
std::vector<GeothermalGeneratorState> CaptureGeothermalGenerators();
/** @param states Detached complete candidate. @return Whether every field/original site/native stock/whole structure agrees before any publication. */
bool ValidateGeothermalGenerators(std::span<const GeothermalGeneratorState> states);
/** @param states Complete unpublished native construction transport; live work/stock remains unchanged. */
void StageLoadedGeothermalGenerators(std::vector<GeothermalGeneratorState> states);
/** @return Whether complete original references and stock/work validate before publishing the staged family without repair. */
bool AcceptLoadedGeothermalGenerators();
/** Reset actual generator bindings and unpublished transport during world reset, after native owners retire. */
void ResetGeothermalGenerators();
/** @return Field-wise authoritative work/history/reference digest; no view, stock mutation or utility allocation occurs. */
uint64_t ComputeGeothermalGeneratorDigest();
/** Clear unpublished native construction/429 operating header/table transport during ordinary world reset or failed-load recovery. */
void ResetGeothermalGeneratorLoadStaging();
/** Validate independent copied construction/429 operating scalars/references without querying live energy, stock or clock. */
void ValidateGeothermalGeneratorLoadPreview();

#endif /* GEOTHERMAL_GENERATOR_H */
