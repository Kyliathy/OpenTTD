/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_path_load.cpp Actual paid multi-hop state on the retained Deep4 native world, with independent copied preview and full reload.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5.4 feature export
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md native artifact routing
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "utility_power_fixture.h"
#include "../utility_power_transfer.h"
#include "../geothermal_electrical.h"
#include "../geothermal_riser.h"
#include "../geothermal_heat.h"
#include "../geothermal_utility_cmd.h"
#include "../excavation_state.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../command_func.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include <charconv>
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *_paid_path_observer = nullptr; ///< Actual accepted-world test callback, never another state owner.

/** @param arguments Exact no-argument startup command. @return Whether the accepted native observer ran. */
static bool ObservePaidPath(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_paid_path_observer != nullptr); (*_paid_path_observer)(); return true;
}

/** Real company expense consumes an explicitly supplied independent thermal offer; it proves preservation of Heat, not an operating collector or plant. */
class PaidPathHeatWork : public UtilityCompanyWork {
public:
	/** @param owner Actual native company paying the complete five-unit work expense. */
	explicit PaidPathHeatWork(CompanyID owner) : UtilityCompanyWork(owner) {}
	/** @return The actual benefit's independent thermal domain. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return Direct-work role; no electrical conversion event is claimed. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return UtilityHeatUse::DirectRecipe; }
};

/** @param tile Actual native world cell. @param remove Retire its installed electrical cell instead. @return Whole ordinary company capital/labour settlement. */
static CommandCost PaidPathCable(TileIndex tile, bool remove = false)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Run the normal construction kernel while the isolated observer owns the thread, then preserve the source world's paused state.
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, UtilityEnergyDomain::Electricity));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, UtilityEnergyDomain::Electricity));
}

/**
 * Supply explicitly declared fixture floor in the accepted Deep4 world without replacing its map, jobs, deposits or infrastructure.
 * @param floor Exact additional rock cells for this isolated paid-machinery persistence boundary; paid excavation/driven bootstrap remain master10.6.
 */
static void SupplyPaidPathFloor(std::span<const TileIndex> floor)
{
	auto snapshot = CaptureGeologyState(); GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	// This is declared test input through the actual complete geology restore
	// boundary. Native commands still own all portals, machinery and cable;
	// no normal clearing permission is treated as excavation or freight proof.
	for (const auto tile : floor) REQUIRE(candidate.terrain.Set(tile, ExcavationState::Excavated));
	snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
	StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
}

/** @param site Actual rock-side paid machinery geometry on an already command-connected pair. @return The real fully funded/native-stock-complete industry identity. */
static IndustryID BuildPaidPathRiser(const GeothermalRiserRequest &site)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Funding and native labour use actual gameplay eligibility without advancing the retained world's unrelated simulation.
	REQUIRE(Command<Commands::BuildGeothermalRiser>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, site.tile,
		std::make_tuple(site.tile, site.portal, site.port, site.access, uint32_t{123})).Succeeded());
	const auto id = FindGeothermalRiserForPortal(site.portal); REQUIRE(Industry::IsValidID(id)); auto &industry = *Industry::Get(id);
	// Actual stock receipt and actual synchronized work own construction.
	// Explicit boundary materials do not claim procurement or carrier delivery.
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
		REQUIRE(ReceiveGeothermalRiserCargo(industry, GetCargoTypeByLabel(input.label), input.quantity) == input.quantity);
	}
	for (uint16_t tick = 0; tick < 296; ++tick) { ++TimerGameTick::counter; AdvanceGeothermalRiser(industry); } // Dispatch only this paid native work owner; retained Deep4 trains/jobs do not advance during fixture setup.
	REQUIRE(IsGeothermalRiserOperational(id)); return id;
}

/** @param from Existing source request. @param to Existing final destination request. @param port Explicit native sending/arriving installed cell. @param quantity Positive exact whole action. @return Actual complete ordered paid route, holding references only. */
static UtilityPathTransferRequest PaidPathRequest(UtilityConsumerID from, UtilityConsumerID to, TileIndex port, uint64_t quantity)
{
	const auto path = FindGeothermalElectricalPath(from, to, port, 74); REQUIRE(path.has_value());
	UtilityPathTransferRequest request{{from, GetUtilityPowerInterval()}, to, port, quantity};
	for (const auto &leg : path->legs) request.legs.push_back({leg.riser, leg.inlet.cells}); // Preserve each actual original paid edge and full inlet; quoted watts/joules never become source publication.
	return request;
}

/** @return Whether this explicit native invocation verifies the geothermal caller; the shared compatibility producer remains separately selectable. */
static bool UseGeothermalFeatureExport()
{
	const auto *caller = std::getenv("OTTD_GEOTHERMAL_EXPORT");
	return caller != nullptr && std::string_view{caller} == "1"; // Only an explicit feature invocation changes the caller; saved accounting has no feature-mode field.
}

/** @param from Existing source. @param to Existing final requested destination. @param port Actual installed sending/arriving housing. @param quantity Exact whole quantity. @return Pure prepared action through the explicitly selected public caller. */
static UtilityPathTransferProposal PreparePaidPathAction(UtilityConsumerID from, UtilityConsumerID to, TileIndex port, uint64_t quantity)
{
	if (UseGeothermalFeatureExport()) return PrepareGeothermalPowerExport({{from, GetUtilityPowerInterval()}, to, port, quantity});
	return PrepareUtilityPathTransfer(PaidPathRequest(from, to, port, quantity));
}

/** @param proposal Exact action prepared before any endpoint change. @return One shared receipt/refusal through the explicitly selected caller, including the feature's own replay entry point. */
static UtilityMutationReceipt CommitPaidPathAction(const UtilityPathTransferProposal &proposal)
{
	if (UseGeothermalFeatureExport()) return ExportGeothermalPower(proposal);
	return CommitUtilityPathTransfer(proposal);
}

/** @param from Existing source. @param to Existing final requested destination. @param port Actual installed sending/arriving housing. @param quantity Exact whole quantity. Settle every paid crossing once through the sole shared owner, proving which public caller actually ran. */
static void TransferPaidPath(UtilityConsumerID from, UtilityConsumerID to, TileIndex port, uint64_t quantity)
{
	const auto proposal = PreparePaidPathAction(from, to, port, quantity); REQUIRE(proposal.error == UtilityPowerError::None);
	CHECK(proposal.request.quantity_j == quantity); CHECK(proposal.request == PaidPathRequest(from, to, port, quantity));
	REQUIRE(CommitPaidPathAction(proposal).error == UtilityPowerError::None); CHECK(CommitPaidPathAction(proposal).error == UtilityPowerError::StaleMutation);
	if (UseGeothermalFeatureExport()) fmt::print("GEOTHERMAL_EXPORT from={} to={} quantity={} replay=refused\n", from.base(), to.base(), quantity);
}

/** @return Exact installed Deep input cell in the accepted native fixture extension. */
static TileIndex PaidPathSourcePort() { return TileXYL(51, 9, MapLayer::DeepUnderground); }
/** @return Actual Underground arriving housing, separately command-built in the accepted native fixture extension. */
static TileIndex PaidPathRelayPort() { return TileXYL(50, 10, MapLayer::Underground); }
/** @return Native intervening cell chosen for original-loss/fresh-repair proof. */
static TileIndex PaidPathSplitCell() { return TileXYL(55, 10, MapLayer::Underground); }

/** @param directory Explicit native artifact home. @param label Exact state checkpoint. Write accepted live authority and publish its independent digest for reload checks. */
static void SavePaidPath(const std::filesystem::path &directory, std::string_view label)
{
	REQUIRE(ValidateGeothermalRisers(CaptureGeothermalRisers())); REQUIRE(ValidateGeothermalUtilityLinks());
	REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	const auto utility = ComputeUtilityPowerDigest(), heat = ComputeGeothermalHeatRouteDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()), risers = ComputeGeothermalRiserDigest();
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(directory / fmt::format("paid-path-{}.sav", label)), false) == SaveLoadResult::Ok);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalHeatRouteDigest() == heat); CHECK(ComputeGeothermalRiserDigest() == risers);
	fmt::print("PAID_PATH_SAVE case={} utility={:016X} heat={:016X} geology={:016X} risers={:016X}\n", label, utility, heat, geology, risers);
}

/** @param directory Actual selected output home. Extend the accepted retained Deep4 world using actual paid command owners and explicit boundary floor/material/electrical/thermal inputs. */
static void WritePaidPathCases(const std::filesystem::path &directory)
{
	REQUIRE_FALSE(HasUtilityPowerState()); REQUIRE(CaptureGeothermalRisers().empty()); REQUIRE(Map::SizeX() == 64); REQUIRE(Map::SizeY() == 64);
	const auto owner = _current_company;
	std::vector<TileIndex> floor;
	// Supply only the declared additional machinery/approach/relay floor.
	// Existing Deep4 paid loop, portal work and all geological resource records
	// remain in this accepted world; no fresh test map replaces the input.
	for (const auto layer : {MapLayer::DeepUnderground, MapLayer::Underground}) {
		for (uint y = 8; y <= 14; ++y) {
			for (uint x = 48; x <= (layer == MapLayer::DeepUnderground ? 54U : 62U); ++x) floor.push_back(TileXYL(x, y, layer));
		}
	}
	SupplyPaidPathFloor(floor);
	const std::array<GeothermalRiserRequest, 2> sites = {{{owner, TileXYL(50, 10, MapLayer::DeepUnderground), TileXYL(51, 10, MapLayer::DeepUnderground), TileXYL(51, 10, MapLayer::DeepUnderground), DiagDirection::SE},
		{owner, TileXYL(58, 10, MapLayer::Underground), TileXYL(59, 10, MapLayer::Underground), TileXYL(59, 10, MapLayer::Underground), DiagDirection::SE}}};
	std::array<IndustryID, 2> machines;
	// Each actual pair/approach is constructed and capital is settled through
	// normal command kernels before its complete native machinery is funded.
	for (size_t index = 0; index < sites.size(); ++index) {
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Preserve the accepted Deep4 pause outside each actual normal command batch.
		const auto &site = sites[index]; const auto upper = MultiLayerMap::TileInLayer(site.portal, index == 0 ? MapLayer::Underground : MapLayer::Surface);
		const auto portal = Command<Commands::BuildLayerPortal>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, upper,
			std::make_tuple(upper, TransportType::Rail, GetTileLayer(site.portal), DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE));
		CAPTURE(index, portal.GetErrorMessage().base(), upper.base()); REQUIRE(portal.Succeeded());
		for (const auto endpoint : {site.portal, upper}) {
			const auto door = GetLayerPortalDoorTile(endpoint);
			REQUIRE(Command<Commands::BuildRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, door,
				std::make_tuple(door, RAILTYPE_RAIL, AxisToTrack(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))), false)).Succeeded());
		}
		REQUIRE(ChangeLayerPortalConnection(site.portal, owner, true, true) == LayerPortalWorkError::None); machines[index] = BuildPaidPathRiser(site);
	}
	REQUIRE(PaidPathCable(PaidPathSourcePort()).Succeeded()); REQUIRE(PaidPathCable(sites[0].port).Succeeded());
	REQUIRE(PaidPathCable(TileXYL(50, 9, MapLayer::DeepUnderground)).Succeeded()); // A real separately installed alternative remains unused by the shortest valid inlet, enabling exact declared-port malformed-file checks.
	for (uint x = 50; x <= 59; ++x) {
		const auto cable = PaidPathCable(TileXYL(x, 10, MapLayer::Underground)); CAPTURE(x, cable.GetErrorMessage().base()); REQUIRE(cable.Succeeded());
	} // Every actual arriving-housing-to-inlet cell must be installed and paid separately without crossing generic machinery cells.
	const auto electricity = RegisterUtilityPowerProducer(owner, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(electricity == UtilityProducerID{0});
	const auto source = RegisterUtilityPowerConsumer(owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000000, std::array{*electricity});
	const auto relay = RegisterUtilityPowerConsumer(owner, MapLayer::Underground, UtilityConsumerEffect::AssemblyWork, 1000000, {});
	const auto target = RegisterUtilityPowerConsumer(owner, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 1000000, {});
	REQUIRE(source == UtilityConsumerID{0}); REQUIRE(relay == UtilityConsumerID{1}); REQUIRE(target == UtilityConsumerID{2});
	REQUIRE(RegisterGeothermalRiserLink(machines[0], *source, *relay).has_value()); REQUIRE(RegisterGeothermalRiserLink(machines[1], *relay, *target).has_value());
	const auto thermal = RegisterUtilityHeatProducer(owner); REQUIRE(thermal == UtilityProducerID{1});
	const auto heat_consumer = RegisterUtilityHeatConsumer(owner, UtilityHeatUse::DirectRecipe, 100000, std::array{*thermal}); REQUIRE(heat_consumer == UtilityConsumerID{3});
	REQUIRE(PublishUtilityGeneration(*electricity, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
	REQUIRE(PublishUtilityHeatGeneration(*thermal, GetUtilityPowerInterval(), 100000, 74) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*source, 500000}, UtilityPowerRequest{*relay, 200000}, UtilityPowerRequest{*target, 600000}}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*heat_consumer, 50000}}) == UtilityPowerError::None);
	}
	PaidPathHeatWork heat_work(owner); REQUIRE(CommitUtilityMutation(PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {*heat_consumer, GetUtilityPowerInterval()}, 10000), &heat_work).error == UtilityPowerError::None);
	SavePaidPath(directory, "root");
	TransferPaidPath(*source, *relay, PaidPathSourcePort(), 100000); SavePaidPath(directory, "first");
	TransferPaidPath(*relay, *target, PaidPathRelayPort(), 60000); SavePaidPath(directory, "staged");
	TransferPaidPath(*source, *target, PaidPathSourcePort(), 40000); SavePaidPath(directory, "branch");
	UtilityCompanyWork work(owner); REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {*target, GetUtilityPowerInterval()}, 20000), &work).error == UtilityPowerError::None);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {*target, GetUtilityPowerInterval()}, 10000)).error == UtilityPowerError::None);
	SavePaidPath(directory, "used");
	const auto stale = PreparePaidPathAction(*source, *target, PaidPathSourcePort(), 1); REQUIRE(stale.error == UtilityPowerError::None);
	REQUIRE(PaidPathCable(PaidPathSplitCell(), true).Succeeded()); CHECK(CommitPaidPathAction(stale).error == UtilityPowerError::StaleMutation); SavePaidPath(directory, "split");
	REQUIRE(PaidPathCable(PaidPathSplitCell()).Succeeded()); SavePaidPath(directory, "repaired");
	TransferPaidPath(*source, *target, PaidPathSourcePort(), 50000); SavePaidPath(directory, "retry");
	REQUIRE(ChangeLayerPortalConnection(sites[0].portal, owner, false, true) == LayerPortalWorkError::None); SavePaidPath(directory, "upstream-loss");
	REQUIRE(ChangeLayerPortalConnection(sites[0].portal, owner, true, true) == LayerPortalWorkError::None);
	REQUIRE(RegisterGeothermalRiserLink(machines[0], *source, *relay).has_value()); TransferPaidPath(*source, *target, PaidPathSourcePort(), 10000); SavePaidPath(directory, "relinked");
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	REQUIRE(PublishUtilityGeneration(*electricity, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
	REQUIRE(PublishUtilityHeatGeneration(*thermal, GetUtilityPowerInterval(), 100000, 74) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*source, 500000}, UtilityPowerRequest{*relay, 0}, UtilityPowerRequest{*target, 600000}}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{*heat_consumer, 50000}}) == UtilityPowerError::None);
	}
	TransferPaidPath(*source, *target, PaidPathSourcePort(), 100000); SavePaidPath(directory, "next-interval");
	REQUIRE(RemoveUtilityPowerProducer(*electricity) == UtilityPowerError::None); SavePaidPath(directory, "source-lost");
	fmt::print("PAID_PATH_BOUNDARY retained_deep4=true actual_paid_commands=true supplied_open_floor=true supplied_materials=true supplied_electricity=true supplied_heat=true operating_geothermal=false transported_bootstrap=false\n");
}

/** @param name Explicit expected original-writer digest environment field. @return Parsed complete64-bit native identity; malformed/missing expectations fail the fixture. */
static uint64_t PaidPathExpectedDigest(const char *name)
{
	const auto *text = std::getenv(name); REQUIRE(text != nullptr); uint64_t result = 0;
	const auto parsed = std::from_chars(text, text + strlen(text), result, 16); REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return result;
}

/** Actual original paid-path files, independent copied malformed preview and recoverable full refusal use the ordinary loader before every observer action. */
TEST_CASE("Actual paid utility multi-hop native files retain source path and Heat authority", "[.utility-power-path-file]")
{
	const auto *input_text = std::getenv("OTTD_PAID_PATH_INPUT"), *mode_text = std::getenv("OTTD_PAID_PATH_CASE"); REQUIRE(input_text != nullptr); REQUIRE(mode_text != nullptr);
	const auto input = ResolveTestInputPath(input_text); const std::string_view mode{mode_text}; const auto directory = GetTestCaseArtifactPaths("utilities", "native-reload").saves;
	bool observed = false; std::exception_ptr failure;
	/** Inspect only actual accepted native authority; copied preview cannot consult or alter this live world's utility/machinery/clock/map/RNG owners. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			if (mode == "create") {
				WritePaidPathCases(directory);
			} else {
				const auto utility = ComputeUtilityPowerDigest(), heat = ComputeGeothermalHeatRouteDigest(), risers = ComputeGeothermalRiserDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
				const auto clock = TimerGameTick::counter; const auto width = Map::SizeX(), height = Map::SizeY(); const auto random = std::array{_random.state[0], _random.state[1]};
				if (mode == "independent") {
					REQUIRE_FALSE(HasUtilityPowerState()); REQUIRE(CaptureGeothermalRisers().empty());
					const auto *candidate = std::getenv("OTTD_PAID_PATH_PREVIEW"); REQUIRE(candidate != nullptr);
					REQUIRE(SaveOrLoad(candidate, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(_load_check_data.utility_power_state.next_share == 5); CHECK(_load_check_data.geothermal_riser_rows.size() == 2); CHECK(_load_check_data.utility_path_rows.size() == 28);
					CHECK(_load_check_data.utility_power_state.producers[1].consumed_lifetime_j == 10000);
				} else {
					CHECK(utility == PaidPathExpectedDigest("OTTD_PAID_PATH_UTILITY")); CHECK(heat == PaidPathExpectedDigest("OTTD_PAID_PATH_HEAT"));
					CHECK(risers == PaidPathExpectedDigest("OTTD_PAID_PATH_RISERS")); CHECK(geology == PaidPathExpectedDigest("OTTD_PAID_PATH_GEOLOGY"));
					REQUIRE(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
					REQUIRE(ValidateGeothermalRisers(CaptureGeothermalRisers())); REQUIRE(ValidateGeothermalUtilityLinks());
					if (mode == "preview" || mode == "refusal" || mode == "native-refusal") { // Scalar and actual map-only faults must both recover without publishing any part of the original paid/Heat graph.
						const auto *bad = std::getenv("OTTD_PAID_PATH_CORRUPT"); REQUIRE(bad != nullptr);
						if (mode == "preview") {
							CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
							REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
						} else {
							if (mode == "native-refusal") REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok); // Independent scalar preview cannot establish actual packed native machinery tiles; full acceptance owns that physical check.
							REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
							CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().next_share == 0); CHECK(CaptureGeothermalRisers().empty());
							REQUIRE(SaveOrLoad(input.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
						}
					} else if (mode == "retry-after-load") {
						REQUIRE(PaidPathCable(PaidPathSplitCell()).Succeeded()); TransferPaidPath(UtilityConsumerID{0}, UtilityConsumerID{2}, PaidPathSourcePort(), 50000);
						SavePaidPath(directory, "read-retry");
					} else if (mode == "resume-use") {
						UtilityCompanyWork work(_current_company); REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume, {UtilityConsumerID{2}, GetUtilityPowerInterval()}, 20000), &work).error == UtilityPowerError::None);
						CHECK(GetUtilityPowerProducer(UtilityProducerID{0})->consumed_lifetime_j == 40000); CHECK(GetUtilityPowerProducer(UtilityProducerID{0})->reserved_j == 450000);
						CHECK(GetUtilityPowerProducer(UtilityProducerID{1})->consumed_lifetime_j == 10000); CHECK(GetUtilityPowerProducer(UtilityProducerID{1})->reserved_j == 40000);
						SavePaidPath(directory, "read-used");
					} else {
						REQUIRE(mode == "reload"); SavePaidPath(directory, fmt::format("read-{}", input.stem().string()));
					}
				}
				// Continuations intentionally settle real additional work/repair.
				// Every other observer, including corrupt preview/full recovery,
				// must preserve the complete accepted original state exactly.
				if (mode != "retry-after-load" && mode != "resume-use") {
					CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalHeatRouteDigest() == heat); CHECK(ComputeGeothermalRiserDigest() == risers); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
					CHECK(TimerGameTick::counter == clock); CHECK(Map::SizeX() == width); CHECK(Map::SizeY() == height); CHECK(std::array{_random.state[0], _random.state[1]} == random);
				}
			}
			fmt::print("PAID_PATH_NATIVE case={} accepted=true\n", mode);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_paid_path_observer, &inspect); IConsole::CmdRegister("utility_paid_path_check", ObservePaidPath);
	const std::string config = GetTestRuntimeConfig().string(), source = input.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", source};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
