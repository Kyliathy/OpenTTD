/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_effect_load.cpp Actual native city offer, custom authority and expiry persistence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 9
 * @see docs/zz-reach2/architecture/towns/archi-towns.md ordinary/custom growth ownership
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../utility_power.h"
#include "../town.h"
#include "../town_cmd.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include <cstdlib>
#include <charconv>
#include "layer_save_fixture.h"

#include "../safeguards.h"

static std::function<void()> *_utility_effect_observer = nullptr; ///< Isolated actual application observer; no alternate town or energy authority.

/** @param arguments Exact native startup command. @return Whether the loaded-world callback ran. */
static bool ObserveUtilityEffect(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_utility_effect_observer != nullptr);
	(*_utility_effect_observer)(); return true;
}

/** Actual metered city benefit/custom/expiry states survive normal native loading without reconsuming the offer. */
TEST_CASE("Native town power effect and custom GS suppression survive actual save files", "[.utility-power-effect-file]")
{
	const auto root = FindTestRepositoryRoot();
	const auto directory = GetTestCaseArtifactPaths("utilities", "native-reload").saves;
	const char *case_value = std::getenv("OTTD_UTILITY_FILE_CASE"), *input_value = std::getenv("OTTD_UTILITY_FILE_INPUT");
	REQUIRE(case_value != nullptr); REQUIRE(input_value != nullptr);
	const std::string_view mode(case_value);
	const std::string input = ResolveTestInputPath(input_value).string();
	bool observed = false; std::exception_ptr failure;
	/** Actual existing Surface town receives only an explicitly registered/settled native offer. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			/** @param name Fixed local output basename. @return None; ordinary save preserves live utility digest. */
			const auto save = [&](std::string_view name) {
				const auto digest = ComputeUtilityPowerDigest();
				AutoRestoreBackup compression(_savegame_format, std::string("lzma"));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / std::string(name)), false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == digest);
			};
			if (mode == "create") {
				REQUIRE_FALSE(HasUtilityPowerState());
				Town *target = nullptr;
				// Choose a real retained city, preserving its houses, cargos and
				// growth conditions; no synthetic town or manually painted site is used.
				for (Town *town : Town::Iterate()) {
					if (GetTileLayer(town->xy) != MapLayer::Surface) continue;
					if (town->flags.Test(TownFlag::CustomGrowth)) continue;
					target = town; break;
				}
				REQUIRE(target != nullptr);
				RefreshUtilityTownGrowthRate(target->index);
				const uint16_t baseline = target->growth_rate;
				REQUIRE(baseline > 1);
				fmt::print("UTILITY_TOWN_BASELINE rate={}\n", baseline);
				const auto source = RegisterUtilityPowerProducer(_local_company, MapLayer::Surface, UtilityGenerationKind::External);
				REQUIRE(source == UtilityProducerID{0});
				const auto consumer = RegisterUtilityPowerConsumer(_local_company, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, 135000, std::array{*source});
				REQUIRE(consumer == UtilityConsumerID{0});
				REQUIRE(BindUtilityTownConsumer(*consumer, target->index) == UtilityPowerError::None);
				REQUIRE(PublishUtilityGeneration(*source, 1, 5000, 1000) == UtilityPowerError::None);
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*consumer, 135000}}) == UtilityPowerError::None);
				}
				SavedRandomSeeds before, after; SaveRandomSeeds(&before);
				REQUIRE(ApplyUtilityTownEffect(*consumer).error == UtilityPowerError::None);
				SaveRandomSeeds(&after);
				CHECK(std::ranges::equal(before.random.state, after.random.state));
				CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
				CHECK(target->growth_rate == std::max<uint>(1, static_cast<uint>(baseline) * 100 / 125));
				save("utility-town-applied.sav");
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, target->index, 37).Succeeded());
				}
				save("utility-town-custom.sav");
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, target->index, 0).Succeeded());
				}
				CHECK(GetUtilityTownPowerBonus(target->index) == 0);
				CHECK(target->growth_rate == baseline);
				save("utility-town-relinquished.sav");
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
				}
				CHECK(target->growth_rate == baseline);
				save("utility-town-expired.sav");
			} else {
				const char *baseline_text = std::getenv("OTTD_UTILITY_TOWN_BASELINE");
				REQUIRE(baseline_text != nullptr);
				const std::string_view encoded(baseline_text); uint16_t baseline = 0;
				const auto parsed = std::from_chars(encoded.data(), encoded.data() + encoded.size(), baseline);
				REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == encoded.data() + encoded.size());
				const auto state = CaptureUtilityPowerState();
				REQUIRE(state.producers.size() == 1); REQUIRE(state.consumers.size() == 1);
				const auto &consumer = state.consumers.front();
				Town *target = Town::GetIfValid(consumer.town);
				REQUIRE(target != nullptr);
				CHECK(state.producers.front().consumed_lifetime_j == 135000);
				if (mode == "applied") {
					CHECK(GetUtilityTownPowerBonus(target->index) == 25);
					CHECK(target->growth_rate == std::max<uint>(1, static_cast<uint>(baseline) * 100 / 125));
				} else if (mode == "custom") {
					CHECK(target->flags.Test(TownFlag::CustomGrowth));
					CHECK(target->growth_rate == 37);
					CHECK(GetUtilityTownPowerBonus(target->index) == 0);
				} else {
					CHECK_FALSE(target->flags.Test(TownFlag::CustomGrowth));
					CHECK(target->growth_rate == baseline);
					CHECK(GetUtilityTownPowerBonus(target->index) == 0);
				}
				const auto before = ComputeUtilityPowerDigest();
				CHECK(ApplyUtilityTownEffect(UtilityConsumerID{0}).error != UtilityPowerError::None);
				CHECK(ComputeUtilityPowerDigest() == before);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save(fmt::format("utility-town-{}-reloaded.sav", mode));
			}
			fmt::print("UTILITY_TOWN_FILE case={} actual_town=true metered_use=true\n", mode);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_utility_effect_observer, &inspect);
	IConsole::CmdRegister("utility_effect_check", ObserveUtilityEffect);
	const std::string config = (GetTestRuntimeConfig()).string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
