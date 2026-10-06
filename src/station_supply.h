/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file station_supply.h Exact finite industry-to-station admission owned by station_cmd.cpp. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 8 */

#ifndef STATION_SUPPLY_H
#define STATION_SUPPLY_H

#include "station_type.h"
#include "cargo_type.h"
#include "source_type.h"
#include "map_layer.h"

/** Native finite supply stops topping up a cargo queue at this explicit whole-unit bound; existing cargo is never discarded. */
static constexpr uint32_t EXACT_STATION_SUPPLY_CAPACITY = UINT16_MAX;

/** Transient pure station admission; it carries no cargo, packet allocation or saved fractional credit. */
struct ExactStationSupplyTicket {
	StationID station = StationID::Invalid(); ///< Actual receiving station.
	CargoType cargo = INVALID_CARGO; ///< Actual runtime slot resolved by the producer.
	Source source{Source::Invalid, SourceType::Industry}; ///< Existing physical industry provenance.
	MapLayer layer = MapLayer::Invalid; ///< Producer's actual world layer.
	Owner exclusivity = INVALID_OWNER; ///< Actual exclusive recipient, if any.
	uint32_t waiting = 0; ///< Expected whole station quantity, including loading reservations.
	uint8_t rating = 0; ///< Expected station rating; zero admits no new finite supply.
	uint16_t amount = 0; ///< Whole units fitting one real packet and the queue; no rating fraction is credited.
};

/**
 * Prepare exact packet/storage admission for a native industry or explicitly validated original steelworks and a genuinely associated same-layer station.
 * @param station Actual destination pool identity.
 * @param cargo Runtime output cargo slot.
 * @param requested Whole physical units the producer is offering, bounded again at commit.
 * @param source Actual industry provenance; no local or translated-name identity is accepted.
 * @param layer Producer's actual layer, checked against both objects.
 * @param exclusivity Actual exclusive recipient or INVALID_OWNER.
 * @return Pure ticket; zero units when service, layer, ownership, packet or storage admission fails.
 */
ExactStationSupplyTicket PrepareExactStationSupply(StationID station, CargoType cargo, uint32_t requested, Source source, MapLayer layer, Owner exclusivity);

/**
 * Revalidate actual station/source/service and append only complete integer units through the existing packet owner.
 * @param ticket Prepared transient admission; stale or edited tickets cannot supply stock.
 * @return Exactly the whole station supply committed, or zero with no station/fraction mutation; caller debits its source by this receipt.
 */
uint32_t CommitExactStationSupply(const ExactStationSupplyTicket &ticket);

#endif /* STATION_SUPPLY_H */
