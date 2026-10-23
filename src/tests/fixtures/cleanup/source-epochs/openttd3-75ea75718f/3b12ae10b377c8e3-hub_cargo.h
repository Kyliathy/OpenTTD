/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_cargo.h Packet-owner transfers for timed launch handling, without vehicle loading or payment. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC paid custody/refunds. */
#ifndef HUB_CARGO_H
#define HUB_CARGO_H
#include "cargopacket.h"

/** Controlled native custody/consumption operations; every unit remains in its actual packet owner. */
class HubCargoTransfer {
public:
	/** Pure preflight, including the one possible split allocation. No packet/cache/RNG mutation. */
	static bool CanMove(const StationCargoList &source, uint quantity);
	/**
	 * Move actual preflighted stock, preserving provenance/age/feeder share and permanently dispatching paid source units.
	 * @param source Actual source queue or job; a moved paid unit loses source refund eligibility.
	 * @param target Distinct actual destination owner, never an additional copied stock ledger.
	 * @param quantity Whole native units already admitted by the caller's route/job boundary.
	 * @return Whether the entire transfer committed; refusal moves nothing.
	 */
	static bool Move(StationCargoList &source, StationCargoList &target, uint quantity);
	static void Age(StationCargoList &cargo);
	/** Deterministic recipe consumption; no RNG, split allocation, vehicle-loading state or payment. */
	static bool Consume(StationCargoList &source, uint quantity);
	/** Native Keep-only freight moves at an explicit physical loading/unloading tile; no payment. */
	static bool CanMove(const VehicleCargoList &source, uint quantity);
	static bool Move(StationCargoList &source, VehicleCargoList &target, uint quantity, TileIndex tile);
	static bool Move(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex tile);
	/** Explicit Space-to-Surface descent rebases the saved travel vector without paying for a layer offset. Target must be empty. */
	static bool ReleaseOrbital(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex release_tile);
};
#endif
