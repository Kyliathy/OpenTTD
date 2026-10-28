/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_lineage_native.cpp Original native Space reconstruction and independent copied receipt comparisons.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-3-come-space.md Group42.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 21; late native failures after field install, packet upgrade and queue install.
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
#include "save_candidate_lifecycle_fixture.h"
#include "save_envelope_fixture.h"
#include "save_envelope_chunks.h"
#include "save_supply_wire_fixture.h"
#include "../asteroid.h"
#include "../space_orders.h"
#include "../saveload/asteroid_field_sl.h"
#include <charconv>
#include <fstream>
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

/** One malformed copy of the loaded source that the full loader must refuse at a known stage. */
struct SpaceLateFailure {
	std::string_view point; ///< The install step the refusal comes after.
	std::string_view reason; ///< Expected native refusal text.
	bool preview_refuses; ///< Whether this source's copied preview also judges the changed fact.
	std::vector<uint8_t> bytes; ///< Complete uncompressed stream at the original version.
};

/** @param payload Original ASTR payload. @return The one layout that decodes it and re-encodes it byte for byte, or no value. */
static std::optional<AsteroidFieldWireEra> FindAsteroidFieldEra(std::span<const uint8_t> payload)
{
	// The source's own era is the one whose codec reproduces its exact bytes.
	for (const auto era : {AsteroidFieldWireEra::Preview, AsteroidFieldWireEra::NativeMining, AsteroidFieldWireEra::StarterInputs,
			AsteroidFieldWireEra::Lifecycle, AsteroidFieldWireEra::SurveyDeparture}) {
		const auto field = DecodeAsteroidField(payload, era); if (!field) continue;
		const auto encoded = EncodeAsteroidField(*field, era);
		if (encoded && std::ranges::equal(*encoded, payload)) return era;
	}
	return std::nullopt;
}

/** @param row One sparse-table row. @param bytes Its stream. @return The row's native pool index. */
static uint32_t SparseRowIndex(const SupplyWireRow &row, std::span<const uint8_t> bytes)
{
	size_t cursor = row.payload;
	/** @return The next index byte, bounded by the row. */
	const auto read = [&]() { REQUIRE(cursor < row.end); return bytes[cursor++]; };
	return static_cast<uint32_t>(ReadSaveGamma(read));
}

/** Refuse three malformed copies of the loaded source late in a full load, then recover with the source (m2o28 Group 21).
 * Each copy fails after one more install step has run: the field, the legacy packet upgrade, the queues. Each refusal
 * must be a ReInit that leaves no field, order issuer or load lineage behind, and the unchanged source must load again
 * with its exact fleet, orders, structures, field, packets, accounts and RNG.
 * @param source The loaded genuine original; it must be identifiable without a digest hint, because the copies cannot carry one.
 */
static void CheckSpaceLateFailures(const char *source)
{
	const auto baseline = CaptureCandidateLiveState(); const auto field = GetAsteroidField();
	const auto fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest(), structures = ComputeSpaceStructureDigest();
	std::ifstream file(source, std::ios::binary); REQUIRE(file.good());
	const std::vector<uint8_t> raw{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}}; REQUIRE(raw.size() > 8);
	const uint16_t version = static_cast<uint16_t>(raw[4] << 8 | raw[5]);
	const auto original = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(raw)); const auto chunks = SupplyWireChunks(original);
	std::vector<SpaceLateFailure> cases;
	{
		// After the field install: one natural packet names a body this field never
		// issued, so the legacy packet upgrade cannot resolve its mineral.
		auto bytes = original; const auto &capa = SupplyWireFind(chunks, "CAPA");
		const auto type = SupplyWireColumn(bytes, capa, "source_type"), id = SupplyWireColumn(bytes, capa, "source_id");
		REQUIRE(field.next_body_id < MAX_ORBITAL_BODIES); bool changed = false;
		for (size_t index = 1; index < capa.rows.size() && !changed; ++index) {
			const auto &row = capa.rows[index];
			if (row.end == row.payload || bytes[row.payload + type] != to_underlying(SourceType::Asteroid)) continue;
			SupplyWireSet(bytes, row.payload + id, field.next_body_id, 2); changed = true;
		}
		REQUIRE(changed);
		cases.push_back({"field install", "Invalid legacy asteroid cargo origin", true, std::move(bytes)});
	}
	{
		// After the packet upgrade: one idle, unleased, empty hull disappears from
		// VEHS, so its saved queue has no native owner when the queues install.
		const SpaceVehicle *idle = nullptr;
		for (const auto *craft : SpaceVehicle::Iterate()) {
			/** @param body Field record. @return Whether it leases this hull. */
			const bool leased = std::ranges::any_of(field.bodies, [&](const AsteroidBody &body) { return body.mining_slot == craft->index; });
			if (craft->cargo.TotalCount() == 0 && !leased && craft->mission.phase != SpaceMissionPhase::Mining) { idle = craft; break; }
		}
		REQUIRE(idle != nullptr);
		const auto &vehicles = SupplyWireFind(chunks, "VEHS");
		const auto row = std::ranges::find_if(vehicles.rows.begin() + 1, vehicles.rows.end(),
			/** @param row Sparse VEHS row. @return Whether it holds the chosen hull. */
			[&](const SupplyWireRow &row) { return row.end != row.payload && SparseRowIndex(row, original) == idle->index.base(); });
		REQUIRE(row != vehicles.rows.end());
		auto bytes = original; bytes.erase(bytes.begin() + row->prefix, bytes.begin() + row->end);
		cases.push_back({"packet upgrade", "Invalid native Space order ownership", true, std::move(bytes)});
	}
	{
		// After the queue install: an unleased body gains a lease held by a hull that
		// is not mining. The field itself accepts any in-range hull; only the final
		// fleet and custody join checks the lease against its miner.
		const auto &astr = SupplyWireFind(chunks, "ASTR");
		const std::span<const uint8_t> payload(original.begin() + astr.payload, original.begin() + astr.end);
		const auto era = FindAsteroidFieldEra(payload); REQUIRE(era.has_value());
		auto changed = *DecodeAsteroidField(payload, *era);
		const SpaceVehicle *holder = nullptr;
		for (const auto *craft : SpaceVehicle::Iterate()) if (craft->mission.phase != SpaceMissionPhase::Mining) { holder = craft; break; }
		const auto free = std::ranges::find(changed.bodies, VehicleID::Invalid(), &AsteroidBody::mining_slot);
		REQUIRE(holder != nullptr); REQUIRE(free != changed.bodies.end()); free->mining_slot = holder->index;
		const auto encoded = EncodeAsteroidField(changed, *era); REQUIRE(encoded.has_value());
		const auto chunk = EnvelopeRiffChunk("ASTR", *encoded);
		auto bytes = original; bytes.erase(bytes.begin() + astr.begin, bytes.begin() + astr.end);
		bytes.insert(bytes.begin() + astr.begin, chunk.begin(), chunk.end());
		cases.push_back({"queue install", "Invalid native Space fleet", false, std::move(bytes)});
	}
	for (auto &failure : cases) {
		INFO(failure.point);
		const auto malformed = EnvelopeStream(std::move(failure.bytes), "OTTN", version);
		const auto preview = LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true);
		if (failure.preview_refuses) { CHECK(preview == SaveLoadResult::Error); CheckDiscardedLoadCandidate(); }
		REQUIRE(LoadWithFilter(EnvelopeNetworkReader(malformed)) == SaveLoadResult::ReInit);
		const auto reason = GetSaveLoadErrorMessage().GetDecodedString(); INFO(reason);
		CHECK(reason.find(failure.reason) != std::string::npos);
		// The null pass revoked the staged and installed Space state; nothing is lent to the next load.
		CHECK(GetAsteroidField() == AsteroidField{}); CHECK(GetNextSpaceVehicleGeneration() == 1); CHECK_FALSE(GetLoadCapabilities().has_value());
		REQUIRE(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CHECK(GetAsteroidField() == field); CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceOrdersDigest() == orders);
		CHECK(ComputeSpaceStructureDigest() == structures);
		const auto recovered = CaptureCandidateLiveState();
		CHECK(recovered.packets == baseline.packets); CHECK(recovered.accounts == baseline.accounts); CHECK(recovered.domains == baseline.domains);
		CHECK(recovered.tick == baseline.tick); CHECK(recovered.random[0] == baseline.random[0]); CHECK(recovered.random[1] == baseline.random[1]);
		fmt::print("SPACE_LATE_FAILURE point=\"{}\" preview_refused={} reason=\"{}\"\n", failure.point, preview == SaveLoadResult::Error, reason);
	}
	fmt::print("SPACE_LATE_FAILURES cases={}\n", cases.size()); // The Catch result, not this line, says whether every check held.
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
			if (std::getenv("OTTD_SPACE_LINEAGE_LATE_FAILURES") != nullptr) { REQUIRE_FALSE(hint.has_value()); CheckSpaceLateFailures(source); }
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
