/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_load.cpp Real native file proof for electrical publication, reservations and source-loss retry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 7B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md feature-owned acceptance
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../economy_func.h"
#include "../utility_power.h"
#include "../multilayer_state.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "../core/backup_type.hpp"
#include <filesystem>
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"
#include <cstdlib>
#include <charconv>

#include "../safeguards.h"

static std::function<void()> *_utility_file_observer = nullptr; ///< Isolated actual-game callback; production owns no test observer.

/**
 * Parse exact fixture issuer output through the repository-approved checked integer parser.
 * @param value Original decimal operation sequence or proposal fingerprint.
 * @return Complete unsigned scalar; malformed/overflow/trailing text fails the focused fixture.
 */
static uint64_t ReadUtilityTicketField(std::string_view value)
{
	uint64_t result = 0;
	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
	REQUIRE(parsed.ec == std::errc{});
	REQUIRE(parsed.ptr == value.data() + value.size());
	return result;
}

/**
 * Run the utility fixture only after the ordinary native loader accepts its world.
 * @param arguments Exact registered command, with no gameplay arguments.
 * @return True when the isolated callback has run.
 */
static bool ObserveUtilityFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_utility_file_observer != nullptr);
	(*_utility_file_observer)();
	return true;
}

/** Publication, partial grants and source loss survive real versioned files without renewed energy or private-state repair. */
TEST_CASE("Native utility authority survives actual publication and reservation files", "[.utility-power-file]")
{
	const auto root = std::filesystem::current_path().parent_path();
	const auto directory = root / "src/tests/fixtures/utility";
	const char *case_value = std::getenv("OTTD_UTILITY_FILE_CASE");
	const char *input_value = std::getenv("OTTD_UTILITY_FILE_INPUT");
	REQUIRE(case_value != nullptr);
	REQUIRE(input_value != nullptr);
	const std::string_view mode(case_value);
	const auto input = std::filesystem::path(input_value);
	REQUIRE(std::filesystem::exists(input));
	REQUIRE(std::filesystem::is_directory(directory));
	const auto config = root / "build/native-art.cfg";
	bool observed = false;
	std::exception_ptr failure;
	/** Actual startup commands create/check/save accounts; this observer never runs an alternate simulation or allocator. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			/** @param name Fixed test-owned output basename; ordinary writer must not mutate live account authority. */
			const auto save = [&](std::string_view name) {
				const auto digest = ComputeUtilityPowerDigest();
				const auto money = Company::Get(_local_company)->money;
				AutoRestoreBackup compression(_savegame_format, std::string("lzma"));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / std::string(name)), false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == digest);
				CHECK(Company::Get(_local_company)->money == money);
			};
			if (mode == "create") {
				CHECK_FALSE(HasUtilityPowerState()); // Older accepted native input has no utility authority.
				// Actual registration allocates every source identity; rates/duration
				// then exercise fractional, positive and explicitly zero publication.
				for (uint32_t id = 0; id < 4; ++id) {
					REQUIRE(RegisterUtilityPowerProducer(_local_company, MapLayer::Surface, UtilityGenerationKind::External) == UtilityProducerID{id});
				}
				const std::array links{UtilityProducerID{0}, UtilityProducerID{1}, UtilityProducerID{2}, UtilityProducerID{3}};
				REQUIRE(RegisterUtilityPowerConsumer(_local_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, links) == UtilityConsumerID{0});
				REQUIRE(RegisterUtilityPowerConsumer(_local_company, MapLayer::Surface, UtilityConsumerEffect::TubeWork, 100, links) == UtilityConsumerID{1});
				REQUIRE(PublishUtilityGeneration(UtilityProducerID{0}, 1, 1, 1) == UtilityPowerError::None);
				REQUIRE(PublishUtilityGeneration(UtilityProducerID{1}, 1, 2, 1000) == UtilityPowerError::None);
				REQUIRE(PublishUtilityGeneration(UtilityProducerID{2}, 1, 1, 1000) == UtilityPowerError::None);
				REQUIRE(PublishUtilityGeneration(UtilityProducerID{3}, 1, 0, 1000) == UtilityPowerError::None);
				save("utility-published.sav");
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					const std::array requests{UtilityPowerRequest{UtilityConsumerID{1}, 100}, UtilityPowerRequest{UtilityConsumerID{0}, 100}};
					REQUIRE(AllocateUtilityPower(1, requests) == UtilityPowerError::None);
				}
				save("utility-granted.sav");
				REQUIRE(RemoveUtilityPowerProducer(UtilityProducerID{1}) == UtilityPowerError::None);
				save("utility-lost.sav");
			} else if (mode == "preview") {
				const auto digest = ComputeUtilityPowerDigest();
				const std::array faults{"missing-table", "bad-count", "bad-carry", "bad-reserve", "bad-owner"};
				// Every malformed candidate goes through the real nondestructive
				// file parser while the accepted grant owner remains unchanged.
				for (const auto fault : faults) {
					const auto path = directory / fmt::format("utility-corrupt-{}.sav", fault);
					CHECK(SaveOrLoad(path.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(_load_check_data.HasErrors());
					CHECK(ComputeUtilityPowerDigest() == digest);
				}
				// Clear preview transport and read the valid file again; prior
				// failures must not supply missing rows or poison a good candidate.
				REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK_FALSE(_load_check_data.HasErrors());
				CHECK(_load_check_data.utility_power_state.producers.size() == 4);
				CHECK(ComputeUtilityPowerDigest() == digest);
			} else if (mode == "company-remove") {
				const auto owner = _local_company;
				UtilityCompanyWork work(owner);
				REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {UtilityConsumerID{0}, 1}, 10), &work).error == UtilityPowerError::None);
				const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {UtilityConsumerID{0}, 1}, 1);
				REQUIRE(ticket.error == UtilityPowerError::None);
				REQUIRE(Company::CanAllocateItem());
				Company *survivor = Company::Create();
				// A real successor keeps the loaded application/save context alive;
				// native property cleanup runs before deleting the actual old pool item.
				_local_company = survivor->index;
				ChangeOwnershipOfCompanyItems(owner, INVALID_OWNER);
				delete Company::Get(owner);
				REQUIRE(Company::CanAllocateItem());
				Company *replacement = Company::Create();
				REQUIRE(replacement->index == owner);
				AutoRestoreBackup reused_actor(_current_company, owner);
				CHECK_FALSE(GetUtilityAllocation(UtilityConsumerID{0}).has_value());
				CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
				const auto state = CaptureUtilityPowerState();
				CHECK_FALSE(state.consumers[0].active);
				CHECK(state.producers[1].consumed_lifetime_j == 10);
				CHECK(state.producers[1].reserved_j == 0);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save("utility-company-retired.sav");
			} else if (mode == "company-retired") {
				const auto state = CaptureUtilityPowerState();
				REQUIRE(state.consumers.size() == 2);
				// Historical numeric ownership remains, but saved liveness prevents
				// a replacement company's actual ID from reviving old reservations.
				AutoRestoreBackup reused_actor(_current_company, state.consumers[0].owner);
				REQUIRE(Company::IsValidID(_current_company));
				CHECK_FALSE(state.consumers[0].active);
				CHECK_FALSE(state.consumers[1].active);
				CHECK_FALSE(GetUtilityAllocation(UtilityConsumerID{0}).has_value());
				CHECK(state.producers[1].consumed_lifetime_j == 10);
				CHECK(state.producers[1].reserved_j == 0);
				CHECK(state.consumers[0].shares[0].released_j == 44);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				const auto digest = ComputeUtilityPowerDigest();
				// Each crafted liveness/free-balance fault goes through the real
				// preview parser while the accepted retired authority stays intact.
				for (const auto fault : {"active", "balance"}) {
					const auto corrupt = directory / fmt::format("utility-company-corrupt-{}.sav", fault);
					REQUIRE(std::filesystem::exists(corrupt));
					CHECK(SaveOrLoad(corrupt.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(_load_check_data.HasErrors());
					CHECK(ComputeUtilityPowerDigest() == digest);
				}
				CHECK(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == digest);
				save("utility-company-retired-reloaded.sav");
			} else if (mode == "settle") {
				UtilityCompanyWork work(_local_company);
				const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {UtilityConsumerID{0}, 1}, 60);
				REQUIRE(ticket.error == UtilityPowerError::None);
				fmt::print("UTILITY_TICKET sequence={} seal={}\n", ticket.sequence_before, ticket.seal);
				REQUIRE(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::None);
				CHECK(CaptureUtilityPowerState().producers[1].consumed_lifetime_j == 54);
				CHECK(CaptureUtilityPowerState().producers[2].consumed_lifetime_j == 6);
				save("utility-consumed.sav");
				const auto unused = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {UtilityConsumerID{0}, 1}, 7);
				REQUIRE(CommitUtilityMutation(unused).error == UtilityPowerError::None);
				save("utility-released.sav");
			} else if (mode == "consumed" || mode == "released") {
				// Both saved phases retain the same settled origin quantities;
				// the release phase differs only in seven additional unused joules.
				const auto state = CaptureUtilityPowerState();
				REQUIRE(state.producers.size() == 4);
				CHECK(state.producers[1].consumed_lifetime_j == 54);
				CHECK(state.producers[2].consumed_lifetime_j == 6);
				REQUIRE(GetUtilityAllocation(UtilityConsumerID{0}).has_value());
				CHECK(GetUtilityAllocation(UtilityConsumerID{0})->granted_energy_j == (mode == "consumed" ? 21 : 14));
				const char *sequence = std::getenv("OTTD_UTILITY_TICKET_SEQUENCE");
				const char *seal = std::getenv("OTTD_UTILITY_TICKET_SEAL");
				REQUIRE(sequence != nullptr); REQUIRE(seal != nullptr);
				UtilityMutationProposal replay;
				replay.error = UtilityPowerError::None; replay.kind = UtilityMutationKind::Consume;
				replay.owner = _local_company; replay.grant = {UtilityConsumerID{0}, 1}; replay.quantity_j = 60;
				replay.sequence_before = ReadUtilityTicketField(sequence); replay.seal = ReadUtilityTicketField(seal);
				const auto digest = ComputeUtilityPowerDigest();
				const Money money = Company::Get(_local_company)->money;
				UtilityCompanyWork work(_local_company);
				CHECK(CommitUtilityMutation(replay, &work).error == UtilityPowerError::StaleMutation);
				CHECK(ComputeUtilityPowerDigest() == digest);
				CHECK(Company::Get(_local_company)->money == money);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save(fmt::format("utility-{}-reloaded.sav", mode));
			} else {
				const auto state = CaptureUtilityPowerState();
				REQUIRE(state.interval == (mode == "resumed" ? 2 : 1));
				REQUIRE(state.next_producer == 4);
				REQUIRE(state.next_consumer == 2);
				REQUIRE(state.producers.size() == 4);
				REQUIRE(state.consumers.size() == 2);
				CHECK(state.producers[0].meter.remainder_mj == (mode == "resumed" ? 54 : 27));
				CHECK(state.producers[1].meter.generation_energy_j == (mode == "resumed" ? 0 : 54));
				CHECK(state.producers[2].meter.generation_energy_j == 27);
				CHECK(state.producers[3].meter.generation_energy_j == 0);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_UTILITY_POWER) != 0);
				if (mode == "published") {
					CHECK_FALSE(state.allocated);
					CHECK(PublishUtilityGeneration(UtilityProducerID{0}, 1, 1, 1) == UtilityPowerError::DuplicatePublication);
				} else {
					REQUIRE(state.allocated);
					const uint64_t available = mode == "granted" ? 81 : 27;
					REQUIRE(GetUtilityAllocation(UtilityConsumerID{0}).has_value());
					REQUIRE(GetUtilityAllocation(UtilityConsumerID{1}).has_value());
					CHECK(GetUtilityAllocation(UtilityConsumerID{0})->granted_energy_j == available);
					CHECK(GetUtilityAllocation(UtilityConsumerID{0})->unmet_energy_j == 100 - available);
					CHECK(GetUtilityAllocation(UtilityConsumerID{1})->granted_energy_j == 0);
					CHECK(PublishUtilityGeneration(UtilityProducerID{0}, state.interval, 1, 1) == UtilityPowerError::AlreadyAllocated);
					// Source loss preserves the surviving slice and exact expired
					// quantity; retry cannot resurrect the retired 54-joule origin.
					// Resume starts from the actual saved loss state; both modes
					// must preserve the same retired identity before epoch rotation.
					if (mode == "lost" || mode == "resume") {
						CHECK_FALSE(state.producers[1].active);
						CHECK(state.consumers[0].shares[0].released_j == 54);
						CHECK(RemoveUtilityPowerProducer(UtilityProducerID{1}) == UtilityPowerError::InvalidReference);
					}
				}
				if (mode == "resume") {
					{
						AutoRestoreBackup system(_current_company, OWNER_DEITY);
						REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
					}
					CHECK_FALSE(GetUtilityAllocation(UtilityConsumerID{0}).has_value());
					REQUIRE(PublishUtilityGeneration(UtilityProducerID{0}, 2, 1, 1) == UtilityPowerError::None);
					CHECK(CaptureUtilityPowerState().producers[0].meter.remainder_mj == 54);
					CHECK(PublishUtilityGeneration(UtilityProducerID{1}, 2, 2, 1000) == UtilityPowerError::InvalidReference);
					REQUIRE(PublishUtilityGeneration(UtilityProducerID{2}, 2, 1, 1000) == UtilityPowerError::None);
					{
						AutoRestoreBackup system(_current_company, OWNER_DEITY);
						const std::array requests{UtilityPowerRequest{UtilityConsumerID{0}, 100}};
						REQUIRE(AllocateUtilityPower(2, requests) == UtilityPowerError::None);
					}
					CHECK(GetUtilityAllocation(UtilityConsumerID{0})->granted_energy_j == 27);
					CHECK(CaptureUtilityPowerState().consumers[0].shares.size() == 1);
					save("utility-resumed.sav");
				}
				save(fmt::format("utility-{}-reloaded.sav", mode));
			}
			fmt::print("UTILITY_FILE case={} digest={:016X} actual_native_world=true\n", mode, ComputeUtilityPowerDigest());
		} catch (...) {
			// Release native language/font/application ownership before Catch
			// reports an observer failure, following the existing loaded fixtures.
			failure = std::current_exception();
		}
		// The real removal case changes the local company. Its scoped command
		// actor has now unwound; restore the application's local input authority
		// before Null video completes its final ordinary InputLoop.
		if (mode == "company-remove") _current_company = _local_company;
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(_utility_file_observer, &inspect);
	IConsole::CmdRegister("utility_file_check", ObserveUtilityFile);
	const std::string config_path = config.string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", input_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
