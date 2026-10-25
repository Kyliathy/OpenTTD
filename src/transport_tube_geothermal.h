/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal.h Original geothermal tube inlet identities and shared forwarding references, without electrical balances or duplicate generation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BBB-6BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TRANSPORT_TUBE_GEOTHERMAL_H
#define TRANSPORT_TUBE_GEOTHERMAL_H

#include "transport_tube.h"
#include "geothermal_utility.h"

static constexpr size_t MAX_TUBE_GEOTHERMAL_SOURCES = 256; ///< Retained physical source history, independently of shared energy accounting limits.

/** Original physical source association; the shared forwarding consumer alone owns reserved original-source slices. */
struct TubeGeothermalSource {
	TubeGeothermalSourceID id{UINT32_MAX}; ///< Nonreused physical inlet identity, never a generation ID.
	IndustryID generator = IndustryID::Invalid(); ///< Original completed native plant, not a nearby replacement.
	UtilityProducerID producer{UINT32_MAX}; ///< That plant's original shared exact-conversion output.
	UtilityConsumerID supply{UINT32_MAX}; ///< Sole owned forwarding reservation; no quantity is copied here.
	CompanyID owner = CompanyID::Invalid(); ///< Actual controller, retained as history after loss.
	TileIndex port = INVALID_TILE; ///< Original native plant boundary, independently checked against its consumed blueprint.
	TileIndex inlet = INVALID_TILE; ///< Actual owned tube facility receiving the installed cable.
	DepotID depot = DepotID::Invalid(); ///< Original native depot, or invalid for a loading platform.
	StationID station = StationID::Invalid(); ///< Original native loading station, or invalid for a depot.
	bool active = true; ///< Demolition/source/company loss retires this incarnation permanently.
	/** @param other Candidate metadata. @return Exact original physical/shared reference equality, without comparing any derived path or quantity. */
	bool operator==(const TubeGeothermalSource &other) const = default;
};

/** Bounded original source transport; no rate, generation, invoice, grant or demand carry belongs to this owner. */
struct TubeGeothermalPowerState {
	uint32_t next_source = 0; ///< Monotonic issuer; retired records remain dense and cannot be reused.
	uint64_t sequence = 0; ///< Reference/lifecycle receipt only, never an electrical epoch or work counter.
	std::vector<TubeGeothermalSource> sources; ///< Original physical associations in issued identity order.
};

/** Pure physical connection result; estimates acquire no reservation or identity. */
struct TubeGeothermalPowerResult {
	TubePowerError error = TubePowerError::Refused; ///< Complete reference admission or unchanged refusal.
	TubeGeothermalSourceID source{UINT32_MAX}; ///< Issued/predicted inlet incarnation only on success.
	UtilityConsumerID supply{UINT32_MAX}; ///< Issued/predicted shared forwarding reservation, not a proxy producer.
};

/** @param generator Original completed dated native plant. @param inlet Actual owned Deep tube depot/platform at the end of purchased cable. @param execute False predicts both reference identities without money, quantity, RNG or counter changes. @return Complete source association or unchanged refusal; this creates no grant or capsule work. */
TubeGeothermalPowerResult ConnectTubeGeothermalPower(IndustryID generator, TileIndex inlet, bool execute = true);
/** @param source Original physical inlet incarnation. @param execute False checks complete owned closure without mutation. @return Whole shared-supply/attachment retirement and reference receipt, preserving the actual generator and other users. */
TubePowerError CloseTubeGeothermalPower(TubeGeothermalSourceID source, bool execute = true);
/** @param source Original retained inlet identity. @return Borrowed reference metadata, or nullptr; re-query after mutation/reset. */
const TubeGeothermalSource *GetTubeGeothermalSource(TubeGeothermalSourceID source);
/** @return Borrowed bounded original native inlet references, without copying quantity or metadata stores; re-query after mutation/reset. */
std::span<const TubeGeothermalSource> GetTubeGeothermalSources();
/** @param source Original active inlet identity. @return Actual current purchased electrical path after native date/owner/port/facility admission; this pure physical view guarantees neither service funds nor a grant. */
std::optional<GeothermalUtilityPath> GetTubeGeothermalSourcePath(TubeGeothermalSourceID source);
/** @return Field-wise copied physical source references; quantities remain exclusively in utility_power.h. */
TubeGeothermalPowerState CaptureTubeGeothermalPowerState();
/** @return Deterministic field-wise source-reference/lifecycle digest, independent of pointers, padding, caches and energy queries. */
uint64_t ComputeTubeGeothermalPowerDigest();
/** @param state Detached original inlet transport. @param utility This candidate's sole electrical owner. @param world True resolves original native machinery/facilities after all owners accept; false checks copied references only. @return Whether dense identities, active permissions and retired history agree without repairing paths or quantities. */
bool ValidateTubeGeothermalPowerState(const TubeGeothermalPowerState &state, const UtilityPowerState &utility, bool world = true);
/** @param state Complete unpublished source transport; no native source or electrical reservation is acquired. */
void StageLoadedTubeGeothermalPowerState(TubeGeothermalPowerState state);
/** @return Whether staged original references validate after shared utility and native generators accept, before publishing without repair. */
bool AcceptLoadedTubeGeothermalPowerState();
/** @param tile Actual changed native transport tile. Retire only original inlets whose saved native facility no longer exists; repairing a wire alone retains physical source identity. */
void InvalidateTubeGeothermalSourcesForTile(TileIndex tile);
/** @param producer Original shared output being retired. Close only its original physical sources and unused forwarding rights before eligibility disappears. */
void InvalidateTubeGeothermalSourcesForProducer(UtilityProducerID producer);
/** @param old_owner Actual original company. @param new_owner Surviving successor or INVALID_OWNER. Retire old associations and preserve their original shared history, requiring explicit fresh sources after takeover. */
void ChangeTubeGeothermalPowerCompanyOwnership(Owner old_owner, Owner new_owner);
/** Reset only physical source metadata during ordinary world reset; shared quantities have their own native reset owner. */
void ResetTubeGeothermalPowerState();

/** Reset only unpublished original geothermal Tube header/rows at world replacement. */
void ResetTubeGeothermalPowerLoadStaging();
/** Validate this preview's complete original plant, source and forwarding references without creating energy. */
void ValidateTubeGeothermalPowerLoadPreview();

#endif /* TRANSPORT_TUBE_GEOTHERMAL_H */
