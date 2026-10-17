/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file assembly_native_fixture.h Reusable actual paid-world commands for orbital Assembly acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups6B-8
 * This adapter creates no fixture hull, input packet, utility account, position or consumed receipt.
 */
#ifndef TESTS_ASSEMBLY_NATIVE_FIXTURE_H
#define TESTS_ASSEMBLY_NATIVE_FIXTURE_H
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../command_func.h"
#include "../core/backup_type.hpp"
#include "../asteroid.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>

/** Non-owning adapter around the genuine paid Core, deployed builders and reusable Surface rocket. */
class AssemblyNativeWorld {
	const TestArtifactPaths &paths;
public:
	SpaceStructure *core = nullptr; ///< Original paid Core, never an invented station or proxy.
	SpaceVehicle *rocket = nullptr; ///< Original paid reusable carrier, preserving its native incarnation.
	std::vector<SpaceVehicle *> builders; ///< Already deployed paid builder hulls, in native pool order.

	/** @param paths Explicit invocation-owned output paths; all gameplay inputs already belong to the loaded native world. */
	explicit AssemblyNativeWorld(const TestArtifactPaths &paths) : paths(paths)
	{
		REQUIRE(ValidateSpaceVehicles());
		core = SpaceStructure::GetIfValid(SpaceStructureID{0}); REQUIRE(core != nullptr); REQUIRE(IsOrbitalModuleOperational(*core));
		// Only existing paid native hulls may drive this proof; commissioning and deployment cannot be replaced with fixture allocations.
		for (auto *craft : SpaceVehicle::Iterate()) {
			if (GetSpaceVehicleModel(*craft) == 14) { REQUIRE(rocket == nullptr); rocket = craft; }
			// Later factory products are actual additional hulls, never substitutes for the original two paid Earth-deployed construction probes used by this adapter.
			if (GetSpaceVehicleModel(*craft) == 20 && craft->construction.placement == SpacePlacement::Orbital && craft->construction.assembly == SpaceStructureRef{}) builders.push_back(craft);
		}
		REQUIRE(rocket != nullptr); REQUIRE(builders.size() == 2);
	}

	/** @param ticks Number of actual synchronized opportunities, independent of GUI pacing. */
	void Advance(uint ticks)
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
		// Rendering/yields do not count as production or travel; only the actual field/native-vehicle schedule advances the requested work.
		for (uint done = 0; done < ticks;) {
			REQUIRE(std::chrono::steady_clock::now() < deadline);
			const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
			if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
			REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
		}
	}

	/** @param ready Actual native-state predicate. @param maximum Maximum synchronized opportunities before this boundary must be reached. */
	void Await(const std::function<bool()> &ready, uint maximum = 3000)
	{
		// Every bounded wait uses actual public state; no timeout substitutes a successful material arrival, physical berth or paid flight receipt.
		for (uint elapsed = 0; !ready(); ++elapsed) { REQUIRE(elapsed < maximum); this->Advance(1); }
	}

	/** @param name Unique checkpoint basename in this invocation. Serialize actual native owners with their exact saved pause state. */
	void Save(std::string_view name) const
	{
		REQUIRE(ValidateSpaceVehicles());
		AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
		AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
	}

	/** @param craft Actual paid hull to order. @param site Exact native destination. @param action Visit or Construct, using the corresponding typed target. */
	void Order(SpaceVehicle &craft, const SpaceStructure &site, SpaceOrderAction action) const
	{
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
		const auto target = action == SpaceOrderAction::Construct ? SpaceOrderTargetKind::Blueprint : SpaceOrderTargetKind::Structure;
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, action, target, uint32_t{site.index.base()}, site.generation));
	}

	/** @param receiver Existing deployed builder. @param cargo Actual Surface stock cargo. @param quantity Whole units shipped through the same paid reusable rocket. */
	void Import(SpaceVehicle &receiver, CargoType cargo, uint16_t quantity)
	{
		const auto home = rocket->construction.home; const auto missions = rocket->transition->missions_completed;
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(receiver.index, receiver.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
		REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, cargo, quantity, false));
		REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
		REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{receiver.index.base()}, receiver.generation));
		// Arrival includes actual paid recovery; elapsed estimates cannot teleport the receiver or credit undelivered construction materials.
		this->Await([&] { return rocket->transition->missions_completed == missions + 1; });
		CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored);
	}
};
#endif
