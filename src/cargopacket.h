/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file cargopacket.h Base class for cargo packets. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, plane-coordinate freight accounting */

#ifndef CARGOPACKET_H
#define CARGOPACKET_H

#include "core/pool_type.hpp"
#include "economy_type.h"
#include "station_type.h"
#include "order_type.h"
#include "cargo_type.h"
#include "source_type.h"
#include "core/multimap.hpp"
#include "saveload/saveload_type.h"

/** Unique identifier for a single cargo packet. */
using CargoPacketID = PoolID<uint32_t, struct CargoPacketIDTag, 0xFFF000, 0xFFFFFF>;
struct CargoPacket;

/** Type of the pool for cargo packets for a little over 16 million packets. */
using CargoPacketPool = Pool<CargoPacket, CargoPacketID, 1024, PoolType::Normal, true>;
/** The actual pool with cargo packets. */
extern CargoPacketPool _cargopacket_pool;

struct GoodsEntry; // forward-declare for Stage() and RerouteStalePackets()

template <class Tinst, class Tcont> class CargoList;
class StationCargoList; // forward-declare, so we can use it in VehicleCargoList.
extern SaveLoadTable GetCargoPacketDesc();

/**
 * Container for cargo from the same location and time.
 */
struct CargoPacket : CargoPacketPool::PoolItem<&_cargopacket_pool> {
private:
	uint16_t count = 0; ///< The amount of cargo in this packet.
	uint16_t periods_in_transit = 0; ///< Amount of cargo aging periods this packet has been in transit.

	Money feeder_share = 0; ///< Value of feeder pickup to be paid for on delivery of cargo.

	TileIndex source_xy = INVALID_TILE; ///< The origin of the cargo.
	Coord2D<int16_t> travelled{0, 0}; ///< If cargo is in station: the vector from the unload tile to the source tile. If in vehicle: an intermediate value.

	Source source{Source::Invalid, SourceType::Industry}; ///< Source of the cargo
	uint64_t company_supply_order = 0; ///< Optional exact legacy receipt ID; zero includes valid non-paying A CompanySupply without any refund link.
	uint8_t company_supply_shipped = 0; ///< Literal original0/1 departure evidence; retain malformed bytes until transport validation, never coerce them to bool.

#ifdef WITH_ASSERT
	bool in_vehicle = false; ///< NOSAVE: Whether this cargo is in a vehicle or not.
#endif /* WITH_ASSERT */

	StationID first_station = StationID::Invalid(); ///< The station where the cargo came from first.
	StationID next_hop = StationID::Invalid(); ///< Station where the cargo wants to go next.

	/** The CargoList caches, thus needs to know about it. */
	template <class Tinst, class Tcont> friend class CargoList;
	friend class VehicleCargoList;
	friend class StationCargoList;
	friend class HubCargoTransfer;
	/** We want this to be saved, right? */
	friend SaveLoadTable GetCargoPacketDesc();
	/** The sole afterload receipt owner may normalize original B purpose only after the complete shared custody check succeeds.
	 * @param family_required Whether the original source requires its complete paid-history family.
	 * @return Whole original-book admission; a refusal performs no packet-purpose normalization.
	 */
	friend bool AcceptLoadedSurfaceSupplyOrders(bool family_required);
	/** Only the original native transport installs a fully decoded version-admitted source; packet identity and all other fields remain original. */
	friend struct CAPAChunkHandler;
public:
	/** @return Portable digest of every actual saved packet scalar, including permanent purpose, U64receipt link and literal departure; excludes pointers and debug-only custody caches. */
	static uint64_t ComputePoolDigest();
	/** @return Exact optional uint64 legacy receipt link; zero never changes the source's permanent paying/non-paying purpose. */
	uint64_t GetCompanySupplyOrder() const { return this->company_supply_order; }
	/** @return Whether these exact linked units have already left refundable original-source custody. */
	bool HasCompanySupplyShipped() const { return this->company_supply_shipped; }
	/** @return Literal original departure scalar for transport validation before any boolean interpretation; valid live packets hold only0/1. */
	uint8_t GetCompanySupplyShippedWire() const { return this->company_supply_shipped; }
	/** Restore original packet evidence once, before complete same-file custody validation or native publication.
	 * @param order Exact nonzero historical receipt identity, losslessly widened from B; never a newly purchased native right.
	 * @param shipped Original saved departure flag, not a new dispatch event or inferred current location.
	 * @return Whether structurally compatible evidence was attached; relinking, zero and ordinary-paying sources refuse unchanged.
	 * This does not validate an order reference or update its counters. The complete candidate custody join must resolve it before installation.
	 */
	bool RestoreCompanySupplyOrder(uint64_t order, bool shipped)
	{
		// Original metadata may describe only already non-paying bought cargo.
		// It cannot relabel ordinary freight or rewrite an existing refund link.
		if (order == 0 || this->company_supply_order != 0 || this->company_supply_shipped || !this->source.IsCompanySupply()) return false;
		this->company_supply_order = order;
		this->company_supply_shipped = shipped;
		return true;
	}
	/** Check a selected fragment before native splitting or cache mutation.
	 * @param quantity Nonzero selected units, at most this packet's physical count.
	 * @return Whether ordinary/unlinked purpose or actual installed legacy history permits departure without mutation.
	 */
	bool CanMarkCompanySupplyShipped(uint16_t quantity) const;
	/** Commit this actual fragment's first departure through the sole installed receipt owner.
	 * @return Complete accounting/flag update or unchanged refusal; already-shipped and unlinked cargo never increment history.
	 * Call only after movement admission and before native cache transfer; original metadata restoration is a different operation.
	 */
	bool MarkCompanySupplyShipped();

	/** Maximum number of items in a single cargo packet. */
	static const uint16_t MAX_COUNT = UINT16_MAX;

	/** Create empty native load storage. @param index Original pool identity; semantic fields are filled and validated by the loader. */
	CargoPacket(CargoPacketID index);
	/** Create actual source cargo with no legacy link. @param index Native pool identity. @param first_station Original receiving station. @param count Nonzero whole units. @param source Permanent native provenance/purpose. */
	CargoPacket(CargoPacketID index, StationID first_station, uint16_t count, Source source);
	/** Restore historical partial transport. @param index Native pool identity. @param count Nonzero whole units. @param periods_in_transit Original age. @param first_station Original source station. @param source_xy Original pickup tile. @param feeder_share Actual accrued feeder credit. */
	CargoPacket(CargoPacketID index, uint16_t count, uint16_t periods_in_transit, StationID first_station, TileIndex source_xy, Money feeder_share);
	/** Copy an actual fragment with all purpose/link/departure evidence. @param index New native pool identity. @param count Nonzero split units. @param feeder_share Original proportional credit. @param original Actual source fragment, whose metadata remains authoritative. */
	CargoPacket(CargoPacketID index, uint16_t count, Money feeder_share, CargoPacket &original);

	/** Destroy the packet. */
	~CargoPacket() { }

	/** Split native quantity and credit while preserving all evidence. @param new_size Whole units to split. @return New fragment, or nullptr on pool exhaustion with the original unchanged. */
	CargoPacket *Split(uint new_size);
	/** @param mineral Original same-file/live-body mineral validated before retirement. @return Whether this packet's exact legacy asteroid ID was upgraded in place, or an already typed origin agrees. Quantity, pool identity, credit, travel and ownership stay unchanged; ordinary/unknown/malformed sources refuse. Called by controlled migration tests now, by the matching loader in Group21. */
	bool UpgradeLegacyAsteroidSource(CargoLabel mineral);
	/** Merge an already eligible native fragment. @param cp Fragment consumed after the owning list's provenance/state and MAX_COUNT checks. */
	void Merge(CargoPacket *cp);
	/** Consume part of this native fragment. @param count Whole units to remove, strictly less than the packet count; remaining purpose/history is unchanged. */
	void Reduce(uint count);

	/**
	 * Sets the station where the packet is supposed to go next.
	 * @param next_hop Next station the packet should go to.
	 */
	void SetNextHop(StationID next_hop)
	{
		this->next_hop = next_hop;
	}

	/**
	 * Update for the cargo being loaded on this tile.
	 *
	 * When a CargoPacket is created, it is moved to a station. But at that
	 * moment in time it is not known yet at which tile the cargo will be
	 * picked up. As this tile is used for payment information, we delay
	 * setting the source_xy till first pickup, getting a better idea where
	 * a cargo started from.
	 *
	 * Further more, we keep track of the amount of tiles the cargo moved
	 * inside a vehicle. This is used in GetDistance() below.
	 *
	 * @param tile Tile the cargo is being picked up from.
	 */
	void UpdateLoadingTile(TileIndex tile)
	{
		if (this->source_xy == INVALID_TILE) {
			this->source_xy = tile;
		}

#ifdef WITH_ASSERT
		assert(!this->in_vehicle);
		this->in_vehicle = true;
#endif /* WITH_ASSERT */

		/* We want to calculate the vector from tile-unload to tile-load. As
		 * we currently only know the latter, add it. When we know where we unload,
		 * we subtract is, giving us our vector (unload - load). */
		this->travelled.x += TileX(tile);
		this->travelled.y += TileY(tile);
	}

	/**
	 * Update for the cargo being unloaded on this tile.
	 *
	 * @param tile Tile the cargo is being dropped off at.
	 */
	void UpdateUnloadingTile(TileIndex tile)
	{
#ifdef WITH_ASSERT
		assert(this->in_vehicle);
		this->in_vehicle = false;
#endif /* WITH_ASSERT */

		this->travelled.x -= TileX(tile);
		this->travelled.y -= TileY(tile);
	}

	/**
	 * Adds some feeder share to the packet.
	 * @param new_share Feeder share to be added.
	 */
	void AddFeederShare(Money new_share)
	{
		this->feeder_share += new_share;
	}

	/**
	 * Gets the number of 'items' in this packet.
	 * @return Item count.
	 */
	inline uint16_t Count() const
	{
		return this->count;
	}

	/**
	 * Gets the amount of money already paid to earlier vehicles in
	 * the feeder chain.
	 * @return Feeder share.
	 */
	inline Money GetFeederShare() const
	{
		return this->feeder_share;
	}

	/**
	 * Gets part of the amount of money already paid to earlier vehicles in
	 * the feeder chain.
	 * @param part Amount of cargo to get the share for.
	 * @return Feeder share for the given amount of cargo.
	 */
	inline Money GetFeederShare(uint part) const
	{
		return this->feeder_share * part / static_cast<uint>(this->count);
	}

	/**
	 * Gets the number of cargo aging periods this cargo has been in transit.
	 * By default a period is 2.5 days (CARGO_AGING_TICKS = 185 ticks), however
	 * vehicle NewGRFs can override the length of the cargo aging period. The
	 * value is capped at UINT16_MAX.
	 * @return Length this cargo has been in transit.
	 */
	inline uint16_t GetPeriodsInTransit() const
	{
		return this->periods_in_transit;
	}

	/**
	 * Gets the source of the packet for subsidy purposes.
	 * @return The source.
	 */
	inline Source GetSource() const
	{
		return this->source;
	}

	/** Immutable production/pickup tile, including the actual extraction plane for orbital cargo. */
	TileIndex GetSourceTile() const { return this->source_xy; }

	/**
	 * Gets the ID of the station where the cargo was loaded for the first time.
	 * @return StationID.
	 */
	inline StationID GetFirstStation() const
	{
		return this->first_station;
	}

	/**
	 * Get the current distance the cargo has traveled.
	 *
	 * @param current_tile Current tile of the cargo.
	 * @return uint The distance (in tiles) traveled.
	 */
	inline uint GetDistance(TileIndex current_tile) const
	{
		assert(this->source_xy != INVALID_TILE);
#ifdef WITH_ASSERT
		assert(this->in_vehicle);
#endif /* WITH_ASSERT */

		/* Distance is always requested when the cargo is still inside the
		 * vehicle. So first finish the calculation for travelled to
		 * become a vector. */
		const int travelled_x = this->travelled.x - static_cast<int>(TileX(current_tile));
		// Keep the saved vector representation. While aboard, same-layer station unload/load pairs telescope its stacked offset to the origin's layer.
		// Remove that offset before comparing plane coordinates, so a vertical portal leg cannot pay for a free move within a station.
		const int source_layer_offset = static_cast<int>(TileY(this->source_xy) - TilePlaneY(this->source_xy));
		const int travelled_plane_y = static_cast<int16_t>(this->travelled.y - source_layer_offset);
		const int travelled_y = travelled_plane_y - static_cast<int>(TilePlaneY(current_tile));

		/* Cargo-movement is a vector that indicates how much the cargo has
		 * actually traveled in a vehicle. This is the distance you get paid
		 * for. However, one could construct a route where this vector would
		 * be really long. To not overpay the player, cap out at the distance
		 * between source and destination.
		 *
		 * This way of calculating is to counter people moving cargo for free
		 * and instantly in stations, where you deliver it in one part of the
		 * station and pick it up in another. By using the actual distance
		 * traveled in a vehicle, using this trick doesn't give you more money.
		 *
		 * However, especially in large networks with large transfer station,
		 * etc, one could actually make the route a lot longer. In that case,
		 * use the actual distance between source and destination.
		 */

		uint distance_travelled = abs(travelled_x) + abs(travelled_y);
		uint distance_source_dest = DistanceManhattan(this->source_xy, current_tile);
		return std::min(distance_travelled, distance_source_dest);
	}

	/**
	 * Gets the ID of station the cargo wants to go next.
	 * @return Next station for this packets.
	 */
	inline StationID GetNextHop() const
	{
		return this->next_hop;
	}

	static void InvalidateAllFrom(Source src);
	static void InvalidateAllFrom(StationID sid);
	static void AfterLoad();
};

/**
 * Simple collection class for a list of cargo packets.
 * @tparam Tinst Actual instantiation of this cargo list.
 */
template <class Tinst, class Tcont>
class CargoList {
public:
	/** The iterator for our container. */
	typedef typename Tcont::iterator Iterator;
	/** The reverse iterator for our container. */
	typedef typename Tcont::reverse_iterator ReverseIterator;
	/** The const iterator for our container. */
	typedef typename Tcont::const_iterator ConstIterator;
	/** The const reverse iterator for our container. */
	typedef typename Tcont::const_reverse_iterator ConstReverseIterator;

	/** Kind of actions that could be done with packets on move. */
	enum class MoveToAction : uint8_t {
		Transfer, ///< Transfer the cargo to the station.
		Deliver, ///< Deliver the cargo to some town or industry.
		Keep, ///< Keep the cargo in the vehicle.
		Load, ///< Load the cargo from the station.
		End, ///< End marker.
	};

protected:
	uint count = 0; ///< Cache for the number of cargo entities.
	uint64_t cargo_periods_in_transit = 0; ///< Cache for the sum of number of cargo aging periods in transit of each entity; comparable to man-hours.

	Tcont packets{}; ///< The cargo packets in this list.

	void AddToCache(const CargoPacket *cp);

	void RemoveFromCache(const CargoPacket *cp, uint count);

	static bool TryMerge(CargoPacket *cp, CargoPacket *icp);

public:
	/** Create the cargo list. */
	CargoList() {}

	~CargoList();

	void OnCleanPool();

	/**
	 * Returns a pointer to the cargo packet list (so you can iterate over it etc).
	 * @return Pointer to the packet list.
	 */
	inline const Tcont *Packets() const
	{
		return &this->packets;
	}

	/**
	 * Returns average number of cargo aging periods in transit for a cargo entity.
	 * @return The before mentioned number.
	 */
	inline uint PeriodsInTransit() const
	{
		return this->count == 0 ? 0 : this->cargo_periods_in_transit / this->count;
	}

	void InvalidateCache();
};

typedef std::list<CargoPacket *> CargoPacketList;

/**
 * CargoList that is used for vehicles.
 */
class VehicleCargoList : public CargoList<VehicleCargoList, CargoPacketList> {
protected:
	/** The (direct) parent of this class. */
	typedef CargoList<VehicleCargoList, CargoPacketList> Parent;

	Money feeder_share; ///< Cache for the feeder share.
	EnumIndexArray<uint, MoveToAction, MoveToAction::End> action_counts{}; ///< Counts of cargo to be transferred, delivered, kept and loaded.

	template <class Taction>
	void ShiftCargo(Taction action);

	template <class Taction>
	void PopCargo(Taction action);

	/**
	 * Assert that the designation counts add up.
	 */
	inline void AssertCountConsistency() const
	{
		assert(this->action_counts[MoveToAction::Keep] +
				this->action_counts[MoveToAction::Deliver] +
				this->action_counts[MoveToAction::Transfer] +
				this->action_counts[MoveToAction::Load] == this->count);
	}

	void AddToCache(const CargoPacket *cp);
	void RemoveFromCache(const CargoPacket *cp, uint count);

	void AddToMeta(const CargoPacket *cp, MoveToAction action);
	void RemoveFromMeta(const CargoPacket *cp, MoveToAction action, uint count);
	uint TryRemoveFromMeta(const CargoPacket *cp, MoveToAction action, uint count);

	static MoveToAction ChooseAction(const CargoPacket *cp, StationID cargo_next,
			StationID current_station, bool accepted, std::span<const StationID> next_station);

public:
	/** The station cargo list needs to control the unloading. */
	friend class StationCargoList;
	/** The super class ought to know what it's doing. */
	friend class CargoList<VehicleCargoList, CargoPacketList>;
	/* So we can use private/protected variables in the saveload code */
	friend class SlVehicleCommon;

	friend class CargoShift;
	friend class CargoTransfer;
	friend class CargoDelivery;
	template <class Tsource>
	friend class CargoRemoval;
	friend class CargoReturn;
	friend class VehicleCargoReroute;
	friend class HubCargoTransfer;

	/**
	 * Returns the first station of the first cargo packet in this list.
	 * @return The before mentioned station.
	 */
	inline StationID GetFirstStation() const
	{
		return this->count == 0 ? StationID::Invalid() : this->packets.front()->first_station;
	}

	/**
	 * Returns total sum of the feeder share for all packets.
	 * @return The before mentioned number.
	 */
	inline Money GetFeederShare() const
	{
		return this->feeder_share;
	}

	/**
	 * Returns the amount of cargo designated for a given purpose.
	 * @param action Action the cargo is designated for.
	 * @return Amount of cargo designated for the given action.
	 */
	inline uint ActionCount(MoveToAction action) const
	{
		return this->action_counts[action];
	}

	/**
	 * Returns sum of cargo on board the vehicle (ie not only
	 * reserved).
	 * @return Cargo on board the vehicle.
	 */
	inline uint StoredCount() const
	{
		return this->count - this->action_counts[MoveToAction::Load];
	}

	/**
	 * Returns sum of cargo, including reserved cargo.
	 * @return Sum of cargo.
	 */
	inline uint TotalCount() const
	{
		return this->count;
	}

	/**
	 * Returns sum of reserved cargo.
	 * @return Sum of reserved cargo.
	 */
	inline uint ReservedCount() const
	{
		return this->action_counts[MoveToAction::Load];
	}

	/**
	 * Returns sum of cargo to be moved out of the vehicle at the current station.
	 * @return Cargo to be moved.
	 */
	inline uint UnloadCount() const
	{
		return this->action_counts[MoveToAction::Transfer] + this->action_counts[MoveToAction::Deliver];
	}

	/**
	 * Returns the sum of cargo to be kept in the vehicle at the current station.
	 * @return Cargo to be kept or loaded.
	 */
	inline uint RemainingCount() const
	{
		return this->action_counts[MoveToAction::Keep] + this->action_counts[MoveToAction::Load];
	}

	void Append(CargoPacket *cp, MoveToAction action = MoveToAction::Keep);

	void AgeCargo();

	void InvalidateCache();

	bool Stage(bool accepted, StationID current_station, std::span<const StationID> next_station, OrderUnloadType unload_type, const GoodsEntry *ge, CargoType cargo, CargoPayment *payment, TileIndex current_tile);

	/**
	 * Marks all cargo in the vehicle as to be kept. This is mostly useful for
	 * loading old savegames. When loading is aborted the reserved cargo has
	 * to be returned first.
	 */
	inline void KeepAll()
	{
		this->action_counts[MoveToAction::Deliver] = 0;
		this->action_counts[MoveToAction::Transfer] = 0;
		this->action_counts[MoveToAction::Load] = 0;
		this->action_counts[MoveToAction::Keep] = this->count;
	}

	/* Methods for moving cargo around. First parameter is always maximum
	 * amount of cargo to be moved. Second parameter is destination (if
	 * applicable), return value is amount of cargo actually moved. */

	template <MoveToAction Tfrom, MoveToAction Tto>
	uint Reassign(uint max_move);
	uint Return(uint max_move, StationCargoList *dest, StationID next_station, TileIndex current_tile);
	uint Unload(uint max_move, StationCargoList *dest, CargoType cargo, CargoPayment *payment, TileIndex current_tile);
	uint Shift(uint max_move, VehicleCargoList *dest);
	uint Truncate(uint max_move = UINT_MAX);
	uint Reroute(uint max_move, VehicleCargoList *dest, StationID avoid, StationID avoid2, const GoodsEntry *ge);

	/**
	 * Are the two CargoPackets mergeable in the context of
	 * a list of CargoPackets for a Vehicle?
	 * @param cp1 First CargoPacket.
	 * @param cp2 Second CargoPacket.
	 * @return True if they are mergeable.
	 */
	static bool AreMergable(const CargoPacket *cp1, const CargoPacket *cp2)
	{
		// Combining distinct paid histories or departure states would either
		// erase finite refund evidence or let returned cargo regain eligibility.
		return cp1->source_xy == cp2->source_xy &&
				cp1->company_supply_order == cp2->company_supply_order && cp1->company_supply_shipped == cp2->company_supply_shipped &&
				cp1->periods_in_transit == cp2->periods_in_transit &&
				cp1->first_station == cp2->first_station &&
				cp1->source == cp2->source;
	}
};

typedef MultiMap<StationID, CargoPacket *> StationCargoPacketMap;
typedef std::map<StationID, uint> StationCargoAmountMap;

/**
 * CargoList that is used for stations.
 */
class StationCargoList : public CargoList<StationCargoList, StationCargoPacketMap> {
protected:
	/** The (direct) parent of this class. */
	typedef CargoList<StationCargoList, StationCargoPacketMap> Parent;

	uint reserved_count; ///< Amount of cargo being reserved for loading.
	/** Remove original station cargo through the same fair-removal owner, optionally protecting bought material.
	 * @param max_move Maximum eligible whole units to discard.
	 * @param cargo_per_source Optional original-source accounting for the ordinary discard path.
	 * @param protect_company_supplies Keep non-paying bought stock out of both discard selection and its lottery weight.
	 * @return Exactly removed eligible native units; no second cargo list or stock account is introduced.
	 */
	uint TruncateImpl(uint max_move, StationCargoAmountMap *cargo_per_source, bool protect_company_supplies);

public:
	/** The super class ought to know what it's doing. */
	friend class CargoList<StationCargoList, StationCargoPacketMap>;
	/* So we can use private/protected variables in the saveload code */
	friend class SlStationGoods;

	friend class CargoLoad;
	friend class CargoTransfer;
	template <class Tsource>
	friend class CargoRemoval;
	friend class CargoReservation;
	friend class CargoReturn;
	friend class StationCargoReroute;
	friend class HubCargoTransfer;

	static void InvalidateAllFrom(Source src);

	template <class Taction>
	bool ShiftCargo(Taction &action, StationID next);

	template <class Taction>
	uint ShiftCargo(Taction action, std::span<const StationID> next, bool include_invalid);

	void Append(CargoPacket *cp, StationID next);

	/**
	 * Check for cargo headed for a specific station.
	 * @param next Station the cargo is headed for.
	 * @return If there is any cargo for that station.
	 */
	inline bool HasCargoFor(std::span<const StationID> next) const
	{
		for (const StationID &station : next) {
			if (this->packets.find(station) != this->packets.end()) return true;
		}
		/* Packets for StationID::Invalid() can go anywhere. */
		return this->packets.find(StationID::Invalid()) != this->packets.end();
	}

	/**
	 * Returns first station of the first cargo packet in this list.
	 * @return The before mentioned station.
	 */
	inline StationID GetFirstStation() const
	{
		return this->count == 0 ? StationID::Invalid() : this->packets.begin()->second.front()->first_station;
	}

	/**
	 * Returns sum of cargo still available for loading at the station.
	 * (i.e. not counting cargo which is already reserved for loading)
	 * @return Cargo on board the vehicle.
	 */
	inline uint AvailableCount() const
	{
		return this->count;
	}

	/**
	 * Returns sum of cargo reserved for loading onto vehicles.
	 * @return Cargo reserved for loading.
	 */
	inline uint ReservedCount() const
	{
		return this->reserved_count;
	}

	/**
	 * Returns total count of cargo at the station, including
	 * cargo which is already reserved for loading.
	 * @return Total cargo count.
	 */
	inline uint TotalCount() const
	{
		return this->count + this->reserved_count;
	}

	/* Methods for moving cargo around. First parameter is always maximum
	 * amount of cargo to be moved. Second parameter is destination (if
	 * applicable), return value is amount of cargo actually moved. */

	uint Reserve(uint max_move, VehicleCargoList *dest, std::span<const StationID> next, TileIndex current_tile);
	uint Load(uint max_move, VehicleCargoList *dest, std::span<const StationID> next, TileIndex current_tile);
	/** Count exact never-shipped linked supplies actually present in this native waiting queue.
	 * @param order Nonzero exact uint64 legacy identity; zero never selects ordinary or unlinked A stock.
	 * @return Whole eligible physical units across routing buckets; no reservation, history or refund is granted.
	 * Returned shipped, carrier-reserved and unrelated-purpose/order fragments are excluded by their actual owner/evidence.
	 */
	uint64_t CountUnshippedCompanySupply(uint64_t order) const;
	/** Remove exactly the recaptured never-shipped linked stock through native caches before a separately admitted refund.
	 * @param order Exact nonzero original legacy identity, already admitted by the caller's canonical receipt/source checks.
	 * @param expected Complete physical quantity from current cancellation preflight; a changed count refuses unchanged.
	 * @return Complete removal, including a legitimate zero-eligible selection, or unchanged refusal. No money/history is modified.
	 * This helper alone grants no cancellation or settlement authority; only the canonical command may close/refund a valid receipt.
	 */
	bool RemoveUnshippedCompanySupply(uint64_t order, uint16_t expected);
	uint Truncate(uint max_move = UINT_MAX, StationCargoAmountMap *cargo_per_source = nullptr);
	/** Discard abandoned transport offers while preserving actual bought company material in the same native station container.
	 * @param max_move Maximum ordinary offered units to discard; owned CompanySupply does not count toward the requested removal.
	 * @return Exactly removed ordinary units; explicit destruction still uses ordinary Truncate/destructor semantics.
	 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD; rating hook activation remains13CCD.
	 */
	uint TruncateAbandonedOffers(uint max_move);
	uint Reroute(uint max_move, StationCargoList *dest, StationID avoid, StationID avoid2, const GoodsEntry *ge);

	/**
	 * Are the two CargoPackets mergeable in the context of
	 * a list of CargoPackets for a Station?
	 * @param cp1 First CargoPacket.
	 * @param cp2 Second CargoPacket.
	 * @return True if they are mergeable.
	 */
	static bool AreMergable(const CargoPacket *cp1, const CargoPacket *cp2)
	{
		// The station queue keeps the same historical boundary as a carrier;
		// a shared cargo label, source and age cannot merge different refund rights.
		return cp1->source_xy == cp2->source_xy &&
				cp1->company_supply_order == cp2->company_supply_order && cp1->company_supply_shipped == cp2->company_supply_shipped &&
				cp1->periods_in_transit == cp2->periods_in_transit &&
				cp1->first_station == cp2->first_station &&
				cp1->source == cp2->source;
	}
};

#endif /* CARGOPACKET_H */
