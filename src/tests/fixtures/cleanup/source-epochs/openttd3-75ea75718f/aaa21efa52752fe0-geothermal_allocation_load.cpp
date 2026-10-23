/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_allocation_load.cpp Actual393 connected shared grants/use/expiry and independent copied-file preview from a retained paid392 world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3-4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; operating generator output retains10.6
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_heat.h"
#include "../geothermal_collector.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../command_func.h"
#include "../transport_tube.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_heat_allocation_observer = nullptr; ///< Test-only native accepted-world callback; source/control/stock/quantity owners remain actual loaded participants.

/** @param arguments Startup callback with no extra arguments. @return Whether the complete accepted-world observer ran. */
static bool ObserveHeatAllocation(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_heat_allocation_observer != nullptr); (*_heat_allocation_observer)(); return true;
}

/** Actual native company expense exercises complete physical heat use; this file asserts no operating generator or recipe output. */
class SavedRoutedExpense final : public GeothermalHeatParticipant {
	CompanyID owner; ///< Actual paying company.
	UtilityHeatUse role; ///< Immutable native whole benefit.
	UtilityCompanyWork expense; ///< Actual native complete expense, without a copied balance.
public:
	/** @param owner Actual company funding the complete five-unit action. @param role Immutable complete thermal role. */
	SavedRoutedExpense(CompanyID owner, UtilityHeatUse role) : owner(owner), role(role), expense(owner) {}
	/** @return Actual immutable generator/direct complete role. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->role; }
	/** @param request Actual owned native request. @return Complete real funding/role admission before reserving scarce heat. */
	bool ValidateDemand(const GeothermalHeatRequest &request) const override
	{
		return request.owner == this->owner && request.use == this->role && Company::Get(this->owner)->money >= Money{5}; // The complete native expense must be fundable before this owner requests useful heat.
	}
	/** @param proposal Exact native shared reservation. @return Complete actual work admission without mutation. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->expense.Validate(proposal); }
	/** @param proposal Exact already checked source quantity. @return Complete actual expense or unchanged refusal. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { return this->expense.Commit(proposal); }
};

/** Actual old absence, connected whole reservations, paid use and native expiry preserve exact original source/path rights through ordinary files. */
TEST_CASE("Connected native heat grants survive actual files and independent malformed previews", "[.geothermal-allocation-file][.geothermal-topology-file]")
{
	const auto root = FindTestRepositoryRoot(), directory = GetTestCaseArtifactPaths("geothermal", "native-reload").saves;
	const char *input_text = std::getenv("OTTD_HEAT_ALLOCATION_INPUT"), *mode_text = std::getenv("OTTD_HEAT_ALLOCATION_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr); const auto input = ResolveTestInputPath(input_text); const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); bool observed = false; std::exception_ptr failure;
	/** Inspect fully accepted actual native owners; the retained predecessor's supplied construction floor/materials remain explicit boundary inputs. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup pause(_pause_mode, PauseModes{});
			const auto collectors = CaptureGeothermalCollectors(); REQUIRE(collectors.size() == 1); REQUIRE(IsGeothermalCollectorOperational(collectors.front().industry));
			const auto producer = collectors.front().heat_producer; REQUIRE(GetUtilityPowerProducer(producer) != nullptr);
			const auto *claim = GetGeothermalSiteClaim(collectors.front().site); REQUIRE(claim != nullptr);
			const auto end = TileAddWrap(claim->site.endpoint, -1, 2);
			const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), cold = ComputeGeothermalSiteDigest(), collector = ComputeGeothermalCollectorDigest();
			const auto generated = GetUtilityPowerProducer(producer)->generated_lifetime_j, consumed = GetUtilityPowerProducer(producer)->consumed_lifetime_j;
			SavedRoutedExpense generator(_current_company, UtilityHeatUse::ElectricalGenerator), direct(_current_company, UtilityHeatUse::DirectRecipe);
			if (mode == "legacy-read") {
				CHECK(CaptureGeothermalHeatConsumers().empty()); CHECK(CaptureGeothermalHeatRoutes().empty()); CHECK(GetGeothermalHeatRouteInterval() == 0);
			} else if (mode == "fixture-grant") {
				REQUIRE(CaptureGeothermalHeatConsumers().empty()); REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, claim->site.endpoint, end).has_value());
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					if (!IsUtilityPowerAllocated()) REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
					REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
				}
				REQUIRE(PublishGeothermalCollectorHeat(collectors.front().industry, GetUtilityPowerInterval(), 1) == UtilityPowerError::None);
				std::vector<GeothermalHeatDemand> demands;
				for (const auto role : {UtilityHeatUse::ElectricalGenerator, UtilityHeatUse::ElectricalGenerator, UtilityHeatUse::DirectRecipe}) {
					const auto id = RegisterGeothermalHeatConsumer(_current_company, end, role, 10000); REQUIRE(id.has_value());
					demands.push_back({{_current_company, *id, end, role, 10000}, role == UtilityHeatUse::DirectRecipe ? &direct : &generator}); // Both roles compete for one original native source over the actual paid four-cell inlet.
				}
				AutoRestoreBackup system(_current_company, OWNER_DEITY);
				REQUIRE(AllocateVentHeat(GetUtilityPowerInterval(), demands) == UtilityPowerError::None);
				REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
				CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == generated + 27000); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == consumed);
			} else {
				const auto bindings = CaptureGeothermalHeatConsumers(); REQUIRE(bindings.size() == 3);
				const auto original_reserved = GetUtilityPowerProducer(producer)->reserved_j, original_first_used = GetUtilityPowerConsumer(bindings[0].consumer)->shares.front().consumed_j;
				if (mode.starts_with("topology-")) {
					const auto bridge = TileAddWrap(claim->site.endpoint, -1, 0); const auto interval = GetUtilityPowerInterval();
					const auto cash = Company::Get(_current_company)->money;
					const auto physical = ComputeGeothermalHeatRouteDigest();
					if (mode == "topology-break-unused" || mode == "topology-break-used") {
						const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {bindings[0].consumer, interval}, 1000); REQUIRE(proposal.error == UtilityPowerError::None);
						const auto before = ComputeUtilityPowerDigest();
						REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, bridge, std::make_tuple(bridge, UtilityEnergyDomain::Heat)).Succeeded());
						CHECK(ComputeUtilityPowerDigest() == before);
						REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, bridge, std::make_tuple(bridge, UtilityEnergyDomain::Heat)).Succeeded());
						CHECK(CommitUtilityMutation(proposal, &generator).error == UtilityPowerError::StaleMutation); CHECK(Company::Get(_current_company)->money == cash - Money{250}); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
					} else if (mode == "topology-repair") {
						REQUIRE_FALSE(GetGeothermalUtilityLink(bridge, UtilityEnergyDomain::Heat).has_value());
						REQUIRE(Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, bridge, std::make_tuple(bridge, UtilityEnergyDomain::Heat)).Succeeded());
						CHECK(Company::Get(_current_company)->money == cash - Money{1000}); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
						CHECK(ConsumeGeothermalHeat({bindings[0].consumer, interval}, 1, generator).error != UtilityPowerError::None);
					} else if (mode == "topology-replan") {
						{ AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); }
						REQUIRE(PublishGeothermalCollectorHeat(collectors.front().industry, GetUtilityPowerInterval(), 1) == UtilityPowerError::None);
						std::vector<GeothermalHeatDemand> demands;
						for (const auto &binding : bindings) {
							const auto role = GetUtilityPowerConsumer(binding.consumer)->effect == UtilityConsumerEffect::HeatDirectRecipe ? UtilityHeatUse::DirectRecipe : UtilityHeatUse::ElectricalGenerator;
							demands.push_back({{_current_company, binding.consumer, binding.endpoint, role, 10000}, role == UtilityHeatUse::DirectRecipe ? &direct : &generator}); // Replan the existing actual shared consumers over fresh paid cells, without reviving old epoch rights.
						}
						{ AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateVentHeat(GetUtilityPowerInterval(), demands) == UtilityPowerError::None); REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None); }
						CHECK(ConsumeGeothermalHeat({bindings[0].consumer, interval}, 1, generator).error != UtilityPowerError::None);
						REQUIRE(ConsumeGeothermalHeat({bindings[0].consumer, GetUtilityPowerInterval()}, 1000, generator).error == UtilityPowerError::None);
						CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == consumed + 1000); CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == generated + 27000);
					} else if (mode == "topology-legacy-stale") {
						auto legacy = CaptureUtilityPowerState(); const auto cell = GetGeothermalUtilityLink(bridge, UtilityEnergyDomain::Heat); REQUIRE(cell.has_value());
						// Explicit valid pre-invalidation393 compatibility input retires
						// only the paid cell scalar; the original native offer/grant/use
						// and original path references are preserved until actual load.
						legacy.links[cell->id.base()].active = false; ++legacy.local_link_revision; ++legacy.operation_sequence;
						StageLoadedUtilityPowerState(std::move(legacy)); REQUIRE(AcceptLoadedUtilityPowerState());
						CHECK(ComputeGeothermalHeatRouteDigest() == physical);
					}
					const bool replanned = mode == "topology-replan" || mode == "topology-replanned-read";
					if (mode != "topology-legacy-stale") CHECK(GetUtilityPowerProducer(producer)->reserved_j == (replanned ? 26000 : 0));
					const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, claim->site.endpoint, end);
					const bool connected = mode == "topology-repair" || mode == "topology-repaired-read" || replanned;
					CHECK(path.has_value() == connected);
					const auto routes = CaptureGeothermalHeatRoutes(); REQUIRE(routes.size() == 3);
					for (const auto &route : routes) {
						CHECK(route.producer == producer); CHECK(route.cells.size() == 4);
						if (replanned) CHECK(route.cells == path->cells); // Fresh allocation must use the actual repaired identity; old historical routes stay original in all prior states.
					}
					if (!replanned) CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == consumed);
				} else if (mode == "fixture-expire") {
					const auto interval = GetUtilityPowerInterval(); ++TimerGameTick::counter; PrepareTubePowerTick();
					CHECK(GetUtilityPowerInterval() == interval + 1); CHECK(CaptureGeothermalHeatRoutes().empty()); CHECK(GetGeothermalHeatRouteInterval() == 0);
					CHECK(GetUtilityPowerProducer(producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == generated + 27000); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == consumed);
					const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_HEAT_ALLOCATION_CORRUPT"); REQUIRE(corrupt != nullptr);
					const auto utility = ComputeUtilityPowerDigest(), physical = ComputeGeothermalHeatRouteDigest();
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
					const auto valid_preview = SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
					INFO(_load_check_data.error_msg); REQUIRE(valid_preview == SaveLoadResult::Ok);
					CHECK((_load_check_data.geothermal_heat_route_expected == std::array<uint32_t, 3>{3, 3, 12})); CHECK(ComputeUtilityPowerDigest() == utility);
				} else if (mode == "fixture-use") {
					const auto cash = Company::Get(_current_company)->money;
					REQUIRE(ConsumeGeothermalHeat({bindings[0].consumer, GetUtilityPowerInterval()}, 5000, generator).error == UtilityPowerError::None);
					REQUIRE(ConsumeGeothermalHeat({bindings[2].consumer, GetUtilityPowerInterval()}, 7000, direct).error == UtilityPowerError::None);
					CHECK(Company::Get(_current_company)->money == cash - Money{10}); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == consumed + 12000);
				}
				if (mode != "fixture-expire" && mode != "expired-read" && !mode.starts_with("topology-")) {
					const auto routes = CaptureGeothermalHeatRoutes(); REQUIRE(routes.size() == 3); CHECK(GetGeothermalHeatRouteInterval() == GetUtilityPowerInterval());
					for (const auto &route : routes) { CHECK(route.producer == producer); CHECK(route.cells.size() == 4); } // Every saved positive slice retains its exact original source and paid path, including consumed history.
					const bool used = mode == "fixture-use" || mode == "used-read";
					CHECK(GetUtilityPowerProducer(producer)->reserved_j == (mode == "preview" ? original_reserved : used ? 15000 : 27000));
					CHECK(GetUtilityPowerConsumer(bindings[0].consumer)->shares.front().consumed_j == (mode == "preview" ? original_first_used : used ? 5000 : 0));
				} else if (!mode.starts_with("topology-")) {
					CHECK(CaptureGeothermalHeatRoutes().empty()); CHECK(GetGeothermalHeatRouteInterval() == 0); CHECK(GetUtilityPowerProducer(producer)->reserved_j == 0);
				}
			}
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalSiteDigest() == cold); CHECK(ComputeGeothermalCollectorDigest() == collector);
			REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None); REQUIRE(ValidateGeothermalUtilityLinks());
			if (mode != "preview") {
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				const auto filename = mode.starts_with("topology-") ? fmt::format("fixture393-heat-{}.sav", mode) : fmt::format("fixture393-heat-allocation-{}.sav", mode);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / filename), false) == SaveLoadResult::Ok);
			}
			fmt::print("HEAT_ALLOCATION_FILE case={} utility={:016X} physical={:016X} interval={} actual_generator_output=false\n", mode, ComputeUtilityPowerDigest(), ComputeGeothermalHeatRouteDigest(), GetUtilityPowerInterval());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_heat_allocation_observer, &inspect); IConsole::CmdRegister("heat_allocation_check", ObserveHeatAllocation);
	const std::string config = (GetTestRuntimeConfig()).string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
