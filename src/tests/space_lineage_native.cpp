/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_lineage_native.cpp Original native Space reconstruction and independent copied receipt comparisons.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-3-come-space.md Group42.
 * Default observation performs no simulation work, command or purchase. Optional44qualification writes new test-cache successors; explicit46first-tick mode advances only isolated component copies.
 */
#include "../stdafx.h"
#include "save_first_tick_fixture.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "../space_vehicle.h"
#include "../space_structure.h"
#include "../space_production.h"
#include "../fios.h"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include <charconv>
#include "../safeguards.h"

static std::function<void()> *space_lineage_observer = nullptr; ///< Invocation callback only, never a Space owner or source of elapsed work.

/** Run the post-load observer through the original startup console boundary.
 * @param args Original one-word startup command; no gameplay arguments or actions are accepted.
 * @return Whether the registered observation ran; assertions propagate through the enclosing native test.
 */
static bool ObserveOriginalSpaceLineage(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(space_lineage_observer != nullptr); (*space_lineage_observer)(); return true;
}

/** Count actual native Space hulls without allowing ordinary vehicles to certify a shared Space original.
 * @return Number of real Vehicle pool objects whose original runtime class is Space.
 */
static size_t CountOriginalNativeSpaceCraft()
{
	size_t count = 0;
	// The native pool's existing iterator includes ordinary vehicles too;
	// only actual Space-class objects can establish preservation of this donor craft.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->type == VehicleType::Space) ++count;
	}
	return count;
}

/** Bind an explicitly selected A original to its independent full-file digest.
 * @param digest Optional exact64-character SHA256 from the retained original catalogue, never a filename or wildcard.
 * @return Per-attempt A hint, or no hint for an already identifiable donor original.
 */
static std::optional<SaveOriginHint> OriginalSpaceOriginHint(const char *digest)
{
	if (digest == nullptr) return std::nullopt;
	const std::string_view hex{digest}; REQUIRE(hex.size() == 64);
	SaveOriginHint hint{SaveDialect::DestinationA, {}};
	// Every original digest byte is explicit. The test cannot turn a partial
	// origin declaration into permission to reinterpret an unrelated raw file.
	for (size_t index = 0; index < hint.source_digest.size(); ++index) {
		unsigned value = 0;
		const auto *begin = hex.data() + 2 * index;
		const auto parsed = std::from_chars(begin, begin + 2, value, 16);
		REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == begin + 2);
		hint.source_digest[index] = static_cast<uint8_t>(value);
	}
	return hint;
}

/** Compare original constructed native receipt owners with their independent same-file copied values.
 * @param production Actual native hull receipts captured before copied parsing.
 * @param structures Actual native structure state, work, material and packet references captured before copied parsing.
 * @param history Actual native retirement aggregates, including original processing history.
 */
static void CheckOriginalSpaceReceipts(const std::vector<SpaceProductionRecord> &production, const std::vector<SpaceStructureRecord> &structures,
	const SpaceStructureRetirementHistory &history)
{
	const auto &data = _load_check_data;
	REQUIRE(production.size() == data.space_production_rows.size()); REQUIRE(structures.size() == data.space_structure_rows.size());
	// Exact generation-qualified receipt identities and every saved construction
	// field must agree, including original replacement settlement/sale evidence.
	for (size_t index = 0; index < production.size(); ++index) {
		CHECK(production[index].craft == data.space_production_rows[index].craft);
		CHECK(production[index].state == data.space_production_rows[index].state);
	}
	// Original structure work/material references remain separate from native
	// hull receipts and from spent retirement aggregates; none can replace another.
	for (size_t index = 0; index < structures.size(); ++index) {
		const auto &native = structures[index], &original = data.space_structure_rows[index];
		CHECK(native.ref == original.ref); CHECK(native.state == original.state);
		CHECK(native.work_ticks == original.work_ticks); CHECK(native.work_fraction == original.work_fraction);
		CHECK(native.work_remainder == original.work_remainder); CHECK(native.last_work_tick == original.last_work_tick);
		CHECK(native.last_work_interval == original.last_work_interval); CHECK(native.age_ticks == original.age_ticks);
		CHECK(native.consumed == original.consumed); CHECK(native.quantities == original.quantities); CHECK(native.inputs == original.inputs);
		REQUIRE(native.module.has_value() == original.module.has_value());
		if (!native.module) continue;
		CHECK(native.module->services_completed == original.module->services_completed);
		CHECK(native.module->manufacture_serial == original.module->manufacture_serial);
		CHECK(native.module->quantities == original.module->quantities); CHECK(native.module->stock == original.module->stock);
		const auto native_jobs = native.module->GetServiceJobs(), original_jobs = original.module->GetServiceJobs();
		// Both real service berths retain immutable role and once-only work
		// receipts; a missing old role stays unknown rather than borrowing a hull.
		for (size_t berth = 0; berth < native_jobs.size(); ++berth) {
			const auto &job = *native_jobs[berth], &saved = *original_jobs[berth];
			CHECK(job.model == saved.model); CHECK(job.craft == saved.craft); CHECK(job.remaining_ticks == saved.remaining_ticks);
			CHECK(job.last_work_interval == saved.last_work_interval); CHECK(job.services_at_start == saved.services_at_start);
			CHECK(job.quantities == saved.quantities); CHECK(job.inputs == saved.inputs); CHECK(job.paused == saved.paused);
		}
		REQUIRE(native.module->manufacture.has_value() == original.module->manufacture.has_value());
		if (!native.module->manufacture) continue;
		const auto &job = *native.module->manufacture, &saved = *original.module->manufacture;
		CHECK(job.phase == saved.phase); CHECK(job.model == saved.model); CHECK(job.serial == saved.serial);
		CHECK(job.source == saved.source); CHECK(job.capital_paid == saved.capital_paid); CHECK(job.berth == saved.berth);
		CHECK(job.reservation_interval == saved.reservation_interval); CHECK(job.work_ticks == saved.work_ticks);
		CHECK(job.last_work_interval == saved.last_work_interval); CHECK(job.work_energy_j == saved.work_energy_j);
		CHECK(job.product == saved.product); CHECK(job.consumed == saved.consumed); CHECK(job.replacement == saved.replacement);
		CHECK(job.quantities == saved.quantities); CHECK(job.inputs == saved.inputs);
		REQUIRE(job.clone.has_value() == saved.clone.has_value());
		if (job.clone) {
			CHECK(job.clone->source == saved.clone->source); CHECK(job.clone->name == saved.clone->name); CHECK(job.clone->orders == saved.clone->orders);
		}
	}
	auto original_history = data.space_structure_retirement_history;
	original_history.processing_batches = data.orbital_retired_recipe_batches;
	original_history.processing_discarded_paid_j = data.orbital_retired_discarded_paid_j;
	CHECK(history == original_history);
}

/** Persist original Space receipts through a new explicit merged successor and second save.
 * @param output Invocation-owned test-cache successor; no retained original is written.
 * Full native Space/facility digests and copied exact work/material/job evidence
 * remain unchanged, including old content2craft inside a new content3catalogue.
 */
static void CheckMergedSpaceSuccessor(const char *output)
{
	REQUIRE(output != nullptr);
	const auto production = CaptureSpaceProduction(); const auto structures = CaptureSpaceStructures();
	const auto history = GetSpaceStructureRetirementHistory();
	const auto fleet_digest = ComputeSpaceVehicleDigest(), structure_digest = ComputeSpaceStructureDigest();
	std::vector<std::pair<CompanyID, Money>> wallets;
	for (const auto *company : Company::Iterate()) wallets.emplace_back(company->index, company->money);
	/** @param path Explicit new successor path. Save actual native state without advancing any work or settlement. */
	const auto save = [&](const std::filesystem::path &path) {
		AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	};
	save(output);
	const auto first_check = SaveOrLoad(output, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
	INFO(_load_check_data.error_msg); REQUIRE(first_check == SaveLoadResult::Ok);
	CheckOriginalSpaceReceipts(production, structures, history);
	REQUIRE(SaveOrLoad(output, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CHECK(ComputeSpaceVehicleDigest() == fleet_digest); CHECK(ComputeSpaceStructureDigest() == structure_digest);
	// Original real wallets are independent of hull/module work metadata;
	// reconstructing paid receipts cannot settle another expense or benefit.
	for (const auto &[id, money] : wallets) { REQUIRE(Company::IsValidID(id)); CHECK(Company::Get(id)->money == money); }
	const auto second = std::string{output} + ".again.sav"; save(second);
	REQUIRE(SaveOrLoad(second, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CheckOriginalSpaceReceipts(production, structures, history);
}

/** Qualify one genuine original through actual full loading before its read-only post-load comparison. */
TEST_CASE("Native original Space lineages preserve constructed and retired receipts", "[.space-lineage-native]")
{
	const char *source = std::getenv("OTTD_SPACE_LINEAGE_SOURCE"); REQUIRE(source != nullptr);
	const bool donor = std::getenv("OTTD_SPACE_LINEAGE_DONOR_SPACE") != nullptr;
	const auto hint = OriginalSpaceOriginHint(std::getenv("OTTD_SPACE_LINEAGE_A_DIGEST"));
	const char *bootstrap = std::getenv("OTTD_SPACE_LINEAGE_BOOTSTRAP_INPUT");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe the reconstructed native world before simulation; failures still stop this isolated engine invocation. */
	std::function<void()> inspect = [&] {
		try {
			// The CLI has no origin-hint argument. Start an independently
			// identifiable donor world, then use the real scoped native API for
			// this exact ambiguous A input; classification remains strict.
			if (hint) REQUIRE(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
			const auto production = CaptureSpaceProduction(); const auto structures = CaptureSpaceStructures();
			const auto history = GetSpaceStructureRetirementHistory();
			const auto craft_count = CountOriginalNativeSpaceCraft();
			REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, hint) == SaveLoadResult::Ok);
			CheckOriginalSpaceReceipts(production, structures, history);
			if (donor) {
				// The retained basename mentions405, but its actual unchanged
				// header is B426. Real shared craft/content2 must survive without
				// enabling A's numerically overlapping manufacturing/heavy layouts.
				REQUIRE(craft_count != 0);
				CHECK(_load_check_data.space_content_version == 2);
				for (const auto &row : production) { CHECK(row.state.content_version <= 2); CHECK(row.state.model < 21); CHECK_FALSE(row.state.replacement.has_value()); }
				CHECK(_load_check_data.space_production_present);
			}
			CHECK(CountOriginalNativeSpaceCraft() == craft_count);
			if (const char *output = std::getenv("OTTD_SPACE_LINEAGE_OUTPUT")) {
				CheckMergedSpaceSuccessor(output);
				if (std::getenv("OTTD_MERGED_FIRST_TICK") != nullptr) CheckFirstNativeTick(source, hint, output);
			}
			fmt::print("SPACE_LINEAGE_NATIVE donor={} crafts={} structures={} retired={} source_preserved=true\n", donor, craft_count, structures.size(), history.structures_removed);
			observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(space_lineage_observer, &inspect); IConsole::CmdRegister("space_lineage_test", ObserveOriginalSpaceLineage);
	const auto config_arg = config.string();
	std::array<std::string_view, 17> args{"openttd_space_lineage_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", bootstrap == nullptr ? source : bootstrap};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
