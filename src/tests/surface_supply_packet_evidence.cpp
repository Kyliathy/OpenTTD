/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_packet_evidence.cpp Native packet evidence preservation without live legacy admission.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native cargo ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-c-supply-receipts.md permanent purpose and finite legacy rights.
 * Packets and links are declared components; a link alone proves neither a funded order nor complete custody.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../cargopacket.h"
#include "../core/random_func.hpp"
#include <memory>
#include "../safeguards.h"

/** Create one explicitly declared native packet component; no purchase or installed book is involved.
 * @param quantity Nonzero original physical units; saturation tests deliberately exceed any real order's allowance.
 * @param order Optional original receipt link, still requiring the complete candidate custody join.
 * @param shipped Declared original departure evidence, never a new accounting event.
 * @param source Permanent ordinary or CompanySupply purpose, independent of optional linkage.
 * @return Owned native packet; the caller must transfer it to exactly one real list or let its guard retire it.
 */
static std::unique_ptr<CargoPacket> EvidencePacket(uint16_t quantity, uint64_t order = 0, bool shipped = false,
	Source source = Source{CompanyID{0}, SourceType::CompanySupply})
{
	REQUIRE(CargoPacket::CanAllocateItem());
	std::unique_ptr<CargoPacket> packet{CargoPacket::Create(StationID{3}, quantity, source)};
	if (order != 0) REQUIRE(packet->RestoreCompanySupplyOrder(order, shipped));
	return packet;
}

/** Restoring original optional evidence cannot create paid purpose or rewrite an existing legacy identity. */
TEST_CASE("Packet legacy linkage is explicit uint64 evidence and never replaces permanent purpose", "[surface-supply-packet-evidence]")
{
	auto ordinary = EvidencePacket(10, 0, false, Source{Source::Invalid, SourceType::Industry});
	CHECK(ordinary->GetCompanySupplyOrder() == 0); CHECK_FALSE(ordinary->HasCompanySupplyShipped());
	CHECK_FALSE(ordinary->RestoreCompanySupplyOrder(1, false)); CHECK_FALSE(ordinary->GetSource().IsCompanySupply());
	auto native = EvidencePacket(10);
	CHECK(native->GetSource().IsCompanySupply()); CHECK(native->GetCompanySupplyOrder() == 0); CHECK_FALSE(native->HasCompanySupplyShipped());
	CHECK_FALSE(native->RestoreCompanySupplyOrder(0, true)); CHECK_FALSE(native->HasCompanySupplyShipped());
	const uint64_t original_id = (uint64_t{1} << 40) + 7;
	REQUIRE(native->RestoreCompanySupplyOrder(original_id, true));
	CHECK(native->GetCompanySupplyOrder() == original_id); CHECK(native->HasCompanySupplyShipped());
	CHECK_FALSE(native->RestoreCompanySupplyOrder(original_id, false));
	CHECK_FALSE(native->RestoreCompanySupplyOrder(original_id + 1, true));
	CHECK(native->GetCompanySupplyOrder() == original_id); CHECK(native->HasCompanySupplyShipped());
}

/** A split retains exactly the original link/state; buyer retirement cannot make either fragment paying cargo. */
TEST_CASE("Packet split and buyer retirement preserve original legacy evidence", "[surface-supply-packet-evidence]")
{
	REQUIRE(CargoPacket::GetNumItems() == 0); // This isolated component must not invalidate another fixture's live buyer.
	const bool shipped = GENERATE(false, true);
	const uint64_t original_id = (uint64_t{1} << 40) + 9;
	auto original = EvidencePacket(10, original_id, shipped);
	std::unique_ptr<CargoPacket> split{original->Split(4)}; REQUIRE(split != nullptr);
	CHECK(original->Count() == 6); CHECK(split->Count() == 4);
	// Every split unit keeps its original purpose and irreversible departure
	// evidence; merely becoming another pool object is not another shipment.
	for (const auto *fragment : {original.get(), split.get()}) {
		CHECK(fragment->GetCompanySupplyOrder() == original_id); CHECK(fragment->HasCompanySupplyShipped() == shipped);
		CHECK(fragment->GetSource() == Source{CompanyID{0}, SourceType::CompanySupply});
	}
	CargoPacket::InvalidateAllFrom(Source{CompanyID{0}, SourceType::CompanySupply});
	// Retiring a buyer invalidates its identity in both fragments while keeping
	// their non-paying purpose and independent original receipt evidence intact.
	for (const auto *fragment : {original.get(), split.get()}) {
		CHECK_FALSE(fragment->GetSource().IsValid()); CHECK(fragment->GetSource().IsCompanySupply());
		CHECK(fragment->GetCompanySupplyOrder() == original_id); CHECK(fragment->HasCompanySupplyShipped() == shipped);
	}
}

/** Both actual list owners merge only matching original evidence; ordinary rating decay still protects unlinked A stock. */
TEST_CASE("Native station and carrier merges keep distinct order and departure histories", "[surface-supply-packet-evidence]")
{
	const bool vehicle = GENERATE(false, true);
	const auto before = CargoPacket::GetNumItems(); const auto random = _random;
	{
		StationCargoList station{}; VehicleCargoList carrier{};
		/** Transfer one declared component to its one actual native list.
		 * @param packet Exclusive packet guard; Append may immediately merge and retire its pool identity.
		 */
		const auto append = [&](std::unique_ptr<CargoPacket> packet) {
			if (vehicle) carrier.Append(packet.release());
			else station.Append(packet.release(), StationID::Invalid());
		};
		auto first = EvidencePacket(2, 1); auto matching = EvidencePacket(3, 1);
		CHECK(StationCargoList::AreMergable(first.get(), matching.get())); CHECK(VehicleCargoList::AreMergable(first.get(), matching.get()));
		auto returned = EvidencePacket(4, 1, true); auto other_order = EvidencePacket(5, 2); auto native = EvidencePacket(6);
		auto ordinary = EvidencePacket(7, 0, false, Source{CompanyID{0}, SourceType::Industry});
		// Same origin/age is deliberately insufficient: each incompatible purpose,
		// identity or departure state must remain a distinct physical fragment.
		for (const auto *other : {returned.get(), other_order.get(), native.get(), ordinary.get()}) {
			CHECK_FALSE(StationCargoList::AreMergable(first.get(), other)); CHECK_FALSE(VehicleCargoList::AreMergable(first.get(), other));
		}
		append(std::move(first)); append(std::move(matching)); append(std::move(returned));
		append(std::move(other_order)); append(std::move(native)); append(std::move(ordinary));
		CHECK(CargoPacket::GetNumItems() == before + 5);
		CHECK((vehicle ? carrier.TotalCount() : station.TotalCount()) == 27);
		if (!vehicle) {
			CHECK(station.TruncateAbandonedOffers(100) == 7); CHECK(station.TotalCount() == 20);
			CHECK(CargoPacket::GetNumItems() == before + 4);
		}
	}
	CHECK(CargoPacket::GetNumItems() == before);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Native packet cardinality bounds remain unchanged for ordinary, unlinked A and linked legacy evidence. */
TEST_CASE("Native packet merging preserves saturated counts for every supply purpose", "[surface-supply-packet-evidence]")
{
	const bool vehicle = GENERATE(false, true); const auto purpose = GENERATE(0, 1, 2);
	const Source source{CompanyID{0}, purpose == 0 ? SourceType::Industry : SourceType::CompanySupply};
	const uint64_t link = purpose == 2 ? 1 : 0;
	const auto before = CargoPacket::GetNumItems();
	{
		StationCargoList station{}; VehicleCargoList carrier{};
		auto full = EvidencePacket(UINT16_MAX, link, false, source); auto extra = EvidencePacket(1, link, false, source);
		CHECK(StationCargoList::AreMergable(full.get(), extra.get())); CHECK(VehicleCargoList::AreMergable(full.get(), extra.get()));
		if (vehicle) { carrier.Append(full.release()); carrier.Append(extra.release()); }
		else { station.Append(full.release(), StationID::Invalid()); station.Append(extra.release(), StationID::Invalid()); }
		CHECK(CargoPacket::GetNumItems() == before + 2); // MAX_COUNT prevents the otherwise eligible merge from wrapping a physical quantity.
		CHECK((vehicle ? carrier.TotalCount() : station.TotalCount()) == 65536);
	}
	CHECK(CargoPacket::GetNumItems() == before);
}
