/* This file is part of OpenTTD, licensed under the GNU General Public License, version 2. */
/**
 * @file save_framing.h Original byte-framing rules shared by native loading and detached lineage inspection.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md.
 * No chunk handler, loader context, live owner or codec state is consulted here.
 */
#ifndef SAVE_FRAMING_H
#define SAVE_FRAMING_H

#include <cstdint>
#include <stdexcept>

/** A framing refusal independent of native load-error cleanup; callers decide how to report it without touching a world. */
class SaveFramingError final : public std::runtime_error {
public:
	/** @param message Exact byte-rule refusal; no native error or pointer-cleanup callback is invoked. */
	explicit SaveFramingError(const char *message) : std::runtime_error(message) {}
};

/** Original table descriptor bit allocation, shared with the native SavegameFileType wrapper. */
inline constexpr uint8_t SAVE_TABLE_FIELD_LENGTH_BIT = 4;

/** Pure interpretation of one native table descriptor; semantic field contracts remain owner-specific. */
struct SaveTableFieldFraming {
	bool terminal; ///< Zero ends this schema before its child schemas.
	uint8_t primitive; ///< Original native four-bit VarFileType value.
	bool has_length; ///< Arrays, strings and structs carry their own native gamma count.
};

/** Decode the native table type/length bits without consulting a descriptor registry.
 * @param encoded Original descriptor byte, including the zero schema terminator.
 * @return Original field bits; validation of supported primitive types belongs to the caller.
 */
constexpr SaveTableFieldFraming DecodeSaveTableField(uint8_t encoded)
{
	return {encoded == 0, static_cast<uint8_t>(encoded & 15), (encoded & (1U << SAVE_TABLE_FIELD_LENGTH_BIT)) != 0};
}

/**
 * Read the original prefix-width integer used for both array lengths and sparse indices.
 * @tparam ReadByte Callable returning the next original uint8_t; exhaustion belongs to its bounded reader.
 * @param read_byte Reader at the first prefix byte; advances only this supplied stream.
 * @return Original unsigned32-bit quantity; the fifth-byte form ignores the historical unused prefix bits.
 * @throws SaveFramingError For the native unsupported prefix range0xF8-0xFF.
 */
template <typename ReadByte>
uint32_t ReadSaveGamma(ReadByte &&read_byte)
{
	const uint8_t prefix = read_byte();
	if (prefix < 0x80) return prefix;
	uint32_t tails;
	uint32_t value;
	if (prefix < 0xC0) { tails = 1; value = prefix & 0x3F; }
	else if (prefix < 0xE0) { tails = 2; value = prefix & 0x1F; }
	else if (prefix < 0xF0) { tails = 3; value = prefix & 0x0F; }
	else if (prefix < 0xF8) { tails = 4; value = 0; }
	else throw SaveFramingError("Unsupported gamma");
	// Consume precisely the prefix's original remaining bytes. The same rule
	// frames native array/table rows and independent inspection, so a forged
	// length cannot make those readers disagree about the next chunk boundary.
	for (uint32_t index = 0; index < tails; ++index) value = (value << 8) | read_byte();
	return value;
}

/**
 * Decode the native28-bit RIFF payload length in its actual read order.
 * @tparam ReadByte Callable returning one original byte with bounded exhaustion handling.
 * @param mode Original type byte; its upper nibble supplies the high length bits.
 * @param read_byte Reader at the first of the three remaining length bytes.
 * @return Exact original payload length, excluding chunk ID/type/length framing.
 */
template <typename ReadByte>
uint32_t ReadSaveRiffLength(uint8_t mode, ReadByte &&read_byte)
{
	uint32_t length = mode >> 4;
	// Fixed network order preserves the native28-bit RIFF contract without
	// host integer layout or unspecified evaluation order of separate reads.
	for (uint32_t index = 0; index < 3; ++index) length = (length << 8) | read_byte();
	return length;
}

/** Pure original array/table row boundary; ordinal/index and gameplay payload interpretation remain with the caller. */
struct SaveArrayRowFraming {
	bool terminal; ///< Zero encoded length ends the whole family; it is not an empty payload row.
	uint32_t payload_bytes; ///< Original encoded quantity minus its nonzero-row bias, including a sparse index when present.
};

/** @param encoded Original prefix-width row length. @return Terminal or exact remaining row span; an encoded1is an empty contiguous row, not the family terminator. */
inline SaveArrayRowFraming DecodeSaveArrayRow(uint32_t encoded)
{
	return {encoded == 0, encoded == 0 ? 0 : encoded - 1};
}

/** @param mode Original chunk type byte. @return Whether the native Table3/SparseTable4contract requires the leading schema row. */
inline bool SaveChunkHasTableHeader(uint8_t mode)
{
	return (mode & 0x0F) == 3 || (mode & 0x0F) == 4;
}

#endif /* SAVE_FRAMING_H */
