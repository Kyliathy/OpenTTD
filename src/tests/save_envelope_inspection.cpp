/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_inspection.cpp Independent framing, malformed input, codec and genuine raw-reader replay cases.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */
#include "../stdafx.h"
#include <filesystem>
#include <fstream>
#include "save_envelope_fixture.h"
#include "../fileio_type.h"

/** Detached native kinds retain RIFF length, table header and sparse index ownership without invoking any handler. */
TEST_CASE("Save envelope inspector bounds actual RIFF table and sparse framing", "[save-envelope-inspection]")
{
	// TEST RIFF carries three explicit bytes. ARRY has an empty contiguous row;
	// SPRS has index127 in its own two-byte row. TABL has one U16 'x' field.
	const auto payload = EnvelopeHex("54455354000000031020304152525901010255005350525302037f66005441424c0305040178000301020000000000");
	auto reader = std::make_shared<EnvelopeTestReader>(EnvelopeStream(payload), 1);
	const auto facts = InspectSaveWireFacts(reader);
	CHECK(facts.framing_complete); CHECK(facts.common_transport_valid); CHECK(facts.original_version == 1);
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Accepted); CHECK(reader->position == 0);
	// Every original truncation, including missing terminator bytes, must fail
	// and rewind without borrowing the next row or allocating a native object.
	for (size_t length = 0; length < reader->input.size(); ++length) {
		auto short_reader = std::make_shared<EnvelopeTestReader>(std::vector<uint8_t>(reader->input.begin(), reader->input.begin() + length));
		CHECK_THROWS_AS(InspectSaveWireFacts(short_reader), SaveFramingError); CHECK(short_reader->position == 0);
	}
}

/** Literal payloads were prepared independently with Python zlib/lzma and an LZO literal run; no native writer supplies expected decoded bytes. */
TEST_CASE("Save envelope codecs replay actual file and buffered network readers", "[save-envelope-inspection]")
{
	const auto decoded = EnvelopeHex("544553540000000310203000000000");
	const std::pair<std::string_view, std::string_view> streams[]{
		{"OTTN", "544553540000000310203000000000"},
#ifdef WITH_ZLIB
		{"OTTZ", "789c0b710d0e6160606016503000520c00132001a4"},
#endif
#ifdef WITH_LIBLZMA
		{"OTTX", "fd377a585a000004e6d6b4460200210116000000742fe5a301000e5445535400000003102030000000000000126397f7fea576680001270fdf1afc6a1fb6f37d010000000004595a"},
#endif
#ifdef WITH_LZO
		{"OTTD", "1c0901e70000001320544553540000000310203000000000110000"},
#endif
	};
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	const auto output = root / "test-cache/merge-supply/save-envelope-codec.bin";
	std::filesystem::create_directories(output.parent_path());
	// Each codec uses the exact same logical input and both actual raw-reader
	// implementations. Repeated inspection must construct fresh decoder state.
	for (const auto &[tag, encoded] : streams) {
		INFO(tag);
		const auto original = EnvelopeStream(EnvelopeHex(encoded), tag);
		{ std::ofstream stream(output, std::ios::binary); stream.write(reinterpret_cast<const char *>(original.data()), original.size()); REQUIRE(stream.good()); }
		auto file = FileHandle::Open(output.string(), "rb"); REQUIRE(file.has_value());
		auto raw_file = CreateSaveFileReader(std::move(*file));
		auto network = EnvelopeNetworkReader(original);
		const auto expected = InspectSaveWireFacts(raw_file);
		CHECK(expected.framing_complete); CHECK(expected.common_transport_valid);
		CHECK(InspectSaveWireFacts(raw_file).source_digest == expected.source_digest);
		CHECK(InspectSaveWireFacts(network).source_digest == expected.source_digest);
		CHECK(network->read_bytes == 0);
		CHECK(EnvelopeReplay(raw_file) == decoded); CHECK(EnvelopeReplay(network) == decoded);
		// A damaged or truncated compressed trailer cannot become a valid short
		// save merely because the logical chunk terminator was already decoded.
		if (tag != "OTTN") {
			auto damaged = original; damaged.back() ^= 0xFF;
			CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(damaged)), SaveFramingError);
			damaged = original; damaged.pop_back();
			CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(damaged)), SaveFramingError);
		}
	}
}

/** Hostile byte claims fail before allocations or world initialization; synthetic signatures are intentionally independent of gameplay pools. */
TEST_CASE("Save envelope rejects duplicate owners malformed headers and work overflow", "[save-envelope-inspection]")
{
	const std::string_view malformed[]{
		"5445535400000000544553540000000000000000", // Duplicate original chunk ID.
		"5445535400000005aa00000000", // Claimed RIFF exceeds actual input.
		"544553540500000000", // Unsupported chunk kind.
		"5441424c030000000000", // Table cannot omit its header row.
		"5441424c03020c0000000000", // Unsupported native primitive type.
		"5441424c0308040178040178000000000000", // Duplicate table column.
		"5441424c03071b0178000000000000", // Missing child struct-header terminator.
		"535052530202800000000000", // Sparse index escapes its own row.
		"544553540000000000000000ff", // Bytes after the decoded chunk terminator.
		"4d4c4c4e000000084d4c4e310001000100000000", // Merged marker contradicts legacy envelope.
	};
	for (const auto hex : malformed) CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(EnvelopeStream(EnvelopeHex(hex)))), SaveFramingError);
	const auto input = EnvelopeStream(EnvelopeHex("544553540000000310203000000000"));
	SaveInspectionLimits limits;
	limits.raw_bytes = input.size() - 1; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input), limits), SaveFramingError);
	limits = {}; limits.decoded_bytes = 14; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input), limits), SaveFramingError);
	limits = {}; limits.chunks = 0; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(input), limits), SaveFramingError);
	const auto table = EnvelopeStream(EnvelopeHex("5441424c0305040178000301020000000000"));
	limits = {}; limits.header_bytes = 3; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(table), limits), SaveFramingError);
	limits = {}; limits.fields = 0; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(table), limits), SaveFramingError);
	limits = {}; limits.rows = 0; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(table), limits), SaveFramingError);
	limits = {}; limits.depth = 0; CHECK_THROWS_AS(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(table), limits), SaveFramingError);
}
