/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_commands_fixture.h Actual owned-company input through native peer command transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group17C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * No test callback executes a command directly or changes vehicle/order/packet state.
 */
#ifndef TESTS_GROUND_NETWORK_COMMANDS_FIXTURE_H
#define TESTS_GROUND_NETWORK_COMMANDS_FIXTURE_H

#include "ground_network_fixture.h"
#include "../vehicle_cmd.h"
#include "../train.h"
#include "../command_func.h"
#include "../core/backup_type.hpp"
#include <charconv>

/**
 * Post real client input and observe its later authoritative physical effects on both peers.
 * @param frame Actual network frame after agreed commands,not a wall-clock or replay index.
 * @param server Whether this application is the real dedicated host rather than its client.
 */
inline void ObserveGroundNetworkCommands(uint32_t frame, bool server)
{
	constexpr std::array<uint32_t, 4> observed = {256, 384, 512, 640};
	if (std::ranges::find(observed, frame) == observed.end()) return;
	const std::string_view input = GroundNetworkArgument("OTTD_GROUND_NETWORK_CAPSULE");
	uint32_t value = 0;
	const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
	REQUIRE(parsed.ec == std::errc{});
	REQUIRE(parsed.ptr == input.data() + input.size());
	const auto id = VehicleID{value};
	const auto *capsule = Train::Get(id);
	REQUIRE(capsule->owner == CompanyID{0});
	const bool stopped = capsule->vehstatus.Test(VehState::Stopped);
	if (frame == 256) {
		CHECK_FALSE(stopped);
	} else {
		CHECK(capsule->name == "Paid peer capsule");
		CHECK(stopped == (frame == 384));
	}
	fmt::print("GROUND_NETWORK_COMMAND_STATE role={} frame={} vehicle={} owner={} stopped={} name={}\n",
		server ? "server" : "client", frame, id.base(), capsule->owner.base(), stopped, capsule->name);
	if (server) return;
	REQUIRE(_local_company == capsule->owner);
	// Native UI input normally posts with the already joined local company.
	// This timer runs inside StateGameLoop's OWNER_NONE phase,so borrow that
	// actual company only for Post and restore it before simulation continues.
	// The real server still verifies the received packet against client_playas.
	AutoRestoreBackup actor{_current_company, _local_company};
	if (frame == 256) {
		CHECK(Command<Commands::RenameVehicle>::Post(id, std::string{"Paid peer capsule"}));
		CHECK(Command<Commands::StartStopVehicle>::Post(capsule->tile, id, true));
	}
	if (frame == 384) {
		CHECK(Command<Commands::StartStopVehicle>::Post(capsule->tile, id, true));
	}
}

#endif /* TESTS_GROUND_NETWORK_COMMANDS_FIXTURE_H */
