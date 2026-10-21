/**
 * @file ground_train_selection.cpp Real native rail identities, packets, source views and purchase guards.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FE.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 * Depots and freight are command-created. Packet loads and speed/hold boundary
 * states are explicitly staged; these cases claim no driven transport or viewport.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "../ground_train_sprite.h"
#include "../train_cmd.h"
#include "../timer/timer_game_tick.h"
#include "../cargopacket.h"
#include "underground_fleet_fixture.h"

#include "../safeguards.h"

/** Real command-built freight, with packet staging confined to explicit draw boundaries. */
class GroundTrainRig : public UndergroundFleetRig {
public:
	TileIndex depot; ///< Actual owned command-built Underground rail depot.
	Train *wagon; ///< Borrowed actual native wagon, destroyed by inherited vehicle teardown.

	/**
	 * Acquire a real selected wagon without inventing an engine or cargo identity.
	 * @param model Actual native mineral wagon5 or sealed wagon6.
	 */
	explicit GroundTrainRig(uint16_t model) : UndergroundFleetRig()
	{
		this->depot = this->BuildDepot(MapLayer::Underground, false);
		this->wagon = Train::From(this->Buy(this->depot, model));
	}

	/**
	 * Stage real packet custody for a single load boundary, without production claims.
	 * @param count Actual whole cargo units represented by a pool-owned native packet.
	 */
	void Load(uint16_t count)
	{
		this->wagon->cargo.Truncate();
		if (count == 0) return;
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(StationID::Invalid(), count, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(this->depot);
		this->wagon->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	}
};

/**
 * Exercise source registrations independently of runtime cargo slots and every drawn load/view phase.
 * @param model Native definition selected through actual EIDS.
 * @param family Expected accepted source body family.
 * @param labels Source-manifest registration order, resolved through native cargo labels.
 * @param separate_variants Whether each label has its own registered source cell.
 */
static void CheckRailSourceMatrix(uint16_t model, GroundSpriteFamily family, std::span<const CargoLabel> labels, bool separate_variants)
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundTrainRig rig(model);
	Train &wagon = *rig.wagon;
	AutoRestoreBackup<TimerGameTick::TickCounter> visual_clock(TimerGameTick::counter, 0);
	// Each permitted refit must resolve from its actual registry label, including
	// manufactured freight whose slots differ from the source's cell ordering.
	for (size_t index = 0; index < labels.size(); ++index) {
		wagon.cargo.Truncate();
		wagon.track = Track::Depot;
		wagon.vehstatus.Set(VehState::Stopped);
		const auto cargo = GetCargoTypeByLabel(labels[index]);
		REQUIRE(IsValidCargoType(cargo));
		REQUIRE(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, wagon.index, cargo, 0, false, true, 1)).Succeeded());
		REQUIRE(wagon.cargo_type == cargo);
		REQUIRE(wagon.cargo_cap > 1);
		wagon.track = Track::X; // Explicit moving draw boundary, not a driven journey.
		wagon.vehstatus.Reset(VehState::Stopped);
		wagon.cur_speed = 8;
		const uint16_t variant = separate_variants ? static_cast<uint16_t>(index) : 0;
		// Exhaust the empty/partial/full distinctions across all real views and
		// both authored wheel phases; drawing must preserve exact packet custody.
		for (uint8_t state = 0; state < 3; ++state) {
			rig.Load(state == 0 ? 0 : state == 1 ? 1 : wagon.cargo_cap);
			const auto packets = *wagon.cargo.Packets();
			const auto cash = Company::Get(_current_company)->money;
			const auto random = _random;
			const auto palette = GetVehiclePalette(&wagon);
			for (uint8_t frame = 0; frame < 2; ++frame) {
				TimerGameTick::counter = frame * 8;
				for (uint8_t view = 0; view < 8; ++view) {
					const auto direction = static_cast<Direction>(view);
					const auto expected = GetGroundSprite(family, {state, frame, view, variant, 0});
					CHECK(GetMultiLayerTrainSprite(wagon, direction, TimerGameTick::counter) == expected);
					VehicleSpriteSeq sequence;
					wagon.GetImage(direction, EngineImageType::OnMap, &sequence);
					REQUIRE(sequence.count == 1);
					CHECK(sequence.seq[0].sprite == *expected);
					CHECK(sequence.seq[0].pal == 0); // Existing company palette owner still applies its remap.
				}
			}
			CHECK(*wagon.cargo.Packets() == packets);
			CHECK(GetVehiclePalette(&wagon) == palette);
			CHECK(Company::Get(_current_company)->money == cash);
			CHECK(_random.state[0] == random.state[0]);
			CHECK(_random.state[1] == random.state[1]);
			CHECK(wagon.cur_speed == 8);
			CHECK(wagon.cargo_type == cargo);
		}
	}
}

/** All seventeen published open-wagon cargo registrations follow actual refits and packet-backed load stages. */
TEST_CASE("Selected open rail wagon resolves all native cargo views and rolling phases", "[ground-train-selection]")
{
	const std::array labels{CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"},
		CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"},
		CargoLabel{"RFMT"}, CargoLabel{"STEL"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"},
		CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"GOOD"}};
	CheckRailSourceMatrix(5, GroundSpriteFamily::VehicleModel05, labels, true);
}

/** Sealed Uranium and Rare Earth share one closed hardware variant while keeping real cargo and capacity owners. */
TEST_CASE("Selected sealed rail wagon resolves both protected native refits", "[ground-train-selection]")
{
	const std::array labels{CargoLabel{"URAN"}, CargoLabel{"RARE"}};
	CheckRailSourceMatrix(6, GroundSpriteFamily::VehicleModel06, labels, false);
}

/** Native service/hold cues never erase loaded custody, and wheels depend on the real coupled head. */
TEST_CASE("Selected rail wagon keeps load priority and actual train head motion", "[ground-train-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	// Both physical wagons obey the same source-declared priority, without
	// granting the sealed model any open-wagon cargo registration or permission.
	for (uint16_t model : {uint16_t{5}, uint16_t{6}}) {
		GroundTrainRig rig(model);
		Train &wagon = *rig.wagon;
		Train *head = Train::From(rig.Buy(rig.depot, 4));
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon.index, head->index, false).Succeeded());
		REQUIRE(wagon.First() == head);
		const auto family = model == 5 ? GroundSpriteFamily::VehicleModel05 : GroundSpriteFamily::VehicleModel06;
		head->flags.Set(VehicleRailFlag::Stuck);
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {3, 1, 6, 0, 0}));
		wagon.track = Track::X;
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {4, 1, 6, 0, 0}));
		rig.Load(1);
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {1, 0, 6, 0, 0}));
		rig.Load(wagon.cargo_cap);
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
		wagon.track = Track::Depot;
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
		head->flags.Reset(VehicleRailFlag::Stuck);
		head->vehstatus.Reset(VehState::Stopped);
		head->cur_speed = 8;
		wagon.cur_speed = 0;
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 8) == GetGroundSprite(family, {2, 1, 6, 0, 0}));
		head->vehstatus.Set(VehState::Stopped);
		CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 1000) == GetGroundSprite(family, {2, 0, 6, 0, 0}));
	}
}

/** Existing flip and static list contexts preserve body facing and leave palette selection to normal drawing. */
TEST_CASE("Selected rail art follows actual train image contexts and flipping", "[ground-train-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundTrainRig rig(5);
	Train &wagon = *rig.wagon;
	wagon.track = Track::X;
	wagon.vehstatus.Reset(VehState::Stopped);
	wagon.cur_speed = 8;
	wagon.flags.Set(VehicleRailFlag::Flipped);
	AutoRestoreBackup<TimerGameTick::TickCounter> clock(TimerGameTick::counter, 8);
	VehicleSpriteSeq sequence;
	wagon.GetImage(Direction::N, EngineImageType::OnMap, &sequence);
	CHECK(sequence.seq[0].sprite == *GetGroundSprite(GroundSpriteFamily::VehicleModel05, {0, 1, 4, 0, 0}));
	wagon.GetImage(Direction::N, EngineImageType::InList, &sequence);
	CHECK(sequence.seq[0].sprite == *GetGroundSprite(GroundSpriteFamily::VehicleModel05, {0, 0, 4, 0, 0}));
	CHECK(wagon.flags.Test(VehicleRailFlag::Flipped));
}

/** Actual definition/date/cargo/layer/profile boundaries refuse unsupported overrides without allocating or changing refits. */
TEST_CASE("Selected rail source binding refuses foreign and unavailable contexts", "[ground-train-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundTrainRig rig(6);
	Train &wagon = *rig.wagon;
	const auto original_engine = wagon.engine_type;
	const auto original_cargo = wagon.cargo_type;
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::Invalid, 0));
	wagon.cargo_type = INVALID_CARGO;
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.cargo_type = GetCargoTypeByLabel(CargoLabel{"COAL"});
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.cargo_type = original_cargo;
	wagon.tile = INVALID_TILE;
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.tile = TileXYL(40, 40, MapLayer::Space);
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.tile = rig.depot;
	wagon.engine_type = GetUndergroundEngine(7);
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.engine_type = EngineID::Invalid();
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	wagon.engine_type = original_engine;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(GetMultiLayerTrainSprite(wagon, Direction::W, 0).has_value());
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetMultiLayerTrainSprite(wagon, Direction::W, 0));
	CHECK_FALSE(GetMultiLayerEnginePurchaseSprite(original_engine, true));
	CHECK(wagon.cargo_type == original_cargo);
	CHECK(wagon.cargo.StoredCount() == 0);
}

/** Actual company/date admission supplies available/locked previews; art changes no buildability or original M1 family. */
TEST_CASE("Selected rail purchase previews preserve actual engine and company guards", "[ground-train-selection]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	GroundTrainRig rig(5);
	// Only these two actual EIDS identities acquire newly selected purchase art.
	// Existing cutters/hoppers/hauler retain their exact already-published slice.
	for (uint16_t model : {uint16_t{5}, uint16_t{6}}) {
		const auto id = GetUndergroundEngine(model);
		Engine *engine = Engine::Get(id);
		const auto family = model == 5 ? GroundSpriteFamily::PurchaseModel05 : GroundSpriteFamily::PurchaseModel06;
		REQUIRE(IsEngineBuildable(id, VehicleType::Train, _current_company));
		CHECK(GetMultiLayerEnginePurchaseSprite(id, true) == GetGroundSprite(family, {0}));
		engine->company_avail.Reset(_current_company);
		const auto available = IsEngineBuildable(id, VehicleType::Train, _current_company);
		CHECK_FALSE(available);
		CHECK(GetMultiLayerEnginePurchaseSprite(id, available) == GetGroundSprite(family, {1}));
		CHECK_FALSE(engine->company_avail.Test(_current_company));
		engine->company_avail.Set(_current_company);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto sealed = GetUndergroundEngine(6);
	CHECK_FALSE(IsEngineBuildable(sealed, VehicleType::Train, _current_company));
	CHECK(GetMultiLayerEnginePurchaseSprite(sealed, false) == GetGroundSprite(GroundSpriteFamily::PurchaseModel06, {1}));
	CHECK_FALSE(GetMultiLayerEnginePurchaseSprite(EngineID::Invalid(), true));
	CHECK_FALSE(GetMultiLayerPurchaseSprite(7, true));
	CHECK(GetMultiLayerEnginePurchaseSprite(GetUndergroundEngine(4), true) == GetMultiLayerSprite(MultiLayerSpriteAsset::PurchaseModel04, {0}));
}
