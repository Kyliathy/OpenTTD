/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight.h Whole actual rocket-to-module packet admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Public Handoff Contract.
 */
#ifndef ORBITAL_FREIGHT_H
#define ORBITAL_FREIGHT_H
#include "space_transition.h"
#include "space_structure_type.h"

/** Quote every physical freight/service label together without reserving capacity or a berth.
 * @param rocket Exact native sender; its existing compartments remain the sole packet owners.
 * @param destination Exact owned operational Core/Storage/Dock incarnation.
 * @param require_berth False quotes departure capacity; true additionally requires this flight's actual admitted arrival.
 * @return Native flight block, with None meaning the complete transfer fits; all queries preserve cargo, money and utility history.
 */
SpaceFlightBlock GetOrbitalModuleFreightBlock(const SpaceVehicle &rocket, SpaceStructureRef destination, bool require_berth);
/** Move a fully admitted flight's actual freight and RFMT/ELEC into the destination's sole inventory.
 * @param rocket Native in-Space delivery hull at its precise shared berth.
 * @param destination Exact owned operational module incarnation.
 * @return Whether every original packet moved; refusal changes nothing. The native flight controller commits its receipt and recovery in the same opportunity.
 */
bool CommitOrbitalModuleFreight(SpaceVehicle &rocket, SpaceStructureRef destination);
#endif
