/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_heat_load.cpp Real390 shared native heat/electric files and retained389 migration; declared accounting/expense participants do not prove an operating plant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group2B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; actual transported generator remains10.6
 */

#include "../stdafx.h"
#include <cstdlib>
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_collector.h"
#include "../industry.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

static std::function<void()> *_utility_heat_observer = nullptr; ///< Test-owned accepted-world callback; ordinary native loading owns all actual quantities and references.

/** Actual five-unit company expense accompanies typed heat use; this boundary fixture claims no generator output, operating recipe or transported bootstrap. */
class UtilityThermalCompanyWork : public UtilityCompanyWork {
	UtilityHeatUse use; ///< Actual declared whole expense participant role.
public:
	/** @param owner Actual paying company. @param use Immutable role matching its registered heat reservation. */
	UtilityThermalCompanyWork(CompanyID owner, UtilityHeatUse use) : UtilityCompanyWork(owner), use(use) {}
	/** @return Domain of this actual whole expense/work admission. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return Immutable direct/generator role; no electrical effect is created by this fixture. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->use; }
};

/** @param state Read-only accepted native owner snapshot. @return Exact field-wise electrical rows/phase, excluding only added thermal records and shared allocator/change counters; this view grants or owns no energy. */
static std::vector<uint64_t> ElectricalFileView(const UtilityPowerState &state)
{
	std::vector<uint64_t> view{state.interval, state.allocated};
	/** @param meter Exact actual electrical fields; append explicit values without host padding or a private meter. */
	const auto account = [&view](const UtilityPowerAccount &meter) {
		view.insert(view.end(), {to_underlying(meter.domain), meter.producer_id.base(), meter.consumer_id.base(), to_underlying(meter.layer),
			meter.accounting_interval, meter.economy_ticks, meter.generation_rate_w, meter.generation_energy_j,
			meter.demand_energy_j, meter.granted_energy_j, meter.unmet_energy_j, meter.remainder_mj});
	};
	// Exact source/consumer/route boundaries retain every original electrical
	// identity, carry, quantity, lifetime, share and effect during thermal writes.
	for (const auto &source : state.producers) {
		if (source.meter.domain != UtilityEnergyDomain::Electricity) continue;
		view.push_back(0); account(source.meter);
		view.insert(view.end(), {source.owner.base(), to_underlying(source.kind), source.reserved_j, source.active, source.generated_lifetime_j, source.consumed_lifetime_j});
	}
	for (const auto &consumer : state.consumers) {
		if (consumer.meter.domain != UtilityEnergyDomain::Electricity) continue;
		view.push_back(1); account(consumer.meter);
		view.insert(view.end(), {consumer.owner.base(), to_underlying(consumer.effect), consumer.capacity_j, consumer.active, consumer.town.base(),
			consumer.town_bonus_percent, consumer.town_effect_applied, consumer.town_effect_suspended, consumer.linked_producers.size()});
		for (const auto source : consumer.linked_producers) view.push_back(source.base()); // Keep exact explicit eligibility, rather than rebuilding a graph from the fixture's coordinates.
		view.push_back(consumer.shares.size());
		for (const auto &share : consumer.shares) view.insert(view.end(), {share.producer.base(), share.allocated_j, share.consumed_j, share.released_j, share.transferred_j, share.imported_link.base()}); // Preserve each original source/route settlement row once.
	}
	for (const auto &link : state.links) {
		view.insert(view.end(), {2, link.id.base(), link.source.base(), link.destination.base(), link.source_endpoint.base(), link.destination_endpoint.base(),
			link.portal_revision, link.capacity_j, link.flow_j, link.active}); // Electrical physical commissioning and once-only flow cannot change during a thermal transaction.
	}
	return view;
}

/** @param arguments Exact zero-argument startup callback. @return Whether the actual accepted-world fixture observer executed. */
static bool ObserveUtilityHeat(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_utility_heat_observer != nullptr); (*_utility_heat_observer)(); return true;
}

/** Exact native shared-domain reservation/use/retirement files and corrupt preview retries preserve original natural, collector and electrical owners. */
TEST_CASE("Native shared heat roles and electrical lineage survive real files", "[.utility-heat-file]")
{
	const auto root = std::filesystem::current_path().parent_path(), directory = root / "src/tests/fixtures/utility";
	const char *input_text = std::getenv("OTTD_UTILITY_HEAT_INPUT"), *mode_text = std::getenv("OTTD_UTILITY_HEAT_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const std::filesystem::path input{input_text}; const std::string_view mode{mode_text};
	REQUIRE(std::filesystem::exists(input)); REQUIRE(std::filesystem::is_directory(directory));
	bool observed = false; std::exception_ptr failure;
	/** Inspect actual accepted native quantities; all feature availability/physical plant construction beyond this accounting boundary retains its geothermal owner. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto before = CaptureUtilityPowerState(); const auto electrical = ElectricalFileView(before);
			const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), collector = ComputeGeothermalCollectorDigest(), cold = ComputeGeothermalSiteDigest();
			if (mode == "legacy-read") {
				CHECK_FALSE(HasUtilityHeatState()); CHECK_FALSE(IsUtilityHeatAllocated());
			} else {
				if (mode == "fixture-heat") {
					REQUIRE_FALSE(HasUtilityHeatState()); const auto collectors = CaptureGeothermalCollectors(); REQUIRE(collectors.size() == 1);
					REQUIRE(IsGeothermalCollectorOperational(collectors[0].industry)); REQUIRE(collectors[0].owner == _current_company);
					const auto *site = GetGeothermalSiteClaim(collectors[0].site); REQUIRE(site != nullptr);
					const auto &vents = GetGeologyWorldState().vents; const auto vent = std::ranges::find(vents, site->site.vent, &GeothermalVentField::id); REQUIRE(vent != vents.end());
					const auto source = RegisterUtilityHeatProducer(_current_company); REQUIRE(source.has_value());
					const auto direct = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::DirectRecipe, 20000, std::array{*source}); REQUIRE(direct.has_value());
					const auto generator = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 10000, std::array{*source}); REQUIRE(generator.has_value());
					REQUIRE(PublishUtilityHeatGeneration(*source, before.interval, vent->thermal_capacity_w, 1) == UtilityPowerError::None);
					{
						AutoRestoreBackup system(_current_company, OWNER_DEITY);
						REQUIRE(AllocateUtilityHeat(before.interval, std::array{UtilityPowerRequest{*generator, 10000}, UtilityPowerRequest{*direct, 20000}}) == UtilityPowerError::None);
					}
				} else { REQUIRE(HasUtilityHeatState()); REQUIRE(IsUtilityHeatAllocated()); }
				const auto state = CaptureUtilityPowerState();
				/** @param row Actual shared native source. @return Whether its immutable domain is thermal, excluding the original electrical participants. */
				const auto heat_source = [](const UtilityPowerProducer &row) { return row.meter.domain == UtilityEnergyDomain::Heat; };
				const auto source = std::ranges::find_if(state.producers, heat_source); REQUIRE(source != state.producers.end());
				const auto generator = std::ranges::find(state.consumers, UtilityConsumerEffect::HeatGenerator, &UtilityPowerConsumer::effect); REQUIRE(generator != state.consumers.end());
				if (mode == "fixture-use") {
					const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {generator->meter.consumer_id, state.interval}, 5000); REQUIRE(proposal.error == UtilityPowerError::None);
					UtilityThermalCompanyWork work(_current_company, UtilityHeatUse::ElectricalGenerator); const auto cash = Company::Get(_current_company)->money;
					REQUIRE(CommitUtilityMutation(proposal, &work).error == UtilityPowerError::None); CHECK(Company::Get(_current_company)->money == cash - Money{5});
				} else if (mode == "preview") {
					const auto *corrupt = std::getenv("OTTD_UTILITY_HEAT_CORRUPT"); REQUIRE(corrupt != nullptr); const auto digest = ComputeUtilityPowerDigest();
					CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == digest);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.utility_power_state.heat_allocated); CHECK(ComputeUtilityPowerDigest() == digest);
				} else {
					CHECK(GetUtilityHeatAllocation(generator->meter.consumer_id)->granted_energy_j == (mode == "used-read" ? 2000 : 7000));
					CHECK(source->consumed_lifetime_j == (mode == "used-read" ? 5000 : 0));
				}
			}
			CHECK(ElectricalFileView(CaptureUtilityPowerState()) == electrical); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
			CHECK(ComputeGeothermalCollectorDigest() == collector); CHECK(ComputeGeothermalSiteDigest() == cold);
			REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
			if (mode != "preview") {
				const auto digest = ComputeUtilityPowerDigest(); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("fixture390-heat-{}.sav", mode)), false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == digest);
			}
			fmt::print("UTILITY_HEAT_FILE case={} shared={:016X} electrical_exact=true natural_collector_exact=true boundary_expense_only=true operating_generator=false\n", mode, ComputeUtilityPowerDigest());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_utility_heat_observer, &inspect); IConsole::CmdRegister("utility_heat_check", ObserveUtilityHeat);
	const std::string config = (root / "build/native-art.cfg").string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
