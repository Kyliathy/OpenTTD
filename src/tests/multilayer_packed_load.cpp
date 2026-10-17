/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_packed_load.cpp Actual application-loader proof for packed M1 art and the preserved native M0 prefix.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B/79C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints8.6/10.4B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG; independent final Space append.
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../spritecache.h"
#include "../table/sprites.h"
#include "../core/format.hpp"
#include "../string_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include <filesystem>
#include <fstream>

#include "../safeguards.h"

/** Observe the native decoder's normal-zoom pixels without changing the headless renderer or its cache. */
class NativeArtEncoder final : public SpriteEncoder {
public:
	/** @return False; the selected source contract contains DOS indexed art. */
	bool Is32BppSupported() override { return false; }

	/**
	 * Preserve the actual loader's normal-zoom palette pixels for source comparison.
	 * @param type Requested native sprite type; selected artwork must be normal.
	 * @param sprites Actual native decoded zoom collection, including scaled previews.
	 * @param allocator Isolated observation storage, never the application's sprite cache.
	 * @return Normal-zoom geometry and every decoded palette index.
	 */
	Sprite *Encode(SpriteType type, const SpriteLoader::SpriteCollection &sprites, SpriteAllocator &allocator) override
	{
		REQUIRE(type == SpriteType::Normal);
		const auto &source = sprites[ZoomLevel::Normal];
		const size_t pixels = static_cast<size_t>(source.width) * source.height;
		auto *output = allocator.Allocate<Sprite>(sizeof(Sprite) + pixels);
		output->width = source.width;
		output->height = source.height;
		output->x_offs = source.x_offs;
		output->y_offs = source.y_offs;
		// Compare authored resolution rather than the renderer's fourfold zoom root.
		// Include transparent holes so missing near-mouth masks cannot pass unnoticed.
		for (size_t i = 0; i < pixels; ++i) output->data[i] = static_cast<std::byte>(source.data[i].m);
		return output;
	}
};

/**
 * Read one explicitly little-endian fixture word, independent of host endianness.
 * @param input Repository-owned binary source expectations.
 * @return Decoded uint16 value; refuse truncated evidence.
 */
static uint16_t ReadArtWord(std::ifstream &input)
{
	std::array<uint8_t, 2> bytes{};
	input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
	REQUIRE(input.good());
	return static_cast<uint16_t>(bytes[0] | static_cast<uint16_t>(bytes[1]) << 8);
}

/**
 * Hash actual decoded 8bpp sprite pixels with the source fixture's portable hash.
 * @param sprite Real source-decoded normal-zoom sprite from the observation encoder.
 * @return FNV-1a32 hash including every transparent and material pixel.
 */
static uint32_t HashArtPixels(const Sprite &sprite)
{
	uint32_t hash = 2166136261U;
	// The normal native decoder/encoder owns all pixel interpretation. Comparing
	// its complete row-major output detects palette or foreground-hole changes.
	for (size_t i = 0; i < static_cast<size_t>(sprite.width) * sprite.height; ++i) {
		hash = (hash ^ std::to_integer<uint8_t>(sprite.data[i])) * 16777619U;
	}
	return hash;
}

/** Hidden isolated process: normal startup loads the actual distributed extra GRF, then one timer observes its real cache. */
TEST_CASE("Actual native loader preserves M0 and loads every selected M1 sprite", "[.m1-packed-art-load]")
{
	const auto root = FindTestRepositoryRoot();
	const auto fixture = root / "logs/proofs/mlm/native-art/expected/m1-packed-sprites.bin";
	const auto config = GetTestRuntimeConfig();
	REQUIRE(std::filesystem::exists(fixture));
	REQUIRE(std::filesystem::exists(config));
	// Windows derives its local data search root from the configuration folder.
	// Refuse missing repository data before native startup can display an error dialog.
	for (const auto name : {"orig_dos.obg", "trg1.grf", "trgc.grf", "trgh.grf", "trgi.grf", "trgt.grf", "openttd.grf"}) {
		REQUIRE(std::filesystem::exists(root / "build/baseset" / name));
	}
	bool observed = false;
	std::exception_ptr failure;
	/** @param elapsed Actual frame count; the observer never changes simulation owners or writes a save. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&observed, &fixture, &failure](uint elapsed) {
		if (observed) return;
		try {
			REQUIRE(elapsed > 0);
			NativeArtEncoder encoder;
			const SpriteFile *origin = GetOriginFile(SPR_MAP_LAYER_BASE);
			REQUIRE(origin != nullptr);
			REQUIRE(StrEqualsIgnoreCase(std::filesystem::path(origin->GetFilename()).filename().string(), "openttd.grf"));
			std::ifstream input(fixture, std::ios::binary);
			REQUIRE(input.good());
			std::array<char, 8> magic{};
			input.read(magic.data(), magic.size());
			REQUIRE(std::string_view(magic.data(), magic.size()) == std::string_view{"MLVART1\0", 8});
			const uint16_t count = ReadArtWord(input);
			const uint16_t bound = ReadArtWord(input);
			uint16_t selected = 0;
			for (const auto &range : _multilayer_m1_sprite_ranges) selected += range.count;
			CHECK(count == 66 + selected); // Complete selected scope, including newly appended collector poses, plus the protected M0 prefix.
			CHECK(bound == MULTILAYER_M1_SPRITE_COUNT); // This retained fixture owns M0/M1; the separate Space preview is checked by its native scene.
			// Each source declaration is checked against the real application-loaded
			// sprite cache. Mock cache dimensions or a source-only parser cannot pass.
			for (uint i = 0; i < count; ++i) {
				const uint16_t offset = ReadArtWord(input);
				const uint16_t width = ReadArtWord(input);
				const uint16_t height = ReadArtWord(input);
				const int16_t x = static_cast<int16_t>(ReadArtWord(input));
				const int16_t y = static_cast<int16_t>(ReadArtWord(input));
				const uint32_t low = ReadArtWord(input);
				const uint32_t expected = low | static_cast<uint32_t>(ReadArtWord(input)) << 16;
				INFO("native map-layer offset " << offset);
				const SpriteID id = SPR_MAP_LAYER_BASE + offset;
				REQUIRE(SpriteExists(id));
				REQUIRE(GetSpriteType(id) == SpriteType::Normal);
				REQUIRE(GetOriginFile(id) == origin);
				UniquePtrSpriteAllocator allocator;
				const auto *sprite = static_cast<const Sprite *>(GetRawSprite(id, SpriteType::Normal, &allocator, &encoder));
				REQUIRE(sprite != nullptr);
				REQUIRE(sprite->width == width);
				REQUIRE(sprite->height == height);
				REQUIRE(sprite->x_offs == x);
				REQUIRE(sprite->y_offs == y);
				REQUIRE(HashArtPixels(*sprite) == expected);
			}
			CHECK(input.peek() == std::ifstream::traits_type::eof());
			REQUIRE(GetSpriteCountForFile(origin->GetFilename(), SPR_MAP_LAYER_BASE, SPR_MAP_LAYER_BASE + bound) == count);
			observed = true;
			_exit_game = true; // End only this isolated read-only graphics observer.
			fmt::print("M1_PACKED_ART_LOADED sprites={} native_bound={} protected=66 selected={} pixel_hashes=exact\n", count, bound, selected);
		} catch (...) {
			// Assertion failures must leave native startup through normal shutdown;
			// unwinding through its game loop would skip font/window destruction.
			failure = std::current_exception();
			observed = true;
			_exit_game = true;
		}
	});
	const std::string config_string = config.string();
	std::array<std::string_view, 15> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_string};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

/** Hidden isolated process observes final Space decoder bytes and the unchanged preview prefix; no paid mission state is claimed. */
TEST_CASE("Actual native loader preserves previews and loads all final Space sprites", "[.space-packed-art-load]")
{
	const auto root = FindTestRepositoryRoot();
	const auto fixture = root / "logs/proofs/mlm/native-art/expected/space-packed-sprites.bin";
	const auto config = GetTestRuntimeConfig();
	REQUIRE(std::filesystem::exists(fixture));
	REQUIRE(std::filesystem::exists(config));
	// Native startup must use the real repository base-set payload; an absent data file must fail before a graphical error dialogue can block the headless observer.
	for (const auto name : {"orig_dos.obg", "trg1.grf", "trgc.grf", "trgh.grf", "trgi.grf", "trgt.grf", "openttd.grf"}) {
		REQUIRE(std::filesystem::exists(root / "build/baseset" / name));
	}
	bool observed = false;
	std::exception_ptr failure;
	/** @param elapsed Native startup frame count; presentation observation never advances a mission, quantity or resource owner. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&observed, &fixture, &failure](uint elapsed) {
		if (observed) return;
		try {
			REQUIRE(elapsed > 0);
			NativeArtEncoder encoder;
			const SpriteFile *origin = GetOriginFile(SPR_MAP_LAYER_BASE);
			REQUIRE(origin != nullptr);
			REQUIRE(StrEqualsIgnoreCase(std::filesystem::path(origin->GetFilename()).filename().string(), "openttd.grf"));
			std::ifstream input(fixture, std::ios::binary);
			REQUIRE(input.good());
			std::array<char, 8> magic{};
			input.read(magic.data(), magic.size());
			REQUIRE(std::string_view(magic.data(), magic.size()) == std::string_view{"SPCART1\0", 8});
			const uint16_t count = ReadArtWord(input);
			const uint16_t bound = ReadArtWord(input);
			const uint16_t start = ReadArtWord(input);
			CHECK(count == 1579); // Eleven original preview identities plus all1568 final source poses.
			CHECK(start == MULTILAYER_M1_SPRITE_COUNT); // Existing M1 and preview starts retain their exact prior values.
			CHECK(bound == SPACE_SPRITE_END);
			CHECK(SPACE_SPRITE_START == start + 11);
			CHECK(MAP_LAYER_SPRITE_COUNT == SPACE_SPRITE_END);
			// Every new native pose and protected preview is decoded by the application's actual GRF/cache owner; source parsers or mocks cannot satisfy pixel/anchor equality.
			for (uint i = 0; i < count; ++i) {
				const uint16_t offset = ReadArtWord(input);
				const uint16_t width = ReadArtWord(input);
				const uint16_t height = ReadArtWord(input);
				const int16_t x = static_cast<int16_t>(ReadArtWord(input));
				const int16_t y = static_cast<int16_t>(ReadArtWord(input));
				const uint32_t low = ReadArtWord(input);
				const uint32_t expected = low | static_cast<uint32_t>(ReadArtWord(input)) << 16;
				INFO("native final Space/preview offset " << offset);
				REQUIRE(offset == start + i);
				const SpriteID id = SPR_MAP_LAYER_BASE + offset;
				REQUIRE(SpriteExists(id));
				REQUIRE(GetSpriteType(id) == SpriteType::Normal);
				REQUIRE(GetOriginFile(id) == origin);
				UniquePtrSpriteAllocator allocator;
				const auto *sprite = static_cast<const Sprite *>(GetRawSprite(id, SpriteType::Normal, &allocator, &encoder));
				REQUIRE(sprite != nullptr);
				REQUIRE(sprite->width == width);
				REQUIRE(sprite->height == height);
				REQUIRE(sprite->x_offs == x);
				REQUIRE(sprite->y_offs == y);
				REQUIRE(HashArtPixels(*sprite) == expected);
			}
			CHECK(input.peek() == std::ifstream::traits_type::eof());
			REQUIRE(GetSpriteCountForFile(origin->GetFilename(), SPR_MAP_LAYER_BASE + start, SPR_MAP_LAYER_BASE + bound) == count);
			observed = true;
			_exit_game = true;
			fmt::print("SPACE_PACKED_ART_LOADED sprites={} final=1568 protected_preview=11 native_start={} native_bound={} pixel_hashes=exact\n", count, start, bound);
		} catch (...) {
			// Native shutdown must still release fonts/windows after an assertion failure; preserve the original exception for Catch after normal startup returns.
			failure = std::current_exception();
			observed = true;
			_exit_game = true;
		}
	});
	const std::string config_string = config.string();
	std::array<std::string_view, 15> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_string};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
