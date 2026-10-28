/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file cargopacket_sl.cpp Code handling saving and loading of cargo packets.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; detached same-file descent quantity admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB; identical native/copied non-paying purpose admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21; full-width typed asteroid origins.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 4 and 15; one CAPA codec carrying both supply and asteroid columns, gated by capability.
 */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "cargo_source_sl.h"
#include "../space.h"
#include "../fios.h"
#include "../surface_supply_order_runtime.h"
#include "compat/cargopacket_sl_compat.h"

#include "../vehicle_base.h"
#include "../station_base.h"

#include "../safeguards.h"

static CargoSourceAsteroidWire _cargo_source_asteroid_wire; ///< Per-row transport scratch only, reset before every native/copied row; no optional host representation reaches SlObject.

/**
 * Savegame conversion for cargopackets.
 */
/* static */ void CargoPacket::AfterLoad()
{
	if (IsSavegameVersionBefore(SaveLoadVersion::CargoSourceTile)) {
		/* If we remove a station while cargo from it is still en route, payment calculation will assume
		 * 0, 0 to be the source of the cargo, resulting in very high payments usually. v->source_xy
		 * stores the coordinates, preserving them even if the station is removed. However, if a game is loaded
		 * where this situation exists, the cargo-source information is lost. in this case, we set the source
		 * to the current tile of the vehicle to prevent excessive profits
		 */
		for (const Vehicle *v : Vehicle::Iterate()) {
			const CargoPacketList *packets = v->cargo.Packets();
			for (VehicleCargoList::ConstIterator it(packets->begin()); it != packets->end(); it++) {
				CargoPacket *cp = *it;
				cp->source_xy = Station::IsValidID(cp->first_station) ? Station::Get(cp->first_station)->xy : v->tile;
			}
		}

		/* Store position of the station where the goods come from, so there
		 * are no very high payments when stations get removed. However, if the
		 * station where the goods came from is already removed, the source
		 * information is lost. In that case we set it to the position of this
		 * station */
		for (Station *st : Station::Iterate()) {
			for (GoodsEntry &ge : st->goods) {
				if (!ge.HasData()) continue;
				const StationCargoPacketMap *packets = ge.GetData().cargo.Packets();
				for (StationCargoList::ConstIterator it(packets->begin()); it != packets->end(); it++) {
					CargoPacket *cp = *it;
					cp->source_xy = Station::IsValidID(cp->first_station) ? Station::Get(cp->first_station)->xy : st->xy;
				}
			}
		}
	}

	if (IsSavegameVersionBefore(SaveLoadVersion::CompanyServiceIntervals)) {
		/* CargoPacket's source should be either StationID::Invalid() or a valid station */
		for (CargoPacket *cp : CargoPacket::Iterate()) {
			if (!Station::IsValidID(cp->first_station)) cp->first_station = StationID::Invalid();
		}
	}

	if (!IsSavegameVersionBefore(SaveLoadVersion::CargoPackets)) {
		/* Only since version 68 we have cargo packets. Savegames from before used
		 * 'new CargoPacket' + cargolist.Append so their caches are already
		 * correct and do not need rebuilding. */
		for (Vehicle *v : Vehicle::Iterate()) v->cargo.InvalidateCache();

		for (Station *st : Station::Iterate()) {
			for (GoodsEntry &ge : st->goods) {
				if (!ge.HasData()) continue;
				ge.GetData().cargo.InvalidateCache();
			}
		}
	}

	if (IsSavegameVersionBefore(SaveLoadVersion::CargoReservation)) {
		for (Vehicle *v : Vehicle::Iterate()) v->cargo.KeepAll();
	}

	/* Before this version, we didn't track how far cargo actually traveled in vehicles. Make best-effort estimates of this. */
	if (IsSavegameVersionBefore(SaveLoadVersion::CargoTravelled)) {
		/* Update the cargo-traveled in stations as if they arrived from the source tile. */
		for (Station *st : Station::Iterate()) {
			for (GoodsEntry &ge : st->goods) {
				if (!ge.HasData()) continue;
				StationCargoList &cargo_list = ge.GetData().cargo;
				for (auto it = cargo_list.Packets()->begin(); it != cargo_list.Packets()->end(); ++it) {
					for (CargoPacket *cp : it->second) {
						if (cp->source_xy != INVALID_TILE && cp->source_xy != st->xy) {
							cp->travelled.x = TileX(cp->source_xy) - TileX(st->xy);
							cp->travelled.y = TileY(cp->source_xy) - TileY(st->xy);
						}
					}
				}
			}
		}

		/* Update the cargo-traveled in vehicles as if they were loaded at the source tile. */
		for (Vehicle *v : Vehicle::Iterate()) {
			for (auto it = v->cargo.Packets()->begin(); it != v->cargo.Packets()->end(); it++) {
				if ((*it)->source_xy != INVALID_TILE) {
					(*it)->UpdateLoadingTile((*it)->source_xy);
				}
			}
		}
	}

#ifdef WITH_ASSERT
	/* in_vehicle is a NOSAVE; it tells if cargo is in a vehicle or not. Restore the value in here. */
	for (Vehicle *v : Vehicle::Iterate()) {
		for (auto it = v->cargo.Packets()->begin(); it != v->cargo.Packets()->end(); it++) {
			(*it)->in_vehicle = true;
		}
	}
#endif /* WITH_ASSERT */
}

/**
 * Wrapper function to get the CargoPacket's internal structure while
 * some of the variables itself are private.
 * @return the saveload description for CargoPackets.
 */
SaveLoadTable GetCargoPacketDesc()
{
	static const SaveLoad _cargopacket_desc[] = {
		SaveLoad::Variable<VarFileType::U16>("source", SLE_OBJECT_ADDRESS(CargoPacket, first_station)),
		SaveLoad::Variable<VarFileType::U32>("source_xy", SLE_OBJECT_ADDRESS(CargoPacket, source_xy)),
		SaveLoad::Variable<VarFileType::U32>("loaded_at_xy", SLE_OBJECT_ADDRESS(CargoPacket, next_hop), SaveLoadVersion::MinVersion, SaveLoadVersion::RemoveLoadedAtXY),
		SaveLoad::Variable<VarFileType::U16>("loaded_at_xy", SLE_OBJECT_ADDRESS(CargoPacket, next_hop), SaveLoadVersion::RemoveLoadedAtXY),
		SaveLoad::Variable<VarFileType::U16>("count", SLE_OBJECT_ADDRESS(CargoPacket, count)),
		SaveLoad::Variable<VarFileType::U8>("days_in_transit", SLE_OBJECT_ADDRESS(CargoPacket, periods_in_transit), SaveLoadVersion::MinVersion, SaveLoadVersion::MoreCargoAge),
		SaveLoad::Variable<VarFileType::U16>("days_in_transit", SLE_OBJECT_ADDRESS(CargoPacket, periods_in_transit), SaveLoadVersion::MoreCargoAge, SaveLoadVersion::PeriodsInTransitRename),
		SaveLoad::Variable<VarFileType::U16>("periods_in_transit", SLE_OBJECT_ADDRESS(CargoPacket, periods_in_transit), SaveLoadVersion::PeriodsInTransitRename),
		SaveLoad::Variable<VarFileType::I64>("feeder_share", SLE_OBJECT_ADDRESS(CargoPacket, feeder_share)),
		SaveLoad::Variable<VarFileType::U8>("source_type", SLE_OBJECT_ADDRESS(CargoPacket, source.type), SaveLoadVersion::RemoveSubsidyStationBinding),
		SaveLoad::Variable<VarFileType::U16>("source_id", SLE_OBJECT_ADDRESS(CargoPacket, source.id), SaveLoadVersion::RemoveSubsidyStationBinding),
		// Original B links are U32; reserve the upper boundary before merged441
		// introduces canonical U64 under the same key. A439/440 never wrote these fields.
		SaveLoad::Variable<VarFileType::U32>("company_supply_order", SLE_OBJECT_ADDRESS(CargoPacket, company_supply_order), SaveLoadVersion::MinVersion, SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
		SaveLoad::Variable<VarFileType::Bool>("company_supply_shipped", SLE_OBJECT_ADDRESS(CargoPacket, company_supply_shipped), SaveLoadVersion::MinVersion, SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
		// Merged canonical links retain U64identities and literal0/1departure bytes.
		SaveLoad::Variable<VarFileType::U64>("company_supply_order", SLE_OBJECT_ADDRESS(CargoPacket, company_supply_order), SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
		SaveLoad::Variable<VarFileType::U8>("company_supply_shipped", SLE_OBJECT_ADDRESS(CargoPacket, company_supply_shipped), SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
		// Typed asteroid origin (body, mineral) through per-row scratch. Only sources with the
		// capability carry it: Space fixes 441-445 and merged 446, never merged 441 (m2o28 Group 15).
		SaveLoad::Variable<VarFileType::U32>("asteroid_body", SLE_GLOBAL_ADDRESS(_cargo_source_asteroid_wire.body)).WithCapability(SaveCapability::AsteroidCargoOrigins),
		SaveLoad::Variable<VarFileType::U32>("asteroid_mineral", SLE_GLOBAL_ADDRESS(_cargo_source_asteroid_wire.mineral)).WithCapability(SaveCapability::AsteroidCargoOrigins),
		SaveLoad::Variable<VarFileType::I16>("travelled.x", SLE_OBJECT_ADDRESS(CargoPacket, travelled.x), SaveLoadVersion::CargoTravelled),
		SaveLoad::Variable<VarFileType::I16>("travelled.y", SLE_OBJECT_ADDRESS(CargoPacket, travelled.y), SaveLoadVersion::CargoTravelled),
	};
	return _cargopacket_desc;
}

/** Require the candidate table's actual quantity, purpose and credit fields in both loading modes.
 * @param desc Decoded candidate-file table; this grants no live packet or copied default the authority to replace an omitted field.
 */
static void CheckCargoPacketSourceFields(const std::vector<SaveLoad> &desc)
{
	if (IsSavegameVersionBefore(SaveLoadVersion::TableChunks)) return;
	// Table-era packets already serialize all four fields. Refusing an omission prevents either loading mode from inventing paying provenance or silently dropping a bought packet's credit.
	for (const std::string_view field : {"count", "source_type", "source_id", "feeder_share"}) {
		if (std::ranges::count(desc, field, &SaveLoad::name) != 1) SlErrorCorrupt("Missing/duplicate native cargo packet quantity/source/credit field");
	}
	// A missing paid-purpose column is material evidence even for an empty
	// book/table. A's original wire has no such columns and cannot impersonate B.
	for (const std::string_view field : {"company_supply_order", "company_supply_shipped"}) {
		const auto count = std::ranges::count(desc, field, &SaveLoad::name);
		if (count != (SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersB) ? 1 : 0)) SlErrorCorrupt("Missing/contradictory supply packet field");
	}
	std::vector<std::string_view> fields; fields.reserve(desc.size());
	// Preserve every original header key, including unknown/skipped fields, so
	// an older file cannot borrow a newer asteroid payload through skip handling.
	for (const auto &field : desc) fields.push_back(field.name);
	if (!ValidateCargoPacketSourceSchema(fields, CargoSourceFormat::Current())) SlErrorCorrupt("Missing or wrong-era native cargo packet source fields");
}

/** Original native packet transport with a bounded read-only quantity/provenance view; preview never creates or publishes an inventory owner. */
struct CAPAChunkHandler : ChunkHandler {
	/** Register the original physical packet family without issuing a packet, cargo or income receipt. */
	CAPAChunkHandler() : ChunkHandler("CAPA", ChunkType::Table) {}

	/** Serialize each actual existing native packet and all of its original quantity/provenance fields. */
	void Save() const override
	{
		SlTableHeader(GetCargoPacketDesc());
		const auto writer = CargoSourceFormat::Current(); // The writer context, never a previously loaded file's lineage.

		// Serialize the sole original native packet pool; every container retains its own references and no transport copy becomes stock.
		for (CargoPacket *cp : CargoPacket::Iterate()) {
			if (!IsValidCargoPacketSource(cp->GetSource(), cp->GetFeederShare(), writer)) SlErrorCorrupt("Invalid native cargo source for saving");
			_cargo_source_asteroid_wire = EncodeCargoSourceAsteroid(cp->GetSource());
			SlSetArrayIndex(cp->index);
			SlObject(cp, GetCargoPacketDesc());
		}
	}

	/** Reconstruct the original native packet identities; containers subsequently bind their sole ownership through normal pointer fixing. */
	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(GetCargoPacketDesc(), _cargopacket_sl_compat);
		CheckCargoPacketSourceFields(slt);
		const auto source_format = CargoSourceFormat::Current(); // The bound lineage of this load attempt.

		int index;
		int previous_index = -1;

		// Native source identities reconstruct before container pointer fixing, preserving the original packet pool rather than issuing replacement cargo.
		while ((index = SlIterateArray()) != -1) {
			// Original identities must be bounded and strictly unique BEFORE
			// native allocation; corruption cannot crash a pool assertion or
			// replace another fragment's paid provenance/custody evidence.
			if (index <= previous_index || static_cast<uint32_t>(index) >= CargoPacketID::End().base()) SlErrorCorrupt("Invalid native cargo packet identity");
			previous_index = index;
			CargoPacket *cp = CargoPacket::CreateAtIndex(CargoPacketID(index));
			_cargo_source_asteroid_wire = {};
			SlObject(cp, slt);
			if (cp->Count() == 0) SlErrorCorrupt("Empty native cargo packet quantity");
			// The candidate file's own era, provenance and feeder credit decide admission; no live company or cargo lookup can convert bought supplies into paying freight.
			// One decoder admits the file's own era, provenance and credit, and attaches a typed asteroid origin only
			// where that era carries one; the controlled CAPA hook installs the result.
			const auto decoded = DecodeCargoPacketSource(cp->GetSource(), _cargo_source_asteroid_wire, cp->GetFeederShare(), source_format);
			if (!decoded) SlErrorCorrupt("Invalid cargo source type/identity/credit");
			cp->source = *decoded;
			// Preserve original B's invalid-Industry purpose until its whole-file
			// receipt/custody proof; an invalid departure byte cannot become true.
			if (cp->GetCompanySupplyShippedWire() > 1 || (cp->GetCompanySupplyOrder() == 0 && cp->HasCompanySupplyShipped())) SlErrorCorrupt("Invalid original supply departure/link");
		}
		if (SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersB) && !ObserveLoadedLegacyBSupplyPacketFields()) SlErrorCorrupt("Repeated original supply packet fields"); // Complete CAPA evidence is unique; a second family cannot replace provenance from the first.
	}
	/** Copy this file's original packet quantities/purpose/credit without allocating live cargo.
	 * @param length Framework chunk extent; pre-table sources retain their original skipped preview.
	 */
	void LoadCheck(size_t length) const override
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TableChunks)) { ChunkHandler::LoadCheck(length); return; }
		if (_load_check_data.native_cargo_packets_present) SlErrorCorrupt("Repeated copied native cargo packet family");
		static const SaveLoad preview[] = {
			SaveLoad::Variable<VarFileType::U16>("count", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, quantity)),
			SaveLoad::Variable<VarFileType::U8>("source_type", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, source.type)),
			SaveLoad::Variable<VarFileType::U16>("source_id", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, source.id)),
			SaveLoad::Variable<VarFileType::U32>("asteroid_body", SLE_GLOBAL_ADDRESS(_cargo_source_asteroid_wire.body)).WithCapability(SaveCapability::AsteroidCargoOrigins),
			SaveLoad::Variable<VarFileType::U32>("asteroid_mineral", SLE_GLOBAL_ADDRESS(_cargo_source_asteroid_wire.mineral)).WithCapability(SaveCapability::AsteroidCargoOrigins),
			SaveLoad::Variable<VarFileType::I64>("feeder_share", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, feeder_share)),
			SaveLoad::Variable<VarFileType::U32>("company_supply_order", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, company_supply_order), SaveLoadVersion::MinVersion, SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
			SaveLoad::Variable<VarFileType::Bool>("company_supply_shipped", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, company_supply_shipped), SaveLoadVersion::MinVersion, SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
			// Merged canonical links retain U64identities and literal0/1departure bytes.
			SaveLoad::Variable<VarFileType::U64>("company_supply_order", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, company_supply_order), SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
			SaveLoad::Variable<VarFileType::U8>("company_supply_shipped", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, company_supply_shipped), SaveLoadVersion::MergedLineage).WithCapability(SaveCapability::SurfaceSupplyOrdersB),
			// Same-file production tile and first station let the copied asteroid-origin check run without live pools.
			SaveLoad::Variable<VarFileType::U32>("source_xy", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, source_tile)),
			SaveLoad::Variable<VarFileType::U16>("source", SLE_OBJECT_ADDRESS(NativeCargoPacketPreview, first_station)),
		};
		const auto desc = SlTableHeader(preview);
		CheckCargoPacketSourceFields(desc);
		const auto source_format = CargoSourceFormat::Current(); // The same context as full loading.
		auto &rows = _load_check_data.native_cargo_packet_rows;
		// Native framing supplies actual indices and original provenance/credit; each detached row must pass the same source predicate as full loading, without allocating or borrowing a live packet.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			_cargo_source_asteroid_wire = {};
			NativeCargoPacketPreview row; row.packet = CargoPacketID{static_cast<uint32_t>(index)}; SlObject(&row, desc);
			// Bounded positive original quantities and strictly issued index order prevent an absent, duplicate or weightless fake packet from validating a descent reference.
			if (row.packet >= CargoPacketID::End() || row.quantity == 0 || rows.size() >= CargoPacketID::End().base() || (!rows.empty() && row.packet <= rows.back().packet)) SlErrorCorrupt("Invalid copied native cargo packet identity/quantity");
			// One decoder admits the file's own era, provenance and credit (the same predicate as full loading), and
			// attaches a typed asteroid origin only where that era carries one.
			const auto decoded = DecodeCargoPacketSource(row.source, _cargo_source_asteroid_wire, row.feeder_share, source_format);
			if (!decoded) SlErrorCorrupt("Invalid copied cargo source type/identity/credit");
			row.source = *decoded;
			// Shipped-without-link or a nonliteral boolean cannot be repaired from
			// whichever container happens to own the packet in this save.
			if (row.company_supply_shipped > 1 || (row.company_supply_order == 0 && row.company_supply_shipped != 0)) SlErrorCorrupt("Invalid copied supply departure/link");
			rows.push_back(row);
		}
		_load_check_data.native_cargo_packets_present = true;
		_load_check_data.legacy_b_supply_packet_fields_present = SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersB);
	}
};

static const CAPAChunkHandler CAPA;
static const ChunkHandlerRef cargopacket_chunk_handlers[] = {
	CAPA,
};

extern const ChunkHandlerTable _cargopacket_chunk_handlers(cargopacket_chunk_handlers);
