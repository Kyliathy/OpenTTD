/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_model.cpp Pure canonical geometry and actual prototype physical-part checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups50-53.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "../transport_tube_model.h"
#include "../safeguards.h"

/** Logical geometry is available before native capsule activation and borrows the sole canonical content rows. */
TEST_CASE("Tube physical source geometry is pure and rejects foreign movement domains", "[transport-tube-model]")
{
	TBMEngineFixture world;
	const auto engines = Engine::GetNumItems(), vehicles = Vehicle::GetNumItems(), packets = CargoPacket::GetNumItems();
	const Money money = Company::Get(_current_company)->money;
	uint capsule_models = 0;
	// Inspect the whole existing catalogue rather than copying a second list of
	// capsule IDs. No ordinary/orbital row may acquire tube geometry from its name.
	for (const auto &capability : GetMultiLayerVehicleCapabilities()) {
		const auto geometry = GetTubeVehicleGeometry(capability.model_id);
		if (capability.transport != MultiLayerVehicleTransport::Tube) {
			CHECK_FALSE(geometry.has_value());
			continue;
		}
		REQUIRE(geometry.has_value());
		CHECK(geometry->catalogue == &capability);
		CHECK(geometry->vehicle_length_units == VEHICLE_LENGTH);
		CHECK(geometry->native_parts == 1);
		CHECK(GetTubeEngine(capability.model_id) == EngineID::Invalid());
		++capsule_models;
	}
	CHECK(capsule_models == 5);
	CHECK_FALSE(GetTubeVehicleGeometry(UINT16_MAX).has_value());
	CHECK_FALSE(GetTubeVehicleGeometry(19).has_value());
	CHECK(Engine::GetNumItems() == engines);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(Company::Get(_current_company)->money == money);
}

/** A command-bought capsule consumes the same native length contract while geometry queries install nothing. */
TEST_CASE("Tube physical source geometry matches the actual command bought prototype", "[transport-tube-model]")
{
	TubePrototypeRig world;
	const auto geometry = GetTubeVehicleGeometry(9);
	REQUIRE(geometry.has_value());
	Train *capsule = world.BuyCapsule();
	CHECK(capsule->gcache.cached_veh_length == geometry->vehicle_length_units);
	CHECK(capsule->gcache.cached_total_length == geometry->vehicle_length_units);
	CHECK(capsule->Next() == nullptr);
	CHECK(geometry->native_parts == 1);
	CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().shorten_factor == VEHICLE_LENGTH - geometry->vehicle_length_units);
	CHECK(capsule->GetEngine()->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Singlehead);
	const auto engines = Engine::GetNumItems(), vehicles = Vehicle::GetNumItems(), packets = CargoPacket::GetNumItems();
	const auto ledger = ComputeUtilityPowerDigest();
	// Group10 now installs the five distinct native definitions. Pure geometry
	// reads still preserve every installed engine/part identity and utility owner.
	for (const auto &capability : GetMultiLayerVehicleCapabilities()) {
		if (capability.transport != MultiLayerVehicleTransport::Tube) continue;
		const auto other = GetTubeVehicleGeometry(capability.model_id);
		REQUIRE(other.has_value());
		CHECK(other->native_parts == geometry->native_parts);
		CHECK(other->vehicle_length_units == geometry->vehicle_length_units);
		CHECK(GetTubeEngine(capability.model_id) != EngineID::Invalid());
	}
	CHECK(GetTubeEngine(9) == capsule->engine_type);
	CHECK(Engine::GetNumItems() == engines);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(ComputeUtilityPowerDigest() == ledger);
}
