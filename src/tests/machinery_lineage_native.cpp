/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file machinery_lineage_native.cpp Original donor native machinery reconstruction and copied receipt comparisons.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md
 * Default observation performs no simulation work, command or purchase. Optional44qualification writes new test-cache successors; explicit46first-tick mode advances only isolated component copies.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "../geothermal_generator.h"
#include "../multilayer_state.h"
#include "../industry.h"
#include "../cargotype.h"
#include "../geothermal_generator_wire.h"
#include "../transport_tube_geothermal.h"
#include "../transport_tube_cmd.h"
#include "../surface_supply_cmd.h"
#include "../command_func.h"
#include "../fios.h"
#include "../town.h"
#include "../table/strings.h"
#include "../strings_func.h"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "save_candidate_lifecycle_fixture.h"
#include "save_supply_wire_fixture.h"
#include "save_first_tick_fixture.h"
#include <charconv>
#include "../safeguards.h"

static std::function<void()> *machinery_observer = nullptr; ///< Invocation callback only; no gameplay state or elapsed work.

/** Run the original post-load observation through the native startup console.
 * @param args One-word startup command, without gameplay operations.
 * @return Whether the registered observation ran; test failures propagate to its enclosing invocation.
 */
static bool ObserveOriginalMachinery(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(machinery_observer != nullptr); (*machinery_observer)(); return true;
}

/** Bind an explicitly selected donor original to its independent whole-file digest.
 * @param digest Exact64-character SHA256 from the retained input catalogue, never a filename-derived format.
 * @return Per-attempt B hint, with no persistent load-context override.
 */
static SaveOriginHint OriginalMachineryHint(const char *digest)
{
	REQUIRE(digest != nullptr); const std::string_view hex{digest}; REQUIRE(hex.size() == 64);
	SaveOriginHint hint{SaveFormat::Geothermal, {}};
	// Every digest byte is mandatory: an origin declaration cannot authorize
	// reinterpretation of another original or an altered negative copy.
	for (size_t index = 0; index < hint.source_digest.size(); ++index) {
		unsigned value = 0; const auto *begin = hex.data() + 2 * index;
		const auto parsed = std::from_chars(begin, begin + 2, value, 16);
		REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == begin + 2); hint.source_digest[index] = static_cast<uint8_t>(value);
	}
	return hint;
}

/** Compare actual native machinery/energy references against this original's independent copied transport.
 * @param plants Native generator construction/service receipts captured before copied parsing.
 * @param utility Sole utility owner's original quantities, identities, conversion and dispatch history.
 * @param sources Original native Tube provider references/history, without another energy balance.
 * @param vehicles Native capsule bindings, fractional demand and once-only opportunity receipts.
 */
static void CheckOriginalMachinery(const std::vector<GeothermalGeneratorState> &plants, const UtilityPowerState &utility,
	const TubeGeothermalPowerState &sources, const std::vector<TubePowerVehicle> &vehicles)
{
	const auto &data = _load_check_data; const auto &copied = data.utility_power_state;
	REQUIRE(plants.size() == data.geothermal_generator_rows.size());
	// Every original paid-work and operating receipt keeps its exact native
	// industry/site/input/output identity. Copying cannot manufacture new progress.
	for (size_t index = 0; index < plants.size(); ++index) CHECK(plants[index] == DecodeGeothermalGenerator(data.geothermal_generator_rows[index]));
	CHECK(utility.interval == copied.interval); CHECK(utility.operation_sequence == copied.operation_sequence);
	CHECK(utility.next_producer == copied.next_producer); CHECK(utility.next_consumer == copied.next_consumer);
	CHECK(utility.next_link == copied.next_link); CHECK(utility.next_share == copied.next_share);
	CHECK(utility.allocated == copied.allocated); CHECK(utility.heat_allocated == copied.heat_allocated);
	REQUIRE(utility.producers.size() == copied.producers.size());
	// Conversion remains an extension of the sole original producer; exact
	// numerator and thermal provenance survive independently of current output.
	for (size_t index = 0; index < utility.producers.size(); ++index) {
		const auto &native = utility.producers[index], &wire = copied.producers[index];
		CHECK(native.meter.producer_id == wire.meter.producer_id); CHECK(native.kind == wire.kind);
		CHECK(native.meter.generation_energy_j == wire.meter.generation_energy_j);
		CHECK(native.generated_lifetime_j == wire.generated_lifetime_j); CHECK(native.consumed_lifetime_j == wire.consumed_lifetime_j);
		CHECK(native.conversion_input == wire.conversion_input); CHECK(native.conversion_efficiency == wire.conversion_efficiency);
		CHECK(native.conversion_capacity_w == wire.conversion_capacity_w); CHECK(native.conversion_remainder == wire.conversion_remainder);
		CHECK(native.conversion_origins == wire.conversion_origins);
	}
	REQUIRE(utility.consumers.size() == copied.consumers.size());
	// Once-only city settlement and physical forwarding references must not
	// become fresh grants or replay an already committed benefit during load.
	for (size_t index = 0; index < utility.consumers.size(); ++index) {
		const auto &native = utility.consumers[index], &wire = copied.consumers[index];
		CHECK(native.meter.consumer_id == wire.meter.consumer_id); CHECK(native.meter.granted_energy_j == wire.meter.granted_energy_j);
		CHECK(native.town_terminal == wire.town_terminal); CHECK(native.town_supply == wire.town_supply); CHECK(native.town_source_port == wire.town_source_port);
		CHECK(native.town_bonus_percent == wire.town_bonus_percent); CHECK(native.town_effect_applied == wire.town_effect_applied);
	}
	CHECK(sources.next_source == data.tube_geothermal_state.next_source); CHECK(sources.sequence == data.tube_geothermal_state.sequence);
	CHECK(sources.sources == data.tube_geothermal_state.sources); REQUIRE(vehicles.size() == data.tube_vehicle_rows.size());
	// Both actual provider roles retain their original source/feed identity;
	// old external defaults cannot silently adopt a geothermal inlet.
	for (size_t index = 0; index < vehicles.size(); ++index) {
		const auto &native = vehicles[index], &wire = data.tube_vehicle_rows[index];
		CHECK(native.consumer == wire.consumer); CHECK(native.source == wire.source); CHECK(native.vehicle == wire.vehicle);
		CHECK(native.remainder_mj == wire.remainder_mj); CHECK(native.prepared_tick == wire.prepared_tick); CHECK(native.consumed_tick == wire.consumed_tick);
		CHECK(native.phase == wire.phase); CHECK(native.wait == wire.wait); CHECK(native.active == wire.active);
		CHECK(native.geothermal_source == wire.geothermal_source); CHECK(native.local_feed == wire.local_feed);
	}
}

/** Write and reconstruct one merged machinery successor without changing original quantities, identities or work.
 * @param output Invocation-owned test-cache successor, separate from the retained source.
 * Complete native owner digests and independently copied fields must agree across
 * saving, full441reconstruction and the second save. No gameplay opportunity runs.
 */
static void CheckMergedMachinerySuccessor(const char *output)
{
	REQUIRE(output != nullptr);
	const auto plants = CaptureGeothermalGenerators(); const auto utility = CaptureUtilityPowerState();
	const auto sources = CaptureTubeGeothermalPowerState(); const auto vehicles = CaptureTubePowerVehicles();
	const auto plant_digest = ComputeGeothermalGeneratorDigest(), utility_digest = ComputeUtilityPowerDigest();
	const auto source_digest = ComputeTubeGeothermalPowerDigest(), vehicle_digest = ComputeTubePowerVehicleDigest();
	const auto canonical = ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()); REQUIRE(canonical);
	std::vector<std::pair<CompanyID, Money>> wallets;
	for (const auto *company : Company::Iterate()) wallets.emplace_back(company->index, company->money);
	/** @param path Explicit invocation-owned output. Save actual owners without renewing service, dispatch or energy. */
	const auto save = [&](const std::filesystem::path &path) {
		AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	};
	save(output);
	const auto first_check = SaveOrLoad(output, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
	INFO(_load_check_data.error_msg); REQUIRE(first_check == SaveLoadResult::Ok);
	CheckOriginalMachinery(plants, utility, sources, vehicles);
	REQUIRE(SaveOrLoad(output, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CHECK(ComputeGeothermalGeneratorDigest() == plant_digest); CHECK(ComputeUtilityPowerDigest() == utility_digest);
	CHECK(ComputeTubeGeothermalPowerDigest() == source_digest); CHECK(ComputeTubePowerVehicleDigest() == vehicle_digest);
	CHECK(ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()) == canonical);
	// Wallets are real native company state, separate from service counters;
	// serialization/reconstruction cannot reapply a committed operating invoice.
	for (const auto &[id, money] : wallets) { REQUIRE(Company::IsValidID(id)); CHECK(Company::Get(id)->money == money); }
	const auto second = std::string{output} + ".again.sav"; save(second);
	REQUIRE(SaveOrLoad(second, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CheckOriginalMachinery(plants, utility, sources, vehicles);
	if (std::getenv("OTTD_MERGED_COMMAND_ESTIMATES") != nullptr) {
		const auto before = CaptureCandidateLiveState();
		const auto aggregate = ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()); REQUIRE(aggregate);
		unsigned quoted = 0;
		// Each actual installed inlet must admit a closure quote while retaining
		// its original grant/history; a stale output must refuse without acquiring
		// a replacement identity after full merged reconstruction.
		for (const auto &source : CaptureTubeGeothermalPowerState().sources) {
			if (!source.active) continue;
			AutoRestoreBackup actor(_current_company, source.owner);
			CHECK(Command<Commands::CloseTubeGeothermalPower>::Do({}, source.id).Succeeded());
			CHECK(Command<Commands::ConnectTubeGeothermalPower>::Do({}, source.generator, source.inlet, UtilityProducerID{UINT32_MAX}).Failed());
			++quoted;
		}
		REQUIRE(quoted > 0);
		CHECK(Command<Commands::CancelSurfaceSupplyOrder>::Do({}, UINT64_MAX).Failed());
		CHECK(Command<Commands::CancelLegacySurfaceSupplyOrder>::Do({}, UINT64_MAX, uint16_t{1}).Failed());
		CHECK(CaptureCandidateLiveState() == before);
		CHECK(ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()) == aggregate);
		fmt::print("MERGED_COMMAND_ESTIMATES_NATIVE current_and_stale=true accounts_packets_rng_camera_equal=true\n");
	}
}

/** Exercise actual alternating full loads and failed-then-valid recovery without allowing stale rights or preview mutation.
 * @param merged Newly produced invocation-owned441successor of this original.
 * @param original Immutable B434two-provider original used for the baseline and final recovery.
 * @param hint Its independent exact-digest original interpretation.
 * The Python producer supplies a separate genuine A440paid input and its original
 * digest. No full-load rollback is assumed; refused native reconstruction revokes
 * partly published rights, then the complete original is explicitly loaded again.
 */
static void CheckNativeCandidateLifecycle(const char *merged, const char *original, const SaveOriginHint &hint)
{
	REQUIRE(merged != nullptr); const auto baseline = CaptureCandidateLiveState(); REQUIRE_FALSE(baseline.camera.empty());
	const char *a_path = std::getenv("OTTD_MACHINERY_LIFECYCLE_A"); REQUIRE(a_path != nullptr);
	auto a_hint = OriginalMachineryHint(std::getenv("OTTD_MACHINERY_LIFECYCLE_A_DIGEST")); a_hint.format = SaveFormat::Orbital;
	const std::array paths{std::string{a_path}, std::string{original}, std::string{merged}};
	// Each actual world replacement must accept its own original family set;
	// copied selections between them cannot mutate the installed world's real view,
	// random state, native fragments, financial accounts or domain ownership.
	for (size_t index = 0; index < paths.size(); ++index) {
		const auto origin = index == 0 ? std::optional{a_hint} : index == 1 ? std::optional{hint} : std::nullopt;
		REQUIRE(SaveOrLoad(paths[index], SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, origin) == SaveLoadResult::Ok);
		const auto expected_book = _load_check_data.surface_supply_orders.snapshot;
		const auto expected_plants = _load_check_data.geothermal_generator_rows.size();
		const auto expected_sources = _load_check_data.tube_geothermal_state.sources;
		const auto expected_bindings = _load_check_data.tube_vehicle_rows;
		REQUIRE(SaveOrLoad(paths[index], SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, origin) == SaveLoadResult::Ok);
		CHECK(CaptureSurfaceSupplyOrders() == expected_book); CHECK(CaptureGeothermalGenerators().size() == expected_plants);
		CHECK(CaptureTubeGeothermalPowerState().sources == expected_sources);
		/** @param actual Actual reconstructed binding. @param expected Independently copied original fields. @return Exact provider/carry/once-only identity equality; no same-slot replacement can supply omitted references. */
		const auto same_binding = [](const TubePowerVehicle &actual, const TubePowerVehicle &expected) {
			return std::tie(actual.consumer, actual.source, actual.vehicle, actual.remainder_mj, actual.prepared_tick, actual.consumed_tick,
				actual.phase, actual.wait, actual.active, actual.geothermal_source, actual.local_feed) ==
				std::tie(expected.consumer, expected.source, expected.vehicle, expected.remainder_mj, expected.prepared_tick, expected.consumed_tick,
					expected.phase, expected.wait, expected.active, expected.geothermal_source, expected.local_feed);
		};
		CHECK(std::ranges::equal(CaptureTubePowerVehicles(), expected_bindings, same_binding));
		const auto installed = CaptureCandidateLiveState();
		// A repeated richer/older preview cannot lend previous Bpower bindings or
		// Apaid receipts to the next candidate, nor select the live camera's layer.
		for (const auto &path : paths) {
			const auto copied_hint = path == a_path ? std::optional{a_hint} : path == original ? std::optional{hint} : std::nullopt;
			REQUIRE(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, copied_hint) == SaveLoadResult::Ok);
			CHECK(CaptureCandidateLiveState() == installed); CHECK_FALSE(GetLoadCapabilities().has_value());
		}
	}
	const auto restored = CaptureCandidateLiveState();
	CHECK(restored.book == baseline.book); CHECK(restored.packets == baseline.packets); CHECK(restored.accounts == baseline.accounts);
	CHECK(restored.domains == baseline.domains); CHECK(restored.tick == baseline.tick);
	CHECK(restored.random[0] == baseline.random[0]); CHECK(restored.random[1] == baseline.random[1]);
	std::ifstream file(merged, std::ios::binary); REQUIRE(file.good());
	const std::vector<uint8_t> raw{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
	auto bytes = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(raw)); const auto chunks = SupplyWireChunks(bytes);
	const auto &sources = SupplyWireFind(chunks, "TGSD"); REQUIRE(sources.rows.size() > 1);
	// Keep classification and scalar framing valid, but point an active source
	// at an absent native plant. Full afterload reaches this physical refusal only
	// after numerical utility and supply owners have already accepted their rows.
	SupplyWireSet(bytes, sources.rows[1].payload + SupplyWireColumn(bytes, sources, "generator"), 63999, 2);
	const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", MERGED_SAVE_LINEAGE_VERSION);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Error);
	CheckDiscardedLoadCandidate(); CHECK(CaptureCandidateLiveState() == restored);
	REQUIRE(LoadWithFilter(EnvelopeNetworkReader(malformed)) == SaveLoadResult::ReInit);
	CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(GetNextSurfaceSupplyOrderID() == 1);
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureTubePowerVehicles().empty());
	CHECK(CaptureTubeGeothermalPowerState().sources.empty()); CHECK(CaptureGeothermalGenerators().empty());
	CHECK_FALSE(GetLoadCapabilities().has_value());
	REQUIRE(SaveOrLoad(original, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
	const auto recovered = CaptureCandidateLiveState();
	CHECK(recovered.book == baseline.book); CHECK(recovered.packets == baseline.packets); CHECK(recovered.accounts == baseline.accounts);
	CHECK(recovered.domains == baseline.domains); CHECK(recovered.tick == baseline.tick);
	CHECK(recovered.random[0] == baseline.random[0]); CHECK(recovered.random[1] == baseline.random[1]);
	fmt::print("CANDIDATE_LIFECYCLE_NATIVE alternating_full=3 repeated_preview=9 late_refusal_recovered=true camera_rng_cargo_accounts_unchanged=true\n");
}

/** Qualify a retained original through full native reconstruction and independent copied observation. */
TEST_CASE("Native original donor machinery preserves service and dispatch receipts", "[.machinery-lineage-native]")
{
	const char *source = std::getenv("OTTD_MACHINERY_SOURCE"); REQUIRE(source != nullptr);
	const auto hint = OriginalMachineryHint(std::getenv("OTTD_MACHINERY_DIGEST"));
	const char *bootstrap = std::getenv("OTTD_MACHINERY_BOOTSTRAP_INPUT"); REQUIRE(bootstrap != nullptr);
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe actual reconstructed owners before simulation, stopping this invocation even on refusal. */
	std::function<void()> inspect = [&] {
		try {
			// Native CLI has no origin-hint option. An identifiable original
			// bootstraps the engine; this exact source then uses the real scoped
			// full-load API and independent digest, including ambiguous empty files.
			const bool townless = std::getenv("OTTD_MACHINERY_TOWNLESS") != nullptr;
			if (townless) {
				// This retained numerical428 fixture has no town. Normal gameplay
				// must retain its native refusal; editor reconstruction can inspect
				// the unchanged original without manufacturing a replacement town.
				CHECK(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::ReInit);
				REQUIRE(Town::GetNumItems() == 0);
				CHECK(GetSaveLoadErrorMessage().GetDecodedString() == GetEncodedString(STR_ERROR_NO_TOWN_IN_SCENARIO).GetDecodedString());
			}
			AutoRestoreBackup mode(_game_mode, townless ? GameMode::Editor : _game_mode);
			REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
			const auto shipped_refined_material = GetCargoTypeByLabel(CargoLabel{"RFMT"});
			const auto full_result = SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint);
			if (std::getenv("OTTD_MACHINERY_PROTOTYPE433") != nullptr) {
				// This earlier transport prototype used compact unit cargo slots,
				// unlike the shipped package. Preserve its physical refusal; neither
				// a cargo rewrite nor relaxed native stock validation is permitted.
				REQUIRE(full_result == SaveLoadResult::ReInit);
				CHECK(GetSaveLoadErrorMessage().GetDecodedString().find("Invalid native geothermal collector authority") != std::string::npos);
				const auto *collector = Industry::GetIfValid(IndustryID{1}); REQUIRE(collector != nullptr);
				CHECK(static_cast<uint>(shipped_refined_material) == 22);
				CHECK(collector->GetCargoAccepted(CargoType{21}) != collector->accepted.end());
				CHECK(collector->GetCargoAccepted(CargoType{22}) == collector->accepted.end());
				fmt::print("MACHINERY_LINEAGE_NATIVE prototype433_expected_refusal=wrong_original_stock_slots original_preserved=true\n");
				observed = true; _pause_mode.Set(PauseMode::Normal); _exit_game = true; return;
			}
			INFO(GetSaveLoadErrorMessage().GetDecodedString());
			REQUIRE(full_result == SaveLoadResult::Ok);
			const auto plants = CaptureGeothermalGenerators(); const auto utility = CaptureUtilityPowerState();
			const auto sources = CaptureTubeGeothermalPowerState(); const auto vehicles = CaptureTubePowerVehicles();
			const auto plant_digest = ComputeGeothermalGeneratorDigest(), utility_digest = ComputeUtilityPowerDigest();
			REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
			CheckOriginalMachinery(plants, utility, sources, vehicles);
			if (std::getenv("OTTD_MACHINERY_LEGACY_EXTERNAL") != nullptr) {
				REQUIRE_FALSE(vehicles.empty());
				// This original predates provider columns. Every actual capsule
				// must retain external-role sentinels, without an inferred local feed.
				for (const auto &vehicle : vehicles) {
					CHECK(vehicle.geothermal_source.base() == UINT32_MAX); CHECK(vehicle.local_feed.base() == UINT32_MAX);
				}
			}

			CHECK(ComputeGeothermalGeneratorDigest() == plant_digest); CHECK(ComputeUtilityPowerDigest() == utility_digest);
			// Rebuildable negative copies are never original-input authorities.
			// Every malformed whole family must refuse both readers; native reload
			// between attempts proves stale staging cannot supply the next candidate.
			if (const char *bad_dir = std::getenv("OTTD_MACHINERY_BAD_DIR")) {
				for (unsigned fault = 0; fault < 23; ++fault) {
					INFO(fault); const auto bad = (std::filesystem::path(bad_dir) / fmt::format("group43-b434-bad-{}.sav", fault)).string();
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) != SaveLoadResult::Ok);
					REQUIRE(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
				}
			}
			if (const char *output = std::getenv("OTTD_MACHINERY_OUTPUT")) {
				CheckMergedMachinerySuccessor(output);
				if (std::getenv("OTTD_MACHINERY_LIFECYCLE_A") != nullptr) CheckNativeCandidateLifecycle(output, source, hint);
				if (std::getenv("OTTD_MERGED_FIRST_TICK") != nullptr) CheckFirstNativeTick(source, hint, output);
			}
			fmt::print("MACHINERY_LINEAGE_NATIVE plants={} sources={} capsules={} sequence={} original_preserved=true\n", plants.size(), sources.sources.size(), vehicles.size(), utility.operation_sequence);
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(machinery_observer, &inspect); IConsole::CmdRegister("machinery_lineage_test", ObserveOriginalMachinery);
	const auto config_arg = config.string();
	std::array<std::string_view, 17> args{"openttd_machinery_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", bootstrap};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
