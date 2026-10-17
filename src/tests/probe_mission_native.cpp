/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_mission_native.cpp Current paid typed missions, company disclosure and actual network command dispatch.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8B.4
 * All hulls/materials originate in accepted paid saves. The second company is an explicit identity-only fixture, with no granted assets or funding.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_structure.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../core/string_consumer.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *probe_mission_observer = nullptr; ///< Current process observer only; native owners retain all state.

/** @param args Parameter-free actual loaded-world console callback. @return Whether the selected observer ran. */
static bool ObserveProbeMission(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(probe_mission_observer != nullptr); (*probe_mission_observer)(); return true;
}

/** Non-owning native test driver; no private position, queue, work, cargo or company-account authority. */
class ProbeMissionWorld {
	const TestArtifactPaths &paths; ///< Exact invocation artifact owner.
public:
	/** @param paths Actual invocation output directories. */
	explicit ProbeMissionWorld(const TestArtifactPaths &paths) : paths(paths) {}
	/** @param ticks Actual synchronized field/vehicle opportunities to await, excluding CargoDist pause yields. */
	void Advance(uint ticks) const
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
		// Count the common world's actual opportunities. A paused scheduler cannot earn scan/extraction work or spend the saved deadline twice.
		for (uint done = 0; done < ticks;) {
			REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
			if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
			REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
		}
	}
	/** @param ready Actual saved-state predicate. @param maximum Maximum real opportunities before refusal. */
	void Await(const std::function<bool()> &ready, uint maximum = 5000) const
	{
		// Arrival and earned work must be observed from original native state; the bound never substitutes progress.
		for (uint count = 0; !ready(); ++count) { REQUIRE(count < maximum); this->Advance(1); }
	}
	/** @param name Current explicit actual-world checkpoint basename. */
	void Save(std::string_view name) const
	{
		REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
		AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
	}
	/** Replace an exact original hull's instructions locally or through actual native server queues.
	 * @param craft Actual paid hull to command. @param entries Ordered typed target identities.
	 * @param network True exercises SendNet, distribution, frame admission and original PostFromNet dispatch; false posts the local twin.
	 */
	void Orders(SpaceVehicle &craft, std::initializer_list<SpaceOrder> entries, bool network) const
	{
		const auto before = ComputeSpaceOrdersDigest(); const auto elapsed = GetAsteroidField().elapsed_ticks;
		AutoRestoreBackup server(_network_server, network); AutoRestoreBackup networking(_networking, network);
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup frame(_frame_counter); AutoRestoreBackup maximum(_frame_counter_max, _frame_counter);
		/** @param edit Actual native edit. @param index Exact insertion position. @param order Full-width typed command arguments. */
		auto issue = [&](SpaceOrderEdit edit, uint8_t index, const SpaceOrder &order) {
			if (network) Command<Commands::EditSpaceOrder>::SendNet({}, craft.owner, craft.index, craft.generation, edit, index, order.action, order.target.kind, order.target.id, order.target.generation);
			else REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, edit, index, order.action, order.target.kind, order.target.id, order.target.generation));
		};
		issue(SpaceOrderEdit::Clear, 0, {SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, 0, 0}}); uint8_t index = 0;
		// Every action goes through its original typed command. The native queue, rather than this observer, applies owner/role/generation validation.
		for (const auto &entry : entries) issue(SpaceOrderEdit::Insert, index++, entry);
		if (network) {
			CHECK(ComputeSpaceOrdersDigest() == before); NetworkDistributeCommands(); NetworkExecuteLocalCommandQueue();
			CHECK(ComputeSpaceOrdersDigest() == before); // Scheduled next-frame commands cannot act early.
			++_frame_counter; NetworkExecuteLocalCommandQueue(); const auto committed = ComputeSpaceOrdersDigest();
			NetworkExecuteLocalCommandQueue(); CHECK(ComputeSpaceOrdersDigest() == committed); NetworkFreeLocalCommandQueue();
		}
		CHECK(GetAsteroidField().elapsed_ticks == elapsed); CHECK(craft.space_orders.entries == std::vector<SpaceOrder>(entries));
	}
	/** Capture the actual company-facing native inspector without advancing simulation or granting knowledge.
	 * @param craft Actual inspected incarnation. @param name Explicit current image filename.
	 */
	void Capture(SpaceCraftRef craft, std::string_view name) const
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		ShowSpaceSurveyWindow(craft); auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); REQUIRE(window != nullptr);
		CaptureNativeArtWindow(*window, paths.images / std::string{name});
	}
};

/** Refuse role-invalid, foreign-owner and stale-incarnation edits without changing original physical authority.
 * @param original Exact paid commanded hull. @param other Valid distinct fixture-company identity, or Invalid when that entry has no second company.
 * @param required_roles Native role bits that this accepted physical entry must actually contain; other inputs supply the remaining roles.
 */
static void CheckProbeMissionRefusals(SpaceVehicle &original, CompanyID other, uint required_roles)
{
	const auto orders = ComputeSpaceOrdersDigest(), fleet = ComputeSpaceVehicleDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
	const auto manifests = ComputeSpaceManifestDigest(), structures = ComputeSpaceStructureDigest(); const auto random = _random;
	const auto money = Company::Get(original.owner)->money; uint roles = 0;
	// Exercise actual deployed mining, survey and construction hulls; a role cannot acquire extraction or disclosure by using another action's target shape.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (craft->construction.placement != SpacePlacement::Orbital) continue;
		const auto model = GetSpaceVehicleModel(*craft); SpaceOrderAction action;
		if (model == 15) { roles |= 1; action = SpaceOrderAction::Survey; }
		else if (model == 19) { roles |= 2; action = SpaceOrderAction::Mine; }
		else if (model == 20) { roles |= 4; action = SpaceOrderAction::Mine; }
		else continue;
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0, action, SpaceOrderTargetKind::Body, 0, 0).Failed());
	}
	CHECK((roles & required_roles) == required_roles);
	if (Company::IsValidID(other)) {
		AutoRestoreBackup foreign(_current_company, other);
		CHECK(CmdEditSpaceOrder(DoCommandFlag::Execute, original.index, original.generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
	}
	CHECK(CmdEditSpaceOrder(DoCommandFlag::Execute, original.index, original.generation + 1, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
	CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
	CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(Company::Get(original.owner)->money == money);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Execute one current paid mission boundary, or genuine pre-knowledge migration, in the actual loaded application. */
TEST_CASE("Paid mission order targets scans companies and network dispatch retain native owners", "[.probe-mission-native]")
{
	const char *input = std::getenv("OTTD_PROBE_MISSION_SOURCE"), *mode = std::getenv("OTTD_PROBE_MISSION_ACTION"); REQUIRE(input != nullptr); REQUIRE(mode != nullptr);
	const char *id_env = std::getenv("OTTD_PROBE_MISSION_VEHICLE"), *generation_env = std::getenv("OTTD_PROBE_MISSION_GENERATION"); REQUIRE(id_env != nullptr); REQUIRE(generation_env != nullptr);
	const auto id = ParseInteger<uint32_t>(id_env); const auto generation = ParseInteger<uint64_t>(generation_env); REQUIRE(id); REQUIRE(generation);
	const std::string_view action{mode}; const bool network = std::getenv("OTTD_PROBE_MISSION_NETWORK") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-mission-native"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only this original loaded world; preserve assertion exceptions until native shutdown has completed. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); ProbeMissionWorld world(paths); const OrbitalBodyID body{0};
			const auto fleet_before = ComputeSpaceVehicleDigest(), field_before = ComputeAsteroidFieldDigest(GetAsteroidField());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeSpaceVehicleDigest() == fleet_before); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field_before);
			if (action == "legacy-unknown") {
				REQUIRE(!GetAsteroidField().bodies.empty()); REQUIRE(GetAsteroidField().knowledge.empty()); world.Advance(148);
				// Old ownerless preview work grants no receipt to any live company, even after a complete modern scan interval has elapsed.
				for (const auto *company : Company::Iterate()) for (const auto &entry : GetAsteroidField().bodies) CHECK_FALSE(GetAsteroidQuantityDisclosure(company->index, entry.id));
				CHECK(GetAsteroidField().knowledge.empty());
			} else {
				auto *craft = ResolveSpaceCraft({VehicleID{*id}, *generation}); REQUIRE(craft != nullptr);
				// Native startup chooses the client's default company. After a saved real takeover, select the original hull's actual acquirer through the ordinary client-company boundary before issuing owned commands.
				if (_local_company != craft->owner) SetLocalCompany(craft->owner);
				REQUIRE(craft->owner == _current_company);
				const auto original = GetSpaceCraftReference(*craft); const auto hulls = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration();
				if (action == "visit-mine-partial") {
					REQUIRE(GetSpaceVehicleModel(*craft) == 15); REQUIRE(craft->cargo.TotalCount() == 0); REQUIRE(craft->manifest != nullptr); REQUIRE(craft->manifest->returns_remaining == 1);
					CheckProbeMissionRefusals(*craft, CompanyID::Invalid(), 1);
					CHECK_FALSE(GetAsteroidQuantityDisclosure(craft->owner, body));
					world.Orders(*craft, {{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, 1, 0}}, {SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}}}, network);
					/** @return Whether real Visit(A) advanced before real Mine(B) earned37 extraction ticks. */
					world.Await([&] { return craft->space_orders.cursor == 1 && craft->mission.work_ticks == 37; });
					CHECK(craft->mission.target == body); CHECK(GetMutableAsteroidBody(body)->mining_slot == craft->index); CHECK(craft->cargo.TotalCount() == 0);
				} else if (action == "visit-mine-next") {
					REQUIRE(craft->space_orders.cursor == 1); REQUIRE(craft->mission.target == body); REQUIRE(craft->mission.work_ticks == 37);
					const auto reserve = GetMutableAsteroidBody(body)->remaining_reserve; world.Advance(37);
					CHECK(craft->cargo.TotalCount() == 2); CHECK(GetMutableAsteroidBody(body)->remaining_reserve == reserve - 2); CHECK_FALSE(GetAsteroidQuantityDisclosure(craft->owner, body));
				} else if (action == "scan-partial") {
					REQUIRE(GetSpaceVehicleModel(*craft) == 19); REQUIRE(GetAsteroidField().knowledge.empty()); REQUIRE(Company::CanAllocateItem());
					auto *other = Company::Create(); SetCompanyManagerFaceStyle(other->face, 0); // Identity-only lifecycle fixture: no money, hull, inventory or recipe is granted.
					CheckProbeMissionRefusals(*craft, other->index, 6); world.Capture(original, "survey-unknown.ppm");
					world.Orders(*craft, {{SpaceOrderAction::Survey, {SpaceOrderTargetKind::Body, 0, 0}}}, network);
					/** @return Whether the original moving-body scan earned37 of its148 native working ticks. */
					world.Await([&] { return craft->space_orders.work_ticks == 37; });
					CHECK_FALSE(GetAsteroidQuantityDisclosure(craft->owner, body)); CHECK_FALSE(GetAsteroidQuantityDisclosure(other->index, body));
					CHECK(GetMutableAsteroidBody(body)->mining_slot != craft->index); world.Capture(original, "survey-partial.ppm");
				} else if (action == "scan-cancel") {
					REQUIRE(craft->space_orders.work_ticks == 37); world.Orders(*craft, {}, network); world.Advance(148);
					CHECK(craft->space_orders.work_ticks == 0); CHECK_FALSE(GetAsteroidQuantityDisclosure(craft->owner, body)); CHECK(GetAsteroidField().knowledge.empty());
					world.Capture(original, "survey-cancelled-unknown.ppm");
				} else if (action == "scan-complete") {
					REQUIRE(craft->space_orders.work_ticks == 37); const auto profit = craft->profit_this_year; const auto remainder = craft->mission.cost_remainder;
					world.Advance(111); CHECK(craft->space_orders.cursor == 1); CHECK(craft->space_orders.work_ticks == 0);
					CHECK(craft->profit_this_year == profit - Money{(remainder + 111 * 500) / 74} * 256); CHECK(craft->mission.cost_remainder == (remainder + 111 * 500) % 74);
					const auto quantity = GetAsteroidQuantityDisclosure(craft->owner, body); REQUIRE(quantity); CHECK(quantity->remaining == GetMutableAsteroidBody(body)->remaining_reserve); CHECK(quantity->initial == GetMutableAsteroidBody(body)->initial_reserve);
					world.Capture(original, "survey-complete-known.ppm");
					// Every other real company still lacks this company's receipt; merely opening the same body inspector cannot confer disclosure.
					for (const auto *other : Company::Iterate()) {
						if (other->index == craft->owner) continue;
						CHECK_FALSE(GetAsteroidQuantityDisclosure(other->index, body)); SetLocalCompany(other->index); world.Capture(original, "survey-foreign-unknown.ppm"); SetLocalCompany(craft->owner);
					}
				} else if (action == "scan-transfer") {
					const auto owner = craft->owner; CompanyID other_id = CompanyID::Invalid();
					// Select the sole explicit identity fixture created before scan work, never a guessed pool ID or replacement company.
					for (const auto *other : Company::Iterate()) if (other->index != owner) { REQUIRE(other_id == CompanyID::Invalid()); other_id = other->index; }
					REQUIRE(other_id != CompanyID::Invalid()); REQUIRE(GetAsteroidQuantityDisclosure(owner, body)); ChangeOwnershipOfCompanyItems(owner, other_id);
					CHECK(craft->owner == other_id); CHECK(GetAsteroidQuantityDisclosure(other_id, body)); CHECK_FALSE(GetAsteroidQuantityDisclosure(owner, body));
					_current_company = _local_company; SetLocalCompany(other_id);
				} else if (action == "company-reuse") {
					CompanyID vacant = CompanyID::Invalid();
					// The previous actual takeover left one empty former owner. Destroy/reallocate that native slot and retain the acquirer's exact original paid hull.
					for (const auto *other : Company::Iterate()) if (other->index != craft->owner) { REQUIRE(vacant == CompanyID::Invalid()); vacant = other->index; }
					REQUIRE(vacant != CompanyID::Invalid()); delete Company::Get(vacant); REQUIRE(Company::CanAllocateItem()); auto *reused = Company::Create(); SetCompanyManagerFaceStyle(reused->face, 0);
					CHECK(reused->index == vacant); CHECK_FALSE(GetAsteroidQuantityDisclosure(reused->index, body)); CHECK(GetAsteroidQuantityDisclosure(craft->owner, body));
				} else FAIL("Unknown current probe mission action");
				CHECK(ResolveSpaceCraft(original) == craft); CHECK(GetSpaceVehicleCount() == hulls); CHECK(GetNextSpaceVehicleGeneration() == next);
			}
			world.Save(fmt::format("probe-mission-{}-430.sav", action));
			fmt::print("PROBE_MISSION native=true action={} network={} vehicle={} generation={} elapsed={} knowledge={}\n", action, network, *id, *generation, GetAsteroidField().elapsed_ticks, GetAsteroidField().knowledge.size());
		} catch (...) { failure = std::current_exception(); }
		_current_company = _local_company; observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(probe_mission_observer, &observe); IConsole::CmdRegister("probe_mission_test", ObserveProbeMission);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
