/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_supply_book.cpp Detached SSOR book shapes: fixed A440, SSM1 tagged, B TABLE, overlaps and refusals.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 12
 * Every payload is authored from the wire contracts below; no native save writer supplies the expected facts.
 */
#include "../stdafx.h"
#include "save_envelope_chunks.h"
#include "../surface_supply_merged_wire.h"
#include <optional>

namespace {

/** The SSOR observations Group 12 adds, gathered so chunk orders compare as one value. */
struct BookObservation {
	SupplyBookShape shape; ///< Observed RIFF book shape.
	SaveEvidenceState state; ///< SSOR family state.
	SaveEvidenceKind kind; ///< SSOR framing kind.
	uint32_t records; ///< Record count of the reading that fits.
	uint16_t schema; ///< Tagged schema, or zero.
	bool tagged_valid; ///< The SSM1 inspector validated the book.
	uint64_t next_id; ///< Tagged issuer frontier, or zero.

	/** @param other Second scan. @return Whether every observation agrees. */
	bool operator==(const BookObservation &other) const = default;
};

/** Scan one stream and check that the raw source is rewound and untouched.
 * @param input Original raw bytes.
 * @return The Group 12 observations.
 */
BookObservation Observe(const std::vector<uint8_t> &input)
{
	auto reader = std::make_shared<EnvelopeTestReader>(input, 5); // Short raw reads.
	const auto facts = InspectSaveWireFacts(reader);
	CHECK(facts.framing_complete);
	CHECK(reader->position == 0);
	CHECK(reader->input == input);
	const auto &supply = facts.Get(SaveLineageFamily::SupplyOrders);
	return {facts.supply_book_shape, supply.state, supply.kind, supply.records, supply.schema, facts.merged_supply_valid, facts.merged_supply_next_id};
}

/** Build a fixed A440-shaped book: little-endian next ID and day, little-endian count, 53 zero bytes per record.
 * @param records Declared and actual record count.
 * @param first_word Optional replacement for the first four bytes, to forge an SSM1 signature.
 * @return Exact SSOR payload of 14 + 53 * records bytes.
 */
std::vector<uint8_t> FixedBook(uint16_t records, std::optional<uint32_t> first_word = std::nullopt)
{
	std::vector<uint8_t> book;
	const uint64_t next_id = uint64_t{records} + 1;
	for (size_t byte = 0; byte < 8; ++byte) book.push_back(static_cast<uint8_t>(next_id >> (8 * byte)));
	book.insert(book.end(), {40, 0, 0, 0}); // Economy day 40, little-endian.
	book.push_back(static_cast<uint8_t>(records)); book.push_back(static_cast<uint8_t>(records >> 8));
	book.insert(book.end(), size_t{53} * records, 0);
	if (first_word) {
		for (size_t byte = 0; byte < 4; ++byte) book[byte] = static_cast<uint8_t>(*first_word >> (8 * (3 - byte)));
	}
	return book;
}

/** @return The independently authored two-era SSM1 book from surface_supply_merged_wire.cpp (113 bytes, next ID 0x100000002). */
std::vector<uint8_t> TaggedBook()
{
	return EnvelopeHex(
		"53534d31" "0001" "0000000100000002" "00000028" "0002"
		"02" "0000000000000002" "00" "0001" "52464d54" "0008" "00000028"
		"0000000000000013" "000aae60" "0002" "0003" "00"
		"01" "0000000100000001" "00" "0001" "52464d54" "0004" "00000028"
		"00" "000a" "0014" "000000000000006c" "0000000000010000" "000007d0" "0000" "01" "00000006");
}

/** Build a valid SSM1 book whose bytes also fit the fixed shape.
 * Eight 39-byte LegacyB rows make 20 + 8 * 39 = 332 bytes; next ID 0x600 puts 06 00 at
 * offsets 12-13, which the fixed reading takes as count 6, and 14 + 53 * 6 is also 332.
 * @return The overlapping payload.
 */
std::vector<uint8_t> OverlappingBook()
{
	std::vector<uint8_t> book = EnvelopeHex("53534d31" "0001" "0000000000000600" "00000028" "0008");
	// Each row repeats the authored LegacyB evidence with its own increasing identity.
	for (uint8_t id = 1; id <= 8; ++id) {
		book.push_back(2); PushEnvelopeInteger(book, id, 8);
		const auto row = EnvelopeHex("00" "0001" "52464d54" "0008" "00000028" "0000000000000013" "000aae60" "0002" "0003" "00");
		book.insert(book.end(), row.begin(), row.end());
	}
	return book;
}

/** The historical B SSOR TABLE descriptor that the scanner requires. */
const EnvelopeFields LEGACY_B_ORDER_FIELDS{
	{"id", 6}, {"owner", 2}, {"station", 4}, {"label", 6}, {"economy_day", 5}, {"calendar_date", 5},
	{"quantity", 4}, {"unit_price", 7}, {"refunded", 4}, {"dispatched", 4}, {"closed", 2},
};

/** The CAPA descriptor of the incoming writer: shared fields plus the typed asteroid key. */
const EnvelopeFields TYPED_CARGO_FIELDS{
	{"source", 4}, {"source_xy", 6}, {"loaded_at_xy", 4}, {"count", 4}, {"periods_in_transit", 4},
	{"feeder_share", 7}, {"source_type", 2}, {"source_id", 4}, {"travelled.x", 3}, {"travelled.y", 3},
	{"asteroid_body", 6}, {"asteroid_mineral", 6},
};

} // namespace

/** The fixed A440 book reads as Fixed in A440 and S441 alike: no version picks the tagged decoder. */
TEST_CASE("Save envelope reads fixed SSOR by shape in A440 and S441", "[save-envelope-inspection]")
{
	const BookObservation fixed{SupplyBookShape::Fixed, SaveEvidenceState::Valid, SaveEvidenceKind::Riff, 3, 0, false, 0};
	CHECK(Observe(EnvelopeChunkStream(440, {EnvelopeRiffChunk("SSOR", FixedBook(3))})) == fixed);
	// S441 shares the merged writer's number. Before Group 12 its fixed book was sent
	// to the tagged decoder and refused; now only its bytes decide.
	const auto s441 = Observe(EnvelopeChunkStream(441, {EnvelopeTableChunk("CAPA", TYPED_CARGO_FIELDS), EnvelopeRiffChunk("SSOR", FixedBook(3))}));
	CHECK(s441 == fixed);
	CHECK(Observe(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", FixedBook(3)), EnvelopeTableChunk("CAPA", TYPED_CARGO_FIELDS)})) == s441);
	CHECK(Observe(EnvelopeChunkStream(445, {EnvelopeRiffChunk("SSOR", FixedBook(0))})).shape == SupplyBookShape::Fixed);
	CheckEnvelopeTruncationsRewind(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", FixedBook(1))}));
}

/** The SSM1 book reads as Tagged in D441 and 446 alike, wherever the marker sits. */
TEST_CASE("Save envelope reads tagged SSOR by signature in D441 and 446", "[save-envelope-inspection]")
{
	const BookObservation tagged{SupplyBookShape::Tagged, SaveEvidenceState::Valid, SaveEvidenceKind::Riff, 2, MERGED_SURFACE_SUPPLY_SCHEMA, true, 0x100000002ULL};
	const auto marker441 = EnvelopeMarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, MERGED_SAVE_LINEAGE_SCHEMA, MERGED_SAVE_LINEAGE_VERSION);
	const auto marker446 = EnvelopeMarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, CURRENT_MERGED_SAVE_LINEAGE_SCHEMA, CURRENT_MERGED_SAVE_VERSION);
	// Marker first and marker last give the same observations.
	CHECK(Observe(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", TaggedBook()), marker441})) == tagged);
	CHECK(Observe(EnvelopeChunkStream(441, {marker441, EnvelopeRiffChunk("SSOR", TaggedBook())})) == tagged);
	CHECK(Observe(EnvelopeChunkStream(446, {marker446, EnvelopeRiffChunk("SSOR", TaggedBook())})) == tagged);
	CHECK(Observe(EnvelopeChunkStream(446, {EnvelopeRiffChunk("SSOR", TaggedBook()), marker446})) == tagged);
	// An empty tagged book keeps its own frontier.
	const auto empty = Observe(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", EnvelopeHex("53534d31" "0001" "0000000000000001" "00000028" "0000"))}));
	CHECK(empty == BookObservation{SupplyBookShape::Tagged, SaveEvidenceState::Valid, SaveEvidenceKind::Riff, 0, MERGED_SURFACE_SUPPLY_SCHEMA, true, 1});
	CheckEnvelopeTruncationsRewind(EnvelopeChunkStream(446, {marker446, EnvelopeRiffChunk("SSOR", TaggedBook())}));
}

/** The historical B TABLE with its SSOH header is not a RIFF book at all. */
TEST_CASE("Save envelope keeps historical B SSOR TABLE outside the RIFF book shapes", "[save-envelope-inspection]")
{
	const auto header = EnvelopeHex("00000001" "00000000" "00000001"); // Schema 1, no rows, next ID 1.
	const auto input = EnvelopeChunkStream(426, {EnvelopeRiffChunk("SSOH", header), EnvelopeTableChunk("SSOR", LEGACY_B_ORDER_FIELDS)});
	const auto observed = Observe(input);
	CHECK(observed == BookObservation{SupplyBookShape::Absent, SaveEvidenceState::Valid, SaveEvidenceKind::Table, 0, 0, false, 0});
	const auto facts = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input));
	CHECK(facts.Get(SaveLineageFamily::SupplyHeader).state == SaveEvidenceState::Valid);
	CHECK(facts.Get(SaveLineageFamily::SupplyHeader).kind == SaveEvidenceKind::Riff);
}

/** A forged signature never falls back to the fixed reading, and a true overlap stays ambiguous. */
TEST_CASE("Save envelope refuses SSOR signature collisions without falling back", "[save-envelope-inspection]")
{
	// A fixed-shaped book whose first word is SSM1 is a recognized tagged payload
	// that fails tagged validation: Malformed, never Fixed.
	for (const uint16_t records : {0, 1, 2}) {
		INFO(records);
		CHECK(Observe(EnvelopeChunkStream(440, {EnvelopeRiffChunk("SSOR", FixedBook(records, MERGED_SURFACE_SUPPLY_MAGIC))})).shape == SupplyBookShape::Malformed);
	}
	// A real overlap: the tagged book validates and the fixed shape fits the same bytes.
	const auto overlap = OverlappingBook();
	REQUIRE(overlap.size() == 332);
	REQUIRE(DecodeMergedSurfaceSupplyOrders(overlap).has_value());
	const auto observed = Observe(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", overlap)}));
	CHECK(observed.shape == SupplyBookShape::Ambiguous);
	CHECK(observed.tagged_valid); // The classifier refusal is covered on detached facts in save_lineage_policy.cpp.
}

/** Truncated, oversized, duplicated and wrongly framed SSOR chunks refuse. */
TEST_CASE("Save envelope refuses truncated oversized duplicated and misframed SSOR", "[save-envelope-inspection]")
{
	// Payloads that fit no shape are recorded Malformed.
	const std::vector<uint8_t> malformed[]{
		EnvelopeHex("53534d"), // Shorter than a signature or a fixed header.
		EnvelopeHex("0100000000000000" "28000000" "0100"), // Fixed header claiming one record with none present.
		[] { auto book = FixedBook(1); book[13] = 0x40; book[12] = 0x01; return book; }(), // Count 16385 exceeds the book limit.
		[] { auto book = TaggedBook(); book.pop_back(); return book; }(), // Truncated tagged book.
		[] { auto book = TaggedBook(); book.push_back(0); return book; }(), // Tagged book with a trailing byte.
	};
	for (const auto &payload : malformed) {
		INFO(payload.size());
		CHECK(Observe(EnvelopeChunkStream(441, {EnvelopeRiffChunk("SSOR", payload)})).shape == SupplyBookShape::Malformed);
	}
	// Framing errors refuse the whole scan and rewind the raw source.
	const auto book = EnvelopeRiffChunk("SSOR", FixedBook(1));
	auto claimed = book; claimed[7] = static_cast<uint8_t>(claimed[7] + 1); // RIFF length one past the payload.
	auto sparse = EnvelopeTableChunk("SSOR", LEGACY_B_ORDER_FIELDS); sparse[4] = 2; // SSOR as a sparse array.
	for (const auto &input : {EnvelopeChunkStream(440, {book, book}), EnvelopeChunkStream(440, {claimed}), EnvelopeChunkStream(426, {sparse})}) {
		auto reader = std::make_shared<EnvelopeTestReader>(input);
		CHECK_THROWS_AS(InspectSaveWireFacts(reader), SaveFramingError);
		CHECK(reader->position == 0);
	}
}
