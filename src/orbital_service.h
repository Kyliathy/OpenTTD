/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_service.h Contained paid Core service and shared electrical work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
 */
#ifndef ORBITAL_SERVICE_H
#define ORBITAL_SERVICE_H
#include "utility_power.h"
#include "space_production.h"

struct SpaceVehicle;
struct SpaceStructureRecord;
/** Player operations on the one contained berth; cancellation returns only unconsumed parts. */
enum class OrbitalServiceControl : uint8_t { Start, Pause, Resume, Cancel, End };
/** @return Whether an owned Core has an actual active panel for common interval publication. */
bool HasOrbitalUtilitySources();
/** @param interval Common ledger epoch. @param ticks Synchronized generation duration. Publish actual commissioned panels once. */
void PublishOrbitalUtilitySources(uint64_t interval, uint32_t ticks);
/** @param requests Common request list, extended with eligible contained jobs before its single allocation. */
void AppendOrbitalUtilityRequests(std::vector<UtilityPowerRequest> &requests);
/** @param craft Actual native hull. @return Whether a Core retains this exact hull's berth, including paused work. */
bool HasOrbitalServiceAssignment(const SpaceVehicle &craft);
/** @param craft Actual native hull. @return Whether this epoch authorizes both active upkeep and one750J service step. */
bool IsOrbitalServiceWorkTick(const SpaceVehicle &craft);
/** Settle one exact electrical debit together with each eligible native service step. */
void TickOrbitalServices();
/** @return Whether installed Core references agree with the already accepted native craft and shared utility owners. */
bool ValidateOrbitalServices();
/** Validate original typed panel/consumer references against a detached ledger, without querying live owners.
 * @param rows Copied native sites. @param power Same snapshot's shared ledger.
 * @return Whether dedicated utility identities, owner/layer/role and saved epochs agree.
 */
bool ValidateOrbitalServiceUtilityRecords(std::span<const SpaceStructureRecord> rows, const UtilityPowerState &power);
#endif
