/* This file is part of OpenTTD, licensed under the GNU General Public License, version 2. */
/**
 * @file sha256.hpp Bounded incremental identity for original byte streams, shared with immutable input verification.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md.
 * Byte identity does not select a dialect or grant gameplay/content authority.
 */
#ifndef CORE_SHA256_HPP
#define CORE_SHA256_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>

/** Incremental SHA256 with one64-byte working block; obtaining a digest never consumes or rewinds the observed source. */
class Sha256 {
	std::array<uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}; ///< Original accumulated compression words.
	std::array<uint8_t, 64> block{}; ///< At most one unfinished original block, never a complete save payload.
	size_t used = 0; ///< Original uncompressed hash-input bytes in the current block.
	uint64_t length = 0; ///< Total original source bytes, bounded before bit-length conversion.

	/** @param state Accumulated digest words, updated with this block. @param block Exactly64original/padding bytes, read in fixed network order. */
	static void Compress(std::array<uint32_t, 8> &state, const std::array<uint8_t, 64> &block)
	{
		static constexpr std::array<uint32_t, 64> constants{
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
		std::array<uint32_t, 64> words{};
		// File identity is byte-exact on every host; explicitly load big-endian
		// words rather than inheriting the machine's native integer byte order.
		for (size_t index = 0; index < 16; ++index) {
			const auto start = index * 4;
			words[index] = uint32_t(block[start]) << 24 | uint32_t(block[start + 1]) << 16 | uint32_t(block[start + 2]) << 8 | block[start + 3];
		}
		// The full standard schedule and rounds bind every input byte. Truncated
		// block/filename fingerprints cannot protect retained save/config contents.
		for (size_t index = 16; index < words.size(); ++index) {
			const auto first = std::rotr(words[index - 15], 7) ^ std::rotr(words[index - 15], 18) ^ (words[index - 15] >> 3);
			const auto second = std::rotr(words[index - 2], 17) ^ std::rotr(words[index - 2], 19) ^ (words[index - 2] >> 10);
			words[index] = words[index - 16] + first + words[index - 7] + second;
		}
		auto a = state[0], b = state[1], c = state[2], d = state[3];
		auto e = state[4], f = state[5], g = state[6], h = state[7];
		for (size_t index = 0; index < words.size(); ++index) {
			const auto upper_e = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
			const auto choose = (e & f) ^ (~e & g);
			const auto first = h + upper_e + choose + constants[index] + words[index];
			const auto upper_a = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
			const auto majority = (a & b) ^ (a & c) ^ (b & c);
			const auto second = upper_a + majority;
			h = g; g = f; f = e; e = d + first; d = c; c = b; b = a; a = first + second;
		}
		const std::array<uint32_t, 8> final{a, b, c, d, e, f, g, h};
		for (size_t index = 0; index < state.size(); ++index) state[index] += final[index];
	}

public:
	/** Begin an empty original byte-stream identity without allocating storage. */
	Sha256() = default;

	/**
	 * Observe every supplied original byte in order, retaining only an unfinished block.
	 * @param bytes Next contiguous original stream fragment; empty fragments change nothing.
	 * @throws std::length_error Before mutation if total bytes would overflow SHA256's64-bit bit-length field.
	 */
	void Update(std::span<const uint8_t> bytes)
	{
		if (bytes.size() > std::numeric_limits<uint64_t>::max() / 8 - this->length) throw std::length_error("SHA256 input exceeds bit-length bound");
		this->length += bytes.size();
		// Fragmentation is an I/O detail, never source identity. Fill/compress
		// each complete original block once regardless of network packet sizes.
		while (!bytes.empty()) {
			const size_t take = std::min(bytes.size(), this->block.size() - this->used);
			std::copy_n(bytes.begin(), take, this->block.begin() + this->used);
			this->used += take; bytes = bytes.subspan(take);
			if (this->used == this->block.size()) { Compress(this->state, this->block); this->used = 0; }
		}
	}

	/** @return Exact32-byte SHA256 in fixed network order; repeated inspection leaves the original streaming state unchanged. */
	std::array<uint8_t, 32> Digest() const
	{
		auto final = *this;
		final.block[final.used++] = 0x80;
		// The original bit length occupies the final eight bytes. A nearly full
		// block needs a separate padding block; no padding becomes source input.
		if (final.used > 56) {
			std::fill(final.block.begin() + final.used, final.block.end(), uint8_t{0});
			Compress(final.state, final.block); final.used = 0;
		}
		std::fill(final.block.begin() + final.used, final.block.begin() + 56, uint8_t{0});
		const uint64_t bits = this->length * 8;
		for (size_t index = 0; index < 8; ++index) final.block[63 - index] = static_cast<uint8_t>(bits >> (index * 8));
		Compress(final.state, final.block);
		std::array<uint8_t, 32> digest{};
		// Explicit byte order makes native network/file observations agree with
		// the existing independent input catalogue on every host architecture.
		for (size_t index = 0; index < digest.size(); ++index) digest[index] = static_cast<uint8_t>(final.state[index / 4] >> (24 - (index % 4) * 8));
		return digest;
	}
};

#endif /* CORE_SHA256_HPP */
