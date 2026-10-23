/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_processor_load.cpp Exact choice/work/batch restoration, finite stocks and actual waiting transport packets. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "layer_window_fixture.h"
#include "mock_environment.h"
#include "../saveload/saveload_func.h"
#include "../train.h"
#include "../engine_func.h"
#include "../vehicle_func.h"
#include "processor_fixture.h"
#include "../multilayer_state.h"
#include "../table/sprites.h"
#include "../table/strings.h"

#include "../safeguards.h"

/** Existing original wagon definition and real packet owner exercise vehicle persistence without a native-fleet purchase/refit claim. */
class ProcessorSaveCarrier {
public:
	Train *wagon = nullptr; ///< Actual vehicle owning a packet loaded through the normal station cargo list.
	/** Keep the ordinary carrier's pool untouched until the in-transit checkpoint requests it. */
	ProcessorSaveCarrier() = default;
	/** @param station Actual loading station. @param cargo Actual output label's mapped runtime slot. */
	void Load(Station &station, CargoType cargo)
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		REQUIRE(Engine::GetNumItems() == 0);
		const EngineID id = _engine_mngr.GetID(VehicleType::Train, 29, INVALID_GRFID); // The original engine owner's published Coal Truck, not a local/native model ID.
		REQUIRE(id != EngineID::Invalid());
		SetupEngines(); // Full-world ENGN transport requires the normal dense original-engine pool, not a single sparse test record.
		Engine *engine = Engine::Get(id);
		REQUIRE(engine->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon);
		REQUIRE(Train::CanAllocateItem());
		this->wagon = Train::Create();
		this->wagon->engine_type = id;
		this->wagon->owner = station.owner;
		this->wagon->SetWagon();
		this->wagon->SetFreeWagon();
		this->wagon->tile = station.xy;
		this->wagon->x_pos = TileX(station.xy) * TILE_SIZE + TILE_SIZE / 2;
		this->wagon->y_pos = TileY(station.xy) * TILE_SIZE + TILE_SIZE / 2;
		this->wagon->direction = Direction::NE;
		this->wagon->track = Track::X;
		this->wagon->vehstatus.Set(VehState::Stopped);
		this->wagon->cargo_type = cargo;
		this->wagon->cargo_cap = engine->VehInfo<RailVehicleInfo>().capacity;
		this->wagon->last_station_visited = station.index;
		const std::array next = {StationID::Invalid()};
		REQUIRE(station.goods[cargo].GetData().cargo.Load(2, &this->wagon->cargo, next, station.xy) == 2);
		CHECK(station.goods[cargo].TotalCount() == 0);
		CHECK(this->wagon->cargo.TotalCount() == 2);
	}
	/** Remove this isolated original wagon and its actual packet before the industry's cargo registry/company teardown. */
	~ProcessorSaveCarrier()
	{
		if (this->wagon == nullptr) return;
		_vehicle_pool.CleanPool();
		ResetVehicleHash();
		_engine_pool.CleanPool();
	}
};

/**
 * Deliver and process one complete declared refinery batch through actual intake/work owners.
 * @param rig Command-funded native inventory owner.
 */
static void ProduceRefineryBatch(ProcessorRig &rig)
{
	const auto *recipe = GetLayerProcessorRecipe(*rig.industry);
	REQUIRE(recipe != nullptr);
	// Every required ingredient arrives as delivered stock; no historical/output counter is fabricated for full-capacity fixtures.
	for (const auto &role : recipe->recipe.inputs) {
		REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	}
	AdvanceLayerProcessor(*rig.industry, recipe->recipe.work_economy_ticks);
}

/** Native side state restores only when the frozen choice, actual unions and complete-batch material history agree. */
TEST_CASE("Native processor restoration preserves actual work stocks and committed batches", "[layer-processor-load]")
{
	ProcessorRig rig;
	ProduceRefineryBatch(rig);
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"CORE"}), 4) == 4);
	AdvanceLayerProcessor(*rig.industry, 74); // Coal is missing: retain actual copper and earned work without a batch.
	const auto rows = CaptureLayerProcessors();
	const auto digest = ComputeLayerProcessorDigest();
	const auto reserve = ComputeGeologyStateDigest(GetGeologyWorldState());
	ResetLayerProcessorState();
	StageLoadedLayerProcessors(rows);
	CHECK(GetLayerProcessorState(rig.industry->index) == nullptr);
	REQUIRE(AcceptLoadedLayerProcessors());
	CHECK(CaptureLayerProcessors() == rows);
	CHECK(ComputeLayerProcessorDigest() == digest);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == reserve);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 4);
	CHECK(rig.Output(CargoLabel{"RFMT"}).waiting == 2);
	CHECK(GetLayerProcessorState(rig.industry->index)->batches.at(MultiLayerRecipeID{"CFMT"}) == 1);
	CHECK(AcceptLoadedLayerProcessors());
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"COAL"}), 1) == 1);
	AdvanceLayerProcessor(*rig.industry, 0);
	CHECK(GetLayerProcessorState(rig.industry->index)->batches.at(MultiLayerRecipeID{"CFMT"}) == 2);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 0);
	CHECK(rig.Output(CargoLabel{"RFMT"}).waiting == 4);
}

/** Missing or foreign keys and wrapped/inconsistent counters refuse before a stock participant can use them. */
TEST_CASE("Malformed processor records cannot activate repaired or invented batches", "[layer-processor-load]")
{
	ProcessorRig rig;
	ProduceRefineryBatch(rig);
	const auto saved = CaptureLayerProcessors();
	auto rows = saved;
	SECTION("Missing native row") { rows.clear(); }
	SECTION("Duplicate owner") { rows.push_back(rows[0]); }
	SECTION("Missing owner") { rows[0].industry = IndustryID::Invalid(); }
	SECTION("Foreign recipe") { rows[0].recipe = MultiLayerRecipeID{"PICE"}; }
	SECTION("Missing batch key") { rows[0].batches.erase(MultiLayerRecipeID{"BFMT"}); }
	SECTION("Foreign batch key") { rows[0].batches.emplace(MultiLayerRecipeID{"XXXX"}, 0); }
	SECTION("Forgotten consumption") { rows[0].consumed[CargoLabel{"CORE"}] = 0; }
	SECTION("Invented production") { ++rows[0].produced[CargoLabel{"RFMT"}]; }
	SECTION("Forged station supply") { rows[0].dispatched = 1; }
	SECTION("Wrapped multiplication") { rows[0].batches[MultiLayerRecipeID{"CFMT"}] = UINT64_MAX; }
	SECTION("Missing revision") { rows[0].revision = 0; }
	SECTION("Unbounded work") { rows[0].work_ticks = 297; }
	SECTION("Unbounded remainder") { rows[0].scale_fraction = 200; }
	SECTION("Invalid actual cargo slot") { rig.industry->accepted[0].cargo = INVALID_CARGO; }
	StageLoadedLayerProcessors(std::move(rows));
	CHECK_FALSE(AcceptLoadedLayerProcessors());
	CHECK(CaptureLayerProcessors() == saved);
	CHECK(rig.Output(CargoLabel{"RFMT"}).waiting == 2);
}

/** Full output is produced by real ingredient batches; in-transit output is held by actual station packets awaiting onward service. */
TEST_CASE("Native processor checkpoints write real stock and packet descriptor families", "[layer-processor-save-file]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ProcessorRig rig(MultiLayerRecipeID{"CFMT"}, 30, 30, true);
	LayerSaveContent content(rig.ActivatedContent());
	ProcessorSaveCarrier transport;
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	std::string suffix;
	SECTION("One missing input") {
		ProduceRefineryBatch(rig);
		REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"CORE"}), 4) == 4);
		AdvanceLayerProcessor(*rig.industry, 74);
		suffix = "missing-input";
	}
	SECTION("Full actual output") {
		const auto capacity = GetLayerProcessorRecipe(*rig.industry)->output_capacity;
		const auto units = GetLayerProcessorRecipe(*rig.industry)->recipe.outputs.front().quantity;
		// Fill only by actual all-input commits; the save must preserve every historical debit and all retained product units.
		for (uint32_t batch = 0; batch < capacity / units; ++batch) ProduceRefineryBatch(rig);
		REQUIRE(rig.Output(CargoLabel{"RFMT"}).waiting == capacity);
		REQUIRE(GetLayerProcessorState(rig.industry->index)->batches.at(MultiLayerRecipeID{"CFMT"}) == capacity / units);
		suffix = "full-output";
	}
	SECTION("Actual output packet in a carrier") {
		ProduceRefineryBatch(rig);
		Station *station = rig.StationAt(TileXYL(29, 30, MapLayer::Surface));
		station->string_id = STR_SV_STNAME; // Full reload rebuilds station signs; the bare catchment fixture must carry a real default name.
		GoodsEntry &goods = station->goods[GetCargoTypeByLabel(CargoLabel{"RFMT"})];
		goods.rating = UINT8_MAX;
		goods.last_speed = 100;
		REQUIRE(DispatchLayerProcessorCargo(*rig.industry) == 2);
		REQUIRE(goods.TotalCount() == 2);
		REQUIRE(CargoPacket::GetNumItems() == 1);
		const CargoPacket *packet = *CargoPacket::Iterate().begin();
		CHECK(packet->GetSource() == Source{rig.industry->index, SourceType::Industry});
		CHECK(packet->GetFirstStation() == station->index);
		CHECK(rig.Output(CargoLabel{"RFMT"}).waiting == 0);
		transport.Load(*station, GetCargoTypeByLabel(CargoLabel{"RFMT"}));
		suffix = "in-transit";
	}
	const auto saved = CaptureLayerProcessors();
	const auto digest = ComputeLayerProcessorDigest();
	REQUIRE(AcceptLoadedLayerProcessors()); // Prove that the written stocks and counters satisfy the same final runtime activation boundary.
	// Since R2FC a test writes its checkpoint to its run-output directory; the committed input of the same family
	// in src/tests/fixtures stays immutable (r2mlvs3mp master finalization, 2026-10-16: the per-tag regression
	// otherwise overwrote it).
	const auto path = GetTestCaseArtifactPaths("processors", "processor-save-file").Output("saves", "processor-" + suffix + ".sav");
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup<GameMode> no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(std::filesystem::file_size(path) > 8);
	CHECK(CaptureLayerProcessors() == saved);
	CHECK(ComputeLayerProcessorDigest() == digest);
	std::ofstream checkpoint(path.string() + ".txt");
	checkpoint << "@see r2mlvs6-mines-and-resource-chains.md, Group 16B. Actual stock/choice/history checkpoint.\n"
		<< fmt::format("MLVS_PROCESSORS_LOADED count={} digest={:016X}\n", saved.size(), digest);
	checkpoint.close();
	REQUIRE(checkpoint.good());
}
