/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest.h Native spacecraft fuel/service packet custody; Plan9 Group2C. */
#ifndef SPACE_MANIFEST_H
#define SPACE_MANIFEST_H
#include "space_production.h"
#include "cargopacket.h"
#include "cargotype.h"
#include "space_pod_type.h"
#include <array>

enum class SpaceSupply : uint8_t { Propellant, RefinedMetal, Electronics, End };
enum class SpaceServiceLocation : uint8_t { EarthWorkshop, Onboard, End };
enum class SpaceFlightBlock : uint8_t;
inline constexpr size_t SPACE_SUPPLY_COUNT = 3;
/** Native Vehicle owns these actual packets. StationCargoList is a container,
 * not a Station identity or an additional station inventory. Freight stays in
 * Vehicle::cargo; fuel/service compartments are separate from freight mass. */
struct SpaceManifest {
	SpaceManifest() = default;
	SpaceManifest(const SpaceManifest &) = delete;
	SpaceManifest &operator=(const SpaceManifest &) = delete;
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
CargoType GetSpaceSupplyCargo(SpaceSupply supply);
uint16_t GetSpaceSupplyCapacity(uint16_t model, SpaceSupply supply);
SpaceManifest &GetOrCreateSpaceManifest(SpaceVehicle &craft);
bool HasSpaceManifestAssets(const SpaceVehicle &craft);
uint64_t GetSpacePayloadMass(const SpaceVehicle &craft);
uint16_t GetSpaceFreightCapacity(uint16_t model, CargoType cargo);
void SyncSpacePassenger(SpaceVehicle &carrier);
void ForgetSpaceManifest(SpaceVehicle &craft, bool cleaning_pool);
void DetachSpaceManifestAtLostCenter(SpaceVehicle &craft);
void TickSpaceManifests();
/** Current saved state authorizes one service work tick; shared by upkeep and execution. */
bool IsSpaceServiceWorkTick(const SpaceVehicle &craft);
/** Pure whole-payload admission at departure and again at delivery. */
SpaceFlightBlock GetSpaceResupplyBlock(const SpaceVehicle &rocket, SpaceCraftRef receiver);
/** Transition-only move of actual packets/pod identities; caller commits its receipt in the same tick. */
bool CommitSpaceResupply(SpaceVehicle &rocket, SpaceCraftRef receiver);
std::vector<SpaceManifestRecord> CaptureSpaceManifests();
bool ValidateSpaceManifestRecords(std::span<const SpaceManifestRecord> records);
bool ValidateSpaceManifests();
bool InstallLoadedSpaceManifests(std::span<const SpaceManifestRecord> records);
uint64_t ComputeSpaceManifestDigest();
void ResetSpaceManifestLoadStaging();
bool AcceptLoadedSpaceManifests();
void ValidateSpaceManifestLoadPreview();
#endif
