/* This file is part of OpenTTD, licensed under the GNU General Public License, version 2. */
/**
 * @file save_envelope_identity.cpp Independent raw identity and fragmentation boundaries before any native load.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md.
 * Literal digests were independently checked with Python hashlib.sha256.
 * The existing catalogue adapter rebuilds its disposable abc input below
 * test-cache/merge-supply/; no original fixture is written or rehashed as a baseline.
 */
#ifdef SAVE_ENVELOPE_IDENTITY_STANDALONE
#define CATCH_CONFIG_MAIN
#else
#include "../stdafx.h"
#endif
#include "../3rdparty/catch2/catch.hpp"
#include "../core/sha256.hpp"
#include "test_input_sha256.h"

/** @param bytes Complete32-byte original digest. @return Lowercase hexadecimal for comparison with independent literal identities. */
static std::string DigestHex(const std::array<uint8_t, 32> &bytes)
{
	static constexpr char hex[] = "0123456789abcdef"; std::string result;
	for (const auto byte : bytes) { result += hex[byte >> 4]; result += hex[byte & 15]; }
	return result;
}

/** One independently specified padding/block boundary; neither expected quantity nor digest is produced by the implementation under test. */
struct ShaIdentityVector {
	size_t length; ///< Number of literal ASCII a bytes in the original stream.
	const char *digest; ///< Independent whole-stream SHA256.
};

/** All original bytes, including final-block boundaries, bind the same identity regardless of read/network fragmentation or repeated observation. */
TEST_CASE("Save origin identity retains all original bytes across fragmented reads", "[save-envelope-identity]")
{
	static constexpr ShaIdentityVector vectors[]{
		{0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
		{1, "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb"},
		{55, "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
		{56, "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
		{63, "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
		{64, "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
		{65, "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"},
		{120, "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c"},
		{1000, "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3"},
	};
	std::array<uint8_t, 1000> input{}; input.fill('a');
	// Independent literal lengths straddle the standard padding and complete
	// block boundaries; packetized input must identify those same original bytes.
	for (const auto &row : vectors) {
		INFO("original bytes " << row.length); const auto bytes = std::span{input}.first(row.length); Sha256 whole, fragmented;
		whole.Update(bytes);
		for (size_t offset = 0; offset < bytes.size(); offset += 17) fragmented.Update(bytes.subspan(offset, std::min<size_t>(17, bytes.size() - offset)));
		CHECK(DigestHex(whole.Digest()) == row.digest); CHECK(whole.Digest() == fragmented.Digest());
		const auto observed = fragmented.Digest(); fragmented.Update({}); CHECK(fragmented.Digest() == observed);
		const std::array<uint8_t, 1> suffix{'b'}; fragmented.Update(suffix); CHECK(fragmented.Digest() != observed);
	}
}

/** The existing immutable-input adapter and production stream identity agree on original bytes without changing a retained input owner. */
TEST_CASE("Existing input catalogue adapter uses shared raw stream identity", "[save-envelope-identity]")
{
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	const auto cache = root / "test-cache" / "merge-supply"; std::filesystem::create_directories(cache);
	const auto path = cache / "sha256-catalogue-adapter.bin";
	{ std::ofstream stream(path, std::ios::binary); REQUIRE(stream.good()); stream.write("abc", 3); REQUIRE(stream.good()); }
	CHECK(TestInputSha256(path) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	Sha256 hash; const std::array<uint8_t, 3> bytes{'a', 'b', 'c'}; hash.Update(bytes);
	CHECK(DigestHex(hash.Digest()) == TestInputSha256(path));
}
