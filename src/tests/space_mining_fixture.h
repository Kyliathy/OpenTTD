/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_fixture.h Shared actual mining/factory participants for target and return admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups18-21.
 * Parent frames/parts/content and the optional legacy15 are declared unit inputs; no gameplay procurement or whole-save claim is made.
 */
#ifndef TESTS_SPACE_MINING_FIXTURE_H
#define TESTS_SPACE_MINING_FIXTURE_H
#include "manufacturing_fixture.h"
#include "../asteroid.h"
#include "../asteroid_generation.h"
#include "../openttd.h"
#include "../space_vehicle_cmd.h"
#include "../space_mining_tuning.h"

/** Original factory products, optional legacy miner and common field/approach/native vehicle driver. */
class FactoryMiningRig : public ManufacturingMaterializationRig {
	AsteroidField previous = GetAsteroidField(); ///< Restore resources only after actual unit hulls are gone.
	AutoRestoreBackup<GameMode> mode{_game_mode, GameMode::Normal};
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}};
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}};
public:
	/** Generate only the isolated finite field; parent factory inputs retain their explicitly declared unit provenance. */
	FactoryMiningRig() { REQUIRE(GenerateWorldAsteroids(12345)); }
	/** Retire native hulls before restoring the caller's resources; parent teardown still owns factory stock and registration cleanup. */
	~FactoryMiningRig()
	{
		// Bulk hull cleanup deliberately skips packet disposal. The fixture's
		// actual cargo owners must release their fragments before its field is
		// restored, preserving isolation for subsequent full custody checks.
		for (auto *vehicle : Vehicle::Iterate()) vehicle->cargo.Truncate();
		_vehicle_pool.CleanPool(); ResetVehicleHash(); CHECK(InstallAsteroidField(std::move(this->previous)));
	}
	/** @param craft Actual original miner. @param body Existing natural source. @param receiver Real owned module. Install an original Mine/module-Unload route through the checked command owner. */
	void Route(SpaceVehicle &craft, uint32_t body, const SpaceStructure &receiver)
	{
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Clear, 0,
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, body, 0).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
			SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, body, 0).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 1,
			SpaceOrderAction::Unload, SpaceOrderTargetKind::Structure, receiver.index.base(), receiver.generation).Succeeded());
	}
	/** @param model Actual role15 or21. @return Real factory-materialized21, or explicitly declared legacy15, with a native Mine/module-Unload route. Neither path assigns mined freight or completed extraction work. */
	SpaceVehicle &Miner(uint16_t model = SPACE_HEAVY_MINER_MODEL)
	{
		REQUIRE((model == SPACE_PROBE_MODEL || model == SPACE_HEAVY_MINER_MODEL));
		SpaceVehicle *craft = nullptr;
		if (model == SPACE_HEAVY_MINER_MODEL) {
			auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks);
			REQUIRE(job.phase == OrbitalManufacturePhase::Complete); craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr);
			REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		} else {
			// This original legacy unit is explicit fixture input, matching the
			// small mining rig. It is not an invented orbital factory recipe for15.
			REQUIRE(EnsureSpaceEngineDefinition(model)); REQUIRE(Vehicle::CanAllocateItem());
			craft = SpaceVehicle::Create(); REQUIRE(RegisterSpaceVehicleIdentity(*craft));
			craft->owner = _current_company; craft->engine_type = GetSpaceProbeEngine();
			craft->cargo_type = GetCargoTypeByLabel(CargoLabel{"NORE"}); craft->cargo_cap = 20;
			const auto *body = FindAsteroidBody(GetAsteroidField(), OrbitalBodyID{0}); REQUIRE(body != nullptr);
			craft->position = craft->home = {body->motion.position.x - 4 * SPACE_Q16, body->motion.position.y - 3 * SPACE_Q16};
			craft->UpdateSpacePosition();
		}
		this->Route(*craft, 0, *core); return *craft;
	}
	/** Offer exactly one original field/common-approach/native-hull opportunity. */
	void Step()
	{
		++TimerGameTick::counter; TickAsteroidField(); PrepareSpaceApproachTick();
		// Original stable fleet order sees the same moved field and shared slots;
		// this fixture owns no private movement, attachment or extraction counter.
		for (auto *craft : SpaceVehicle::Iterate()) REQUIRE(craft->Tick());
	}
	/** @param craft Actual routed miner. @return Its original body after bounded native pursuit earns the exclusive lease. */
	AsteroidBody &Attach(SpaceVehicle &craft)
	{
		// No initial attachment pose or lease is assigned; each native opportunity
		// performs the same movement/admission as the regular fleet driver.
		for (uint tick = 0; craft.mission.phase != SpaceMissionPhase::Mining; ++tick) { REQUIRE(tick < 5000); this->Step(); }
		auto *body = GetMutableAsteroidBody(craft.mission.target); REQUIRE(body != nullptr); REQUIRE(body->mining_slot == craft.index); return *body;
	}
};
#endif /* TESTS_SPACE_MINING_FIXTURE_H */
