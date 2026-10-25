/**
 * @file ground_road_selection.cpp Command-created carriers select exact source views without changing transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FF.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 * Loads, speed and traffic counters are explicit native boundary fixtures;
 * these tests do not claim a driven haulage or actual viewport result.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "../ground_road_sprite.h"
#include "../timer/timer_game_tick.h"
#include "../cargopacket.h"
#include "../network/network.h"
#include "underground_fleet_fixture.h"

#include "ground_render_camera_fixture.h"
#include "../safeguards.h"

/** Real native road carrier and owned depot; inherited teardown preserves ordinary vehicle/cargo custody. */
class GroundRoadRig : public UndergroundFleetRig {
public:
	TileIndex depot; ///< Command-built actual Underground road depot.
	RoadVehicle *carrier; ///< Borrowed actual engine-defined physical carrier.

	/**
	 * Buy a real road carrier through ordinary native admission.
	 * @param model Canonical open truck7 or sealed carrier8.
	 */
	explicit GroundRoadRig(uint16_t model) : UndergroundFleetRig()
	{
		this->depot = this->BuildDepot(MapLayer::Underground, true);
		this->carrier = RoadVehicle::From(this->Buy(this->depot, model));
	}

	/**
	 * Stage an explicitly disclosed native packet load for one drawing boundary.
	 * @param count Whole units owned by an actual CargoPacket, rather than a presentation inventory.
	 */
	void Load(uint16_t count)
	{
		this->carrier->cargo.Truncate();
		if (count == 0) return;
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(StationID::Invalid(), count, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(this->depot);
		this->carrier->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	}
};

/**
 * Exercise native refits, independent source cargo registration, loads and body views.
 * @param model Actual EIDS road design.
 * @param family Expected accepted body source family.
 * @param labels Accepted source-manifest cargo registration order.
 * @param separate_variants Whether labels have separate source cells or share sealed hardware.
 */
static void CheckRoadSourceMatrix(uint16_t model, GroundSpriteFamily family, std::span<const CargoLabel> labels, bool separate_variants)
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundRoadRig rig(model);
	RoadVehicle &carrier = *rig.carrier;
	AutoRestoreBackup<TimerGameTick::TickCounter> clock(TimerGameTick::counter, 0);
	// Refit through real cargo labels and command admission. Source cell order
	// is independent of runtime slots, including native Earth/Rock spoil.
	for (size_t index = 0; index < labels.size(); ++index) {
		carrier.cargo.Truncate();
		carrier.state = RVSB_IN_DEPOT;
		carrier.vehstatus.Set(VehState::Stopped);
		carrier.cur_speed = 0; // Ordinary depot refit requires actual stationary custody, not only its stopped flag.
		const auto cargo = GetCargoTypeByLabel(labels[index]);
		REQUIRE(IsValidCargoType(cargo));
		REQUIRE(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, carrier.index, cargo, 0, false, true, 1)).Succeeded());
		REQUIRE(carrier.cargo_type == cargo);
		REQUIRE(carrier.cargo_cap > 1);
		carrier.state = to_underlying(Trackdir::X_NE); // Explicit rolling draw boundary, without a journey claim.
		carrier.vehstatus.Reset(VehState::Stopped);
		carrier.cur_speed = 8;
		const uint16_t variant = separate_variants ? static_cast<uint16_t>(index) : 0;
		// Exhaust actual empty/partial/full packet stages across the eight native
		// views and two authored tyre phases, retaining packet and palette owners.
		for (uint8_t state = 0; state < 3; ++state) {
			const uint16_t load = state == 0 ? 0 : state == 1 ? 1 : carrier.cargo_cap;
			rig.Load(load);
			const auto packets = *carrier.cargo.Packets();
			const auto cash = Company::Get(_current_company)->money;
			const auto random = _random;
			const auto palette = GetVehiclePalette(&carrier);
			for (uint8_t frame = 0; frame < 2; ++frame) {
				TimerGameTick::counter = frame * 8;
				for (uint8_t view = 0; view < 8; ++view) {
					const auto direction = static_cast<Direction>(view);
					const auto expected = GetGroundSprite(family, {state, frame, view, variant, 0});
					CHECK(GetGroundRoadSprite(carrier, direction, TimerGameTick::counter) == expected);
					VehicleSpriteSeq sequence;
					carrier.GetImage(direction, EngineImageType::OnMap, &sequence);
					REQUIRE(sequence.count == 1);
					CHECK(sequence.seq[0].sprite == *expected);
					CHECK(sequence.seq[0].pal == 0);
				}
			}
			CHECK(*carrier.cargo.Packets() == packets);
			CHECK(carrier.cargo.StoredCount() == load);
			CHECK(GetVehiclePalette(&carrier) == palette);
			CHECK(Company::Get(_current_company)->money == cash);
			CHECK(_random.state[0] == random.state[0]);
			CHECK(_random.state[1] == random.state[1]);
			CHECK(carrier.cur_speed == 8);
			CHECK(carrier.cargo_type == cargo);
		}
	}
}

/** Every accepted bulk/spoil cargo follows actual native refit ownership and its independent source cell. */
TEST_CASE("Selected open mine truck resolves all ten cargo views and rolling phases", "[ground-road-selection]")
{
	const std::array labels{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"},
		CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"ERTH"}, CargoLabel{"ROCK"}};
	CheckRoadSourceMatrix(7, GroundSpriteFamily::VehicleModel07, labels, true);
}

/** Sealed freight keeps its actual Uranium/Rare Earth labels and packet-backed fill meter inside one shared shell. */
TEST_CASE("Selected sealed road carrier resolves both protected cargo refits", "[ground-road-selection]")
{
	const std::array labels{CargoLabel{"URAN"}, CargoLabel{"RARE"}};
	CheckRoadSourceMatrix(8, GroundSpriteFamily::VehicleModel08, labels, false);
}

/** Traffic holds differ from deliberate stopping/crashes, and never erase a real load or animate stopped tyres. */
TEST_CASE("Selected road carrier keeps traffic service and packet priority truthful", "[ground-road-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	// Apply the accepted priority to both hardware families; sealed stock
	// never becomes an open cargo variant when traffic or servicing changes.
	for (uint16_t model : {uint16_t{7}, uint16_t{8}}) {
		GroundRoadRig rig(model);
		RoadVehicle &carrier = *rig.carrier;
		const auto family = model == 7 ? GroundSpriteFamily::VehicleModel07 : GroundSpriteFamily::VehicleModel08;
		carrier.blocked_ctr = 5;
		carrier.vehstatus.Reset(VehState::Stopped);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {3, 1, 6, 0, 0}));
		carrier.state = to_underlying(Trackdir::X_NE);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {4, 1, 6, 0, 0}));
		carrier.vehstatus.Set(VehState::Stopped);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {0, 0, 6, 0, 0}));
		carrier.vehstatus.Reset(VehState::Stopped);
		carrier.vehstatus.Set(VehState::Crashed);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {0, 0, 6, 0, 0}));
		carrier.vehstatus.Reset(VehState::Crashed);
		rig.Load(1);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {1, 0, 6, 0, 0}));
		rig.Load(carrier.cargo_cap);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
		carrier.state = RVSB_IN_DEPOT;
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
		carrier.state = to_underlying(Trackdir::X_NE);
		carrier.cur_speed = 8;
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {2, 1, 6, 0, 0}));
		carrier.vehstatus.Set(VehState::Crashed);
		CHECK(GetGroundRoadSprite(carrier, Direction::W, 8) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
		carrier.vehstatus.Reset(VehState::Crashed); // Ordinary fixture destructor expects an intact parked vehicle.
	}
}

/** Static list images and native phase refresh leave route, speed, stock, money and RNG with their original owners. */
TEST_CASE("Selected road image and derived cache refresh cannot advance transport", "[ground-road-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundRoadRig rig(7);
	RoadVehicle &carrier = *rig.carrier;
	carrier.state = to_underlying(Trackdir::X_NE);
	carrier.vehstatus.Reset(VehState::Stopped);
	carrier.cur_speed = 8;
	rig.Load(1);
	AutoRestoreBackup<TimerGameTick::TickCounter> clock(TimerGameTick::counter, 8);
	VehicleSpriteSeq sequence;
	carrier.GetImage(Direction::N, EngineImageType::OnMap, &sequence);
	CHECK(sequence.seq[0].sprite == *GetGroundSprite(GroundSpriteFamily::VehicleModel07, {1, 1, 0, 0, 0}));
	carrier.GetImage(Direction::N, EngineImageType::InList, &sequence);
	CHECK(sequence.seq[0].sprite == *GetGroundSprite(GroundSpriteFamily::VehicleModel07, {1, 0, 0, 0, 0}));
	const auto physics = std::tuple(carrier.cur_speed, carrier.state, carrier.blocked_ctr, carrier.x_pos, carrier.y_pos, carrier.dest_tile);
	const auto cash = Company::Get(_current_company)->money;
	const auto random = _random;
	const auto packets = *carrier.cargo.Packets();
	carrier.sprite_cache.revalidate_before_draw = false;
	RefreshGroundRoadArt(carrier, 7);
	CHECK_FALSE(carrier.sprite_cache.revalidate_before_draw);
	{
		AutoRestoreBackup dedicated(_network_dedicated, true);
		RefreshGroundRoadArt(carrier, 8);
		CHECK_FALSE(carrier.sprite_cache.revalidate_before_draw);
	}
	RefreshGroundRoadArt(carrier, 8);
	CHECK(std::tuple(carrier.cur_speed, carrier.state, carrier.blocked_ctr, carrier.x_pos, carrier.y_pos, carrier.dest_tile) == physics);
	CHECK(*carrier.cargo.Packets() == packets);
	CHECK(carrier.cargo.StoredCount() == 1);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
}

/** Native capability, cargo, date, layer and profile refusals preserve actual physical vehicle identity. */
TEST_CASE("Selected road source binding refuses unsupported native contexts", "[ground-road-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundRoadRig rig(8);
	RoadVehicle &carrier = *rig.carrier;
	const auto original_engine = carrier.engine_type;
	const auto original_cargo = carrier.cargo_type;
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::Invalid, 0));
	carrier.cargo_type = INVALID_CARGO;
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	carrier.cargo_type = GetCargoTypeByLabel(CargoLabel{"COAL"});
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	carrier.cargo_type = original_cargo;
	carrier.tile = INVALID_TILE;
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	carrier.tile = TileXYL(45, 40, MapLayer::Space);
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	carrier.tile = rig.depot;
	carrier.engine_type = GetUndergroundEngine(5);
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	carrier.engine_type = EngineID::Invalid();
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	CHECK_FALSE(GetMultiLayerEnginePurchaseSprite(carrier.engine_type, true));
	carrier.engine_type = original_engine;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(GetGroundRoadSprite(carrier, Direction::W, 0).has_value());
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetGroundRoadSprite(carrier, Direction::W, 0));
	CHECK_FALSE(GetMultiLayerEnginePurchaseSprite(original_engine, true));
	CHECK(carrier.cargo_type == original_cargo);
	CHECK(carrier.cargo.StoredCount() == 0);
}

/** Actual company and date guards supply purchase locks while completed rail and M1 previews keep exact identities. */
TEST_CASE("Selected road purchases retain native engine company and calendar guards", "[ground-road-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundRoadRig rig(7);
	// Separate actual road EIDS identities select only their authored available/
	// locked cues, leaving engine dates and ordinary company flags unchanged.
	for (uint16_t model : {uint16_t{7}, uint16_t{8}}) {
		const auto id = GetUndergroundEngine(model);
		Engine *engine = Engine::Get(id);
		const auto family = model == 7 ? GroundSpriteFamily::PurchaseModel07 : GroundSpriteFamily::PurchaseModel08;
		REQUIRE(IsEngineBuildable(id, VehicleType::Road, _current_company));
		CHECK(GetMultiLayerEnginePurchaseSprite(id, true) == GetGroundSprite(family, {0}));
		engine->company_avail.Reset(_current_company);
		const auto available = IsEngineBuildable(id, VehicleType::Road, _current_company);
		CHECK_FALSE(available);
		CHECK(GetMultiLayerEnginePurchaseSprite(id, available) == GetGroundSprite(family, {1}));
		CHECK_FALSE(engine->company_avail.Test(_current_company));
		engine->company_avail.Set(_current_company);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK_FALSE(IsEngineBuildable(GetUndergroundEngine(8), VehicleType::Road, _current_company));
	CHECK(GetMultiLayerEnginePurchaseSprite(GetUndergroundEngine(8), false) == GetGroundSprite(GroundSpriteFamily::PurchaseModel08, {1}));
	CHECK_FALSE(GetMultiLayerPurchaseSprite(9, true));
	CHECK(GetMultiLayerEnginePurchaseSprite(GetUndergroundEngine(5), true) == GetGroundSprite(GroundSpriteFamily::PurchaseModel05, {0}));
	CHECK(GetMultiLayerEnginePurchaseSprite(GetUndergroundEngine(4), true) == GetMultiLayerSprite(MultiLayerSpriteAsset::PurchaseModel04, {0}));
}

/** Actual main/extra cameras and hidden-building presentation preserve the physical source owner and its live state. */
TEST_CASE("Merged road freight selection retains physical cargo across cameras", "[merged-ground-camera]")
{
	PrepareGroundRenderCameraLanguage();
	GroundRoadRig rig(7); rig.Load(10);
	/** @return Actual public consumer identities at the unchanged visual phase; no raster or earned work is claimed. */
	const auto query = [&] { return std::vector<SpriteID>{GetGroundRoadSprite(*rig.carrier, Direction::NE, 24).value()}; };
	CheckGroundRenderCameraPurity(query);
}
