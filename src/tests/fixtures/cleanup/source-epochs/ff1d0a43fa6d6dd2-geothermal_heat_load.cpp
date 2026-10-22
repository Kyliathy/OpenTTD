/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_heat_load.cpp Real391 completed-collector meter references, reservations and native next-tick reload; physical links/generator retain later groups.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md independent preview
 */

#include "../stdafx.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_collector.h"
#include "../transport_tube.h"
#include "../industry.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_thermal_source_observer = nullptr; ///< Test-only accepted-world callback; ordinary loading resolves every native owner first.

/** Actual typed company expense exercises heat settlement without impersonating a generator or direct factory. */
class ThermalSourceExpense : public UtilityCompanyWork {
public:
	/** @param owner Actual paying company in the accepted saved world. */
	explicit ThermalSourceExpense(CompanyID owner) : UtilityCompanyWork(owner) {}
	/** @return Domain of this actual whole expense boundary. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return Immutable generator-role expense; no electrical output is produced. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return UtilityHeatUse::ElectricalGenerator; }
};

/** @param arguments Startup callback and no extra arguments. @return Whether the accepted-world observer ran. */
static bool ObserveThermalSource(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_thermal_source_observer != nullptr); (*_thermal_source_observer)(); return true;
}

/** Genuine389/390 predecessors acquire no implicit source; current native offers/grants/use survive exact files and malformed previews leave live state intact. */
TEST_CASE("Actual collector shared source survives native files and next tick", "[.geothermal-heat-file]")
{
	const auto root = std::filesystem::current_path().parent_path(), directory = root / "src/tests/fixtures/geothermal";
	const char *input_text = std::getenv("OTTD_THERMAL_SOURCE_INPUT"), *mode_text = std::getenv("OTTD_THERMAL_SOURCE_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const std::filesystem::path input{input_text}; const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); bool observed = false; std::exception_ptr failure;
	/** Inspect fully accepted actual owners; every supplied floor/material and accounting-only role remains explicitly a boundary fixture. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup pause(_pause_mode, PauseModes{});
			const auto rows = CaptureGeothermalCollectors(); REQUIRE(rows.size() == 1); REQUIRE(IsGeothermalCollectorOperational(rows[0].industry));
			const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), cold = ComputeGeothermalSiteDigest();
			if (mode == "legacy-read") {
				CHECK(rows[0].heat_producer == UtilityProducerID{UINT32_MAX});
			} else if (mode == "fixture-offer") {
				REQUIRE(rows[0].heat_producer == UtilityProducerID{UINT32_MAX});
				const auto interval = GetUtilityPowerInterval();
				REQUIRE(PublishGeothermalCollectorHeat(rows[0].industry, interval, 1) == UtilityPowerError::None);
				const auto producer = GetGeothermalCollectorState(rows[0].industry)->heat_producer;
				const auto consumer = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 20000, std::array{producer}); REQUIRE(consumer.has_value());
				AutoRestoreBackup system(_current_company, OWNER_DEITY);
				REQUIRE(AllocateUtilityHeat(interval, std::array{UtilityPowerRequest{*consumer, 20000}}) == UtilityPowerError::None);
				if (!IsUtilityPowerAllocated()) REQUIRE(AllocateUtilityPower(interval, {}) == UtilityPowerError::None);
			} else {
				const auto producer = rows[0].heat_producer; const auto *source = GetUtilityPowerProducer(producer); REQUIRE(source != nullptr);
				CHECK(source->meter.domain == UtilityEnergyDomain::Heat); CHECK(source->meter.generation_rate_w == 1000000);
				if (mode == "fixture-next") {
					const auto interval = GetUtilityPowerInterval(), generated = source->generated_lifetime_j, used = source->consumed_lifetime_j;
					++TimerGameTick::counter; PrepareTubePowerTick();
					CHECK(GetUtilityPowerInterval() == interval + 1); CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == generated + 27000);
					CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == used);
					const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_THERMAL_SOURCE_CORRUPT"); REQUIRE(corrupt != nullptr);
					const auto utility = ComputeUtilityPowerDigest(), collector = ComputeGeothermalCollectorDigest();
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalCollectorDigest() == collector);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.geothermal_collector_rows[0].heat_producer == producer.base());
				} else {
					const auto state = CaptureUtilityPowerState();
					const auto consumer = std::ranges::find(state.consumers, UtilityConsumerEffect::HeatGenerator, &UtilityPowerConsumer::effect); REQUIRE(consumer != state.consumers.end());
					if (mode == "fixture-use") {
						const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {consumer->meter.consumer_id, state.interval}, 5000);
						ThermalSourceExpense expense(_current_company); REQUIRE(CommitUtilityMutation(proposal, &expense).error == UtilityPowerError::None);
					} else if (mode == "next-read") {
						CHECK(source->consumed_lifetime_j == 5000); CHECK(source->generated_lifetime_j == 54000); CHECK(source->reserved_j == 0);
					} else {
						CHECK(source->consumed_lifetime_j == (mode == "used-read" ? 5000 : 0));
						CHECK(GetUtilityHeatAllocation(consumer->meter.consumer_id)->granted_energy_j == (mode == "used-read" ? 15000 : 20000));
						const auto before = ComputeUtilityPowerDigest();
						CHECK(PublishGeothermalCollectorHeat(rows[0].industry, state.interval, 1) == UtilityPowerError::AlreadyAllocated); CHECK(ComputeUtilityPowerDigest() == before);
					}
				}
			}
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(ComputeGeothermalSiteDigest() == cold);
			REQUIRE(ValidateGeothermalCollectors(CaptureGeothermalCollectors())); REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
			if (mode != "preview") {
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("fixture391-thermal-{}.sav", mode)), false) == SaveLoadResult::Ok);
			}
			fmt::print("THERMAL_SOURCE_FILE case={} shared={:016X} collector={:016X} physical_links=false operating_generator=false\n", mode, ComputeUtilityPowerDigest(), ComputeGeothermalCollectorDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_thermal_source_observer, &inspect); IConsole::CmdRegister("thermal_source_check", ObserveThermalSource);
	const std::string config = (root / "build/native-art.cfg").string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
