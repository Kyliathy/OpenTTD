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
#include <cstdint>
#include "../core/sha256.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

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
	Sha256 hash;
	std::array<uint8_t, 65536> block{};
	uint64_t length = 0;
	// Stream the full exact input through the shared identity primitive. A
	// short read failure cannot masquerade as a successful prefix identity.
	while (true) {
		stream.read(reinterpret_cast<char *>(block.data()), block.size());
		const auto count = static_cast<size_t>(stream.gcount());
		length += count;
		if (stream.bad() || (stream.fail() && !stream.eof())) throw std::runtime_error("input hash read failed");
		hash.Update(std::span{block}.first(count));
		if (stream.eof()) break;
	}
	// A simultaneous writer cannot leave a successful identity spanning two
	// different inputs, even if it happens to preserve the final byte length.
	if (length != size || size != std::filesystem::file_size(path) || modified != std::filesystem::last_write_time(path)) throw std::runtime_error("input changed while hashing");
	static constexpr char hex[] = "0123456789abcdef";
	std::string result;
	result.reserve(64);
	// Preserve the catalogue's lowercase hexadecimal encoding; identity is
	// produced by the same byte-stream primitive used for native preflight.
	for (const auto byte : hash.Digest()) {
		result += hex[byte >> 4]; result += hex[byte & 15];
	}
	return result;
}

#endif /* TESTS_TEST_INPUT_SHA256_H */
