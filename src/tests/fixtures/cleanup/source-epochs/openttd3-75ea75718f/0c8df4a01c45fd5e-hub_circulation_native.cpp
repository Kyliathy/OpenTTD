/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_circulation_native.cpp Actual repeated Goods handling/relink with canonical packet history and no freight income.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group20.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include <map>
#include <set>
#include <tuple>
#include "../safeguards.h"

static std::function<void()> *circulation_observer = nullptr; ///< Observer only; native inventories, commands and company ledgers own every circulation leg.

/** @param args Parameter-free loaded-world callback. @return Whether the installed native observer ran. */
static bool ObserveCirculation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(circulation_observer != nullptr); (*circulation_observer)(); return true;
}

/** @param name Actual saved cosmetic station name. @return Its unique normal native inventory owner. */
static Station *CirculationMember(std::string_view name)
{
	Station *result = nullptr;
	for (auto *station : Station::Iterate()) if (station->name == name) { REQUIRE(result == nullptr); result = station; } // Resolve only the actual Group19 boundary stations, without another station or packet registry.
	REQUIRE(result != nullptr); return result;
}

/** Native packet history key: recorded age, original source/type/tile/first station and routing hop. Saved CAPA travel vectors are checked independently; GetDistance requires in-vehicle custody. */
using CirculationHistory = std::tuple<uint16_t, uint32_t, SourceType, TileIndex, StationID, StationID>;
/** Weighted units/feeder-share by immutable per-unit history; this is a read-only fixture census, never a gameplay stock owner. */
using CirculationCensus = std::map<CirculationHistory, std::pair<uint, Money>>;

/** @param cargo Native physical station/job list being observed. @param seen Pool IDs already encountered across actual owners, used only to reject duplicate custody. @param result Aggregate physical units and feeder share by their retained provenance/history. */
static void ObserveCirculationCargo(const StationCargoList &cargo, std::set<CargoPacketID> &seen, CirculationCensus &result)
{
	// Neutral handling can split/merge packets while moving its eight-unit throughput. Aggregate by actual history and check each physical packet appears in exactly one list, rather than pretending allocation IDs must stay unchanged.
	for (const auto &[hop, packets] : *cargo.Packets()) for (const auto *packet : packets) {
		REQUIRE(seen.insert(packet->index).second); const auto source = packet->GetSource();
		const CirculationHistory key{packet->GetPeriodsInTransit(), source.id, source.type, packet->GetSourceTile(), packet->GetFirstStation(), hop};
		auto &totals = result[key]; totals.first += packet->Count(); totals.second += packet->GetFeederShare();
	}
}

/** Repeatedly circulate the original actually recovered Goods through native handling and completed relinks; no new cargo, vehicle or payment authority exists. */
TEST_CASE("Native hub circulation preserves physical Goods history and charges fees without final income", "[.hub-circulation]")
{
	const char *input = std::getenv("OTTD_HUB_CIRCULATION_SOURCE"); REQUIRE(input != nullptr); const auto paths = GetTestCaseArtifactPaths("space", "hub-circulation"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect only the ordinary loaded Group19 paid recovery world, preserving assertion failure through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running(_pause_mode, PauseModes{}); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			auto *rail = CirculationMember("Space boundary rail"), *lorry = CirculationMember("Space boundary lorry"); const auto first = GetLaunchHubMembership(rail->index); REQUIRE(first); const auto home = first->center;
			auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); REQUIRE(GetLaunchHubMembership(lorry->index)->center == home); REQUIRE_FALSE(HasSurfaceHandlingForStation(center->index));
			const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(goods)); REQUIRE(rail->goods[goods].TotalCount() == 20); REQUIRE(lorry->goods[goods].TotalCount() == 20); REQUIRE(center->goods[goods].TotalCount() == 0);
			auto *company = Company::Get(center->owner); const auto income = company->cur_economy.income, expenses = company->cur_economy.expenses, cash = company->money; const auto deliveries = company->cur_economy.delivered_cargo; const auto fleet = GetSpaceVehicleCount();
			const auto handling_expenses = company->yearly_expenses[0][ExpensesType::Other];
			/** @param edge Optional saved boundary whose observed actual packet IDs are emitted for public CAPA transport checks. @return A read-only census of all actual Goods station/handling owners, including historical Goods. */
			auto census = [&](std::string_view edge = {}) {
				CirculationCensus result; std::set<CargoPacketID> seen;
				// Observe every native station so a purported neutral cycle cannot silently duplicate or hide Goods in another inventory.
				for (const auto *station : Station::Iterate()) if (station->goods[goods].HasData()) ObserveCirculationCargo(station->goods[goods].GetData().cargo, seen, result);
				for (const auto ref : GetLaunchCenterReferences()) for (const auto &job : ResolveLaunchCenter(ref)->launch_hub->handling) if (job->cargo == goods) ObserveCirculationCargo(job->packets, seen, result); // Assigned packets remain sole native owners while station storage is empty.
				if (!edge.empty()) for (const auto packet : seen) fmt::print("HUB_CARGO edge={} packet={}\n", edge, packet.base()); // Only boundary observation IDs select the saved records; they confer no gameplay ownership or mutation permission.
				return result;
			};
			const auto history = census(); REQUIRE(!history.empty());
			/** @param suffix Exact before/after circulation boundary, written by the native owner without field normalization. */
			auto save = [&](std::string_view suffix) {
				CHECK(census(suffix) == history);
				REQUIRE(ValidateSpaceVehicles()); REQUIRE(ValidateSurfaceHandling()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("hub-circulation-{}-423.sav", suffix)), false) == SaveLoadResult::Ok);
			};
			/** @param member Actual selected same-company boundary member. @param reverse Member-to-center supply handling, otherwise center-to-member collection. */
			auto leg = [&](Station &member, bool reverse) {
				const auto link = GetLaunchHubMembership(member.index); REQUIRE(link); const auto money = company->money;
				REQUIRE(Command<Commands::StartSurfaceHandling>::Post(member.index, link->member.generation, home.station, home.generation, goods, uint16_t{20}, reverse)); CHECK(company->money == money); CHECK(census() == history);
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60); uint ticks = 0;
				// Every real throughput opportunity can split ownership between the job and receiver. Observe the full census after each actual tick; animation/stalled calls cannot transfer or age cargo.
				while (HasSurfaceHandlingForStation(member.index)) {
					REQUIRE(ticks < 20); REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; } REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++ticks; CHECK(census() == history);
					CHECK(company->cur_economy.income == income); CHECK(company->cur_economy.delivered_cargo == deliveries);
				}
			};
			save("before"); uint64_t previous_generation = 0;
			// Alternating both real members tests thirty-two completed inbound/outbound cycles. Short transfers preserve recorded transit periods; ordinary station waiting itself does not age native cargo, and no cycle may reset its prior periods or provenance.
			for (uint cycle = 0; cycle < 32; ++cycle) {
				auto *member = cycle % 2 == 0 ? rail : lorry; leg(*member, true); REQUIRE(center->goods[goods].TotalCount() == 20); REQUIRE(member->goods[goods].TotalCount() == 0); leg(*member, false);
				REQUIRE(member->goods[goods].TotalCount() == 20); REQUIRE(center->goods[goods].TotalCount() == 0); const auto old = GetLaunchHubMembership(member->index); REQUIRE(old);
				REQUIRE(Command<Commands::SetLaunchHub>::Post(member->index, StationID::Invalid(), uint64_t{0})); CHECK_FALSE(GetLaunchHubMembership(member->index)); CHECK(census() == history);
				REQUIRE(Command<Commands::SetLaunchHub>::Post(member->index, home.station, home.generation)); const auto current = GetLaunchHubMembership(member->index); REQUIRE(current); CHECK(current->member.generation > old->member.generation); CHECK(current->member.generation > previous_generation); previous_generation = current->member.generation; CHECK(census() == history);
			}
			// Handling debits the canonical yearly Other category; the economy summary intentionally includes only vehicle running/property/interest expenses. Published fees may also coexist with native Space upkeep in Other.
			const Money minimum_fees = Money{64} * 20 * CargoSpec::Get(goods)->weight / 16; CHECK(company->money <= cash - minimum_fees); CHECK(company->yearly_expenses[0][ExpensesType::Other] >= handling_expenses + minimum_fees); CHECK(company->cur_economy.expenses <= expenses);
			CHECK(company->cur_economy.income == income); CHECK(company->cur_economy.delivered_cargo == deliveries); CHECK(GetSpaceVehicleCount() == fleet); save("after");
			fmt::print("HUB_CIRCULATION native=true cycles=32 legs=64 physical_goods=40 all_station_history=true canonical_age_preserved=true no_income=true actual_fees=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(circulation_observer, &observe); IConsole::CmdRegister("hub_circulation_test", ObserveCirculation);
	const std::string source{input}, configuration = config.string(); std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
