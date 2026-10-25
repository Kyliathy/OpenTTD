/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_vehicle_sprite.cpp Actual purchased capsules retain packet custody and native service/source ownership through every packed body pose.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_meter_fixture.h"
#include "../transport_tube_sprite.h"
#include "../ground_sprite.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/**
 * Snapshot original packets, work, money, native physics and both RNG streams independently of display caches.
 * @param train Actual command-purchased capsule; no copied inventory becomes a simulation owner.
 * @return Exact comparable native owner observations before or after read-only pose/cache queries.
 */
static auto CaptureTubeSpriteOwners(const Train &train)
{
	std::vector<std::tuple<CargoPacketID, uint16_t, uint16_t, Money, TileIndex, uint64_t, bool>> packets;
	// Observe each actual packet's identity, age, payment origin and paid purpose;
	// equal totals alone would not expose a graphics-induced custody replacement.
	for (const CargoPacket *packet : *train.cargo.Packets()) {
		packets.emplace_back(packet->index, packet->Count(), packet->GetPeriodsInTransit(), packet->GetFeederShare(),
			packet->GetSourceTile(), packet->GetCompanySupplyOrder(), packet->HasCompanySupplyShipped());
	}
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		Company::Get(train.owner)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]},
		train.tile, train.x_pos, train.y_pos, train.z_pos, train.direction, train.cur_speed, train.acceleration,
		train.gcache.cached_total_length, train.gcache.cached_veh_length, train.reliability, train.vehstatus,
		train.flags, train.cargo_cap, train.cargo.StoredCount(), std::move(packets)};
}

/**
 * Exercise every authored facing and lamp phase through both the pure and actual native image selectors.
 * @param train Actual purchased native physical part, retaining its original packets and binding.
 * @param asset Independently declared original model family.
 * @param state Independently expected load/status row from explicit native fixture actions.
 */
static void CheckTubeBodyViews(Train &train, GroundSpriteFamily asset, uint8_t state)
{
	const auto before = CaptureTubeSpriteOwners(train);
	// Every direction/phase must retain the same actual packet/service owners.
	// The source's two phases pulse a lamp even at rest, never a wheel or hull.
	for (uint8_t frame = 0; frame < 2; ++frame) {
		for (uint8_t view = 0; view < 8; ++view) {
			CAPTURE(state, frame, view);
			const auto direction = static_cast<Direction>(view);
			const auto expected = GetGroundSprite(asset, {state, frame, view}); REQUIRE(expected.has_value());
			CHECK(GetTubeTrainSprite(train, direction, frame * 8) == expected);
			CHECK(GetMultiLayerTrainSprite(train, direction, frame * 8) == expected);
			// Changing only the renderer's clock argument cannot prepare or spend
			// energy. Native GetImage itself reads the unchanged simulation clock.
			VehicleSpriteSeq image; train.GetImage(direction, EngineImageType::OnMap, &image);
			REQUIRE(image.count == 1);
			CHECK(image.seq[0].sprite == *GetGroundSprite(asset, {state, static_cast<uint8_t>((TimerGameTick::counter / 8) % 2), view}));
		}
	}
	CHECK_FALSE(GetTubeTrainSprite(train, Direction::Invalid, 0).has_value());
	CHECK(CaptureTubeSpriteOwners(train) == before);
	// This is an explicit native body-flip boundary input, not a reversal or
	// teleport. GetImage must apply the existing native policy exactly once.
	{
		AutoRestoreBackup flags(train.flags); train.flags.Set(VehicleRailFlag::Flipped);
		VehicleSpriteSeq image; train.GetImage(Direction::NE, EngineImageType::OnMap, &image);
		REQUIRE(image.count == 1);
		CHECK(image.seq[0].sprite == *GetTubeTrainSprite(train, ReverseDir(Direction::NE), TimerGameTick::counter));
	}
	// A nonvisible vehicle needs a deferred native cache refresh, without a
	// front-only MarkDirty physics rebuild, packet change or work opportunity.
	train.sprite_cache.is_viewport_candidate = false; train.sprite_cache.last_direction = train.direction;
	RefreshMultiLayerTrainArt(train, 16);
	CHECK(train.sprite_cache.revalidate_before_draw);
	CHECK(CaptureTubeSpriteOwners(train) == before);
}

/** All720 original body poses consume actual five-model packets, paid source loss/recovery and native once-tick service. */
TEST_CASE("Five actual capsule bodies preserve load across native service and source waits", "[tube-capsule-art-body]")
{
	const uint16_t model = GENERATE(9, 10, 11, 12, 13);
	TubeMeterRig world;
	AutoRestoreBackup date(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2050}, 0, 1));
	AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{2050});
	world.Bind();
	Train *train = world.capsule;
	// Every later model is a real command purchase of its own frozen native
	// engine. The declared2050 fixture availability does not change production
	// purchase dates or turn one refitted vehicle into five model identities.
	if (model != 9) {
		Engine::Get(GetTubeEngine(model))->company_avail.Set(_current_company);
		const auto [result, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute,
			world.depot, GetTubeEngine(model), false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(result.Succeeded()); train = Train::Get(id);
		REQUIRE(BindTubePowerVehicle(train->index, world.source).has_value());
	}
	const std::array assets{GroundSpriteFamily::VehicleModel09, GroundSpriteFamily::VehicleModel10,
		GroundSpriteFamily::VehicleModel11, GroundSpriteFamily::VehicleModel12, GroundSpriteFamily::VehicleModel13};
	REQUIRE(train->Next() == nullptr); CHECK(train->gcache.cached_veh_length == VEHICLE_LENGTH);
	// Each actual load level keeps its original packets across a real repair,
	// source closure and explicit paid reconnection. These are boundary-loaded
	// cargo fixtures, not claimed station loading or useful-route acceptance.
	for (uint8_t load = 0; load < 3; ++load) {
		CAPTURE(model, load);
		const uint target = load == 0 ? 0 : load == 1 ? 1 : train->cargo_cap;
		if (target > train->cargo.StoredCount()) {
			REQUIRE(CargoPacket::CanAllocateItem());
			train->cargo.Append(CargoPacket::Create(StationID::Invalid(), target - train->cargo.StoredCount(), Source{Source::Invalid, SourceType::Industry}));
			train->MarkDirty(); // Actual fixture loading refreshes native weight/capacity caches before the read-only boundary.
		}
		train->reliability = train->GetEngine()->reliability;
		++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(GetTubePowerVehicle(train->index)->wait == TubePowerWait::None);
		CheckTubeBodyViews(*train, assets[model - 9], load);
		// Declared wear requests genuine native depot service. Artwork observes
		// that demand before the actual train tick repairs and consumes540J once.
		train->reliability = 100;
		++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(GetTubePowerPhase(*train) == TubePowerPhase::DepotService);
		REQUIRE(GetTubePowerVehicle(train->index)->wait == TubePowerWait::None);
		const auto used = GetUtilityPowerProducer(world.source)->consumed_lifetime_j;
		CheckTubeBodyViews(*train, assets[model - 9], 3 + load);
		REQUIRE(train->Tick());
		CHECK(train->reliability == train->GetEngine()->reliability); CHECK(train->IsStoppedInDepot());
		CHECK(GetUtilityPowerProducer(world.source)->consumed_lifetime_j == used + 540);
		REQUIRE(train->Tick()); CHECK(GetUtilityPowerProducer(world.source)->consumed_lifetime_j == used + 540);
		REQUIRE(CloseTubeExternalPower(world.source) == TubePowerError::None);
		++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(GetTubePowerVehicle(train->index)->wait == TubePowerWait::Topology);
		CheckTubeBodyViews(*train, assets[model - 9], 6 + load);
		const auto restored = ConnectTubeExternalPower(world.portal, world.depot, world.path);
		REQUIRE(restored.error == TubePowerError::None);
		REQUIRE(ReconnectTubePowerVehicle(train->index, restored.producer).has_value()); world.source = restored.producer;
		CHECK(train->cargo.StoredCount() == target); CHECK(train->IsStoppedInDepot());
	}
}

/** Both preview states consume actual canonical engine identity without changing company/date purchase authority. */
TEST_CASE("Five actual capsule purchase families retain locked identity and foreign fallback", "[tube-capsule-art-purchase]")
{
	TubePrototypeRig world;
	const auto cash = Company::Get(_current_company)->money;
	const auto random = std::array{_random.state[0], _random.state[1]};
	const std::array assets{GroundSpriteFamily::PurchaseModel09, GroundSpriteFamily::PurchaseModel10,
		GroundSpriteFamily::PurchaseModel11, GroundSpriteFamily::PurchaseModel12, GroundSpriteFamily::PurchaseModel13};
	// Both available and locked art must preserve each original model identity,
	// irrespective of whether this early fixture can actually buy the engine.
	for (uint16_t model = 9; model <= 13; ++model) {
		const EngineID engine = GetTubeEngine(model); REQUIRE(Engine::IsValidID(engine));
		const auto availability = Engine::Get(engine)->company_avail;
		for (bool available : {false, true}) {
			const auto expected = GetGroundSprite(assets[model - 9], {static_cast<uint8_t>(available ? 0 : 1)});
			REQUIRE(expected.has_value());
			CHECK(GetTubeEnginePurchaseSprite(engine, available) == expected);
			CHECK(GetMultiLayerEnginePurchaseSprite(engine, available) == expected);
		}
		CHECK(Engine::Get(engine)->company_avail == availability);
		// Retained engines keep their hull identity at dates that refuse a new
		// purchase; presentation cannot unlock a future model through its image.
		CHECK(GetTubeEnginePurchaseSprite(engine, false).has_value());
	}
	CHECK_FALSE(GetTubeEnginePurchaseSprite(EngineID::Invalid(), true).has_value());
	CHECK_FALSE(GetTubeEnginePurchaseSprite(GetUndergroundEngine(4), true).has_value());
	CHECK(GetMultiLayerPurchaseSprite(5, true).has_value()); // A conventional Ground purchase family remains bound beside the imported capsules.
	CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
}
