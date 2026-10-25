/* This file is part of OpenTTD, licensed under the GNU General Public License, version 2. */
/**
 * @file save_envelope_framing.cpp Independent native byte-boundary vectors without a loader, world, codec or chunk handler.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md.
 */
#ifdef SAVE_ENVELOPE_FRAMING_STANDALONE
#define CATCH_CONFIG_MAIN
#else
#include "../stdafx.h"
#endif
#include "../3rdparty/catch2/catch.hpp"
#include "../saveload/save_framing.h"
#include <array>
#include <span>

/** Detached explicit byte span; exhaustion cannot call native pointer cleanup or a gameplay handler. */
class FramingBytes {
	std::span<const uint8_t> bytes; ///< Only this test's original byte input, never a global ReadBuffer.
	size_t position = 0; ///< Actual consumed byte offset, independent of an encoded claimed length.
public:
	/** @param bytes Complete explicit input; no payload is allocated or inferred. */
	explicit FramingBytes(std::span<const uint8_t> bytes) : bytes(bytes) {}
	/** @return Next original byte, or a detached framing exception at exact exhaustion. */
	uint8_t Read()
	{
		if (this->position == this->bytes.size()) throw SaveFramingError("Truncated original bytes");
		return this->bytes[this->position++];
	}
	/** @return Number of actually consumed original bytes, not an encoded quantity. */
	size_t Used() const { return this->position; }
};

/** Explicit original integer encoding plus independent expected value; unused bytes are not part of this stream. */
struct GammaVector {
	std::array<uint8_t, 5> bytes; ///< Literal native prefix and following bytes.
	size_t count; ///< Exact supplied byte count.
	uint32_t value; ///< Independently specified decoded boundary value.
};

/** Literal vectors cover every native prefix width, including the historical ignored bits in the fifth-byte prefix. */
TEST_CASE("Shared native gamma framing preserves original width and byte order", "[save-envelope-framing]")
{
	static constexpr GammaVector vectors[]{
		{{0}, 1, 0}, {{0x7F}, 1, 127}, {{0x80, 0x80}, 2, 128}, {{0xBF, 0xFF}, 2, 16383},
		{{0xC0, 0x40, 0}, 3, 16384}, {{0xDF, 0xFF, 0xFF}, 3, 2097151},
		{{0xE0, 0x20, 0, 0}, 4, 2097152}, {{0xEF, 0xFF, 0xFF, 0xFF}, 4, 268435455},
		{{0xF0, 0x10, 0, 0, 0}, 5, 268435456}, {{0xF0, 0xFF, 0xFF, 0xFF, 0xFF}, 5, UINT32_MAX},
		{{0xF7, 0x12, 0x34, 0x56, 0x78}, 5, 0x12345678},
	};
	// Full literal encodings verify native integer boundaries. Each shorter
	// prefix must fail without consuming a byte from another row or owner.
	for (const auto &row : vectors) {
		FramingBytes bytes(std::span{row.bytes}.first(row.count));
		/** @return Next original test byte, with detached bounded exhaustion. */
		auto read = [&bytes]() { return bytes.Read(); };
		CHECK(ReadSaveGamma(read) == row.value); CHECK(bytes.Used() == row.count);
		for (size_t length = 0; length < row.count; ++length) {
			FramingBytes truncated(std::span{row.bytes}.first(length));
			/** @return Next explicitly truncated byte; no native loader is present. */
			auto short_read = [&truncated]() { return truncated.Read(); };
			CHECK_THROWS_AS(ReadSaveGamma(short_read), SaveFramingError);
		}
	}
	// Unsupported prefix bits cannot become an arbitrary extra byte count or
	// unbounded field length, even when the remaining input looks plausible.
	for (uint16_t prefix = 0xF8; prefix <= 0xFF; ++prefix) {
		const std::array<uint8_t, 1> input{static_cast<uint8_t>(prefix)}; FramingBytes bytes(input);
		/** @return Original unsupported-prefix input byte. */
		auto read = [&bytes]() { return bytes.Read(); };
		CHECK_THROWS_AS(ReadSaveGamma(read), SaveFramingError);
	}
}

/** RIFF high bits, empty contiguous rows and explicit terminators keep their distinct original meanings before any gameplay payload can be dispatched. */
TEST_CASE("Shared native RIFF and array table boundaries retain original framing", "[save-envelope-framing]")
{
	const std::array<uint8_t, 3> input{0x01, 0x23, 0x45}; FramingBytes bytes(input);
	/** @return Original RIFF length byte in actual stream order. */
	auto read = [&bytes]() { return bytes.Read(); };
	CHECK(ReadSaveRiffLength(0xA0, read) == 0x0A012345); CHECK(bytes.Used() == 3);
	const std::array<uint8_t, 3> maximum{0xFF, 0xFF, 0xFF}; FramingBytes largest(maximum);
	/** @return Next byte of the original maximum-length descriptor, without allocating that payload. */
	auto read_largest = [&largest]() { return largest.Read(); };
	CHECK(ReadSaveRiffLength(0xF0, read_largest) == 268435455);
	CHECK(DecodeSaveArrayRow(0).terminal); CHECK(DecodeSaveArrayRow(0).payload_bytes == 0);
	CHECK_FALSE(DecodeSaveArrayRow(1).terminal); CHECK(DecodeSaveArrayRow(1).payload_bytes == 0);
	CHECK(DecodeSaveArrayRow(6).payload_bytes == 5); CHECK(DecodeSaveArrayRow(UINT32_MAX).payload_bytes == 4294967294U);
	CHECK_FALSE(SaveChunkHasTableHeader(0)); CHECK_FALSE(SaveChunkHasTableHeader(1)); CHECK_FALSE(SaveChunkHasTableHeader(2));
	CHECK(SaveChunkHasTableHeader(3)); CHECK(SaveChunkHasTableHeader(4)); CHECK(SaveChunkHasTableHeader(0x13));
	CHECK(DecodeSaveTableField(0).terminal);
	CHECK(DecodeSaveTableField(6).primitive == 6); CHECK_FALSE(DecodeSaveTableField(6).has_length);
	CHECK(DecodeSaveTableField(0x1B).primitive == 11); CHECK(DecodeSaveTableField(0x1B).has_length);
}
