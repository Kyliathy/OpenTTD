/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_transfer_load.cpp Real commissioned portal flow, original-source imports and topology-loss file proof.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C exact parent/segment native transport
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md actual Connected/lifecycle authority
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../utility_power.h"
#include "../utility_power_lineage.h"
#include "../fios.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "../economy_func.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include <cstdlib>
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"

#include "../safeguards.h"

static std::function<void()> *_utility_transfer_observer = nullptr; ///< Isolated native observer; ordinary simulation owns physical topology and saved authority.

/** @param arguments Exact no-argument startup command. @return Whether the actual accepted-world observer ran. */
static bool ObserveUtilityTransfer(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_utility_transfer_observer != nullptr);
	(*_utility_transfer_observer)();
	return true;
}

/** Real portal flow is conserved across imported use, sealing, reconnection and complete native loading. */
TEST_CASE("Native utility transfer preserves original generation through real portal topology and saves", "[.utility-power-transfer-file]")
{
	const auto root = FindTestRepositoryRoot();
	const auto directory = GetTestCaseArtifactPaths("utilities", "native-reload").saves;
	const char *input_value = std::getenv("OTTD_UTILITY_FILE_INPUT");
	const char *case_value = std::getenv("OTTD_UTILITY_FILE_CASE");
	REQUIRE(input_value != nullptr); REQUIRE(case_value != nullptr);
	const auto input = ResolveTestInputPath(input_value);
	const std::string_view mode(case_value);
	REQUIRE(std::filesystem::exists(input));
	bool observed = false;
	std::exception_ptr failure;
	/** Register/meter only after real native acceptance; physical sealing/reconnection uses the existing portal owner. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			/** @param name Fixed test-owned save basename; real writer must preserve every live utility quantity. */
			const auto save = [&](std::string_view name) {
				const auto digest = ComputeUtilityPowerDigest();
				AutoRestoreBackup compression(_savegame_format, std::string("lzma"));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / std::string(name)), false) == SaveLoadResult::Ok);
				CHECK(ComputeUtilityPowerDigest() == digest);
			};
			if (mode == "company-takeover") {
				const auto before = CaptureUtilityPowerState();
				REQUIRE(std::ranges::any_of(before.links, &UtilityPowerLink::active));
				const auto owner = _local_company;
				UtilityCompanyWork work(owner);
				const auto ticket = PrepareUtilityMutation(UtilityMutationKind::Consume, {UtilityConsumerID{0}, before.interval}, 1);
				REQUIRE(ticket.error == UtilityPowerError::None);
				REQUIRE(Company::CanAllocateItem());
				Company *buyer = Company::Create();
				_local_company = buyer->index;
				ChangeOwnershipOfCompanyItems(owner, buyer->index);
				AutoRestoreBackup buyer_actor(_current_company, buyer->index);
				const auto after = CaptureUtilityPowerState();
				CHECK(CommitUtilityMutation(ticket, &work).error == UtilityPowerError::StaleMutation);
				CHECK(after.producers[0].owner == buyer->index);
				CHECK(after.producers[0].generated_lifetime_j == before.producers[0].generated_lifetime_j);
				CHECK(after.producers[0].consumed_lifetime_j == before.producers[0].consumed_lifetime_j);
				// Every physical route retires before tile ownership changes, with
				// exact original flow/use retained and unused destination imports expired.
				for (const auto &link : after.links) CHECK_FALSE(link.active);
				for (const auto &consumer : after.consumers) {
					CHECK(consumer.owner == buyer->index);
					CHECK(consumer.active);
					// Retained imported shares expose zero usable energy; takeover
					// must obtain a fresh physically admitted route for new imports.
					for (const auto &share : consumer.shares) {
						if (share.imported_link.base() == UINT32_MAX) continue;
						CHECK(share.allocated_j == share.consumed_j + share.released_j + share.transferred_j);
					}
				}
				CHECK(ValidateUtilityPowerState(after, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save("utility-company-route-takeover.sav");
			} else if (mode == "lineage-independent") {
				REQUIRE_FALSE(HasUtilityPowerState());
				const char *candidate = std::getenv("OTTD_UTILITY_LINEAGE_PREVIEW"); REQUIRE(candidate != nullptr);
				const auto before = ComputeUtilityPowerDigest(), clock = TimerGameTick::counter;
				const auto random = std::array{_random.state[0], _random.state[1]}; const auto width = Map::SizeX(), height = Map::SizeY();
				REQUIRE(SaveOrLoad(candidate, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.utility_power_state.next_share == 2); REQUIRE(_load_check_data.utility_path_rows.size() == 1);
				CHECK(_load_check_data.utility_power_state.consumers[1].shares.front().parent == UtilityShareID{0});
				CHECK(ComputeUtilityPowerDigest() == before); CHECK(TimerGameTick::counter == clock); CHECK(Map::SizeX() == width); CHECK(Map::SizeY() == height);
				CHECK(std::array{_random.state[0], _random.state[1]} == random);
			} else if (mode == "lineage-preview" || mode == "lineage-refusal") {
				// The accepted source has actual generated/imported quantities.
				// Corrupt copies may neither borrow its paths during preview nor
				// leave partly accepted lineage after a failed native full load.
				const char *bad = std::getenv("OTTD_UTILITY_LINEAGE_CORRUPT"); REQUIRE(bad != nullptr);
				const auto before = ComputeUtilityPowerDigest(), clock = TimerGameTick::counter;
				const auto random = std::array{_random.state[0], _random.state[1]};
				if (mode == "lineage-preview") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == before); CHECK(TimerGameTick::counter == clock); CHECK(std::array{_random.state[0], _random.state[1]} == random);
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.utility_power_state.next_share == 2); CHECK(_load_check_data.utility_path_rows.size() == 1);
				} else {
					REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					CHECK(CaptureUtilityPowerState().next_share == 0); CHECK(CaptureUtilityPowerState().producers.empty());
					REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				}
				CHECK(ComputeUtilityPowerDigest() == before);
			} else if (mode == "create") {
				REQUIRE_FALSE(HasUtilityPowerState());
				TileIndex endpoint = INVALID_TILE;
				// Use the actual retained Surface/Underground access pair; neither
				// a copied fixture map nor a newly painted room supplies eligibility.
				for (const auto &record : CaptureLayerPortalWorkRecords()) {
					if (record.state != LayerPortalWorkState::Connected) continue;
					if (GetTileLayer(record.pair_key) != MapLayer::Surface) continue;
					if (GetTileLayer(GetLayerPortalPairTile(record.pair_key)) != MapLayer::Underground) continue;
					if (record.company != _local_company) continue;
					endpoint = record.pair_key; break;
				}
				REQUIRE(endpoint != INVALID_TILE);
				const auto generator = RegisterUtilityPowerProducer(_local_company, MapLayer::Surface, UtilityGenerationKind::External);
				REQUIRE(generator == UtilityProducerID{0});
				const auto source = RegisterUtilityPowerConsumer(_local_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 100, std::array{*generator});
				const auto destination = RegisterUtilityPowerConsumer(_local_company, MapLayer::Underground, UtilityConsumerEffect::TubeWork, 100, {});
				REQUIRE(source == UtilityConsumerID{0}); REQUIRE(destination == UtilityConsumerID{1});
				const auto forward = RegisterUtilityPowerLink(*source, *destination, endpoint, 50);
				const auto backward = RegisterUtilityPowerLink(*destination, *source, GetLayerPortalPairTile(endpoint), 50);
				REQUIRE(forward == UtilityLinkID{0}); REQUIRE(backward == UtilityLinkID{1});
				CHECK_FALSE(RegisterUtilityPowerLink(*source, *destination, endpoint, 50).has_value());
				REQUIRE(PublishUtilityGeneration(*generator, 1, 3, 1000) == UtilityPowerError::None);
				{
					AutoRestoreBackup system(_current_company, OWNER_DEITY);
					REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*source, 100}, UtilityPowerRequest{*destination, 100}}) == UtilityPowerError::None);
				}
				const auto ticket = PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*source, 1}, 40, *destination, *forward);
				REQUIRE(ticket.error == UtilityPowerError::None);
				const auto before = ComputeUtilityPowerDigest();
				CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*source, 1}, 40, *destination, *forward).seal == ticket.seal);
				CHECK(ComputeUtilityPowerDigest() == before);
				REQUIRE(CommitUtilityMutation(ticket).error == UtilityPowerError::None);
				CHECK(CommitUtilityMutation(ticket).error == UtilityPowerError::StaleMutation);
				CHECK(GetUtilityAllocation(*source)->granted_energy_j == 41);
				CHECK(GetUtilityAllocation(*destination)->granted_energy_j == 40);
				CHECK(CaptureUtilityPowerState().producers.front().meter.generation_energy_j == 81);
				CHECK(CaptureUtilityPowerState().producers.front().reserved_j == 81);
				CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*source, 1}, 11, *destination, *forward).error == UtilityPowerError::InsufficientGrant);
				CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*destination, 1}, 1, *source, *backward).error == UtilityPowerError::InsufficientGrant);
				CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save("utility-transferred.sav");
				UtilityCompanyWork work(_local_company);
				const auto use = PrepareUtilityMutation(UtilityMutationKind::Consume, {*destination, 1}, 10);
				REQUIRE(CommitUtilityMutation(use, &work).error == UtilityPowerError::None);
				save("utility-import-consumed.sav");
				REQUIRE(ChangeLayerPortalConnection(endpoint, _local_company, false, false) == LayerPortalWorkError::None);
				REQUIRE(ChangeLayerPortalConnection(endpoint, _local_company, false, true) == LayerPortalWorkError::None);
				CHECK_FALSE(CaptureUtilityPowerState().links.front().active);
				CHECK(GetUtilityAllocation(*source)->granted_energy_j == 41);
				CHECK(GetUtilityAllocation(*destination)->granted_energy_j == 0);
				CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == 10);
				CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save("utility-transfer-loss.sav");
				REQUIRE(ChangeLayerPortalConnection(endpoint, _local_company, true, true) == LayerPortalWorkError::None);
				CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*source, 1}, 1, *destination, *forward).error == UtilityPowerError::InvalidTopology);
				const auto rebuilt = RegisterUtilityPowerLink(*source, *destination, endpoint, 50);
				REQUIRE(rebuilt == UtilityLinkID{2});
				const auto extra = PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {*source, 1}, 5, *destination, *rebuilt);
				REQUIRE(CommitUtilityMutation(extra).error == UtilityPowerError::None);
				CHECK(GetUtilityAllocation(*source)->granted_energy_j == 36);
				CHECK(GetUtilityAllocation(*destination)->granted_energy_j == 5);
				CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save("utility-transfer-rebuilt.sav");
			} else {
				const auto phase = mode.starts_with("legacy-") ? mode.substr(7) : mode;
				const auto state = CaptureUtilityPowerState();
				REQUIRE(state.producers.size() == 1); REQUIRE(state.consumers.size() == 2);
				REQUIRE(state.links.size() == (phase == "rebuilt" ? 3 : 2));
				CHECK(state.producers.front().meter.generation_energy_j == 81);
				CHECK(state.links.front().flow_j == 40);
				CHECK(state.consumers[1].shares.front().producer == UtilityProducerID{0});
				CHECK(state.consumers[1].shares.front().imported_link == UtilityLinkID{0});
				CHECK(state.next_share == (phase == "rebuilt" ? 3 : 2));
				CHECK(state.consumers[0].shares.front().id == UtilityShareID{0}); CHECK(state.consumers[1].shares.front().parent == UtilityShareID{0});
				CHECK(state.consumers[1].shares.front().segment == std::vector{UtilityLinkID{0}});
				const uint64_t available = phase == "transferred" ? 40 : phase == "consumed" ? 30 : phase == "rebuilt" ? 5 : 0;
				CHECK(GetUtilityAllocation(UtilityConsumerID{1})->granted_energy_j == available);
				CHECK(state.producers.front().consumed_lifetime_j == (phase == "transferred" ? 0 : 10));
				CHECK(PrepareUtilityMutation(UtilityMutationKind::TransferLayer, {UtilityConsumerID{1}, 1}, 1, UtilityConsumerID{0}, UtilityLinkID{1}).error != UtilityPowerError::None);
				CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
				save(fmt::format("utility-transfer-{}-reloaded.sav", mode));
			}
			fmt::print("UTILITY_TRANSFER case={} original_source=true physical_portal=true digest={:016X}\n", mode, ComputeUtilityPowerDigest());
		} catch (...) { failure = std::current_exception(); }
		// Actual takeover changed the local company; after scoped command
		// actors unwind, ordinary input dispatch must use that new local owner.
		if (mode == "company-takeover") _current_company = _local_company;
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_utility_transfer_observer, &inspect);
	IConsole::CmdRegister("utility_transfer_check", ObserveUtilityTransfer);
	const std::string config = (GetTestRuntimeConfig()).string(), input_path = input.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", input_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
