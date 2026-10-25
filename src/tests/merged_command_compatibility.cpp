/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file merged_command_compatibility.cpp Independent wire bytes and pre-dispatch merged peer refusal.
 * @see docs/zz-reach2/architecture/commands/archi-commands.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../network/core/network_game_info.h"
#include "../network/core/config.h"
#include "../rev.h"
#include "../safeguards.h"

/** Native U64 intents and donor raw-label transport retain their distinct exact bytes. */
TEST_CASE("Merged supply codecs retain U64 intents and original raw cargo labels", "[merged-command-codec]")
{
	using Native = CommandTraits<Commands::PlaceSurfaceSupplyOrder>::Args;
	static_assert(std::is_same_v<Native, std::tuple<StationID, uint32_t, uint16_t, uint64_t>>);
	const Native native{StationID{0x1234}, 0x52464D54, 100, UINT64_C(0x0102030405060708)};
	const std::array<uint8_t, 16> expected{0x34, 0x12, 0x54, 0x4d, 0x46, 0x52, 100, 0, 8, 7, 6, 5, 4, 3, 2, 1};
	const auto bytes = EndianBufferWriter<CommandDataBuffer>::FromValue(native);
	CHECK(std::ranges::equal(bytes, expected)); CHECK(EndianBufferReader::ToValue<Native>(bytes) == native);
	// This is the original B argument type, not an executable historical command.
	// Its U32 issuer and raw label must never be reinterpreted as A's U64 tuple.
	using OriginalB = std::tuple<StationID, CargoLabel, uint16_t, uint32_t>;
	const OriginalB original{StationID{0x1234}, CargoLabel{"RFMT"}, 100, 7};
	const std::array<uint8_t, 12> donor_expected{0x34, 0x12, 0x52, 0x46, 0x4d, 0x54, 100, 0, 7, 0, 0, 0};
	// Reproduce the pinned donor's raw-label adapter in this test only. Current
	// dispatch deliberately has no original B purchase tuple or replay adapter.
	CommandDataBuffer donor; EndianBufferWriter writer(donor); writer << std::get<0>(original);
	for (uint8_t byte : std::get<1>(original)) writer << byte;
	writer << std::get<2>(original) << std::get<3>(original);
	OriginalB decoded{}; EndianBufferReader reader(donor); reader >> std::get<0>(decoded);
	for (uint8_t &byte : std::get<1>(decoded)) reader >> byte;
	reader >> std::get<2>(decoded) >> std::get<3>(decoded);
	CHECK(std::ranges::equal(donor, donor_expected)); CHECK(decoded == original);
	CHECK(EndianBufferReader::ToValue<Native>(donor) != native);
}

/** The real version gate refuses original and stale merged peers before mixed command admission. */
TEST_CASE("Merged native network identity refuses original and stale protocol peers", "[merged-peer-identity]")
{
	const std::string current(GetNetworkRevisionString());
	REQUIRE(current.size() < NETWORK_REVISION_LENGTH); REQUIRE(IsNetworkCompatibleVersion(current));
	const auto marker = current.find("+m441c223p"); REQUIRE(marker != std::string::npos);
	CHECK_FALSE(IsNetworkCompatibleVersion(current.substr(0, marker))); // Same actual hash, original protocol.
	std::string stale = current; stale[marker + 2] = '0'; CHECK_FALSE(IsNetworkCompatibleVersion(stale));
	stale = current; stale[marker + 6] = '0'; CHECK_FALSE(IsNetworkCompatibleVersion(stale));
	stale = current; stale.back() = stale.back() == '0' ? '1' : '0'; CHECK_FALSE(IsNetworkCompatibleVersion(stale));
	CHECK_FALSE(IsNetworkCompatibleVersion("20261023-master-gcfeff84db5"));
	CHECK_FALSE(IsNetworkCompatibleVersion("20261020-master-g75ea75718f"));
	CHECK_FALSE(IsNetworkCompatibleVersion(""));
	NetworkGameInfo info{}; info.server_revision = current; CheckGameCompatibility(info); CHECK(info.version_compatible);
	info.server_revision = current.substr(0, marker); CheckGameCompatibility(info); CHECK_FALSE(info.version_compatible); CHECK_FALSE(info.compatible);
	if (!_openttd_revision_tagged) {
		const auto hash = current.find_last_of('-'); REQUIRE(hash != std::string::npos);
		CHECK(IsNetworkCompatibleVersion("alias" + current.substr(hash)));
		CHECK_FALSE(IsNetworkCompatibleVersion("dbg_seed-alias" + current.substr(hash)));
	}
}
