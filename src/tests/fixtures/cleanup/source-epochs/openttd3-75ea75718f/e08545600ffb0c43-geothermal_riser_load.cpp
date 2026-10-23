/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_load.cpp Real native construction files, independent copied preview and ordinary accepted-world continuations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3
 */

#include "../stdafx.h"
#include <any>
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "geothermal_riser_fixture.h"
#include "geothermal_electrical_fixture.h"
#include "layer_save_fixture.h"
#include "layer_window_fixture.h"
#include "mock_environment.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../spoil_receiver.h"
#include "../layer_industry_processor.h"
#include "../geothermal_collector.h"
#include "../console_internal.h"
#include "../economy_func.h"
#include "../landscape_cmd.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include <charconv>
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *_riser_file_observer = nullptr; ///< Test-only accepted-world observer; native loader owns all actual map/stock/control references.

/** @param arguments Exact zero-argument startup command. @return Whether the accepted native-world observer ran. */
static bool ObserveRiserFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_riser_file_observer != nullptr); (*_riser_file_observer)(); return true;
}

/** Real command-funded double relay saves complete, physically split and freshly repaired cable identities; supplied floor/materials do not prove driven bootstrap. */
TEST_CASE("Write actual native electrical relay files", "[.geothermal-relay-write]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); NativeElectricalRelayRig rig(true);
	REQUIRE(InitialiseSpoilReceiverDefinitions(rig.ActivatedContent())); REQUIRE(InitialiseLayerMineDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseLayerProcessorDefinitions(rig.ActivatedContent())); REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); LayerSaveContent content(rig.ActivatedContent());
	AutoRestoreBackup local(_local_company, rig.request.owner); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	TimerGameCalendar::SetDate(TimerGameCalendar::date, {});
	const auto directory = GetTestCaseArtifactPaths("geothermal", "riser-native-files").saves;
	/** @param name Exact paid-geometry checkpoint. Save the native owners without transferring or generating electricity. */
	const auto save = [&](std::string_view name) {
		REQUIRE(ValidateGeothermalRisers(CaptureGeothermalRisers())); REQUIRE(ValidateGeothermalUtilityLinks());
		REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("riser-{}.sav", name)), false) == SaveLoadResult::Ok);
	};
	rig.InstallSourceCable(); rig.InstallRelayCable(); save("relay-connected");
	REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground), UtilityEnergyDomain::Electricity, true).Succeeded()); save("relay-split");
	REQUIRE(ElectricalCableCommand(TileXYL(25, 20, MapLayer::Underground)).Succeeded()); save("relay-repaired");
	fmt::print("RISER_RELAY_WRITE actual_commands=true boundary_open_floor=true boundary_material_delivery=true operating_export=false\n");
}

/** Real command-funded machinery fixtures use the normal native writer, declared open test geology and explicit native material receipts; no transported bootstrap is claimed. */
TEST_CASE("Write actual native riser construction lifecycle files", "[.geothermal-riser-write]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	NativeRiserRig rig(false, true);
	// Match ordinary exact-package catalogue activation. Missing delivered
	// definitions would otherwise be added by normal loading and obscure an
	// exact native mapping comparison despite preserving the riser's own ID.
	REQUIRE(InitialiseSpoilReceiverDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseLayerMineDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseLayerProcessorDefinitions(rig.ActivatedContent()));
	REQUIRE(InitialiseGeothermalCollectorDefinitions(rig.ActivatedContent()));
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START; // Bare fixture naming-town retains the normal native generator identity on full reload.
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); // Actual save ownership toggles the busy cursor even in a windowless focused writer.
	LayerSaveContent content(rig.ActivatedContent());
	AutoRestoreBackup local(_local_company, rig.request.owner);
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	TimerGameCalendar::SetDate(TimerGameCalendar::date, {});
	const auto directory = GetTestCaseArtifactPaths("geothermal", "riser-native-files").saves;
	/** @param name Exact native construction checkpoint. Write actual authority without advancing it, duplicating native stock or creating electrical flow. */
	const auto save = [&](std::string_view name) {
		REQUIRE(ValidateGeothermalRisers(CaptureGeothermalRisers()));
		const auto before = ComputeGeothermalRiserDigest();
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu); // Serialize exact native state without constructing a progress window in the isolated fixture.
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("riser-{}.sav", name)), false) == SaveLoadResult::Ok);
		CHECK(ComputeGeothermalRiserDigest() == before);
	};
	REQUIRE(rig.Fund().Succeeded()); save("funded");
	/** @param name Exact deliberately invalid native-owner checkpoint. Keep dependent riser scalars valid so only full native floor/stock acceptance can refuse it. */
	const auto save_invalid_native = [&](std::string_view name) {
		REQUIRE_FALSE(ValidateGeothermalRisers(CaptureGeothermalRisers()));
		AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("riser-{}.sav", name)), false) == SaveLoadResult::Ok);
	};
	SetIndustryConstructionStage(rig.request.tile, 2); save_invalid_native("invalid-native-stage"); SetIndustryConstructionStage(rig.request.tile, 0);
	auto steel = rig.Riser().GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}));
	steel->waiting = 5; save_invalid_native("invalid-native-stock"); steel->waiting = 0;
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 4) == 4); REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4); save("shortage");
	REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2); REQUIRE(rig.Deliver(CargoLabel{"BATY"}, 2) == 2); rig.Tick(148); save("partial");
	rig.Tick(148); save("complete");
	const auto source = RegisterUtilityPowerConsumer(rig.request.owner, GetTileLayer(rig.request.portal), UtilityConsumerEffect::AssemblyWork, 1000000, {});
	const auto destination = RegisterUtilityPowerConsumer(rig.request.owner, GetTileLayer(rig.destination), UtilityConsumerEffect::TubeWork, 1000000, {});
	REQUIRE(source.has_value()); REQUIRE(destination.has_value());
	REQUIRE(RegisterGeothermalRiserLink(rig.Riser().index, *source, *destination).has_value()); save("linked");
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, false, true) == LayerPortalWorkError::None); save("sealed");
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, true, true) == LayerPortalWorkError::None); save("reopened");
	REQUIRE(RegisterGeothermalRiserLink(rig.Riser().index, *source, *destination).has_value()); save("relinked");
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.destination).Succeeded()); save("retired");
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, rig.destination, TransportType::Rail, MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, true, true) == LayerPortalWorkError::None); save("rebuilt-portal");
	{
		AutoRestoreBackup spectator(_local_company, COMPANY_SPECTATOR); // Native bankruptcy transfers the local viewer before abandoning that company's infrastructure.
		ChangeOwnershipOfCompanyItems(rig.request.owner, INVALID_OWNER);
	}
	save("abandoned");
	fmt::print("RISER_NATIVE_WRITE paid_funding=true boundary_open_floor=true boundary_material_delivery=true transported_bootstrap=false\n");
}

/** Full native loading/independent preview reads actual saved owners and exercises the immediate next construction tick without rewriting any supplied input. */
TEST_CASE("Read actual native riser files and independent malformed previews", "[.geothermal-riser-file]")
{
	const char *input_text = std::getenv("OTTD_RISER_FILE_INPUT"), *mode_text = std::getenv("OTTD_RISER_FILE_CASE");
	REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const auto input = ResolveTestInputPath(input_text);
	const std::string_view mode{mode_text};
	const auto directory = GetTestCaseArtifactPaths("geothermal", "riser-native-files").saves;
	bool observed = false; std::exception_ptr failure;
	/** Inspect only actual accepted native authority; copied preview must preserve this world's complete construction/native-stock/clock/RNG owners. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto rows = CaptureGeothermalRisers();
			const auto before = ComputeGeothermalRiserDigest();
			const auto utility = ComputeUtilityPowerDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			if (mode == "independent-preview" || mode == "independent-relay-preview") {
				REQUIRE(rows.empty());
				const auto *candidate = std::getenv("OTTD_RISER_FILE_PREVIEW"); REQUIRE(candidate != nullptr);
				const auto clock = TimerGameTick::counter;
				const auto width = Map::SizeX(), height = Map::SizeY();
				const auto random = std::array{_random.state[0], _random.state[1]};
				const auto mappings = _industry_mngr.mappings;
				REQUIRE(SaveOrLoad(candidate, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.geothermal_riser_rows.size() == (mode == "independent-relay-preview" ? 2 : 1));
				CHECK(CaptureGeothermalRisers().empty()); CHECK(ComputeGeothermalRiserDigest() == before);
				CHECK(Map::SizeX() == width); CHECK(Map::SizeY() == height); CHECK(TimerGameTick::counter == clock);
				CHECK(std::array{_random.state[0], _random.state[1]} == random);
				// Same-file native type resolution must leave every live allocator
				// entry untouched, including worlds with no installed riser family.
				REQUIRE(mappings.size() == _industry_mngr.mappings.size());
				for (size_t index = 0; index < mappings.size(); ++index) {
					CHECK(mappings[index].grfid == _industry_mngr.mappings[index].grfid);
					CHECK(mappings[index].entity_id == _industry_mngr.mappings[index].entity_id);
					CHECK(mappings[index].substitute_id == _industry_mngr.mappings[index].substitute_id);
				}
			} else if (mode == "legacy") {
				CHECK(rows.empty());
			} else {
				REQUIRE(rows.size() == (mode.starts_with("relay") ? 2 : 1)); REQUIRE(ValidateGeothermalRisers(rows));
				auto *industry = Industry::GetIfValid(rows[0].industry); REQUIRE(industry != nullptr);
				// Scalar and native-only faults exercise separate refusal stages;
				// neither can publish a partly accepted paid machinery family.
				if (mode == "full-refusal" || mode == "full-native-refusal" || mode == "relay-refusal" || mode == "relay-native-refusal") {
					const auto *bad = std::getenv("OTTD_RISER_FILE_CORRUPT"); REQUIRE(bad != nullptr);
					if (mode == "full-native-refusal" || mode == "relay-native-refusal") REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok); // Scalar previews cannot establish actual native tile/stock readiness; full acceptance owns these checks.
					REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					CHECK(CaptureGeothermalRisers().empty());
					// Error-path Null fixup must leave a recoverable native world.
					// A second ordinary full load accepts only the valid original,
					// without leaking staged paid work or forgiving its bad copy.
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(ComputeGeothermalRiserDigest() == before);
				} else if (mode == "preview" || mode == "relay-preview") {
					const auto *bad = std::getenv("OTTD_RISER_FILE_CORRUPT"); REQUIRE(bad != nullptr);
					const auto clock = TimerGameTick::counter; const auto random = std::array{_random.state[0], _random.state[1]};
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeGeothermalRiserDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility);
					CHECK(TimerGameTick::counter == clock); CHECK(std::array{_random.state[0], _random.state[1]} == random);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.geothermal_riser_rows.size() == rows.size()); CHECK(ComputeGeothermalRiserDigest() == before);
				} else if (mode == "relay" || mode == "relay-split") {
					REQUIRE(ValidateGeothermalUtilityLinks());
					const auto path = FindGeothermalElectricalPath(UtilityConsumerID{0}, UtilityConsumerID{2}, TileXYL(21, 19, MapLayer::DeepUnderground), 74);
					if (mode == "relay-split") CHECK_FALSE(path.has_value());
					else { REQUIRE(path.has_value()); REQUIRE(path->legs.size() == 2); CHECK(path->capacity_w == 200000); CHECK(path->capacity_j == 399600); CHECK(path->legs[1].inlet.cells.size() == 12); }
					CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(GetUtilityPowerConsumer(UtilityConsumerID{2})->meter.granted_energy_j == 0);
				} else if (mode == "next-tick") {
					REQUIRE(rows[0].work_ticks == 148);
					AutoRestoreBackup pause(_pause_mode, PauseModes{}); AdvanceGeothermalRiser(*industry);
					CHECK(GetGeothermalRiserState(industry->index)->work_ticks == 148);
					++TimerGameTick::counter; AdvanceGeothermalRiser(*industry);
					CHECK(GetGeothermalRiserState(industry->index)->work_ticks == 149);
				} else {
					const auto *expected_text = std::getenv("OTTD_RISER_FILE_EXPECTED"); REQUIRE(expected_text != nullptr);
					uint16_t expected = 0;
					const auto parsed = std::from_chars(expected_text, expected_text + strlen(expected_text), expected);
					REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == expected_text + strlen(expected_text));
					CHECK(rows[0].work_ticks == expected);
					if (mode == "retired") { CHECK(rows[0].portal_retired); CHECK_FALSE(IsGeothermalRiserOperational(industry->index)); }
					if (mode == "sealed") { CHECK_FALSE(rows[0].portal_retired); CHECK_FALSE(IsGeothermalRiserOperational(industry->index)); }
					if (mode == "complete") CHECK(IsGeothermalRiserOperational(industry->index));
					if (mode == "link") {
						CHECK(IsGeothermalRiserOperational(industry->index));
						REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
						size_t active = 0;
						// Actual saved shared edges retain the original machinery and
						// physical rate; no fixture invents a generator or grant.
						for (const auto &edge : GetUtilityPowerLinks()) {
							if (edge.kind != UtilityLinkKind::ElectricalRiser || !edge.active) continue;
							++active; CHECK(edge.riser == industry->index); CHECK(edge.riser_capacity_w == 200000); CHECK(edge.flow_j == 0); CHECK(edge.capacity_j == 0);
						}
						CHECK(active == 1); CHECK(CaptureUtilityPowerState().producers.empty());
					}
					if (mode == "shortage") {
						CHECK(industry->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 4);
						CHECK(industry->GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"BATY"}))->waiting == 0);
					}
				}
			}
			CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			// Preview/refusal processes leave no resave that could be mistaken
			// for successful admission of the deliberately malformed candidate.
			if (mode != "preview" && mode != "full-refusal" && mode != "full-native-refusal" && mode != "independent-preview" &&
				mode != "relay-preview" && mode != "relay-refusal" && mode != "relay-native-refusal" && mode != "independent-relay-preview") {
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("riser-read-{}-{}.sav", mode, input.stem().string())), false) == SaveLoadResult::Ok);
			}
			fmt::print("RISER_NATIVE_FILE case={} digest={:016X} rows={}\n", mode, ComputeGeothermalRiserDigest(), rows.size());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_riser_file_observer, &inspect); IConsole::CmdRegister("geothermal_riser_check", ObserveRiserFile);
	const std::string config = GetTestRuntimeConfig().string(), source = input.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", source};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
