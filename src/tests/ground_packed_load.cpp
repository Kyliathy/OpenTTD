/**
 * @file ground_packed_load.cpp Actual complete-byte loading of selected underground and protected native art.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18B; explicit current source inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.2.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This observer starts the real application without loading or changing a paid world.
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "test_candidate_sprite_input.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../spritecache.h"
#include "../table/sprites.h"
#include "../core/format.hpp"
#include "../string_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../safeguards.h"

/** Read actual normal-zoom palette indices from the native decoder into isolated observation storage. */
class GroundNativeArtEncoder final : public SpriteEncoder {
public:
	/** @return False; approved selected artwork contains native indexed pixels. */
	bool Is32BppSupported() override { return false; }

	/** Preserve actual native decoded geometry and every palette index without changing the game cache.
	 * @param type Requested real sprite type; this source contract requires normal artwork.
	 * @param sprites Actual native decoded zoom collection.
	 * @param allocator Isolated observation storage, separate from the application's cache.
	 * @return Full native indexed geometry/pixels at the authored normal zoom.
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
		// Include internal transparency and remap-mask pixels; geometry-only or
		// RGB previews cannot prove that an occupied foreground was preserved.
		for (size_t i = 0; i < pixels; ++i) output->data[i] = static_cast<std::byte>(source.data[i].m);
		return output;
	}
};

/** Read an explicit little-endian expectation word without relying on the host byte order.
 * @param input Repository-owned full-pixel expectation stream.
 * @return Decoded word; truncated evidence fails the native test.
 */
static uint16_t ReadGroundWord(std::ifstream &input)
{
	std::array<uint8_t, 2> bytes{};
	input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
	REQUIRE(input.good());
	return static_cast<uint16_t>(bytes[0] | static_cast<uint16_t>(bytes[1]) << 8);
}

/** Hash every actual native palette byte as a supplemental portable expectation fingerprint.
 * @param sprite Real normal-zoom native decoded sprite, including transparent pixels.
 * @return FNV-1a32 over the complete row-major index buffer.
 */
static uint32_t HashGroundPixels(const Sprite &sprite)
{
	uint32_t hash = 2166136261U;
	// Full-buffer hashing supplements exact byte equality and catches a
	// malformed fixture before any missing part can count as accepted artwork.
	for (size_t i = 0; i < static_cast<size_t>(sprite.width) * sprite.height; ++i) {
		hash = (hash ^ std::to_integer<uint8_t>(sprite.data[i])) * 16777619U;
	}
	return hash;
}

/** Hidden isolated native startup observes every existing and newly appended palette byte and anchor. */
TEST_CASE("Actual native loader preserves complete prefix and loads selected underground artwork", "[.ground-packed-art-load]")
{
	const auto fixture = GetTestExpectedSprites("ground");
	const auto config = GetTestRuntimeConfig();
	REQUIRE(std::filesystem::exists(fixture));
	REQUIRE(std::filesystem::exists(config));
	// Normal original base graphics and the current shared extra GRF must
	// exist before native startup; no fallback mock can satisfy loader coverage.
	for (const auto name : {"orig_dos.obg", "trg1.grf", "trgc.grf", "trgh.grf", "trgi.grf", "trgt.grf", "openttd.grf"}) {
		REQUIRE(std::filesystem::exists(config.parent_path() / "baseset" / name));
	}
	bool observed = false;
	std::exception_ptr failure;
	/** @param elapsed Actual startup frames only; no paid-world or utility/cargo owner is advanced. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&observed, &fixture, &failure](uint elapsed) {
		if (observed) return;
		try {
			REQUIRE(elapsed > 0);
			GroundNativeArtEncoder encoder;
			SpriteFile *origin = GetOriginFile(SPR_MAP_LAYER_BASE);
			REQUIRE(origin != nullptr);
			REQUIRE(StrEqualsIgnoreCase(std::filesystem::path(origin->GetFilename()).filename().string(), "openttd.grf"));
			ObserveTestCandidateSpriteFile(*origin);
			std::ifstream input(fixture, std::ios::binary);
			REQUIRE(input.good());
			std::array<char, 8> magic{};
			input.read(magic.data(), magic.size());
			REQUIRE(std::string_view(magic.data(), magic.size()) == std::string_view{"GRDART1\0", 8});
			const uint16_t count = ReadGroundWord(input);
			const uint16_t bound = ReadGroundWord(input);
			const uint16_t start = ReadGroundWord(input);
			const uint16_t new_count = ReadGroundWord(input);
			CHECK(count == 22018);
			CHECK(new_count == 14254);
			CHECK(start == GROUND_SPRITE_START);
			CHECK(bound == GROUND_SPRITE_END);
			CHECK(LAUNCH_FLEET_SPRITE_END == start);
			CHECK(MAP_LAYER_SPRITE_COUNT == GROUND_SPRITE_END);
			uint prior_seen = 0;
			uint new_seen = 0;
			uint16_t previous = 0;
			// Each logical native ID is decoded independently, including aliases,
			// hidden vents and absent parents. No hash-only or representative
			// preview substitutes for complete bytes and native world registration.
			for (uint i = 0; i < count; ++i) {
				const uint16_t offset = ReadGroundWord(input);
				const uint16_t width = ReadGroundWord(input);
				const uint16_t height = ReadGroundWord(input);
				const int16_t x = static_cast<int16_t>(ReadGroundWord(input));
				const int16_t y = static_cast<int16_t>(ReadGroundWord(input));
				const uint32_t low = ReadGroundWord(input);
				const uint32_t expected_hash = low | static_cast<uint32_t>(ReadGroundWord(input)) << 16;
				std::vector<uint8_t> expected_pixels(static_cast<size_t>(width) * height);
				input.read(reinterpret_cast<char *>(expected_pixels.data()), expected_pixels.size());
				REQUIRE(input.good());
				INFO("native selected-ground offset " << offset);
				REQUIRE(offset < bound);
				if (i != 0) REQUIRE(offset > previous);
				previous = offset;
				if (offset < start) ++prior_seen;
				else ++new_seen;
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
				REQUIRE(HashGroundPixels(*sprite) == expected_hash);
				REQUIRE(std::equal(expected_pixels.begin(), expected_pixels.end(), reinterpret_cast<const uint8_t *>(sprite->data)));
			}
			CHECK(input.peek() == std::ifstream::traits_type::eof());
			CHECK(prior_seen == 7764);
			CHECK(new_seen == new_count);
			REQUIRE(GetSpriteCountForFile(origin->GetFilename(), SPR_MAP_LAYER_BASE, SPR_MAP_LAYER_BASE + bound) == count);
			observed = true;
			_exit_game = true;
			fmt::print("GROUND_PACKED_ART_LOADED sprites={} protected={} new={} families=47 native_start={} native_bound={} full_pixel_bytes=exact\n", count, prior_seen, new_seen, start, bound);
		} catch (...) {
			// Finish ordinary application shutdown before propagating a failed
			// observation, so no renderer or font/window handle remains live.
			failure = std::current_exception();
			observed = true;
			_exit_game = true;
		}
	});
	const std::string config_string = config.string();
	std::array<std::string_view, 15> arguments = {"openttd_ground_art_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_string};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
