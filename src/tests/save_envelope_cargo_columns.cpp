/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_cargo_columns.cpp Detached CAPA column and MLLN tuple observations for every merged-era writer.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 11; builders shared with Group 12 in save_envelope_chunks.h.
 * Every stream is written byte by byte below; no native save writer supplies the expected facts.
 */
#include "../stdafx.h"
#include "save_envelope_chunks.h"
#include "../core/sha256.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

using CargoFields = EnvelopeFields; ///< One CAPA descriptor, in the file's own field order.

/** The CAPA fields every 424+ writer declares, so the scanner's shared-history rule is met. */
const CargoFields BASE_CARGO_FIELDS{
	{"source", 4}, {"source_xy", 6}, {"loaded_at_xy", 4}, {"count", 4}, {"periods_in_transit", 4},
	{"feeder_share", 7}, {"source_type", 2}, {"source_id", 4}, {"travelled.x", 3}, {"travelled.y", 3},
};
const CargoFields CANONICAL_SUPPLY{{"company_supply_order", 8}, {"company_supply_shipped", 2}}; ///< Merged writer: U64 receipt link, U8 flag.
const CargoFields LEGACY_SUPPLY{{"company_supply_order", 6}, {"company_supply_shipped", 1}}; ///< Historical B: U32 link, Bool flag.
const CargoFields ASTEROID_ORIGIN{{"asteroid_body", 6}, {"asteroid_mineral", 6}}; ///< Space fixes typed origin: two U32 keys.

/** @param parts Field groups in order. @return One CAPA descriptor. */
CargoFields Fields(std::initializer_list<const CargoFields *> parts) { return JoinEnvelopeFields(parts); }
/** @param fields Exact CAPA descriptor. @param rows Zero-filled packet rows. @return Complete CAPA chunk. */
std::vector<uint8_t> CargoChunk(const CargoFields &fields, size_t rows = 0) { return EnvelopeTableChunk("CAPA", fields, rows); }
/** @param magic Payload magic. @param schema Marker schema. @param writer Marker writer. @return Complete MLLN chunk. */
std::vector<uint8_t> MarkerChunk(uint32_t magic, uint16_t schema, uint16_t writer) { return EnvelopeMarkerChunk(magic, schema, writer); }
/** @param version Outer version. @param chunks Chunks in file order. @return Original raw save bytes. */
std::vector<uint8_t> Stream(uint16_t version, std::initializer_list<std::vector<uint8_t>> chunks) { return EnvelopeChunkStream(version, chunks); }

/** The observations Group 11 adds, gathered so marker-first and marker-last scans compare as one value. */
struct ColumnObservation {
	SupplyColumnEncoding supply; ///< Observed supply pair encoding.
	SaveEvidenceState asteroid; ///< Observed typed origin key.
	SaveSupplyColumns legacy_supply; ///< Compatibility view for the existing classifier rules.
	bool merged_cargo; ///< Merged descriptor proof.
	SaveEvidenceState marker; ///< MLLN family state.
	uint32_t magic; ///< Observed marker magic.
	uint16_t schema; ///< Observed marker schema.
	uint16_t writer; ///< Observed marker writer.

	/** @param other Second scan. @return Whether every observation agrees. */
	bool operator==(const ColumnObservation &other) const = default;
};

/** Scan one stream and check that the raw source was neither changed nor left mid-stream.
 * @param input Original raw bytes.
 * @param limits Work ceilings for this scan.
 * @return The Group 11 observations.
 */
ColumnObservation Observe(const std::vector<uint8_t> &input, const SaveInspectionLimits &limits = {})
{
	auto reader = std::make_shared<EnvelopeTestReader>(input, 7); // Short raw reads, as from a network buffer.
	const auto facts = InspectSaveWireFacts(reader, limits);
	CHECK(facts.framing_complete);
	CHECK(reader->position == 0); // Rewound after success.
	CHECK(reader->input == input); // Header and payload untouched.
	Sha256 digest; digest.Update(std::span{input});
	CHECK(facts.source_digest == digest.Digest());
	const auto &marker = facts.Get(SaveLineageFamily::Marker);
	return {facts.cargo_supply_encoding, facts.asteroid_origin_columns, facts.supply_columns, facts.merged_cargo_valid,
		marker.state, facts.marker_magic, facts.marker_schema, facts.marker_writer_version};
}

} // namespace

/** D441, S441, S445 and the reserved 446 each declare their own CAPA columns and marker; the scanner reads them without a version guess. */
TEST_CASE("Save envelope observes each writer's CAPA columns and marker tuple", "[save-envelope-inspection]")
{
	const auto canonical = Fields({&BASE_CARGO_FIELDS, &CANONICAL_SUPPLY});
	const auto typed = Fields({&BASE_CARGO_FIELDS, &ASTEROID_ORIGIN});
	const auto combined = Fields({&BASE_CARGO_FIELDS, &CANONICAL_SUPPLY, &ASTEROID_ORIGIN});
	const auto merged441 = MarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, MERGED_SAVE_LINEAGE_SCHEMA, MERGED_SAVE_LINEAGE_VERSION);
	const auto merged446 = MarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, CURRENT_MERGED_SAVE_LINEAGE_SCHEMA, CURRENT_MERGED_SAVE_VERSION);

	const auto d441 = Observe(Stream(441, {CargoChunk(canonical, 1), merged441}));
	CHECK(d441 == ColumnObservation{SupplyColumnEncoding::CanonicalU64U8, SaveEvidenceState::Absent, SaveSupplyColumns::Absent, true,
		SaveEvidenceState::Valid, MERGED_SAVE_LINEAGE_MAGIC, MERGED_SAVE_LINEAGE_SCHEMA, MERGED_SAVE_LINEAGE_VERSION});

	// The incoming writer has no supply columns and no marker at either end of its range.
	for (const uint16_t version : {441, 445}) {
		INFO(version);
		CHECK(Observe(Stream(version, {CargoChunk(typed, 1)})) == ColumnObservation{SupplyColumnEncoding::Absent, SaveEvidenceState::Valid,
			SaveSupplyColumns::Absent, false, SaveEvidenceState::Absent, 0, 0, 0});
	}

	const auto c446 = Observe(Stream(446, {CargoChunk(combined, 1), merged446}));
	CHECK(c446 == ColumnObservation{SupplyColumnEncoding::CanonicalU64U8, SaveEvidenceState::Valid, SaveSupplyColumns::Absent, true,
		SaveEvidenceState::Valid, MERGED_SAVE_LINEAGE_MAGIC, CURRENT_MERGED_SAVE_LINEAGE_SCHEMA, CURRENT_MERGED_SAVE_VERSION});

	// Column facts come from the descriptor, so an empty CAPA gives the same answer as one with packets.
	CHECK(Observe(Stream(441, {CargoChunk(canonical), merged441})) == d441);
	CHECK(Observe(Stream(446, {CargoChunk(combined), merged446})) == c446);
	CHECK(Observe(Stream(445, {CargoChunk(typed)})).asteroid == SaveEvidenceState::Valid);

	// Marker first or last: the envelope version is known before any chunk, so order changes nothing.
	CHECK(Observe(Stream(441, {merged441, CargoChunk(canonical, 1)})) == d441);
	CHECK(Observe(Stream(446, {merged446, CargoChunk(combined, 1)})) == c446);

	// Historical B's pair keeps its legacy meaning, independently of the outer number.
	const auto legacy = Observe(Stream(434, {CargoChunk(Fields({&BASE_CARGO_FIELDS, &LEGACY_SUPPLY}))}));
	CHECK(legacy.supply == SupplyColumnEncoding::LegacyU32Bool);
	CHECK(legacy.legacy_supply == SaveSupplyColumns::Complete);
	CHECK_FALSE(legacy.merged_cargo);
}

/** Half pairs, other widths and any marker tuple other than 441/1 or 446/2 are recorded as malformed and never fall back. */
TEST_CASE("Save envelope records malformed CAPA columns and marker tuples", "[save-envelope-inspection]")
{
	const CargoFields order_only{{"company_supply_order", 8}};
	const CargoFields mixed{{"company_supply_order", 8}, {"company_supply_shipped", 1}};
	const CargoFields body_only{{"asteroid_body", 6}};
	const CargoFields narrow_body{{"asteroid_body", 4}, {"asteroid_mineral", 6}};
	// Each broken supply descriptor is malformed under both its own and the legacy view.
	for (const auto *broken : {&order_only, &mixed}) {
		const auto observed = Observe(Stream(441, {CargoChunk(Fields({&BASE_CARGO_FIELDS, broken}))}));
		CHECK(observed.supply == SupplyColumnEncoding::Malformed);
		CHECK(observed.legacy_supply == SaveSupplyColumns::Malformed);
		CHECK_FALSE(observed.merged_cargo);
	}
	// A typed origin key is all or nothing at full width.
	for (const auto *broken : {&body_only, &narrow_body}) {
		CHECK(Observe(Stream(445, {CargoChunk(Fields({&BASE_CARGO_FIELDS, broken}))})).asteroid == SaveEvidenceState::Malformed);
	}
	// Only the two merged allocations exist, each bound to its own envelope.
	const std::tuple<uint16_t, uint32_t, uint16_t, uint16_t> tuples[]{
		{441, MERGED_SAVE_LINEAGE_MAGIC, 2, 441}, // Schema 2 belongs to 446.
		{446, MERGED_SAVE_LINEAGE_MAGIC, 1, 446}, // Schema 1 belongs to 441.
		{446, MERGED_SAVE_LINEAGE_MAGIC, 1, 441}, // A 441 marker inside a 446 envelope.
		{441, MERGED_SAVE_LINEAGE_MAGIC ^ 1, 1, 441}, // Wrong magic.
		{445, MERGED_SAVE_LINEAGE_MAGIC, 2, 445}, // A Space fixes version is never a merged allocation.
		{1, MERGED_SAVE_LINEAGE_MAGIC, 1, 441}, // A legacy envelope cannot carry a merged marker.
	};
	for (const auto &[version, magic, schema, writer] : tuples) {
		INFO(version << " " << schema << " " << writer);
		CHECK(Observe(Stream(version, {MarkerChunk(magic, schema, writer)})).marker == SaveEvidenceState::Malformed);
	}
	// The tuple is read only after exact eight-byte framing; a short marker is a framing error.
	auto short_marker = MarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, MERGED_SAVE_LINEAGE_SCHEMA, MERGED_SAVE_LINEAGE_VERSION);
	short_marker[7] = 7; short_marker.pop_back();
	CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(Stream(441, {short_marker}))), SaveFramingError);
}

/** The new observations stay inside the existing work ceilings and keep rewind on failure. */
TEST_CASE("Save envelope column observations keep bounded work and rewind", "[save-envelope-inspection]")
{
	const auto combined = Fields({&BASE_CARGO_FIELDS, &CANONICAL_SUPPLY, &ASTEROID_ORIGIN});
	const auto input = Stream(446, {CargoChunk(combined, 2), MarkerChunk(MERGED_SAVE_LINEAGE_MAGIC, CURRENT_MERGED_SAVE_LINEAGE_SCHEMA, CURRENT_MERGED_SAVE_VERSION)});
	// Exactly the work the stream needs passes; one unit less refuses before any more is done.
	SaveInspectionLimits limits;
	limits.fields = static_cast<uint32_t>(combined.size()); limits.rows = 2; limits.chunks = 2;
	CHECK(Observe(input, limits).supply == SupplyColumnEncoding::CanonicalU64U8);
	for (int ceiling = 0; ceiling < 3; ++ceiling) {
		INFO(ceiling);
		auto tight = limits;
		if (ceiling == 0) --tight.fields;
		if (ceiling == 1) --tight.rows;
		if (ceiling == 2) --tight.chunks;
		auto reader = std::make_shared<EnvelopeTestReader>(input);
		CHECK_THROWS_AS(InspectSaveWireFacts(reader, tight), SaveFramingError);
		CHECK(reader->position == 0);
	}
	CheckEnvelopeTruncationsRewind(input);
	CheckEnvelopeTruncationsRewind(Stream(441, {CargoChunk(Fields({&BASE_CARGO_FIELDS, &ASTEROID_ORIGIN}))}));
}

/** Optional real S445 files: Group 3's donor 445 reconstructions, when the caller names their directory.
 * Set OTTD_DONOR445_DIR to test-cache/merge-oct28/donor445 to run it; without it the case only warns.
 */
TEST_CASE("Save envelope observes typed CAPA columns in real donor 445 saves", "[save-envelope-inspection]")
{
	const char *directory = std::getenv("OTTD_DONOR445_DIR");
	if (directory == nullptr) { WARN("OTTD_DONOR445_DIR is not set; no real donor 445 save was scanned"); return; }
#ifndef WITH_LIBLZMA
	WARN("Donor 445 saves are OTTX; this build has no LZMA decoder"); return;
#else
	size_t scanned = 0;
	// Each real S445 file must show the incoming writer's shape: typed origin
	// columns, no supply pair and no merged marker. Its outer number is unchanged.
	for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".sav") continue;
		INFO(entry.path().string());
		std::ifstream stream(entry.path(), std::ios::binary); REQUIRE(stream.good());
		const std::vector<uint8_t> input{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
		auto reader = std::make_shared<EnvelopeTestReader>(input);
		const auto facts = InspectSaveWireFacts(reader);
		CHECK(facts.original_version == 445);
		CHECK(facts.cargo_supply_encoding == SupplyColumnEncoding::Absent);
		CHECK(facts.asteroid_origin_columns == SaveEvidenceState::Valid);
		CHECK(facts.Get(SaveLineageFamily::Marker).state == SaveEvidenceState::Absent);
		CHECK(reader->position == 0);
		++scanned;
	}
	CHECK(scanned > 0);
#endif
}
