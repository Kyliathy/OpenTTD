/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_mining_fixture.h Shared isolated native mining participants; this unit adapter claims no paid procurement or whole-world save acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups10/21
 */
#ifndef TESTS_PROBE_MINING_FIXTURE_H
#define TESTS_PROBE_MINING_FIXTURE_H
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../asteroid_generation.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_approach.h"
#include "../multilayer_space_content.h"
#include "../core/backup_type.hpp"
#include "../vehicle_func.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "excavation_fixture.h"
#include "mock_environment.h"

/** Isolated actual generated field, legacy native hulls and original packet/approach owners; never saved as a paid world. */
class ProbeExtractionRig {
	ExcavationFixture world;
	AsteroidField previous = GetAsteroidField(); ///< Caller field restored after unit hulls retire.
	GRFConfigList previous_content; ///< Caller descriptors, never an alternative content registry.
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}}; ///< Explicit one-tonne unit fixture dimension.
	AutoRestoreBackup<GameMode> mode{_game_mode, GameMode::Normal};
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}};
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter};
public:
	/** Activate exact declared unit descriptors before native Engine admission; allocate no cargo or work here. */
	ProbeExtractionRig()
	{
		MockEnvironment::Instance(); previous_content.swap(_grfconfig);
		// Native Engine identity must match this isolated world's selected package, without reading user-wide content or borrowing another fixture's descriptors.
		for (const auto &descriptor : world.ActivatedContent()) {
			auto copy = std::make_unique<GRFConfig>(); copy->ident = descriptor->ident; copy->version = descriptor->version; copy->status = descriptor->status; _grfconfig.push_back(std::move(copy));
		}
		REQUIRE(GenerateWorldAsteroids(12345)); REQUIRE(InitialiseSpaceEngineDefinition());
	}
	/** Dispose each native hull's sole packet holding before restoring caller field, descriptors and clock. */
	~ProbeExtractionRig() { _vehicle_pool.CleanPool(); ResetVehicleHash(); CHECK(InstallAsteroidField(std::move(previous))); _grfconfig.clear(); previous_content.swap(_grfconfig); }
	/** Install an explicitly generated isolated Space performance world before creating any hull.
	 * @param field Complete native generated field, including any declared in-band cluster geometry.
	 * Only Space's tile geometry is initialized on larger maps; unused terrestrial fixture owners supply no performance or whole-save claims.
	 */
	void ResetField(AsteroidField field)
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		// Real native hull coordinates must use the matching Map geometry; a small map with a large proxy field would measure an unsupported world.
		if (Map::SizeX() != field.width || Map::SizeY() != field.height) {
			Map::Allocate(field.width, field.height, MAX_MAP_LAYERS); MultiLayerMap::InitialiseLayer(MapLayer::Space); ResetVehicleHash();
		}
		REQUIRE(InstallAsteroidField(std::move(field)));
	}
	/** @param position Declared bounded initial plane point. @param assign True issues the actual body0 mission now; false permits explicit same-tick command ordering.
	 * @return One actual isolated legacy15, without a paid commissioning claim.
	 */
	SpaceVehicle &ProbeAt(SpacePoint position, bool assign = true)
	{
		REQUIRE(Vehicle::CanAllocateItem()); auto *craft = SpaceVehicle::Create(); REQUIRE(RegisterSpaceVehicleIdentity(*craft));
		craft->owner = _current_company; craft->engine_type = GetSpaceProbeEngine(); craft->cargo_type = GetCargoTypeByLabel(CargoLabel{"NORE"}); craft->cargo_cap = 20;
		craft->position = position; craft->home = position; craft->UpdateSpacePosition();
		if (assign) REQUIRE(CmdSetSpaceMission(DoCommandFlag::Execute, craft->index, uint32_t{0}).Succeeded());
		return *craft;
	}
	/** @param separation Declared initial tile distance left of body0. @param assign True starts body0 pursuit now; false retains explicit command ordering.
	 * @return Actual isolated native legacy15 with its original empty hold.
	 */
	SpaceVehicle &Probe(uint16_t separation = 4, bool assign = true)
	{
		const auto &body = GetAsteroidField().bodies.front();
		return ProbeAt({body.motion.position.x - separation * SPACE_Q16, body.motion.position.y - 3 * SPACE_Q16}, assign);
	}
	/** Offer one actual field, shared arbitration and native vehicle opportunity in stable native pool order. */
	void Step()
	{
		++TimerGameTick::counter; TickAsteroidField(); PrepareSpaceApproachTick();
		// Every competing original hull sees the same freshly moved body and admitted shared slots; later pool order cannot duplicate an occupied attachment.
		for (auto *craft : SpaceVehicle::Iterate()) REQUIRE(craft->Tick());
	}
	/** @param craft Actual unit hull. @return Its actual body after ordinary bounded pursuit acquires the exclusive lease. */
	AsteroidBody &Attach(SpaceVehicle &craft)
	{
		// Native pursuit and arbitration earn attachment; declaring this unit world does not place either hull directly on a mining slot.
		for (uint elapsed = 0; craft.mission.phase != SpaceMissionPhase::Mining; ++elapsed) { REQUIRE(elapsed < 3000); Step(); }
		auto *body = GetMutableAsteroidBody(craft.mission.target); REQUIRE(body); REQUIRE(body->mining_slot == craft.index); return *body;
	}
};


#endif /* TESTS_PROBE_MINING_FIXTURE_H */
