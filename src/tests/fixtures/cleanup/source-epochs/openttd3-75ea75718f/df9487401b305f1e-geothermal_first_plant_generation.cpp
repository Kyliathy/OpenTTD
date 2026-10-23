/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_generation.cpp Real Surface town service on the original paid first-plant world, using only normal GameLoop opportunities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15BBB/15BBC actual player-command connection/recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group17B actual hidden/visible native replay.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../geothermal_electrical.h"
#include "../geothermal_service_cmd.h"
#include "../road_cmd.h"
#include "../settings_cmd.h"
#include "../geothermal_utility_cmd.h"
#include "../economy_func.h"
#include "../company_cmd.h"
#include "../landscape_cmd.h"
#include "../town_cmd.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../misc_cmd.h"
#include "../video/null_v.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../saveload/saveload_func.h"
#include "../viewport_func.h"
#include "../map_layer_gui.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../safeguards.h"

static std::function<void()> *_first_plant_generation_observer = nullptr; ///< Invocation-only startup observer; never a saved job, energy or town owner.
static std::function<void()> *_first_plant_completed_frame_observer = nullptr; ///< Test-only observation after the entire ordinary GameLoop, never between synchronized owner phases.

/** Native null-driver specialization that observes completed ordinary frames; it changes no scheduler, timer, geometry or quantity. */
class FirstPlantFrameDriver final : public VideoDriver_Null {
public:
	/** Run the complete ordinary native game/input/window sequence before each read-only scenario checkpoint decision. */
	void MainLoop() override
	{
		// Bound a native destination-construction run without splitting one
		// synchronized world frame at an internal TimerGameTick callback.
		for (uint frame = 0; frame < 12005; ++frame) {
			if (_exit_game) break;
			::GameLoop(); ::InputLoop(); ::UpdateWindows();
			if (_exit_game) break;
			if (_first_plant_completed_frame_observer != nullptr) (*_first_plant_completed_frame_observer)();
		}
	}
	/** @return Explicit test-driver identity; rendering/startup remain the ordinary native null driver. */
	std::string_view GetName() const override { return "first-plant-observer"; }
};

/** Explicit-only test factory; ordinary null/native production drivers keep their existing behavior. */
class FirstPlantFrameDriverFactory final : public DriverFactoryBase {
public:
	/** Register a zero-priority explicit test name without changing automatic video selection. */
	FirstPlantFrameDriverFactory() : DriverFactoryBase(Driver::Type::Video, 0, "first-plant-observer", "First-plant completed-frame observer") {}
	/** @return An ordinary native null driver with a completed-frame observation point. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<FirstPlantFrameDriver>(); }
};
static FirstPlantFrameDriverFactory _first_plant_frame_driver_factory; ///< Test binary registration only; actual game builds contain no observer driver.


/** @param path Invocation-owned retained output. Save actual owners while retaining a normal paused-file policy. */
static void SaveFirstPlantGeneration(const std::filesystem::path &path)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param arguments Native startup command. @return Success after observing the real ordinary loaded world. */
static bool ObserveFirstPlantGeneration(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_first_plant_generation_observer != nullptr); (*_first_plant_generation_observer)(); return true;
}

/** @return The one original actual Surface service, with no fixture allocation or demand. */
static UtilityConsumerID FindFirstPlantTownService()
{
	UtilityConsumerID result{UINT32_MAX};
	// Shared retained consumer references, rather than a test-local ID, resolve
	// the actual paid terminal after both ordinary load and native operation.
	for (const auto &consumer : CaptureUtilityPowerState().consumers) {
		if (consumer.town != TownID{0}) continue; // Terminal destruction clears service references but preserves this actual original native town association.
		REQUIRE(result.base() == UINT32_MAX); result = consumer.meter.consumer_id;
	}
	return result;
}

/**
 * Connect actual paid native risers to the real town at a newly paid Surface terminal.
 * @param terminal Original ordinary station funded by this run in the town's native authority area.
 * @param generator Original existing native powerhouse, which the normal driver has actually bound.
 * @param through_command Explicitly exercise the new synchronized whole connection; false retains the earlier separately documented API scenario.
 * @param recovery Reconnect the retained actual town instead of creating its first accounts.
 * @param new_edges Exact physical edges invalidated by the explicitly selected native repair scenario.
 * @return Shared final town consumer; all identities come from their public owning APIs.
 */
static UtilityConsumerID ConnectFirstPlantTownService(StationID terminal, const GeothermalGeneratorState &generator, bool through_command,
	bool recovery = false, uint32_t new_edges = 0)
{
	if (through_command) {
		GeothermalTownServiceRequest request; request.generator = generator.industry; request.terminal = terminal; request.town = TownID{0};
		const auto risers = CaptureGeothermalRisers(); REQUIRE(risers.size() == 2);
		// Resolve the original two funded machinery identities by actual sending
		// plane; no account/link ID or replacement fixture is supplied to the command.
		for (const auto &riser : risers) {
			if (GetTileLayer(riser.site.portal) == MapLayer::DeepUnderground) request.first_riser = riser.industry;
			if (GetTileLayer(riser.site.portal) == MapLayer::Underground) request.second_riser = riser.industry;
		}
		const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
		const auto native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
		const auto random = std::array{_random.state[0], _random.state[1]};
		const auto proposed = ConnectGeothermalTownService(request, false); REQUIRE(proposed.error == UtilityPowerError::None);
		REQUIRE(Command<Commands::ConnectGeothermalTownService>::Do({}, request.generator, request.first_riser,
			request.second_riser, request.terminal, request.town).Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest);
		REQUIRE(Command<Commands::ConnectGeothermalTownService>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
			GetGeothermalGeneratorPort(generator.industry), std::make_tuple(request.generator, request.first_riser,
			request.second_riser, request.terminal, request.town)).Succeeded());
		const auto after = CaptureUtilityPowerState();
		CHECK(after.next_consumer == before.next_consumer + (recovery ? 0 : 3)); CHECK(after.next_link == before.next_link + (recovery ? new_edges : 2));
		CHECK(after.next_producer == before.next_producer); CHECK(after.next_share == before.next_share);
		CHECK(after.operation_sequence == before.operation_sequence + 1);
		CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash);
		CHECK((std::array{_random.state[0], _random.state[1]} == random));
		CHECK(FindFirstPlantTownService() == proposed.destination);
		fmt::print("FIRST_PLANT_GENERATION_CONNECTION command=1 source={} relay={} destination={} first={} second={}\n",
			proposed.source.base(), proposed.relay.base(), proposed.destination.base(), proposed.links[0].base(), proposed.links[1].base());
		return proposed.destination;
	}
	const auto source = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 135,
		std::array{generator.electrical_producer});
	const auto relay = RegisterUtilityPowerConsumer(_current_company, MapLayer::Underground, UtilityConsumerEffect::AssemblyWork, 135, {});
	const auto destination = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::TownGrowth, 135, {});
	REQUIRE(source.has_value()); REQUIRE(relay.has_value()); REQUIRE(destination.has_value());
	REQUIRE(BindUtilityTownConsumer(*destination, TownID{0}) == UtilityPowerError::None);
	const auto risers = CaptureGeothermalRisers(); REQUIRE(risers.size() == 2);
	// Both original paid machines have separate actual bills and directions.
	// The zero-demand relay transports only references; it is never a generator.
	for (const auto &riser : risers) {
		const bool deep = GetTileLayer(riser.site.tile) == MapLayer::DeepUnderground;
		REQUIRE(RegisterGeothermalRiserLink(riser.industry, deep ? *source : *relay, deep ? *relay : *destination).has_value());
	}
	const auto port = GetGeothermalGeneratorPort(generator.industry); const auto digest = ComputeUtilityPowerDigest();
	const auto cash = Company::Get(_current_company)->money; const auto random = std::array{_random.state[0], _random.state[1]};
	// Real wrong-role/plane/terminal requests and foreign authority must fail
	// before any reference, money, quantity or RNG state can be published.
	CHECK(BindUtilityTownService(*destination, *relay, terminal, port, false) != UtilityPowerError::None);
	CHECK(BindUtilityTownService(*destination, *source, StationID::Invalid(), port, false) != UtilityPowerError::None);
	CHECK(BindUtilityTownService(*source, *source, terminal, port, false) != UtilityPowerError::None);
	CHECK(BindUtilityTownService(*destination, *source, terminal, MultiLayerMap::TileInLayer(port, MapLayer::Surface), true) != UtilityPowerError::None);
	{
		AutoRestoreBackup foreign{_current_company, COMPANY_SPECTATOR};
		CHECK(BindUtilityTownService(*destination, *source, terminal, port, false) != UtilityPowerError::None);
	}
	CHECK(BindUtilityTownService(*destination, *source, terminal, port, false) == UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(Company::Get(_current_company)->money == cash); CHECK((std::array{_random.state[0], _random.state[1]} == random));
	REQUIRE(BindUtilityTownService(*destination, *source, terminal, port) == UtilityPowerError::None);
	CHECK(FindGeothermalElectricalPath(*source, *destination, port, 1)->legs.size() == 2);
	return *destination;
}

/** The retained paid world generates and exports actual native electricity into a real town growth owner. */
TEST_CASE("Original first plant powers a real Surface town through both original paid risers", "[.geothermal-first-plant-generation-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FIRST_PLANT_ACTION"); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	const char *command_text = std::getenv("OTTD_FIRST_PLANT_TOWN_COMMAND");
	const bool through_command = command_text != nullptr && std::string_view{command_text} == "1"; // Only an explicit acceptance run selects the new whole command; retained earlier scenarios keep their original owner API path.
	REQUIRE((action == "prepare" || action == "operate" || action == "inspect" || action == "boundary" || action == "repair" || action == "view")); // Setup funds real destination work; operation/view consume only saved physical service, inspection advances no game frame.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	bool started = false, completed = false; std::exception_ptr failure; uint frames = 0; StationID terminal = StationID::Invalid();
	Money before = 0; uint64_t geology = 0, original_use = 0; UtilityConsumerID service{UINT32_MAX}; UtilityProducerID original_output{UINT32_MAX};
	Window *view = nullptr; ///< Borrowed actual main viewport for17B replay; never a source of simulation time or electrical rights.
	const char *boundary_text = std::getenv("OTTD_FIRST_PLANT_BOUNDARY"); const std::string_view boundary = boundary_text == nullptr ? "" : boundary_text;
	/** Actual ordinary afterload is the sole source of all machinery, money, floor and paid bill history. */
	std::function<void()> begin = [&] {
		try {
			Backup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(CaptureGeothermalGenerators().size() <= 1); REQUIRE(CaptureGeothermalCollectors().size() <= 1); REQUIRE(CaptureGeothermalRisers().size() == 2);
			before = Company::Get(_current_company)->money; geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto inspected = paths.Output("saves", "first-plant-generation-inspect.sav"); SaveFirstPlantGeneration(inspected);
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", before.base(), inspected.string());
			if (action == "inspect") { completed = true; _exit_game = true; actor.Restore(); return; }
			REQUIRE(CaptureGeothermalGenerators().size() == 1);
			const auto original = CaptureGeothermalGenerators().front(); original_output = original.electrical_producer;
			if (original_output.base() != UINT32_MAX) original_use = GetUtilityPowerProducer(original_output)->consumed_lifetime_j;
			service = FindFirstPlantTownService();
			if (action == "prepare") {
				REQUIRE(service.base() == UINT32_MAX);
				// The original native town initially has no houses. Paid ordinary
				// roads and funded building work let normal town growth create a
				// real populated destination before any electrical demand is made.
				REQUIRE(Command<Commands::BuildRoadLong>::Do(DoCommandFlag::Execute, TileXYL(12, 10, MapLayer::Surface), TileXYL(8, 10, MapLayer::Surface),
					ROADTYPE_ROAD, Axis::X, DisallowedRoadDirections{}, false, false, false).Succeeded());
				const TileIndex platform = TileXYL(13, 10, MapLayer::Surface);
				REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, platform, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{1},
					STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false).Succeeded()); terminal = GetStationIndex(platform);
				// The retained reduced bootstrap scenario explicitly disabled town
				// development. Ordinary synchronized settings enable real native
				// growth/funding; neither command creates a house or energy demand.
				REQUIRE(Command<Commands::ChangeSetting>::Do(DoCommandFlag::Execute, "economy.fund_buildings", 1).Succeeded());
				REQUIRE(Command<Commands::ChangeSetting>::Do(DoCommandFlag::Execute, "economy.town_growth_rate", 2).Succeeded());
				const auto funded = Command<Commands::TownAction>::Do(DoCommandFlag::Execute, TownID{0}, TownAction::FundBuildings); INFO("native funding allowed=" << _settings_game.economy.fund_buildings << " money=" << Company::Get(_current_company)->money.base()); REQUIRE(funded.Succeeded());
				fmt::print("FIRST_PLANT_GENERATION_DESTINATION terminal={} funding={} paid={}\n", terminal.base(), funded.GetCost().base(), (before - Company::Get(_current_company)->money).base());
			} else { REQUIRE(service.base() != UINT32_MAX); }
			if (action == "boundary") {
				REQUIRE(service.base() != UINT32_MAX); const auto binding = *GetUtilityPowerConsumer(service);
				const auto layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
				// Each isolated invocation changes a real original physical/owner
				// participant. No artificial demand or incoming energy is injected.
				if (boundary == "path") REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, TileXYL(54, 30, MapLayer::Underground), UtilityEnergyDomain::Electricity).Succeeded());
				else if (boundary == "sealed") REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, layout.deep_export, false).Succeeded());
				else if (boundary == "terminal") {
					const auto platform = Station::Get(binding.town_terminal)->train_station.tile;
					REQUIRE(Command<Commands::RemoveFromRailStation>::Do(DoCommandFlag::Execute, platform, platform, false).Succeeded());
				} else if (boundary == "source") REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, GetGeothermalGeneratorPort(original.industry)).Succeeded());
				else if (boundary == "all-sources") {
					REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, GetGeothermalGeneratorPort(original.industry)).Succeeded());
					REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, DescribeFirstPlantLayout(GetGeologyWorldState().vents.front()).collector).Succeeded());
				}
				else if (boundary == "company") {
					// Ordinary single-player liquidation requires another real
					// company and completes deletion after temporary cleanup funds.
					// Neither those transient funds nor a half-removed player may
					// survive into an accepted save or future service opportunity.
					// Give the reduced original scenario ordinary player names before native competitor/news formatting; its original unset name IDs are not valid generated names.
					REQUIRE(Command<Commands::RenamePresident>::Do(DoCommandFlag::Execute, "First Plant Operator").Succeeded());
					REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "First Plant Works").Succeeded());
					const auto former = _current_company;
					REQUIRE(Command<Commands::CompanyControl>::Do(DoCommandFlag::Execute, CompanyCtrlAction::NewAI, CompanyID::Invalid(), CompanyRemoveReason::Manual, ClientID::Invalid).Succeeded());
					REQUIRE(Company::GetNumItems() == 2);
					REQUIRE(Command<Commands::CompanyControl>::Do(DoCommandFlag::Execute, CompanyCtrlAction::Delete, former, CompanyRemoveReason::Manual, ClientID::Invalid).Succeeded());
					actor.Change(_local_company); actor.Update(); // Native liquidation selected the remaining client; the startup observer must not restore the now-deleted former player.
					REQUIRE_FALSE(Company::IsValidID(former));
					geology = ComputeGeologyStateDigest(GetGeologyWorldState()); // Actual abandonment releases geological ownership; later frames must retain this legitimate new state.

				}
				else if (boundary == "town") RemoveUtilityTownBindings(binding.town); // Focused actual deletion callback; ordinary town destruction/ID-reuse remains covered by its owning native effect fixture.
				else if (boundary == "custom") {
					AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(CmdTownGrowthRate(DoCommandFlag::Execute, binding.town, 37).Succeeded());
				} else FAIL("Unknown explicit native boundary");
			}
			if (action == "repair") {
				const TileIndex gap = TileXYL(54, 30, MapLayer::Underground);
				if (!GetGeothermalUtilityLink(gap, UtilityEnergyDomain::Electricity).has_value()) REQUIRE(Command<Commands::BuildGeothermalUtilityLink>::Do(DoCommandFlag::Execute, gap, UtilityEnergyDomain::Electricity).Succeeded());
				const auto layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
				const auto record = GetLayerPortalWorkRecord(layout.deep_export); REQUIRE(record != nullptr);
				const bool recommissioned = record->state != LayerPortalWorkState::Connected;
				if (recommissioned) REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, layout.deep_export, true).Succeeded());
				if (through_command) {
					const auto retained = service;
					const TileIndex platform = TileXYL(13, 10, MapLayer::Surface);
					// An actually removed terminal pays its normal reconstruction
					// invoice before the original service account may reconnect.
					if (!IsTileType(platform, TileType::Station)) {
						REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, platform, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{1},
							STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false).Succeeded());
					}
					terminal = GetStationIndex(platform);
					service = ConnectFirstPlantTownService(terminal, original, true, true, recommissioned ? 1 : 0); REQUIRE(service == retained);
					CHECK(GetUtilityPowerProducer(original_output)->consumed_lifetime_j == original_use);
					const auto repaired = paths.Output("saves", "first-plant-town-repaired.sav"); SaveFirstPlantGeneration(repaired);
					fmt::print("FIRST_PLANT_GENERATION_SAVE stage=repaired cash={} path={}\n", Company::Get(_current_company)->money.base(), repaired.string());
				} else {
					// A real repaired pair obtains fresh shared references; tombstones
					// and original settled history are never revived or renumbered.
					const auto links = CaptureUtilityPowerState().links;
					for (const auto &riser : CaptureGeothermalRisers()) {
						if (std::ranges::any_of(links, [&](const auto &link) { return link.active && link.kind == UtilityLinkKind::ElectricalRiser && link.riser == riser.industry; })) continue; // An intact original paid edge is reused; only a retired edge needs a fresh reference.
						const auto previous = std::ranges::find_if(links, [&](const auto &link) { return link.kind == UtilityLinkKind::ElectricalRiser && link.riser == riser.industry; });
						REQUIRE(previous != links.end()); REQUIRE(RegisterGeothermalRiserLink(riser.industry, previous->source, previous->destination).has_value());
					}
				}
			}
			if (action == "view") {
				view = FindWindowById(WindowClass::MainWindow, 0); REQUIRE(view != nullptr); REQUIRE(view->viewport != nullptr);
				SetViewportLayer(view, MapLayer::DeepUnderground); CHECK(view->viewport->layer == MapLayer::DeepUnderground);
			}
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true; actor.Restore();
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** Observe one fully completed normal game frame; never advance a timer, publish energy or manufacture a town house. */
	std::function<void()> completed_frame = [&] {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; ++frames; INFO("actual game frames=" << frames);
			if (view != nullptr) {
				// Alternate actual visible/hidden Deep after complete native frames.
				// The camera must not change the next shared service or cargo step.
				const auto layer = frames % 2 == 0 ? MapLayer::DeepUnderground : MapLayer::Surface;
				SetViewportLayer(view, layer); CHECK(view->viewport->layer == layer);
			}
			INFO("town population=" << Town::Get(TownID{0})->cache.population << " houses=" << Town::Get(TownID{0})->cache.num_houses << " grow counter=" << Town::Get(TownID{0})->grow_counter << " rate=" << Town::Get(TownID{0})->growth_rate); REQUIRE(frames < 12000); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			if (action == "boundary") {
				const auto *output = GetUtilityPowerProducer(original_output); REQUIRE(output != nullptr); CHECK(output->consumed_lifetime_j == original_use);
				if (frames < 4) return;
				CHECK(GetUtilityTownPowerBonus(TownID{0}) == 0);
				if (boundary == "terminal") CHECK(GetUtilityPowerConsumer(service)->town_terminal == StationID::Invalid());
				if (boundary == "town") CHECK(GetUtilityPowerConsumer(service)->town == TownID::Invalid());
				if (boundary == "company") CHECK_FALSE(GetUtilityPowerConsumer(service)->active);
				if (boundary == "custom") CHECK(Town::Get(TownID{0})->growth_rate == 37);
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
				const auto saved = paths.Output("saves", fmt::format("first-plant-blocked-{}.sav", boundary)); SaveFirstPlantGeneration(saved);
				fmt::print("FIRST_PLANT_GENERATION_SAVE stage=blocked cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
				fmt::print("FIRST_PLANT_GENERATION_READY action=boundary boundary={} frames={} electrical_used={} cash={}\n", boundary, frames, output->consumed_lifetime_j, Company::Get(_current_company)->money.base());
				completed = true; started = false; _exit_game = true; return;
			}
			const auto generators = CaptureGeothermalGenerators(); REQUIRE(generators.size() == 1); const auto &generator = generators.front();
			if (action == "prepare") {
				const Town *town = Town::Get(TownID{0});
				if (frames % 4000 == 0) fmt::print("FIRST_PLANT_DESTINATION_PROGRESS frames={} houses={} population={} grow_counter={} rate={}\n", frames, town->cache.num_houses, town->cache.population, town->grow_counter, town->growth_rate); // Bounded native diagnostics distinguish a house under real construction from an unreachable growth target.
				if (town->cache.population == 0 || generator.electrical_producer.base() == UINT32_MAX) return; // Normal growth and real native conversion must exist before this actual service may register demand.
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
				service = ConnectFirstPlantTownService(terminal, generator, through_command);
				const auto saved = paths.Output("saves", "first-plant-town-connected.sav"); SaveFirstPlantGeneration(saved);
				fmt::print("FIRST_PLANT_GENERATION_SAVE stage=connected cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
				fmt::print("FIRST_PLANT_GENERATION_READY action=prepare frames={} spent={} population={} service={} cash={}\n", frames, (before - Company::Get(_current_company)->money).base(), town->cache.population, service.base(), Company::Get(_current_company)->money.base());
				completed = true; started = false; _exit_game = true; return;
			}
			const auto *destination = GetUtilityPowerConsumer(service); REQUIRE(destination != nullptr);
			const auto *producer = GetUtilityPowerProducer(generator.electrical_producer); REQUIRE(producer != nullptr);
			const uint64_t used = producer->consumed_lifetime_j;
			if (used == 0) return;
			CHECK(destination->town_effect_applied); CHECK(GetUtilityTownPowerBonus(destination->town) == 25); CHECK(used % 135 == 0);
			const auto *town = Town::Get(destination->town); CHECK(town->cache.population > 0);
			const auto digest = ComputeUtilityPowerDigest(); RunUtilityTownServices(); CHECK(ComputeUtilityPowerDigest() == digest);
			const uint64_t uses = used / 135;
			if (uses != 37 && uses < 74) return; // Keep one real interrupted original, then finish a bounded complete service interval.
			REQUIRE(uses <= 74);
			const auto saved = paths.Output("saves", uses == 37 ? "first-plant-generation-partial.sav" : "first-plant-generation-complete.sav"); SaveFirstPlantGeneration(saved);
			fmt::print("FIRST_PLANT_GENERATION_SAVE stage={} cash={} path={}\n", uses == 37 ? "partial" : "complete", Company::Get(_current_company)->money.base(), saved.string());
			fmt::print("FIRST_PLANT_GENERATION_READY action=operate uses={} electrical_used={} electrical_generated={} service_ticks={} service_remainder={} growth_rate={} population={} spent={} cash={}\n", uses, used, producer->generated_lifetime_j, generator.active_service_ticks, generator.service_remainder, town->growth_rate, town->cache.population, (before - Company::Get(_current_company)->money).base(), Company::Get(_current_company)->money.base());
			if (uses == 37) return;
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded()); completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	};
	AutoRestoreBackup frame_observer{_first_plant_completed_frame_observer, &completed_frame};
	AutoRestoreBackup observer{_first_plant_generation_observer, &begin}; IConsole::CmdRegister("first_plant_generate", ObserveFirstPlantGeneration);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "first-plant-observer:ticks=12005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
