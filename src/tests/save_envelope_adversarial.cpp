/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_adversarial.cpp Empty paid-history headers, nested schemas and changing raw-byte identity refusals.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 11; malformed CAPA pairs are recorded facts that the classifier refuses.
 */
#include "../stdafx.h"
#include "save_envelope_fixture.h"

/** A source changing between identity and parsing, with unchanged readable envelope and valid RIFF framing. */
class ChangingEnvelopeReader final : public LoadFilter {
	EnvelopeTestReader before, after; ///< Two explicit valid byte streams, never filesystem timestamps or origin labels.
	uint8_t resets = 0; ///< First pass observes before; parsing and final rewind observe after.
public:
	/** @param before Initial complete original. @param after Different complete original returned after rewind. */
	ChangingEnvelopeReader(std::vector<uint8_t> before, std::vector<uint8_t> after) : LoadFilter(nullptr), before(std::move(before)), after(std::move(after)) {}
	/** @param buf Destination. @param len Capacity. @return Actual bytes of this pass's original input. */
	size_t Read(uint8_t *buf, size_t len) override { return (this->resets < 2 ? this->before : this->after).Read(buf, len); }
	/** Simulate an external source change while faithfully resetting its raw offset. */
	void Reset() override { ++this->resets; this->before.Reset(); this->after.Reset(); }
};

/** Construct only an empty original CAPA header from independent source field signatures.
 * @param fields Exact original names/types supplied by this test, independent of scanner helpers.
 * @return Modern424 envelope with one empty CAPA table; missing other owners intentionally prevent whole-world admission.
 */
static std::vector<uint8_t> EmptyPacketTable(const std::vector<std::pair<std::string_view, uint8_t>> &fields)
{
	std::vector<uint8_t> header;
	// Each explicit source field gets its literal native type and short name;
	// no save descriptor, chunk handler or changed writer produces this oracle.
	for (const auto &[name, type] : fields) {
		REQUIRE(name.size() < 128);
		header.push_back(type); header.push_back(static_cast<uint8_t>(name.size()));
		header.insert(header.end(), name.begin(), name.end());
	}
	header.push_back(0);
	std::vector<uint8_t> payload{'C', 'A', 'P', 'A', 3};
	const auto length = header.size() + 1;
	REQUIRE(length < 16384);
	if (length >= 128) payload.push_back(static_cast<uint8_t>(0x80 | (length >> 8)));
	payload.push_back(static_cast<uint8_t>(length));
	payload.insert(payload.end(), header.begin(), header.end());
	payload.insert(payload.end(), 5, 0); // Original array terminator followed by four-byte chunk terminator.
	return EnvelopeStream(std::move(payload), "OTTN", 424);
}

/** Empty pools still need both exact supply columns; missing/wrong types cannot default paid packet provenance. */
TEST_CASE("Save envelope empty CAPA requires whole exact supply signatures", "[save-envelope-inspection]")
{
	std::vector<std::pair<std::string_view, uint8_t>> fields{
		{"source", 4}, {"source_xy", 6}, {"loaded_at_xy", 4}, {"count", 4}, {"periods_in_transit", 4},
		{"feeder_share", 7}, {"source_type", 2}, {"source_id", 4}, {"travelled.x", 3}, {"travelled.y", 3},
	};
	const auto absent = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EmptyPacketTable(fields)));
	CHECK(absent.supply_columns == SaveSupplyColumns::Absent); CHECK_FALSE(absent.common_transport_valid);
	fields.push_back({"company_supply_order", 6});
	// A half pair is recorded as malformed in both views and refuses before any
	// other rule; it never reads as the historical absence of supply columns.
	const auto half = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EmptyPacketTable(fields)));
	CHECK(half.cargo_supply_encoding == SupplyColumnEncoding::Malformed); CHECK(half.supply_columns == SaveSupplyColumns::Malformed);
	CHECK(ClassifySaveFormat(half).status == SaveLineageStatus::Corrupt);
	fields.push_back({"company_supply_shipped", 1});
	const auto complete = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EmptyPacketTable(fields)));
	CHECK(complete.supply_columns == SaveSupplyColumns::Complete); CHECK(complete.cargo_supply_encoding == SupplyColumnEncoding::LegacyU32Bool);
	CHECK(ClassifySaveFormat(complete).status == SaveLineageStatus::Corrupt); // Typed CAPA alone cannot invent the other common owners or a required B family.
	fields.back().second = 2;
	// A U32 link with a U8 flag belongs to no writer: recorded malformed, never narrowed or widened.
	const auto mixed = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EmptyPacketTable(fields)));
	CHECK(mixed.cargo_supply_encoding == SupplyColumnEncoding::Malformed); CHECK(mixed.supply_columns == SaveSupplyColumns::Malformed);
}

/** Nested schemas and sparse tables retain original header/index ownership, while depth/count budgets are checked before descending. */
TEST_CASE("Save envelope nested headers and sparse tables stay within their original owners", "[save-envelope-inspection]")
{
	// NEST has struct 'x' and child U16 'y', then no rows. STBL has
	// one U16 field and original sparse index5 followed by exactly two bytes.
	const auto input = EnvelopeStream(EnvelopeHex("4e45535403091b01780004017900005354424c040504017800040501020000000000"));
	CHECK(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input)).framing_complete);
	SaveInspectionLimits limits; limits.depth = 1;
	CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input), limits), SaveFramingError);
	limits = {}; limits.fields = 1;
	CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input), limits), SaveFramingError);
	// All upper RIFF length bits remain a bounded claim; this provides no
	// large payload and must fail without allocating its advertised extent.
	CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EnvelopeStream(EnvelopeHex("54455354f0ffffff00000000")))), SaveFramingError);
}

/** Source identity includes bytes after codec completion and refuses a changed source, even when both logical framings are valid. */
TEST_CASE("Save envelope digest cannot bind another raw source or only a decoder prefix", "[save-envelope-inspection]")
{
	const auto first = EnvelopeStream(EnvelopeHex("544553540000000310203000000000"));
	const auto second = EnvelopeStream(EnvelopeHex("544553540000000311203000000000"));
	CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<ChangingEnvelopeReader>(first, second)), SaveFramingError);
#ifdef WITH_ZLIB
	const auto compressed = EnvelopeStream(EnvelopeHex("789c0b710d0e6160606016503000520c00132001a4"), "OTTZ");
	const auto original = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(compressed));
	auto with_tail = compressed; with_tail.insert(with_tail.end(), {0xA5, 0x5A, 0x10});
	const auto tailed = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(with_tail));
	CHECK(original.framing_complete); CHECK(tailed.framing_complete);
	CHECK(tailed.source_digest != original.source_digest);
	CHECK(EnvelopeReplay(std::make_shared<EnvelopeTestReader>(with_tail)) == EnvelopeHex("544553540000000310203000000000"));
#endif
}
