/**
 * @file ground_sprite_selection.cpp Compile and verify the real selected-ground source-pose boundary.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20; retained independent original tuple bytes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Expected tuples/IDs come from the independent qualified source producer,
 * not a second copy of the native pose-offset formula.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../ground_sprite.h"
#include "../core/format.hpp"
#include "test_artifact_paths.h"
#include <fstream>
#include <set>
#include <type_traits>

#include "../safeguards.h"

/** Exact exported handoff signature; transient source axes never become a saved gameplay owner. */
using GroundPoseSignature = std::optional<SpriteID> (*)(GroundSpriteFamily, const MultiLayerSpritePose &);
static_assert(std::is_same_v<decltype(&GetGroundSprite), GroundPoseSignature>);
static_assert(std::is_same_v<std::underlying_type_t<GroundSpriteFamily>, uint8_t>);

/** Read one source-owned byte and refuse truncated independent evidence.
 * @param input Exact source tuple fixture stream.
 * @return The next serialized byte, independent of signed host char representation.
 */
static uint8_t ReadGroundPoseByte(std::ifstream &input)
{
	uint8_t value = 0;
	input.read(reinterpret_cast<char *>(&value), sizeof(value));
	REQUIRE(input.good());
	return value;
}

/** Read a little-endian source-owned word without assuming native byte order.
 * @param input Exact source tuple fixture stream.
 * @return Serialized unsigned word; incomplete evidence fails immediately.
 */
static uint16_t ReadGroundPoseWord(std::ifstream &input)
{
	const auto low = ReadGroundPoseByte(input);
	return low | static_cast<uint16_t>(ReadGroundPoseByte(input)) << 8;
}

/** Actual compiled export must agree with every independently emitted source tuple/native ID. */
TEST_CASE("Typed selected ground poses resolve every independent qualified source tuple", "[ground-sprite-contract]")
{
	const auto fixture = FindTestRepositoryRoot() / "src/tests/fixtures/graphics/ground-pose-tuples.bin";
	std::ifstream input(fixture, std::ios::binary);
	REQUIRE(input.good());
	std::array<char, 8> magic{};
	input.read(magic.data(), magic.size());
	REQUIRE(std::string_view(magic.data(), magic.size()) == std::string_view{"GRDPOS1\0", 8});
	const auto count = ReadGroundPoseWord(input);
	CHECK(count == 14254);
	CHECK(ReadGroundPoseWord(input) == GROUND_SPRITE_START);
	CHECK(ReadGroundPoseWord(input) == GROUND_SPRITE_END);
	CHECK(ReadGroundPoseByte(input) == static_cast<uint8_t>(GroundSpriteFamily::End));
	std::set<SpriteID> resolved;
	// Source fixture order and native offsets are independent of the resolver.
	// Exhaustive agreement catches swapped state/view/cell/parent axes, omitted
	// static aliases and accidental allocation into another published family.
	for (uint i = 0; i < count; ++i) {
		const auto family = static_cast<GroundSpriteFamily>(ReadGroundPoseByte(input));
		MultiLayerSpritePose pose;
		pose.state = ReadGroundPoseByte(input);
		pose.frame = ReadGroundPoseByte(input);
		pose.direction = ReadGroundPoseByte(input);
		pose.tile = ReadGroundPoseWord(input);
		pose.part = ReadGroundPoseByte(input);
		const auto expected = ReadGroundPoseWord(input);
		INFO("independent selected-ground source offset " << expected);
		const auto sprite = GetGroundSprite(family, pose);
		REQUIRE(sprite.has_value());
		CHECK(*sprite == SPR_MAP_LAYER_BASE + expected);
		REQUIRE(resolved.insert(*sprite).second);
	}
	CHECK(input.peek() == std::ifstream::traits_type::eof());
	CHECK(resolved.size() == count);
	CHECK(*resolved.begin() == SPR_MAP_LAYER_BASE + GROUND_SPRITE_START);
	CHECK(*resolved.rbegin() == SPR_MAP_LAYER_BASE + GROUND_SPRITE_END - 1);
	fmt::print("GROUND_POSE_CONTRACT source_tuples={} families=47 native_start={} native_bound={} independent_ids=exact\n", count, GROUND_SPRITE_START, GROUND_SPRITE_END);
}

/** Every undeclared source axis refuses, rather than spilling into a valid neighboring pose/family. */
TEST_CASE("Typed selected ground poses refuse every malformed source axis", "[ground-sprite-contract]")
{
	CHECK_FALSE(GetGroundSprite(GroundSpriteFamily::End, {}));
	CHECK_FALSE(GetGroundSprite(static_cast<GroundSpriteFamily>(255), {}));
	// Each actual source family has different physical dimensions. Check its
	// own first invalid state/frame/view/cell/parent boundary, including single
	// phase controls and high-variant cargo/guideway families without wrapping.
	for (size_t index = 0; index < _ground_sprite_ranges.size(); ++index) {
		const auto family = static_cast<GroundSpriteFamily>(index);
		const auto &range = _ground_sprite_ranges[index];
		REQUIRE(GetGroundSprite(family, {}));
		MultiLayerSpritePose bad;
		bad.state = range.states; CHECK_FALSE(GetGroundSprite(family, bad));
		bad = {}; bad.frame = range.frames; CHECK_FALSE(GetGroundSprite(family, bad));
		bad = {}; bad.direction = range.directions; CHECK_FALSE(GetGroundSprite(family, bad));
		bad = {}; bad.tile = range.tiles; CHECK_FALSE(GetGroundSprite(family, bad));
		bad = {}; bad.part = range.parts; CHECK_FALSE(GetGroundSprite(family, bad));
	}
}
