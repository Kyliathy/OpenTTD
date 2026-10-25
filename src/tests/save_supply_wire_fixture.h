/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_supply_wire_fixture.h Bounded original-byte offsets for focused malformed supply inputs.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * Original files remain immutable; these helpers edit detached decompressed copies only.
 */
#ifndef TESTS_SAVE_SUPPLY_WIRE_FIXTURE_H
#define TESTS_SAVE_SUPPLY_WIRE_FIXTURE_H
#include "save_envelope_fixture.h"

/** Exact original row framing; offsets refer only to one supplied detached byte vector. */
struct SupplyWireRow {
	size_t prefix, payload, end; ///< Original gamma start, payload start and exclusive end.
};

/** Exact original chunk framing, without assigning any gameplay meaning or owner. */
struct SupplyWireChunk {
	std::string id; ///< Original four-byte family identifier.
	size_t begin, payload, end; ///< Complete framing start, RIFF payload and exclusive end.
	uint8_t kind; ///< Original native wire nibble.
	std::vector<SupplyWireRow> rows; ///< TABLE schema first, followed by actual framed rows, including empty CAPA holes.
};

/** Read one actual original byte with a test-fixture extent check.
 * @param bytes Entire detached original stream.
 * @param cursor Original read position, advanced exactly once.
 * @return Original byte; malformed fixture construction fails before an out-of-range read.
 */
inline uint8_t SupplyWireByte(std::span<const uint8_t> bytes, size_t &cursor)
{
	REQUIRE(cursor < bytes.size()); return bytes[cursor++];
}

/** Locate actual chunk/row spans using native framing only, without invoking a save writer.
 * @param bytes Entire decompressed retained original, including terminal ID.
 * @return Original disjoint spans; no gameplay payload is interpreted or reconstructed.
 */
inline std::vector<SupplyWireChunk> SupplyWireChunks(std::span<const uint8_t> bytes)
{
	std::vector<SupplyWireChunk> chunks;
	size_t cursor = 0;
	// Original lengths bound each step; locating a family cannot accidentally
	// match its ID inside another payload or rewrite unrelated owner evidence.
	while (cursor < bytes.size()) {
		REQUIRE(bytes.size() - cursor >= 4);
		const auto begin = cursor;
		const std::string id(bytes.begin() + cursor, bytes.begin() + cursor + 4); cursor += 4;
		if (id == std::string(4, '\0')) { REQUIRE(cursor == bytes.size()); break; }
		const auto mode = SupplyWireByte(bytes, cursor);
		SupplyWireChunk chunk{id, begin, cursor, 0, static_cast<uint8_t>(mode & 15), {}};
		/** @return Next bounded original framing byte. */
		const auto read = [&]() { return SupplyWireByte(bytes, cursor); };
		if (chunk.kind == 0) {
			const auto length = ReadSaveRiffLength(mode, read); chunk.payload = cursor;
			REQUIRE(length <= bytes.size() - cursor); cursor += length;
		} else {
			REQUIRE(chunk.kind <= 4);
			// The native nonzero-length bias distinguishes real empty ordinal
			// holes from the family terminator; sparse index bytes stay in payload.
			while (true) {
				const auto prefix = cursor;
				const auto length = ReadSaveGamma(read);
				if (length == 0) break;
				REQUIRE(length - 1 <= bytes.size() - cursor);
				chunk.rows.push_back({prefix, cursor, cursor + length - 1}); cursor += length - 1;
			}
		}
		chunk.end = cursor; chunks.push_back(std::move(chunk));
	}
	return chunks;
}

/** Resolve one exact retained original family, never a byte-pattern match in its payload.
 * @param chunks Previously bounded original spans.
 * @param id Exact four-byte family.
 * @return Borrowed unique original span; absence/duplicates are invalid fixture inputs.
 */
inline const SupplyWireChunk &SupplyWireFind(const std::vector<SupplyWireChunk> &chunks, std::string_view id)
{
	REQUIRE(std::ranges::count(chunks, id, &SupplyWireChunk::id) == 1);
	return *std::ranges::find(chunks, id, &SupplyWireChunk::id);
}

/** Read a fixed flat column's original offset without assuming native struct layout.
 * @param bytes Detached original decompressed stream.
 * @param chunk Exact original TABLE whose first row is its schema.
 * @param name Actual original field name.
 * @return Byte offset within each flat record; only the original fixed integral columns used by these cases are supported.
 */
inline size_t SupplyWireColumn(std::span<const uint8_t> bytes, const SupplyWireChunk &chunk, std::string_view name)
{
	REQUIRE(chunk.kind == 3); REQUIRE_FALSE(chunk.rows.empty());
	size_t cursor = chunk.rows.front().payload, offset = 0;
	// Read actual field order/types from the original header. Host packing or
	// today's descriptor order cannot select a different price or packet link.
	while (cursor < chunk.rows.front().end) {
		const auto type = SupplyWireByte(bytes, cursor);
		if (type == 0) break;
		/** @return Next bounded original schema byte. */
		const auto read = [&]() { return SupplyWireByte(bytes, cursor); };
		const auto length = ReadSaveGamma(read);
		REQUIRE(length <= chunk.rows.front().end - cursor);
		const std::string field(bytes.begin() + cursor, bytes.begin() + cursor + length); cursor += length;
		if (field == name) return offset;
		REQUIRE(type >= 1); REQUIRE(type <= 8);
		offset += size_t{1} << ((type - 1) / 2);
	}
	FAIL("Missing original flat column"); return 0;
}

/** Replace one same-length name in an original flat schema without altering rows/framing.
 * @param bytes Detached mutable original copy.
 * @param chunk Exact original TABLE span.
 * @param original Original field name, required once in the bounded schema.
 * @param replacement Deliberate contradictory field name of identical encoded size.
 */
inline void SupplyWireRename(std::vector<uint8_t> &bytes, const SupplyWireChunk &chunk, std::string_view original, std::string_view replacement)
{
	REQUIRE(original.size() == replacement.size()); REQUIRE_FALSE(chunk.rows.empty());
	const auto begin = bytes.begin() + chunk.rows.front().payload, end = bytes.begin() + chunk.rows.front().end;
	const auto found = std::search(begin, end, original.begin(), original.end()); REQUIRE(found != end);
	std::copy(replacement.begin(), replacement.end(), found);
}

/** Replace one original fixed network-order scalar without resizing its family.
 * @param bytes Detached mutable original copy.
 * @param offset Actual bounded original field offset.
 * @param value Deliberate test scalar, never a native payment/quantity mutation.
 * @param width Original scalar width in bytes.
 */
inline void SupplyWireSet(std::vector<uint8_t> &bytes, size_t offset, uint64_t value, size_t width)
{
	REQUIRE(offset <= bytes.size()); REQUIRE(width <= bytes.size() - offset);
	// Original wire order remains intact; only the explicitly selected scalar
	// changes, so unrelated map/company/container evidence stays the original input.
	for (size_t byte = 0; byte < width; ++byte) bytes[offset + byte] = static_cast<uint8_t>(value >> (8 * (width - byte - 1)));
}

/** Read an explicitly selected original fixed scalar without native host layout.
 * @param bytes Detached original decompressed stream.
 * @param offset Actual original column offset.
 * @param width Original network-order scalar width.
 * @return Exact unsigned original bits; signed semantic admission belongs to the real loader.
 */
inline uint64_t SupplyWireGet(std::span<const uint8_t> bytes, size_t offset, size_t width)
{
	REQUIRE(offset <= bytes.size()); REQUIRE(width <= bytes.size() - offset);
	uint64_t value = 0;
	// Preserve the original scalar's byte order while locating paid input,
	// never a live packet or a receipt-derived replacement quantity.
	for (size_t byte = 0; byte < width; ++byte) value = (value << 8) | bytes[offset + byte];
	return value;
}
#endif /* TESTS_SAVE_SUPPLY_WIRE_FIXTURE_H */
