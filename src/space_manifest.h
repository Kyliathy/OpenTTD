/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest.h Native spacecraft fuel/service packet custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group2C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; heavy resupply uses original physical containers and grants no service completion.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; versioned pending returns retain the original descent/quota owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; existing onboard work consumes immutable per-model parts and earned opportunities.
 */
#ifndef SPACE_MANIFEST_H
#define SPACE_MANIFEST_H
#include "space_production.h"
#include "cargopacket.h"
#include "cargotype.h"
#include "space_pod_type.h"
#include <array>
#include <optional>

/** Stable original physical compartment identities; availability/capacity never creates packets or service work. */
enum class SpaceSupply : uint8_t { Propellant, RefinedMetal, Electronics, End };
/** Sole original service-work location; actual workshop/queue owners separately admit each paid countdown. */
enum class SpaceServiceLocation : uint8_t { EarthWorkshop, Onboard, End };

/** Immutable bill and work budget for an existing service-capable probe role; this value owns no parts, countdown, berth or allowance. */
struct SpaceServiceTuning {
	uint16_t ticks; ///< Earned native opportunities required before actual completion.
	std::array<uint8_t, 2> inputs; ///< Whole actual RFMT/ELEC units retained until final consumption, in compartment order.
	uint8_t returns; ///< Replacement bounded allowance, granted only by actual once-only completion.
};

/** Query the immutable service bill without admitting a location, registering a producer or changing physical state.
 * @param model Canonical paid logical role; miners15/21 use their shared mining contract and19/20 retain the original probe defaults.
 * @return Exact service quantities, or absence for an unrelated/unknown role.
 */
std::optional<SpaceServiceTuning> GetSpaceServiceTuning(uint16_t model);
enum class SpaceFlightBlock : uint8_t;
inline constexpr size_t SPACE_SUPPLY_COUNT = 3;
/** Native Vehicle owns these actual packets. StationCargoList is a container,
 * not a Station identity or an additional station inventory. Freight stays in
 * Vehicle::cargo; fuel/service compartments are separate from freight mass. */
struct SpaceManifest {
	/** Construct empty native compartments; paid producers separately establish stock, custody and allowance. */
	SpaceManifest() = default;
	/** @param other Sole packet owner whose copying is forbidden to prevent duplicate physical inventory. */
	SpaceManifest(const SpaceManifest &other) = delete;
	/** @param other Sole packet owner whose assignment is forbidden to prevent duplicate physical inventory. */
	SpaceManifest &operator=(const SpaceManifest &other) = delete;
	std::array<StationCargoList, SPACE_SUPPLY_COUNT> supplies{};
	SpaceCraftRef passenger, carrier;
	std::vector<SpacePodRef> pods;
	uint16_t age_ticks = 0, service_ticks = 0;
	uint8_t returns_remaining = 0;
	uint8_t pending_returns = 0;
	SpaceServiceLocation service_location = SpaceServiceLocation::EarthWorkshop;
	bool service_paused = false;
	uint64_t fuel_burned = 0, services_completed = 0;
};
/** Detached same-file physical compartment/custody/history references; this record owns no packets or allowance producer. */
struct SpaceManifestRecord {
	SpaceCraftRef craft, passenger, carrier;
	std::vector<SpacePodRef> pods;
	uint16_t age_ticks = 0, service_ticks = 0;
	uint8_t returns_remaining = 0;
	uint8_t pending_returns = 0;
	SpaceServiceLocation service_location = SpaceServiceLocation::EarthWorkshop;
	bool service_paused = false;
	uint64_t fuel_burned = 0, services_completed = 0;
	std::array<std::vector<std::pair<CargoPacketID, StationID>>, SPACE_SUPPLY_COUNT> supplies;
};
/** @param supply Explicit physical compartment kind. @return Actual runtime label slot, or invalid when unsupported. */
CargoType GetSpaceSupplyCargo(SpaceSupply supply);
/** @param model Canonical logical hull model. @param supply Explicit physical compartment kind. @return Its frozen whole-unit capacity, without granting stock. */
uint16_t GetSpaceSupplyCapacity(uint16_t model, SpaceSupply supply);
/** Validate the heavy supply capability at the source-version boundary without consulting native pools.
 * @param model Actual native or same-file paid logical role.
 * @param record Actual copied compartment/custody/history fields.
 * @param heavy_resupply Whether the source schema includes the original heavy supply producer.
 * @param heavy_descents Whether the source includes the separately delivered heavy descent producer; default retains the433 supply-only contract.
 * @param heavy_service Whether the source includes actual heavy onboard service; default retains the434 descent-only contract.
 * @return Whether each source owns only delivered heavy supply/descent/onboard capabilities; full physical packet, placement and queue admission remains with its native/copied owners.
 */
bool IsSpaceHeavyResupplyStateSupported(uint16_t model, const SpaceManifestRecord &record, bool heavy_resupply, bool heavy_descents = false, bool heavy_service = false);
/** @param craft Actual sole native hull owner. @return Its existing manifest or newly empty compartments; no stock, shells, allowance or work is granted. */
SpaceManifest &GetOrCreateSpaceManifest(SpaceVehicle &craft);
/** @param craft Actual native hull. @return Whether its real compartments/custody/work retain physical assets that cannot be discarded. */
bool HasSpaceManifestAssets(const SpaceVehicle &craft);
/** @param craft Actual carrier. @return Complete physical freight/service/pod/passenger payload mass in sixteenths, or UINT64_MAX for invalid custody. */
uint64_t GetSpacePayloadMass(const SpaceVehicle &craft);
/** @param model Canonical logical freight role. @param cargo Actual physical runtime label. @return Whole units within that role's mass/refit bound; zero denies unsupported cargo. */
uint16_t GetSpaceFreightCapacity(uint16_t model, CargoType cargo);
/** @param carrier Actual original carrier; synchronize only its exact sole paid passenger's physical pose. */
void SyncSpacePassenger(SpaceVehicle &carrier);
/** @param craft Actual departing/destroyed hull. @param cleaning_pool Whether native bulk teardown owns packet disposal. */
void ForgetSpaceManifest(SpaceVehicle &craft, bool cleaning_pool);
/** @param craft Actual hull losing its exact home; retain paid passengers, parts, shells and paused work without creating a replacement facility. */
void DetachSpaceManifestAtLostCenter(SpaceVehicle &craft);
/** Age real compartment packets and advance only actual admitted original service jobs once per normal native opportunity. */
void TickSpaceManifests();
/** @param craft Actual native service owner. @return Whether current saved parts, pause, location and original slot authorize one work tick for both upkeep and execution. */
bool IsSpaceServiceWorkTick(const SpaceVehicle &craft);
/** @param rocket Actual sole paid payload carrier. @param receiver Exact surviving orbital hull incarnation.
 * @return Whole-payload refusal or admission at departure and again at delivery; no container, allowance or stock is created.
 */
SpaceFlightBlock GetSpaceResupplyBlock(const SpaceVehicle &rocket, SpaceCraftRef receiver);
/** @param rocket Actual original OrbitalDelivery owner with an uncommitted handover receipt. @param receiver Exact original orbital hull incarnation.
 * @return Whether every actual part/shell/freight owner moved once; caller commits its flight receipt in the same native tick.
 */
bool CommitSpaceResupply(SpaceVehicle &rocket, SpaceCraftRef receiver);
/** @return Complete copied native compartment, custody and work history references in actual pool order. */
std::vector<SpaceManifestRecord> CaptureSpaceManifests();
/** Check detached transport bounds before actual same-file/native model and packet joins.
 * @param records Complete detached manifest family.
 * @param heavy_onboard Whether transport includes the heavy296-opportunity producer; default retains the original148 bound. This broad bound grants no role its own service rights.
 * @return Whether structural capacities, histories and custody/packet references are bounded and exclusive.
 */
bool ValidateSpaceManifestRecords(std::span<const SpaceManifestRecord> records, bool heavy_onboard = false);
/** @return Whether actual native packet owners, paid roles, custody and current work/navigation dependencies agree. */
bool ValidateSpaceManifests();
/** @param records Complete staged original compartment references. @return Whether fresh native manifests installed without normalizing, granting or duplicating inventory/work. */
bool InstallLoadedSpaceManifests(std::span<const SpaceManifestRecord> records);
/** @return Deterministic actual compartment, custody, paid history and packet identity digest. */
uint64_t ComputeSpaceManifestDigest();
/** Discard unpublished file staging without touching real compartments or service work. */
void ResetSpaceManifestLoadStaging();
/** @return Whether staged native manifests pass source-version and complete native ownership admission before installation. */
bool AcceptLoadedSpaceManifests();
/** Validate copied same-file manifests and their actual producers without consulting or changing the open world's pools. */
void ValidateSpaceManifestLoadPreview();
#endif
