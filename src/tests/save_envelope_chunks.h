/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope_chunks.h Byte-level chunk builders for detached scanner fixtures; no native save writer is involved.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 11-12
 */
#ifndef TESTS_SAVE_ENVELOPE_CHUNKS_H
#define TESTS_SAVE_ENVELOPE_CHUNKS_H
#include "save_envelope_fixture.h"
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

/** One table field as a file's own descriptor declares it: exact name and native type code. */
using EnvelopeField = std::pair<std::string_view, uint8_t>;
/** One complete table descriptor, in the file's own field order. */
using EnvelopeFields = std::vector<EnvelopeField>;

/** Append one native gamma length (one or two bytes, enough for these fixtures).
 * @param out Stream being written.
 * @param value Length to encode; must be below 16384.
 */
inline void PushEnvelopeGamma(std::vector<uint8_t> &out, size_t value)
{
	REQUIRE(value < 16384);
	if (value >= 128) out.push_back(static_cast<uint8_t>(0x80 | (value >> 8)));
	out.push_back(static_cast<uint8_t>(value));
}

/** Append one network-order scalar.
 * @param out Stream being written.
 * @param value Scalar value.
 * @param width Byte width, at most eight.
 */
inline void PushEnvelopeInteger(std::vector<uint8_t> &out, uint64_t value, size_t width)
{
	for (size_t byte = width; byte-- > 0;) out.push_back(static_cast<uint8_t>(value >> (8 * byte)));
}

/** @param type Native scalar type code. @return Its fixed byte width in a flat table row. */
inline size_t EnvelopeFieldWidth(uint8_t type)
{
	static constexpr uint8_t widths[]{0, 1, 1, 2, 2, 4, 4, 8, 8, 2};
	REQUIRE(type < std::size(widths));
	return widths[type];
}

/** Join descriptor parts in order.
 * @param parts Field groups, appended in the order given.
 * @return One descriptor.
 */
inline EnvelopeFields JoinEnvelopeFields(std::initializer_list<const EnvelopeFields *> parts)
{
	EnvelopeFields fields;
	for (const auto *part : parts) fields.insert(fields.end(), part->begin(), part->end());
	return fields;
}

/** Build one contiguous TABLE chunk with its descriptor and zero-filled rows.
 * @param id Four-byte chunk identifier.
 * @param fields Exact flat descriptor, in file order.
 * @param rows Number of zero-filled rows; zero gives an empty table.
 * @return Complete chunk bytes, including the row terminator.
 */
inline std::vector<uint8_t> EnvelopeTableChunk(std::string_view id, const EnvelopeFields &fields, size_t rows = 0)
{
	REQUIRE(id.size() == 4);
	std::vector<uint8_t> header;
	// Each field is its literal type byte, a gamma name length and the name.
	for (const auto &[name, type] : fields) {
		header.push_back(type); PushEnvelopeGamma(header, name.size());
		header.insert(header.end(), name.begin(), name.end());
	}
	header.push_back(0);
	std::vector<uint8_t> chunk(id.begin(), id.end());
	chunk.push_back(3);
	PushEnvelopeGamma(chunk, header.size() + 1);
	chunk.insert(chunk.end(), header.begin(), header.end());
	size_t row_bytes = 0;
	for (const auto &field : fields) row_bytes += EnvelopeFieldWidth(field.second);
	// A contiguous row is a biased gamma length followed by every scalar.
	for (size_t row = 0; row < rows; ++row) {
		PushEnvelopeGamma(chunk, row_bytes + 1);
		chunk.insert(chunk.end(), row_bytes, 0);
	}
	chunk.push_back(0); // Row terminator.
	return chunk;
}

/** Build one RIFF chunk.
 * @param id Four-byte chunk identifier.
 * @param payload Exact payload bytes, below 16 MiB.
 * @return Complete chunk bytes with the native RIFF length.
 */
inline std::vector<uint8_t> EnvelopeRiffChunk(std::string_view id, const std::vector<uint8_t> &payload)
{
	REQUIRE(id.size() == 4);
	REQUIRE(payload.size() < (size_t{1} << 24));
	std::vector<uint8_t> chunk(id.begin(), id.end());
	chunk.push_back(0);
	PushEnvelopeInteger(chunk, payload.size(), 3);
	chunk.insert(chunk.end(), payload.begin(), payload.end());
	return chunk;
}

/** Build one eight-byte MLLN marker chunk.
 * @param magic Payload magic.
 * @param schema Marker schema.
 * @param writer Marker writer version.
 * @return Complete chunk bytes.
 */
inline std::vector<uint8_t> EnvelopeMarkerChunk(uint32_t magic, uint16_t schema, uint16_t writer)
{
	std::vector<uint8_t> payload;
	PushEnvelopeInteger(payload, magic, 4); PushEnvelopeInteger(payload, schema, 2); PushEnvelopeInteger(payload, writer, 2);
	return EnvelopeRiffChunk("MLLN", payload);
}

/** Wrap chunks and the stream terminator in an uncompressed envelope.
 * @param version Outer envelope version.
 * @param chunks Complete chunks in file order.
 * @return Original raw save bytes.
 */
inline std::vector<uint8_t> EnvelopeChunkStream(uint16_t version, std::initializer_list<std::vector<uint8_t>> chunks)
{
	std::vector<uint8_t> payload;
	for (const auto &chunk : chunks) payload.insert(payload.end(), chunk.begin(), chunk.end());
	payload.insert(payload.end(), 4, 0);
	return EnvelopeStream(std::move(payload), "OTTN", version);
}

/** Check that every strict prefix of a stream refuses and rewinds the raw source.
 * @param input Complete original raw bytes.
 */
inline void CheckEnvelopeTruncationsRewind(const std::vector<uint8_t> &input)
{
	// Each prefix ends inside some header, descriptor, row or payload; none may
	// borrow bytes or leave the source positioned mid-stream.
	for (size_t length = 0; length < input.size(); ++length) {
		auto reader = std::make_shared<EnvelopeTestReader>(std::vector<uint8_t>(input.begin(), input.begin() + length));
		CHECK_THROWS_AS(InspectSaveWireFacts(reader), SaveFramingError);
		CHECK(reader->position == 0);
	}
}

#endif /* TESTS_SAVE_ENVELOPE_CHUNKS_H */
