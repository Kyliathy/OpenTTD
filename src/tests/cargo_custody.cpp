/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_custody.cpp Actual native owner traversal and unchanged-observation checks.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native physical ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md declared component scope.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "native_cargo_custody_fixture.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/** Every actual family supplies its own cargo and queue identity exactly once, without allocating or changing native state. */
TEST_CASE("Native cargo visitor covers all owners including four manufacture inputs", "[native-cargo-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; rig.Populate();
	REQUIRE(rig.expected.size() == 19);
	const auto packets = CargoPacket::GetNumItems(); const auto vehicles = Vehicle::GetNumItems();
	const auto pods = SpacePod::GetNumItems(); const auto sites = SpaceStructure::GetNumItems(); const auto stations = Station::GetNumItems();
	const auto cash = Company::Get(_current_company)->money; const auto random = _random;
	std::set<CargoPacketID> observed;
	/** Compare each actual callback to independent fixture declarations.
	 * @param packet Actual visited fragment.
	 * @param cargo Container meaning resolved by production traversal.
	 * @param queue Actual station goods queue or non-station sentinel.
	 * @return Whether every original identity/meaning/quantity is exactly represented once.
	 */
	const auto verify = [&](const CargoPacket &packet, CargoType cargo, StationID queue) {
		const auto expected = rig.expected.find(packet.index);
		if (expected == rig.expected.end()) return false;
		return observed.insert(packet.index).second && CargoSpec::Get(cargo)->label == expected->second.label && queue == expected->second.queue && packet.Count() == expected->second.quantity;
	};
	CHECK(VisitNativeCargoPackets(verify)); CHECK(observed.size() == rig.expected.size());
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(Vehicle::GetNumItems() == vehicles); CHECK(SpacePod::GetNumItems() == pods);
	CHECK(SpaceStructure::GetNumItems() == sites); CHECK(Station::GetNumItems() == stations); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	// A second observation must see the same actual references, not a modified
	// queue, detached copy, consumed packet or newly created empty owner.
	observed.clear(); CHECK(VisitNativeCargoPackets(verify)); CHECK(observed.size() == 19);
}

/** Empty consumers and consumer refusal stop observation without acquiring any owner or creating goods data. */
TEST_CASE("Native cargo visitor refuses absent consumers and preserves an empty world", "[native-cargo-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig;
	CHECK_FALSE(VisitNativeCargoPackets({}));
	size_t calls = 0;
	/** @param packet Unused original packet. @param cargo Unused actual meaning. @param queue Unused actual queue. @return False on any unexpected call. */
	const auto refuse = [&]([[maybe_unused]] const CargoPacket &packet, [[maybe_unused]] CargoType cargo, [[maybe_unused]] StationID queue) { ++calls; return false; };
	CHECK(VisitNativeCargoPackets(refuse)); CHECK(calls == 0);
	rig.Populate(); const auto count = CargoPacket::GetNumItems();
	CHECK_FALSE(VisitNativeCargoPackets(refuse)); CHECK(calls == 1); CHECK(CargoPacket::GetNumItems() == count);
}

/** Missing physical references, undefined cargo/recipes and occupied undeclared slots cannot silently disappear from custody. */
TEST_CASE("Native cargo visitor refuses unresolved owner evidence without repairing it", "[native-cargo-custody]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeCargoCustodyRig rig; rig.Populate();
	const auto fault = GENERATE(0, 1, 2, 3, 4);
	const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	/** @param packet Existing physical packet. @param cargo Existing meaning. @param queue Existing queue. @return True; only traversal-owned structural checks are tested here. */
	const auto accept = []([[maybe_unused]] const CargoPacket &packet, [[maybe_unused]] CargoType cargo, [[maybe_unused]] StationID queue) { return true; };
	switch (fault) {
		case 0: {
			auto *refs = const_cast<CargoPacketList *>(rig.train->cargo.Packets()); refs->push_back(nullptr);
			const bool accepted = VisitNativeCargoPackets(accept); refs->pop_back(); // Restore the deliberately corrupt test reference before native teardown.
			CHECK_FALSE(accepted); break;
		}
		case 1: {
			const auto cargo = rig.train->cargo_type; rig.train->cargo_type = INVALID_CARGO;
			CHECK_FALSE(VisitNativeCargoPackets(accept)); rig.train->cargo_type = cargo; break;
		}
		case 2:
			rig.factory->module->manufacture->model = 0; CHECK_FALSE(VisitNativeCargoPackets(accept)); rig.factory->module->manufacture->model = 21; break;
		case 3:
			rig.factory->module->manufacture->model = 15; CHECK_FALSE(VisitNativeCargoPackets(accept)); rig.factory->module->manufacture->model = 21; break;
		case 4: {
			const auto kind = rig.blueprint->state.kind; rig.blueprint->state.kind = static_cast<SpaceBuildKind>(UINT8_MAX);
			CHECK_FALSE(VisitNativeCargoPackets(accept)); rig.blueprint->state.kind = kind; break;
		}
	}
	CHECK(VisitNativeCargoPackets(accept)); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}
