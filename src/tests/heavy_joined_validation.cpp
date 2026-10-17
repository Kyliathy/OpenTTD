/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_joined_validation.cpp Detached same-file cargo joins cover ordinary carriers, stock, handling and every Space container.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD
 * These metadata-only fixtures issue no runtime hull, packet, module, service receipt or inventory rights. Native paid-world evidence verifies actual producers separately.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../fios.h"
#include "../multilayer_content.h"
#include "../saveload/space_cargo_preview.h"
#include "../safeguards.h"

/** Populate separate original reference families without allocating native cargo or validating unrelated gameplay roles.
 * @param data Empty detached preview; actual CAPA IDs are unique and quantities are positive before the join is tested.
 */
static void FillJoinedCargoPreview(LoadCheckData &data)
{
	data.native_cargo_packets_present = data.native_vehicle_cargo_present = data.native_station_cargo_present = true;
	// One distinct declared same-file packet represents each inspected container; these records are transport metadata, never a private cargo pool or a paid-world substitute.
	for (uint32_t id = 10; id <= 20; ++id) data.native_cargo_packet_rows.push_back({CargoPacketID{id}, static_cast<uint16_t>(id - 9)});
	data.native_station_cargo_refs = {11}; data.native_ordinary_vehicle_cargo_refs = {12};
	NativeSpaceVehiclePreview hull; hull.vehicle = VehicleID{22}; hull.type = VehicleType::Space; hull.cargo_packet_refs = {13}; data.native_space_vehicle_rows.push_back(hull);
	SurfaceHandlingRecord handling; handling.packets = {{CargoPacketID{13}, StationID::Invalid()}}; data.surface_handling_rows.push_back(handling);
	SpaceManifestRecord manifest; manifest.supplies[1] = {{CargoPacketID{14}, StationID::Invalid()}}; data.space_manifest_rows.push_back(manifest);
	SpaceStructureRecord site; site.inputs[0] = {{CargoPacketID{15}, StationID::Invalid()}}; auto &module = site.module.emplace();
	module.stock[CargoType{0}] = {{CargoPacketID{16}, StationID::Invalid()}}; module.service.inputs[0] = {{CargoPacketID{17}, StationID::Invalid()}};
	module.dock_service.inputs[1] = {{CargoPacketID{18}, StationID::Invalid()}}; module.manufacture.emplace().inputs[3] = {{CargoPacketID{19}, StationID::Invalid()}};
	data.space_structure_rows.push_back(site); SpaceDescentRecord descent; descent.packets = {{CargoPacketID{20}, StationID::Invalid()}}; data.space_descent_rows.push_back(descent);
}

/** All ordinary/Space containers join without consulting active pools; a missing whole native family cannot borrow a previous preview's completeness. */
TEST_CASE("Copied cargo ownership joins every actual native and feature family", "[heavy-joined-validation]")
{
	LoadCheckData data; FillJoinedCargoPreview(data); REQUIRE(ValidateSpaceCargoPreviewOwners(data)); CHECK(data.native_cargo_packet_rows.size() == 11);
	data.native_station_cargo_present = false; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); data.native_station_cargo_present = true;
	data.native_vehicle_cargo_present = false; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); data.native_vehicle_cargo_present = true;
	data.native_cargo_packets_present = false; CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data)); data.native_cargo_packets_present = true;
	CHECK(ValidateSpaceCargoPreviewOwners(data)); CHECK(data.space_structure_rows.front().module->dock_service.inputs[1].front().first == CargoPacketID{18});
}

/** Cross-family aliases and missing physical references must refuse regardless of which native or feature owner appears first. */
TEST_CASE("Copied cargo joins reject ordinary aliases to paid Space reservations", "[heavy-joined-validation]")
{
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8); LoadCheckData data; FillJoinedCargoPreview(data);
	switch (fault) {
		case 0: data.native_station_cargo_refs.push_back(13); break; // Station claims original Space freight.
		case 1: data.native_ordinary_vehicle_cargo_refs.push_back(21); break; // Ordinary carrier claims descent freight.
		case 2: data.native_station_cargo_refs.push_back(18); break; // Ordinary stock claims first-berth service input.
		case 3: data.surface_handling_rows.front().packets.push_back({CargoPacketID{19}, StationID::Invalid()}); break; // Handling claims manufacture input.
		case 4: data.space_manifest_rows.front().supplies[2].push_back({CargoPacketID{18}, StationID::Invalid()}); break; // Onboard compartment claims second-berth input.
		case 5: data.space_structure_rows.front().module->stock[CargoType{1}] = {{CargoPacketID{20}, StationID::Invalid()}}; break; // Operational module claims descent freight.
		case 6: data.native_ordinary_vehicle_cargo_refs.push_back(0); break; // A null native reference cannot create a packet.
		case 7: data.native_station_cargo_refs.push_back(CargoPacketID::End().base() + 1); break; // Original native reference exceeds physical pool bounds.
		case 8: data.space_descent_rows.front().packets.front().first = CargoPacketID{30}; break; // An absent same-file packet supplies no quantity.
	}
	CHECK_FALSE(ValidateSpaceCargoPreviewOwners(data));
}

/** Independent configured metadata must catch coherent receipt mass changes while retaining genuine unknown legacy receipts. */
TEST_CASE("Copied descent mass joins same-file configured cargo dimensions", "[heavy-joined-validation]")
{
	LoadCheckData data; data.gameplay_profile_id = to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry);
	data.native_cargo_dimensions_present = true; data.native_cargo_dimension_rows = {{CargoType{3}, CargoLabel{"NORE"}, 24}};
	SpaceDescentRecord job; job.cargo = CargoType{3}; job.sender_model = 21; job.unit_weight = 24; data.space_descent_rows.push_back(job);
	CHECK(ValidateSpaceCargoPreviewDimensions(data)); data.space_descent_rows.front().unit_weight = 16;
	CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); // A self-consistent forged job cannot redefine configured mass.
	data.space_descent_rows.front().sender_model = SPACE_DESCENT_LEGACY_SENDER_MODEL; data.space_descent_rows.front().unit_weight = 0;
	CHECK(ValidateSpaceCargoPreviewDimensions(data)); // Genuine old unknown receipts are never assigned a historical mass by migration.
	data.native_cargo_dimension_rows.push_back(data.native_cargo_dimension_rows.front()); CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data));
	data.native_cargo_dimension_rows.resize(1); data.native_cargo_dimensions_present = false; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data));
	data.native_cargo_dimensions_present = true; data.gameplay_profile_id = to_underlying(MultiLayerGameplayProfileID::Classic);
	CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); data.native_cargo_dimension_rows.clear(); CHECK(ValidateSpaceCargoPreviewDimensions(data));
}
