/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_pursuit.cpp Moving-edge geometry and explicit isolated lost-target/deadline states.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group9
 * Geometry and legacy component starting states are declared fixtures; separate native paid deployment supplies route acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../multilayer_space_content.h"
#include "../vehicle_func.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "../safeguards.h"

/** Pursuit must follow current reflected coordinates, retaining a fixed assignment allowance. */
TEST_CASE("Probe pursuit crosses a tile and reflected travel edge before interception", "[probe-pursuit]")
{
	const auto band = GetSpaceTravelBand(64, 64);
	// Test both outward horizontal edges independently. The original shared movement functions, rather than a copied pursuit implementation, own every fractional step and reflection.
	for (const int32_t sign : {-1, 1}) {
		SpaceMotion body{{sign > 0 ? band.max.x - 5 * SPACE_Q16 / 4 : band.min.x + 5 * SPACE_Q16 / 4, 32 * SPACE_Q16}, sign * 16384, 0, 0, 0};
		const auto initial = body.position; SpacePoint craft{initial.x - sign * 28 * SPACE_Q16, initial.y};
		const auto offset = SpacePoint{SPACE_Q16, -SPACE_Q16 / 2}; const SpacePoint first{initial.x + offset.x, initial.y + offset.y};
		const auto allowance = GetSpaceMiningApproachTicks(craft, first); uint8_t remainder = 0; bool reflected = false, changed_tile = false, arrived = false;
		// The body moves first, matching the native schedule. Following its initial tile would miss the reflected attachment and cannot satisfy the final actual point.
		for (uint tick = 0; tick < allowance; ++tick) {
			AdvanceSpaceMotion(body, band); reflected |= body.velocity_x != sign * 16384;
			changed_tile |= body.position.x / SPACE_Q16 != initial.x / SPACE_Q16;
			const SpacePoint current{body.position.x + offset.x, body.position.y + offset.y};
			if (MoveSpaceSurveyTowards(craft, current, remainder)) { arrived = true; CHECK(craft == current); break; }
		}
		CHECK(arrived); CHECK(reflected); CHECK(changed_tile); CHECK(body.position != initial); CHECK(craft != first);
	}
}

/** Explicit legacy native component world; its initial two-unit hold is not a paid gameplay procurement or mission claim. */
class ProbePursuitExitRig {
	ExcavationFixture world;
	AsteroidField previous = GetAsteroidField(); ///< Original field restored after this isolated hull retires.
	GRFConfigList previous_content; ///< Caller package identity, restored after this component's actual native hull retires.
public:
	SpaceVehicle *craft = nullptr; ///< Sole actual native component hull, not a fake parallel cargo container.
	/** Declare one legacy native hull/hold and a generated finite target for independently controlled failure inputs. */
	ProbePursuitExitRig()
	{
		MockEnvironment::Instance(); previous_content.swap(_grfconfig);
		// Declare the same exact public package selected by this isolated world so native Engine validation cannot borrow stale content identity from another test.
		for (const auto &descriptor : world.ActivatedContent()) {
			auto copy = std::make_unique<GRFConfig>(); copy->ident = descriptor->ident; copy->version = descriptor->version; copy->status = descriptor->status; _grfconfig.push_back(std::move(copy));
		}
		REQUIRE(GenerateWorldAsteroids(12345)); REQUIRE(InitialiseSpaceEngineDefinition()); REQUIRE(Vehicle::CanAllocateItem());
		craft = SpaceVehicle::Create(); REQUIRE(RegisterSpaceVehicleIdentity(*craft)); craft->owner = _current_company; craft->engine_type = GetSpaceProbeEngine();
		craft->cargo_type = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(craft->cargo_type)); craft->cargo_cap = 20;
		craft->position = {10 * SPACE_Q16, 20 * SPACE_Q16}; craft->home = craft->position; craft->vehstatus.Reset(VehState::Stopped);
		craft->mission.target = OrbitalBodyID{0}; craft->mission.phase = SpaceMissionPhase::Approach; craft->mission.approach_ticks_left = 10;
		REQUIRE(CargoPacket::CanAllocateItem()); auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{2}, Source{0, SourceType::Asteroid});
		packet->UpdateLoadingTile(TileXYL(20, 30, MapLayer::Space)); craft->cargo.Append(packet); craft->UpdateSpacePosition();
	}
	/** Retire this component's physical hold before restoring the caller's field and native hash. */
	~ProbePursuitExitRig() { _vehicle_pool.CleanPool(); ResetVehicleHash(); CHECK(InstallAsteroidField(std::move(previous))); _grfconfig.clear(); previous_content.swap(_grfconfig); }
};

/** Target loss, depletion, deadline and paid allowance failures preserve the original physical holding. */
TEST_CASE("Probe pursuit failures retain cargo while choosing a bounded explicit exit", "[probe-pursuit]")
{
	ProbePursuitExitRig rig; auto &craft = *rig.craft; const auto packet = craft.cargo.Packets()->front()->index;
	SECTION("Missing target") { craft.mission.target = INVALID_ORBITAL_BODY; }
	SECTION("Depleted target") { GetMutableAsteroidBody(craft.mission.target)->remaining_reserve = 0; }
	SECTION("Frozen deadline exhausted") { craft.mission.approach_ticks_left = 0; }
	SECTION("Paid allowance exhausted") {
		// Explicit copied component service state exercises the real admission guard; it grants no paid shell, allowance or actual service history.
		craft.construction.content_version = MULTILAYER_SPACE_CONTENT_VERSION; craft.construction.model = 15;
		craft.manifest = std::make_unique<SpaceManifest>(); craft.manifest->returns_remaining = 0;
	}
	const auto remaining = GetMutableAsteroidBody(OrbitalBodyID{0})->remaining_reserve;
	REQUIRE(craft.Tick()); CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.cargo.Packets()->front()->index == packet);
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{0})->remaining_reserve == remaining); CHECK(craft.extracted_units == 0); CHECK(craft.mission.work_ticks == 0);
	CHECK((craft.mission.phase == SpaceMissionPhase::Return || craft.mission.phase == SpaceMissionPhase::Holding));
}
