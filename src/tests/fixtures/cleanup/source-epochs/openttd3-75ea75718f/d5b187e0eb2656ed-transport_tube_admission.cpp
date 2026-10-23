/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_admission.cpp Actual capsule calendar/location, clone, refit and renewal custody boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group12D mandatory spoil separation.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "transport_tube_fixture.h"
#include "tbm_exchange_fixture.h"
#include "../transport_tube_model.h"
#include "../engine_func.h"
#include "../autoreplace_cmd.h"
#include "../vehicle_gui.h"
#include "../widgets/build_vehicle_widget.h"
#include "../safeguards.h"

/** Every model obeys its own introduction and the independent Deep gate despite forced availability flags. */
TEST_CASE("Capsule purchase commands and availability share all five calendar boundaries", "[transport-tube-admission-date]")
{
	const uint16_t model = GENERATE(uint16_t{9}, uint16_t{10}, uint16_t{11}, uint16_t{12}, uint16_t{13});
	TubePrototypeRig world;
	const auto capability = GetTubeVehicleCapability(GetTubeEngine(model));
	REQUIRE(capability.has_value());
	const EngineID engine = capability->engine;
	Engine::Get(engine)->company_avail.Set(_current_company);
	const TileIndex deep = TileXYL(44, 40, MapLayer::DeepUnderground);
	world.Open(std::array{deep, TileAddWrap(deep, 0, 1)});
	REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, deep, world.tube, DiagDirection::SE).Succeeded());
	const auto introduced = TimerGameCalendar::ConvertYMDToDate(capability->catalogue->introduction_year, 0, 1);
	TimerGameCalendar::date = introduced - 1;
	const auto count = Vehicle::GetNumItems();
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random, interactive = _interactive_random;
	CHECK_FALSE(IsEngineBuildable(engine, VehicleType::Train, _current_company));
	// Both command modes must reject every physical depot before any native
	// pool, packet, company or RNG owner changes; shared tube dates do not suffice.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		for (TileIndex depot : {world.depot, deep}) {
			CHECK(std::get<0>(CmdBuildVehicle(flags, depot, engine, false, INVALID_CARGO, ClientID::Invalid)).Failed());
		}
	}
	CHECK(Vehicle::GetNumItems() == count);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(_random.state, random.state));
	CHECK(std::ranges::equal(_interactive_random.state, interactive.state));
	TimerGameCalendar::date = introduced;
	CHECK(IsEngineBuildable(engine, VehicleType::Train, _current_company));
	CHECK(std::get<0>(CmdBuildVehicle({}, world.depot, engine, false, INVALID_CARGO, ClientID::Invalid)).Succeeded());
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, world.depot, engine, false, INVALID_CARGO, ClientID::Invalid)).Succeeded());
	const bool deep_unlocked = capability->catalogue->introduction_year >= TimerGameCalendar::Year{1980};
	CHECK(std::get<0>(CmdBuildVehicle({}, deep, engine, false, INVALID_CARGO, ClientID::Invalid)).Succeeded() == deep_unlocked);
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, deep, engine, false, INVALID_CARGO, ClientID::Invalid)).Succeeded() == deep_unlocked);
	CHECK(Vehicle::GetNumItems() == count + 1 + (deep_unlocked ? 1 : 0));
}

/** A real depot purchase window remains disabled before tube introduction and while Deep is locked. */
TEST_CASE("Native tube depot purchase window uses the same layer and calendar admission", "[transport-tube-admission-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig world;
	TileIndex depot = world.depot;
	bool deep = false;
	SECTION("first Underground capsule") {}
	SECTION("locked Deep depot") { deep = true; }
	if (deep) {
		depot = TileXYL(44, 40, MapLayer::DeepUnderground);
		world.Open(std::array{depot, TileAddWrap(depot, 0, 1)});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, depot, world.tube, DiagDirection::SE).Succeeded());
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{deep ? 1979 : 1969}, 11, 31);
	LayerWindowTestUI ui(WindowClass::BuildVehicle, _current_company);
	ShowBuildVehicleWindow(depot, VehicleType::Train);
	Window *window = FindWindowById(WindowClass::BuildVehicle, depot.base());
	REQUIRE(window != nullptr);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_BV_BUILD));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{deep ? 1980 : 1970}, 0, 1);
	// Native train lists preserve an absent selection on rebuild. Reopen at the
	// unlocked date to exercise the normal first-available selection, without
	// inventing an automatic-selection policy for an already open empty window.
	CloseWindowByClass(WindowClass::BuildVehicle);
	Window::DeleteClosedWindows();
	ShowBuildVehicleWindow(depot, VehicleType::Train);
	window = FindWindowById(WindowClass::BuildVehicle, depot.base());
	REQUIRE(window != nullptr);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_BV_BUILD));
	CHECK(Vehicle::GetNumItems() == 0);
}

/** Loaded labels and every actual packet survive unsupported forms, relabels and reduced native refit capacities. */
TEST_CASE("Capsule loaded refit refuses packet loss before changing native cargo or RNG", "[transport-tube-admission-custody]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	const CargoType original = capsule->cargo_type;
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	const CargoPacketID packet_id = packet->index;
	capsule->cargo.Append(packet);
	const auto count = Vehicle::GetNumItems();
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random, interactive = _interactive_random;
	const CargoType iron = GetCargoTypeByLabel(CargoLabel{"IORE"});
	const CargoType chemicals = GetCargoTypeByLabel(CargoLabel{"CHEM"});
	Engine::Get(capsule->engine_type)->info.refit_mask.Set(chemicals);
	// Supported-but-different labels and a broadened wrong-form mask both refuse
	// before generic refitting could discard existing packets or change their meaning.
	for (CargoType cargo : {iron, chemicals}) {
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
			CHECK(std::get<0>(CmdRefitVehicle(flags, capsule->index, cargo, 0, false, true, 1)).Failed());
		}
	}
	{
		AutoRestoreBackup<uint8_t> capacity{Engine::Get(capsule->engine_type)->VehInfo<RailVehicleInfo>().capacity, uint8_t{8}};
		CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, capsule->index, original, 0, false, true, 1)).Failed());
	}
	CHECK(capsule->cargo_type == original);
	CHECK(capsule->cargo.TotalCount() == 17);
	CHECK(CargoPacket::GetIfValid(packet_id) == packet);
	CHECK(Vehicle::GetNumItems() == count);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(_random.state, random.state));
	CHECK(std::ranges::equal(_interactive_random.state, interactive.state));
	Train *empty = world.BuyCapsule();
	CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, empty->index, iron, 0, false, true, 1)).Succeeded());
	CHECK(empty->cargo_type == iron);
}

/** NoTest clone and renewal validate actual destinations/dates/capacity before allocating or losing custody. */
TEST_CASE("Capsule clone and renewal preserve cargo and paid service on admission refusal", "[transport-tube-admission-custody]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	world.InstallPaidFixturePower(*capsule);
	const auto ordinary = world.BuildDepot(MapLayer::Underground, false);
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
	capsule->cargo.Append(packet);
	const auto count = Vehicle::GetNumItems();
	const auto cash = Company::Get(_current_company)->money;
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest(), binding = ComputeTubePowerVehicleDigest();
	const auto random = _random, interactive = _interactive_random;
	CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, ordinary, capsule->index, false)).Failed());
	{
		AutoRestoreBackup date{TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31)};
		CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, world.depot, capsule->index, false)).Failed());
	}
	Company *company = Company::Get(_current_company);
	company->settings.engine_renew = true;
	company->settings.engine_renew_months = 0;
	capsule->age = capsule->max_age + 1;
	REQUIRE(capsule->NeedsAutorenewing(company));
	{
		AutoRestoreBackup<uint8_t> capacity{Engine::Get(capsule->engine_type)->VehInfo<RailVehicleInfo>().capacity, uint8_t{8}};
		CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, capsule->index).GetErrorMessage() == STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	}
	CHECK(capsule->cargo.TotalCount() == 17);
	CHECK(CargoPacket::GetIfValid(packet->index) == packet);
	CHECK(Vehicle::GetNumItems() == count);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(ComputeUtilityPowerDigest() == ledger);
	CHECK(ComputeTubePowerDigest() == physical);
	CHECK(ComputeTubePowerVehicleDigest() == binding);
	CHECK(std::ranges::equal(_random.state, random.state));
	CHECK(std::ranges::equal(_interactive_random.state, interactive.state));
	const auto [cloned, id] = CmdCloneVehicle(DoCommandFlag::Execute, world.depot, capsule->index, false);
	REQUIRE(cloned.Succeeded());
	CHECK(Train::Get(id)->cargo.TotalCount() == 0);
	CHECK(GetTubePowerVehicle(id) == nullptr);
	CHECK(capsule->cargo.TotalCount() == 17);
	const VehicleID original_id = capsule->index;
	const CargoPacketID packet_id = packet->index;
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, original_id).Succeeded());
	CHECK_FALSE(Vehicle::IsValidID(original_id));
	Train *renewed = nullptr;
	// The clone stays empty and unbound. Identify the separately renewed real
	// carrier by its original packet custody, without relying on pool allocation order.
	for (Train *train : Train::Iterate()) {
		if (train->index != id) renewed = train;
	}
	REQUIRE(renewed != nullptr);
	CHECK(renewed->cargo.TotalCount() == 17);
	CHECK(CargoPacket::GetIfValid(packet_id) == packet);
	CHECK(GetTubePowerVehicle(renewed->index) == nullptr);
	CHECK(GetTubePowerVehicle(original_id) == nullptr);
}

/** Native capsules may carry independent Earth, but cannot acquire the TBM's mandatory wagon/Surface-origin assignment. */
TEST_CASE("Tube capsule cannot replace the actual TBM secondary hauler or loaded assignment", "[transport-tube-spoil-separation]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	TBMExchangeRig rig;
	rig.Assign();
	REQUIRE(InitialiseTubeContentDefinitions(rig.fixture.package));
	const RailType tube = GetTubeRailType();
	const TileIndex depot = TileXYL(45, 45, MapLayer::Underground);
	Engine::Get(GetTubeEngine(9))->company_avail.Set(_current_company);
	Company::Get(_current_company)->avail_railtypes.Set(tube);
	const auto entrance = TileIndexDiffCByDiagDir(DiagDirection::SE);
	rig.fixture.world.Open(std::array{depot, TileAddWrap(depot, entrance.x, entrance.y)});
	const auto depot_result = CmdBuildTrainDepot(DoCommandFlag::Execute, depot, tube, DiagDirection::SE);
	if (depot_result.Failed()) INFO("Native capsule depot refusal: " << GetString(depot_result.GetErrorMessage()));
	REQUIRE(depot_result.Succeeded());
	const auto earth = rig.Wagons(TBMConsistRole::EarthWagon).front()->cargo_type;
	const auto [built, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, depot,
		GetTubeEngine(9), false, earth, ClientID::Invalid);
	REQUIRE(built.Succeeded()); // Independent later/mined Earth remains a legitimate capsule cargo form.
	const Train *capsule = Train::Get(id);
	CHECK(capsule->cargo_type == earth);
	CHECK(capsule->cargo_cap > 0);
	const auto digest = ComputeTBMExchangeDigest();
	const auto cash = Company::Get(_current_company)->money;
	SECTION("command estimates and execution refuse before binding") {
		// Neither a paid design nor its native Train class can grant the cross-layer
		// role; the existing ordinary hauler and real wagon sets remain unchanged.
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
			CHECK(Command<Commands::AssignTBMHauler>::Do(flags, rig.project.id, id).Failed());
		}
		CHECK(GetTBMExchangeAssignmentForHauler(id) == nullptr);
	}
	SECTION("saved-reference publication also refuses capsule substitution") {
		const auto original_assignments = CaptureTBMExchangeAssignments();
		const auto sets = CaptureTBMWagonSets();
		auto forged = original_assignments;
		REQUIRE(forged.size() == 1);
		forged.front().hauler = id; // Fault injection changes one actual captured reference, allocating no invented identity.
		StageLoadedTBMExchange(forged, GetNextTBMExchangeAssignmentID(), sets, GetNextTBMWagonSetID());
		CHECK_FALSE(AcceptLoadedTBMExchange());
		CHECK(ComputeTBMExchangeDigest() == digest);
		// Restore through the same real loader after the deliberate refusal probe,
		// including when this case is run against the older permissive implementation.
		StageLoadedTBMExchange(original_assignments, GetNextTBMExchangeAssignmentID(), sets, GetNextTBMWagonSetID());
		REQUIRE(AcceptLoadedTBMExchange());
	}
	CHECK(ComputeTBMExchangeDigest() == digest);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index) != nullptr);
	CHECK(GetTBMExchangeAssignmentForHauler(id) == nullptr);
}
