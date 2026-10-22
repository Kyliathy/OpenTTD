/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file test_input_sha256.h Portable SHA256 for the existing immutable input catalogue.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10.
 * Implements the SHA256 byte identity already used by Python without adding a
 * platform crypto dependency. This helper owns no gameplay or asset identities.
 */

#ifndef TESTS_TEST_INPUT_SHA256_H
#define TESTS_TEST_INPUT_SHA256_H

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

/**
 * Compress one complete SHA256 transport block, using unsigned modulo arithmetic.
 * @param state Eight accumulated digest words, updated with this block.
 * @param block Exactly64 input/padding bytes in their original order.
 */
inline void CompressTestInputSha256(std::array<uint32_t, 8> &state, const std::array<uint8_t, 64> &block)
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

/**
 * Read the full immutable input identity without modifying or buffering the file.
 * @param path Explicit contained file whose bytes must match the input catalogue.
 * @return Lowercase64-character SHA256, identical to Python hashlib.sha256.
 * @throws std::runtime_error If reading fails or the input changes during hashing.
 */
inline std::string TestInputSha256(const std::filesystem::path &path)
{
	const auto size = std::filesystem::file_size(path);
	const auto modified = std::filesystem::last_write_time(path);
	std::ifstream stream(path, std::ios::binary);
	if (!stream) throw std::runtime_error("cannot read input hash: " + path.string());
	std::array<uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
	std::array<uint8_t, 64> block{};
	uint64_t length = 0;
	// Stream every block so large Classic saves have bounded working memory.
	// A read failure must never yield a successful digest of only a prefix.
	while (true) {
		stream.read(reinterpret_cast<char *>(block.data()), block.size());
		const auto count = static_cast<size_t>(stream.gcount());
		length += count;
		// EOF closes a complete digest; other stream failures invalidate identity.
		if (stream.bad() || (stream.fail() && !stream.eof())) throw std::runtime_error("input hash read failed");
		if (count == block.size()) {
			CompressTestInputSha256(state, block);
			continue;
		}
		block[count] = 0x80;
		std::fill(block.begin() + count + 1, block.end(), uint8_t{0});
		// SHA256 reserves eight bytes for bit length. A nearly full final
		// block therefore requires its own following padding block.
		if (count >= 56) {
			CompressTestInputSha256(state, block);
			block.fill(0);
		}
		const auto bits = length * 8;
		for (size_t index = 0; index < 8; ++index) block[63 - index] = static_cast<uint8_t>(bits >> (index * 8));
		CompressTestInputSha256(state, block);
		break;
	}
	// A simultaneous writer cannot leave a successful identity spanning two
	// different inputs, even if it happens to preserve the final byte length.
	if (length != size || size != std::filesystem::file_size(path) || modified != std::filesystem::last_write_time(path)) throw std::runtime_error("input changed while hashing");
	static constexpr char hex[] = "0123456789abcdef";
	std::string result;
	result.reserve(64);
	for (const auto word : state) {
		for (int shift = 28; shift >= 0; shift -= 4) result += hex[(word >> shift) & 15];
	}
	return result;
}

#endif /* TESTS_TEST_INPUT_SHA256_H */
