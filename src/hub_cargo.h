/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_cargo.h Packet-owner transfers for timed launch handling, without vehicle loading or payment. */
#ifndef HUB_CARGO_H
#define HUB_CARGO_H
#include "cargopacket.h"

class HubCargoTransfer {
public:
	/** Pure preflight, including the one possible split allocation. No packet/cache/RNG mutation. */
	static bool CanMove(const StationCargoList &source, uint quantity);
	/** Move the preflighted quantity, preserving routing, provenance, age and feeder share. */
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
