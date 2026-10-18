/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_wire.cpp Independent literal wire/geometry and original station-owner retention boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC.
 * Literal metadata/packets are declared components, not procurement or a driven carrier/construction chain.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply_order_wire.h"
#include "../map_layer.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Parse an independently specified literal field layout; this test helper never calls the production codec to construct expected bytes.
 * @param hex Explicit two-digit lowercase hexadecimal native bytes.
 * @return Exact independent byte sequence.
 */
static std::vector<uint8_t> SurfaceSupplyLiteral(std::string_view hex)
{
	std::vector<uint8_t> bytes; REQUIRE(hex.size() % 2 == 0);
	// Each declared byte belongs to the independently frozen53-byte field layout; expected native framing is never borrowed from the encoder under test.
	for (size_t offset = 0; offset < hex.size(); offset += 2) {
		/** @param value One trusted literal lowercase hex digit. @return Its numeric nibble, without gameplay lookup or state. */
		const auto digit = [](char value) { return value <= '9' ? value - '0' : value - 'a' + 10; };
		bytes.push_back(static_cast<uint8_t>((digit(hex[offset]) << 4) | digit(hex[offset + 1])));
	}
	return bytes;
}

/** The literal fixes every original field width/order/endian independently, including historical invalid packet and distinct economy/calendar values. */
TEST_CASE("Surface order native metadata codec matches an independent exact literal", "[surface-supply-order-wire]")
{
	const auto literal = SurfaceSupplyLiteral("0200000000000000640000000100010000000000000000000300140014005354454c0200c8000000000000000000010000000000bc07000064000000000000ffffff00");
	REQUIRE(literal.size() == 67); const auto decoded = DecodeSurfaceSupplyOrders(literal); REQUIRE(decoded.has_value());
	CHECK(decoded->snapshot.next_id == 2); CHECK(decoded->saved_day == TimerGameEconomy::Date{100}); REQUIRE(decoded->snapshot.records.size() == 1);
	const auto &record = decoded->snapshot.records[0]; CHECK(record.id == 1); CHECK(record.origin == StationID{3}); CHECK(record.origin_x == 20); CHECK(record.origin_y == 20);
	CHECK(record.purchaser == CompanyID{0}); CHECK(record.owner == CompanyID{0}); CHECK(record.label == CargoLabel{"STEL"}); CHECK(record.quantity == 2);
	CHECK(record.cost == 200); CHECK(record.inflation_prices == 65536); CHECK(record.purchase_year == TimerGameCalendar::Year{1980});
	CHECK(record.purchase_day == TimerGameEconomy::Date{100}); CHECK(record.packet_buyer == 0); CHECK(record.state == SurfaceSupplyOrderState::Pending); CHECK(record.issued_packet == CargoPacketID::Invalid());
	const auto encoded = EncodeSurfaceSupplyOrders(*decoded); REQUIRE(encoded.has_value()); CHECK(*encoded == literal);
}

/** Exact length/count admission precedes record allocation, and a corrupt unsigned cost cannot wrap into a refund-capable signed amount. */
TEST_CASE("Surface order codec refuses truncated trailing oversized and wrapped-money families", "[surface-supply-order-wire]")
{
	auto bytes = SurfaceSupplyLiteral("0200000000000000640000000100010000000000000000000300140014005354454c0200c8000000000000000000010000000000bc07000064000000000000ffffff00");
	const auto fault = GENERATE(0, 1, 2, 3, 4);
	switch (fault) {
		case 0: bytes.resize(13); break;
		case 1: bytes.pop_back(); break;
		case 2: bytes.push_back(0); break;
		case 3: bytes[12] = 0xFF; bytes[13] = 0xFF; break;
		case 4: bytes[43] = 0x80; break; // Header14 + original record cost offset22 + high byte7.
	}
	CHECK_FALSE(DecodeSurfaceSupplyOrders(bytes));
	SurfaceSupplyOrderTransport transport; transport.snapshot.next_id = 0; CHECK_FALSE(EncodeSurfaceSupplyOrders(transport));
}

/** Explicit native storage/controller ownership decodes the candidate geometry even when no map or a differently sized map is live. */
TEST_CASE("Copied tile coordinates use original map geometry and canonical layer meaning", "[surface-supply-order-wire]")
{
	const auto plane = GENERATE(0, 1, 2, 3);
	const std::array<TileIndex, 4> literals{TileIndex{4015}, TileIndex{12207}, TileIndex{20399}, TileIndex{28591}};
	const auto position = MultiLayerMap::DecodeStoredTile(literals[plane], 128, 64, 4); REQUIRE(position.has_value());
	CHECK(position->x == 47); CHECK(position->y == 31); CHECK(position->layer == static_cast<MapLayer>(plane));
	CHECK_FALSE(MultiLayerMap::DecodeStoredTile(literals[plane], 0, 64, 4)); CHECK_FALSE(MultiLayerMap::DecodeStoredTile(literals[plane], 127, 64, 4));
	CHECK_FALSE(MultiLayerMap::DecodeStoredTile(literals[plane], 128, 64, 0)); CHECK_FALSE(MultiLayerMap::DecodeStoredTile(INVALID_TILE, 128, 64, 4));
}

/** The original packet owner removes only abandoned ordinary offers; paid material with active or retired buyer stays in the same container until an explicit stock operation. */
TEST_CASE("Station offer discard protects bought stock without a second inventory or changed ordinary removal", "[surface-supply-order-wire]")
{
	StationCargoList stock{}; // Native station fields value-initialize the reservation counter; the standalone component must do the same.
	REQUIRE(CargoPacket::CanAllocateItem(3)); // Reserve the complete declared fixture before native pool allocation; no production purchase authority is implied.
	stock.Append(CargoPacket::Create(StationID{3}, uint16_t{5}, Source{CompanyID{0}, SourceType::CompanySupply}), StationID::Invalid());
	stock.Append(CargoPacket::Create(StationID{3}, uint16_t{6}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	stock.Append(CargoPacket::Create(StationID{3}, uint16_t{7}, Source{Source::Invalid, SourceType::CompanySupply}), StationID::Invalid());
	CHECK(stock.TruncateAbandonedOffers(100) == 6); CHECK(stock.TotalCount() == 12);
	const auto random = _random; CHECK(stock.TruncateAbandonedOffers(100) == 0);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(stock.Truncate() == 12); CHECK(stock.TotalCount() == 0);
	StationCargoList legacy{}, offered{};
	REQUIRE(CargoPacket::CanAllocateItem(2)); // Both ordinary controls must pass native pool capacity before either component packet is created.
	legacy.Append(CargoPacket::Create(StationID{3}, uint16_t{8}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	offered.Append(CargoPacket::Create(StationID{3}, uint16_t{8}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	const auto before = _random; CHECK(legacy.Truncate(3) == 3); const auto after = _random; _random = before;
	CHECK(offered.TruncateAbandonedOffers(3) == 3); CHECK(legacy.TotalCount() == offered.TotalCount());
	CHECK(_random.state[0] == after.state[0]); CHECK(_random.state[1] == after.state[1]);
}
