/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_magma.cpp Paid2000 construction and useful generation on the original earned first-plant world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8FCA-8FCB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBC actual retained-town source replacement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../cheat_func.h"
#include "../cheat_type.h"
#include "../surface_supply_cmd.h"
#include "../console_internal.h"
#include "../geothermal_electrical.h"
#include "../geothermal_service_cmd.h"
#include "../geothermal_utility_cmd.h"
#include "../landscape_cmd.h"
#include "../misc_cmd.h"
#include "../map_layer_gui.h"
#include "../vehicle_cmd.h"
#include "../openttd.h"
#include "../video/null_v.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_magma_start = nullptr; ///< Invocation-only ordinary afterload observer, never a saved job or quantity.
static std::function<void()> *_magma_frame = nullptr; ///< Observe only complete ordinary frames, after all synchronized owners have run.

/** Explicit test driver using unchanged native simulation/input/window phases and completed-frame checkpoints. */
class FirstPlantMagmaDriver final : public VideoDriver_Null {
public:
	/** Run bounded ordinary frames; no timer, arrival, progress or utility amount is manufactured by the observer. */
	void MainLoop() override
	{
		// Each checkpoint follows the whole native frame so interrupted saves
		// and uninterrupted operation retain exactly the same opportunity count.
		for (uint frame = 0; frame < 8005; ++frame) {
			if (_exit_game) break;
			::GameLoop(); ::InputLoop(); ::UpdateWindows();
			if (_exit_game) break;
			if (_magma_frame != nullptr) (*_magma_frame)();
		}
	}
	/** @return Explicit-only driver identity; the production null driver remains unchanged. */
	std::string_view GetName() const override { return "first-plant-magma"; }
};

/** Zero-priority factory prevents test observation from changing automatic native video selection. */
class FirstPlantMagmaFactory final : public DriverFactoryBase {
public:
	/** Register the explicit-only native test driver. */
	FirstPlantMagmaFactory() : DriverFactoryBase(Driver::Type::Video, 0, "first-plant-magma", "First-plant Magma completed-frame observer") {}
	/** @return Native null driver with an observation point after each complete ordinary frame. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<FirstPlantMagmaDriver>(); }
};
static FirstPlantMagmaFactory _first_plant_magma_factory; ///< Test binary only; production builds contain no scenario observer.

/** @param arguments Actual game-start invocation. @return True after the registered ordinary afterload observer runs. */
static bool StartFirstPlantMagma(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_magma_start != nullptr); (*_magma_start)(); return true;
}

/** @param paths Invocation-owned outputs. @param stage Actual observed checkpoint. Save native owners without advancing a frame or changing their retained pause policy. */
static void SaveFirstPlantMagma(const TestArtifactPaths &paths, std::string_view stage)
{
	const auto path = paths.Output("saves", fmt::format("first-plant-magma-{}.sav", stage));
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	fmt::print("FIRST_PLANT_MAGMA_SAVE stage={} cash={} path={}\n", stage, Company::Get(_current_company)->money.base(), path.string());
}

/** @return Original three-stop paid model4 carrier, retaining its implicit native visits and four actual model5 holds. */
static Train *FindFirstPlantMagmaCarrier()
{
	Train *result = nullptr;
	// The old one-order spoil hauler shares this engine model. Only the
	// retained three-manual-stop construction train is this physical carrier.
	for (Train *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || train->engine_type != GetUndergroundEngine(4) || train->GetNumManualOrders() != 3) continue;
		REQUIRE(result == nullptr); result = train;
	}
	REQUIRE(result != nullptr); return result;
}

/** @param layout Original earned geometry. @return Borrowed actual new-model work owner, without inventing a participant or source. */
static const GeothermalGeneratorState *FindFirstPlantMagma(const FirstPlantLayout &layout)
{
	const auto *industry = Industry::GetByTile(layout.magma); REQUIRE(industry != nullptr);
	const auto *state = GetGeothermalGeneratorState(industry->index); REQUIRE(state != nullptr);
	REQUIRE(state->model == GeothermalGeneratorModel::MagmaLoop); return state;
}

/** @param town Original real town. @return Its one original native consumer, including while physical service is explicitly disconnected. */
static UtilityConsumerID FindFirstPlantMagmaTown(TownID town)
{
	UtilityConsumerID result{UINT32_MAX};
	// Reuse the native town association; registering a replacement consumer
	// would lose its original settled effect and violate unique town authority.
	for (const auto &consumer : CaptureUtilityPowerState().consumers) {
		if (consumer.town != town) continue;
		REQUIRE(result.base() == UINT32_MAX); result = consumer.meter.consumer_id;
	}
	REQUIRE(result.base() != UINT32_MAX); return result;
}

/** @param origin Actual original Surface supplier. Reconcile every paid unit against native custody and all five retained complete/construction bills. */
static void CheckFirstPlantMagmaMaterials(StationID origin)
{
	const std::array labels{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}};
	// Keep the original1980 machinery, so its consumed bill remains a live
	// native receipt. The52 additional units cannot disappear into demolition.
	for (const auto label : labels) {
		uint32_t bought = 0, dispatched = 0, physical = 0, consumed = 0;
		const auto cargo = GetCargoTypeByLabel(label);
		for (const auto &order : GetSurfaceSupplyOrderHistory()) {
			if (order.label != label) continue;
			REQUIRE(order.station == origin); REQUIRE(order.refunded == 0);
			bought += order.quantity; dispatched += order.dispatched;
		}
		// All stations, vehicles and native industry stores own their actual
		// remaining packets/stock. Reservations and dispatch history add no units.
		for (const Station *station : Station::Iterate()) physical += station->goods[cargo].TotalCount();
		for (const Vehicle *vehicle : Vehicle::Iterate()) if (vehicle->cargo_type == cargo) physical += vehicle->cargo.TotalCount();
		for (const Industry *industry : Industry::Iterate()) {
			const auto accepted = industry->GetCargoAccepted(cargo); if (accepted != industry->accepted.end()) physical += accepted->waiting;
		}
		/** @param kind Actual frozen bill. @param committed Actual whole-bill native receipt. Count only consumed material, never another inventory. */
		const auto bill = [&](MultiLayerGeothermalConstructionKind kind, bool committed) {
			if (!committed) return;
			for (const auto &input : GetMultiLayerGeothermalConstruction(kind)->inputs) if (input.label == label) consumed += input.quantity; // Frozen bill lines identify the committed original cargo meaning.
		};
		for (const auto &state : CaptureGeothermalCollectors()) bill(MultiLayerGeothermalConstructionKind::Collector, state.materials_committed);
		for (const auto &state : CaptureGeothermalRisers()) bill(MultiLayerGeothermalConstructionKind::Riser, state.materials_committed);
		for (const auto &state : CaptureGeothermalGenerators()) bill(state.model == GeothermalGeneratorModel::VentTap ? MultiLayerGeothermalConstructionKind::Powerhouse : MultiLayerGeothermalConstructionKind::MagmaPowerhouse, state.materials_committed);
		CHECK(bought == physical + consumed); CHECK(dispatched <= bought);
		fmt::print("FIRST_PLANT_MAGMA_MATERIAL label={} bought={} dispatched={} physical={} consumed={}\n", std::string(label.begin(), label.end()), bought, dispatched, physical, consumed);
	}
}

/** @param origin Actual original Surface supplier. Buy only the additional frozen Magma bill missing from original monotonic paid history, including a genuine funded-file continuation. */
static void BuyFirstPlantMagmaInputs(StationID origin)
{
	// The four retained original bills account for the first64 paid units.
	// Compare purchase obligations, never mistake undispatched history for
	// physical inventory or replace stock lost by an earlier failed run.
	for (const auto &part : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::MagmaPowerhouse)->inputs) {
		uint32_t required = part.quantity, purchased = 0;
		for (const auto kind : {MultiLayerGeothermalConstructionKind::Collector, MultiLayerGeothermalConstructionKind::Powerhouse,
			MultiLayerGeothermalConstructionKind::Riser, MultiLayerGeothermalConstructionKind::Riser}) {
			for (const auto &original : GetMultiLayerGeothermalConstruction(kind)->inputs) if (original.label == part.label) required += original.quantity;
		}
		for (const auto &order : GetSurfaceSupplyOrderHistory()) if (order.label == part.label) purchased += order.quantity;
		REQUIRE(purchased <= required); const auto missing = static_cast<uint16_t>(required - purchased);
		if (missing == 0) continue;
		const auto quote = PrepareSurfaceSupplyOrder(origin, part.label, missing); REQUIRE(quote.error == SurfaceSupplyError::None);
		REQUIRE(Command<Commands::OrderSurfaceSupply>::Do(DoCommandFlag::Execute, origin, part.label, missing, quote.next_order).Succeeded());
		fmt::print("FIRST_PLANT_MAGMA_INVOICE command=supply label={} units={} cost={}\n", std::string(part.label.begin(), part.label.end()), missing, quote.cost.base());
	}
}

/** @param layout Original earned floor and paid original utilities. Install only additional domain cells, paying actual native invoices once. */
static void WireFirstPlantMagma(const FirstPlantLayout &layout)
{
	std::vector<TileIndex> branch;
	const uint x = layout.vent.x, y = layout.vent.y;
	// Route east of the retained4x4 Vent-Tap, south below it, then into the
	// Magma port. The original station on the west side remains undisturbed.
	for (uint column = x + 4; column <= x + 8; ++column) branch.push_back(TileXYL(column, y + 1, MapLayer::DeepUnderground));
	for (uint row = y + 2; row <= y + 6; ++row) branch.push_back(TileXYL(x + 8, row, MapLayer::DeepUnderground));
	for (int column = x + 7; column >= static_cast<int>(x + 4); --column) branch.push_back(TileXYL(column, y + 6, MapLayer::DeepUnderground));
	branch.push_back(layout.magma);
	// Heat and electricity are separately purchased physical domain cells,
	// even where their routes occupy the same already-earned map tile.
	for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) {
		for (const auto tile : branch) {
			if (GetGeothermalUtilityLink(tile, domain).has_value()) continue;
			REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
			const auto cost = Command<Commands::BuildGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, domain);
			INFO("Magma utility tile=" << TileX(tile) << "," << TileY(tile) << " domain=" << to_underlying(domain)); REQUIRE(cost.Succeeded());
			fmt::print("FIRST_PLANT_MAGMA_INVOICE command=utility domain={} tile={} cost={}\n", to_underlying(domain), tile.base(), cost.GetCost().base());
		}
	}
}

/** @param generator Actual ordinary-driver-bound2000 machinery. @param through_command Use the synchronized atomic replacement; false retains the earlier API scenario. Reconnect the same real town and upper relay, obtaining only a fresh Deep forwarding edge. */
static void ConnectFirstPlantMagma(const GeothermalGeneratorState &generator, bool through_command)
{
	const auto destination = FindFirstPlantMagmaTown(TownID{0});
	UtilityConsumerID relay{UINT32_MAX}; IndustryID deep = IndustryID::Invalid();
	// Preserve the original upper edge and relay. The old Deep forwarding
	// edge was explicitly retired before the new physical model was funded.
	for (const auto &link : CaptureUtilityPowerState().links) {
		if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || link.destination != destination) continue;
		REQUIRE(relay.base() == UINT32_MAX); relay = link.source;
	}
	for (const auto &riser : CaptureGeothermalRisers()) if (GetTileLayer(riser.site.tile) == MapLayer::DeepUnderground) deep = riser.industry;
	REQUIRE(relay.base() != UINT32_MAX); REQUIRE(deep != IndustryID::Invalid());
	if (through_command) {
		GeothermalTownServiceRequest request{generator.industry, deep, IndustryID::Invalid(), StationID{3}, TownID{0}};
		for (const auto &riser : CaptureGeothermalRisers()) { // Resolve the original paid upper machinery by its actual sending plane.
			if (GetTileLayer(riser.site.portal) == MapLayer::Underground) request.second_riser = riser.industry;
		}
		const auto before = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
		const auto native = ComputeGeothermalGeneratorDigest(); const auto cash = Company::Get(_current_company)->money;
		const auto random = _random;
		const auto proposed = ConnectGeothermalTownService(request, false); REQUIRE(proposed.error == UtilityPowerError::None);
		CHECK(proposed.destination == destination); CHECK(proposed.relay == relay);
		REQUIRE(Command<Commands::ConnectGeothermalTownService>::Do({}, request.generator, request.first_riser, request.second_riser, request.terminal, request.town).Succeeded());
		CHECK(ComputeUtilityPowerDigest() == digest);
		REQUIRE(Command<Commands::ConnectGeothermalTownService>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
			GetGeothermalGeneratorPort(generator.industry), std::make_tuple(request.generator, request.first_riser, request.second_riser, request.terminal, request.town)).Succeeded());
		const auto after = CaptureUtilityPowerState();
		CHECK(after.next_consumer == before.next_consumer + 1); CHECK(after.next_link == before.next_link + 1);
		CHECK(after.next_producer == before.next_producer); CHECK(after.next_share == before.next_share); CHECK(after.operation_sequence == before.operation_sequence + 1);
		CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
		CHECK(GetUtilityPowerLink(proposed.links[1])->source == relay);
		fmt::print("FIRST_PLANT_GENERATION_CONNECTION command=1 source={} relay={} destination={} first={} second={}\n",
			proposed.source.base(), proposed.relay.base(), proposed.destination.base(), proposed.links[0].base(), proposed.links[1].base());
		return;
	}
	const auto source = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 135, std::array{generator.electrical_producer}); REQUIRE(source.has_value());
	REQUIRE(RegisterGeothermalRiserLink(deep, *source, relay).has_value());
	REQUIRE(BindUtilityTownService(destination, *source, StationID{3}, GetGeothermalGeneratorPort(generator.industry)) == UtilityPowerError::None);
	REQUIRE(FindGeothermalElectricalPath(*source, destination, GetGeothermalGeneratorPort(generator.industry), 1)->legs.size() == 2);
}

/** Actual paid second-model construction and operation never replaces the original world or its retained quantity owners. */
TEST_CASE("Original paid first-plant world builds and uses the2000 Magma model", "[.geothermal-first-plant-magma-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *text = std::getenv("OTTD_FIRST_PLANT_ACTION"); REQUIRE(text != nullptr); const std::string_view action{text};
	const char *command_text = std::getenv("OTTD_FIRST_PLANT_TOWN_COMMAND");
	const bool through_command = command_text != nullptr && std::string_view{command_text} == "1"; // Opt-in acceptance must execute the actual synchronized transaction; ordinary historical replay retains its original scenario.
	REQUIRE((action == "upgrade" || action == "deliver" || action == "wire" || action == "operate" || action == "inspect" || action == "view" || action == "boundary" || action == "retry")); // Each stage consumes only actual saved native prerequisites; inspection grants no frame or work, and actual calendar/view controls cannot supply energy.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	bool started = false, completed = false, partial = false, outbound = false, withdrawing = false; uint frames = 0;
	std::exception_ptr failure; FirstPlantLayout layout; VehicleID carrier; StationID origin;
	GeothermalGeneratorState original; uint64_t geology = 0; Money before = 0;
	GeothermalGeneratorState blocked; uint64_t prior_use = 0, prior_generation = 0, prior_heat_use = 0;
	Window *view = nullptr; ///< Borrowed real main viewport for actual hidden/visible replay, never a simulation owner.
	/** Resolve every participant from ordinary afterload before any public command or normal frame. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(GetGeologyWorldState().vents.size() == 1); layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
			const auto generators = CaptureGeothermalGenerators(); const auto old = std::ranges::find(generators, GeothermalGeneratorModel::VentTap, &GeothermalGeneratorState::model); REQUIRE(old != generators.end()); original = *old;
			REQUIRE(GetUtilityPowerProducer(original.electrical_producer)->consumed_lifetime_j == 9990);
			before = Company::Get(_current_company)->money; geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto inspected = paths.Output("saves", "first-plant-magma-inspect.sav"); SaveFirstPlantMagma(paths, "inspect");
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", before.base(), inspected.string());
			if (action == "inspect") { completed = true; _exit_game = true; return; }
			Train *head = FindFirstPlantMagmaCarrier(); carrier = head->index;
			origin = StationID::Invalid();
			for (const Order &order : head->Orders()) if (order.IsType(OT_GOTO_STATION)) { origin = order.GetDestination().ToStationID(); break; } // The first retained manual stop is the original paid Surface supplier.
			REQUIRE(origin == StationID{0});
			if (action == "upgrade") {
				const GeothermalSiteRequest request{layout.vent.id, _current_company, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, layout.magma, layout.magma, DiagDirection::SE};
				REQUIRE(TimerGameCalendar::year.base() == 1980);
				const auto digest = ComputeGeothermalSiteDigest(); REQUIRE(ClaimGeothermalSite(request, false).error == GeothermalError::Unavailable); CHECK(ComputeGeothermalSiteDigest() == digest);
				const auto money = Company::Get(_current_company)->money; const auto utility = ComputeUtilityPowerDigest(); const auto economy_date = TimerGameEconomy::date;
				REQUIRE(ChangeCheatCalendarYear(2000) == 2000); CHECK(_cheats.change_date.been_used); CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeUtilityPowerDigest() == utility);
				if (TimerGameEconomy::UsingWallclockUnits()) CHECK(TimerGameEconomy::date == economy_date);
				fmt::print("FIRST_PLANT_MAGMA_DATE native_cheat=1 calendar={} economic={}\n", TimerGameCalendar::date.base(), TimerGameEconomy::date.base());
				RemoveUtilityTownServiceTerminal(StationID{3});
				// Retire only the old Deep numerical edge, preserving the paid
				// original riser, upper edge and every settled1980 source amount.
				for (const auto &link : CaptureUtilityPowerState().links) {
					if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || link.source != UtilityConsumerID{1}) continue;
					REQUIRE(RemoveUtilityPowerLink(link.id) == UtilityPowerError::None);
				}
				REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, layout.powerhouse, UtilityEnergyDomain::Heat).Succeeded());
				// Ordinary clearing removes only owned rails from the separately
				// paid4x4 floor; native admission must refuse any unearned cell.
				for (uint row = 0; row < 4; ++row) for (uint column = 0; column < 4; ++column) {
					const auto tile = TileAddWrap(layout.magma, column, row); REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated); REQUIRE(GetTBMSupportTile(tile) == nullptr);
					if (IsPlainRailTile(tile)) REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, tile).Succeeded());
					REQUIRE(IsTileType(tile, TileType::Clear));
				}
				const auto claim = ClaimGeothermalSite(request); REQUIRE(claim.error == GeothermalError::None);
				const auto cost = Command<Commands::BuildGeothermalGenerator>::Do(DoCommandFlag::Execute, layout.magma, claim.id, uint32_t{123}); REQUIRE(cost.Succeeded()); CHECK(cost.GetCost() == Money{360000});
				SaveFirstPlantMagma(paths, "funded");
				// Actual paid orders create normal station packets for the same
				// already-purchased carrier; no bill stock is seeded directly.
				BuyFirstPlantMagmaInputs(origin);
				CheckFirstPlantMagmaMaterials(origin); SaveFirstPlantMagma(paths, "purchased"); completed = true; _exit_game = true; return;
			}
			if (action == "deliver") BuyFirstPlantMagmaInputs(origin);
			if (action == "wire") { REQUIRE(IsGeothermalGeneratorConstructionComplete(FindFirstPlantMagma(layout)->industry)); WireFirstPlantMagma(layout); SaveFirstPlantMagma(paths, "wired"); }
			if (action == "view") {
				view = FindWindowById(WindowClass::MainWindow, 0); REQUIRE(view != nullptr); REQUIRE(view->viewport != nullptr);
				SetViewportLayer(view, MapLayer::DeepUnderground); CHECK(view->viewport->layer == MapLayer::DeepUnderground);
			}
			// Date refusal and recovery observe the same already-settled native
			// machinery. These copies are assertions only, never replacement stock,
			// utility publication or a second account for the retained quantities.
			if (action == "boundary" || action == "retry") {
				blocked = *FindFirstPlantMagma(layout);
				const auto *output = GetUtilityPowerProducer(blocked.electrical_producer); REQUIRE(output != nullptr);
				prior_use = output->consumed_lifetime_j; prior_generation = output->generated_lifetime_j;
				prior_heat_use = GetUtilityPowerProducer(CaptureGeothermalCollectors().front().heat_producer)->consumed_lifetime_j;
				if (action == "boundary") {
					const char *boundary = std::getenv("OTTD_FIRST_PLANT_BOUNDARY"); REQUIRE(boundary != nullptr); REQUIRE(std::string_view{boundary} == "date");
					REQUIRE(ChangeCheatCalendarYear(1999) == 1999);
					CHECK_FALSE(PrepareGeothermalGeneratorDemand(blocked.industry, 1).has_value());
				} else {
					REQUIRE(TimerGameCalendar::year.base() == 1999); REQUIRE(ChangeCheatCalendarYear(2000) == 2000);
				}
				fmt::print("FIRST_PLANT_MAGMA_DATE native_cheat=1 calendar={} economic={}\n", TimerGameCalendar::date.base(), TimerGameEconomy::date.base());
			}
			if (action == "deliver" && head->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, carrier, true).Succeeded()); // Only a physically stopped retained carrier needs its ordinary departure command.
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** Every observation follows a complete ordinary frame, preserving native labour, cargo, energy and continuation timing. */
	std::function<void()> frame = [&] {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; ++frames;
			REQUIRE(*GetGeothermalGeneratorState(original.industry) == original);
			CHECK(GetUtilityPowerProducer(original.electrical_producer)->consumed_lifetime_j == 9990);
			const auto *generator = FindFirstPlantMagma(layout); Train *head = Train::Get(carrier);
			if (view != nullptr) {
				const auto layer = frames % 2 == 0 ? MapLayer::DeepUnderground : MapLayer::Surface;
				SetViewportLayer(view, layer); CHECK(view->viewport->layer == layer);
			}
			if (action == "boundary") {
				const auto *output = GetUtilityPowerProducer(generator->electrical_producer); REQUIRE(output != nullptr);
				CHECK(*generator == blocked); CHECK(output->consumed_lifetime_j == prior_use); CHECK(output->generated_lifetime_j == prior_generation);
				CHECK(GetUtilityPowerProducer(CaptureGeothermalCollectors().front().heat_producer)->consumed_lifetime_j == prior_heat_use);
				CHECK(GetUtilityTownPowerBonus(TownID{0}) == 0);
				if (frames < 4) return;
				SaveFirstPlantMagma(paths, "blocked-date");
			} else if (action == "retry") {
				const auto *output = GetUtilityPowerProducer(generator->electrical_producer); REQUIRE(output != nullptr);
				CHECK(generator->active_service_ticks == blocked.active_service_ticks + 1); CHECK(output->generated_lifetime_j == prior_generation + 9450);
				CHECK(output->consumed_lifetime_j == prior_use + 135); CHECK(GetUtilityTownPowerBonus(TownID{0}) == 25);
				CHECK(GetUtilityPowerProducer(CaptureGeothermalCollectors().front().heat_producer)->consumed_lifetime_j == prior_heat_use + 27000);
				SaveFirstPlantMagma(paths, "retried");
			} else if (action == "deliver") {
				REQUIRE_FALSE(head->vehstatus.Test(VehState::Crashed)); uint32_t aboard = 0;
				for (const Train *part = head; part != nullptr; part = part->Next()) aboard += part->cargo.StoredCount(); // Native holds exclusively own remaining transported construction units.
				if (!outbound && aboard != 0 && GetTileLayer(head->tile) == MapLayer::Surface && !head->current_order.IsType(OT_LOADING)) { CheckFirstPlantMagmaMaterials(origin); SaveFirstPlantMagma(paths, "outbound"); outbound = true; } // Retain actual loaded departure before either paid shaft.
				if (!partial && generator->work_ticks != 0 && generator->work_ticks < 888) { CheckFirstPlantMagmaMaterials(origin); SaveFirstPlantMagma(paths, "partial-construction"); partial = true; } // Labour must be earned by the native work owner after actual delivery.
				if (!IsGeothermalGeneratorConstructionComplete(generator->industry)) return;
				if (!withdrawing && aboard == 0 && GetTileLayer(head->tile) == MapLayer::Surface && head->last_station_visited == origin && !head->current_order.IsType(OT_LOADING)) { REQUIRE(Command<Commands::SendVehicleToDepot>::Do(DoCommandFlag::Execute, carrier, DepotCommandFlags{}, VehicleListIdentifier{}).Succeeded()); withdrawing = true; } // Actual empty return precedes ordinary depot withdrawal; no train is teleported.
				if (!head->IsStoppedInDepot()) return;
				CheckFirstPlantMagmaMaterials(origin); CHECK(CargoPacket::GetNumItems() == 0); SaveFirstPlantMagma(paths, "construction-complete");
			} else if (action == "wire") {
				if (generator->electrical_producer.base() == UINT32_MAX) return;
				ConnectFirstPlantMagma(*generator, through_command); SaveFirstPlantMagma(paths, "connected");
			} else {
				const auto *source = GetUtilityPowerProducer(generator->electrical_producer); REQUIRE(source != nullptr); const auto uses = source->consumed_lifetime_j / 135;
				CHECK(source->consumed_lifetime_j % 135 == 0); CHECK(uses <= 74); CHECK(GetUtilityTownPowerBonus(TownID{0}) == 25);
				if (uses != 37 && uses < 74) return; // Keep one genuine interrupted native useful-service original before the complete74-use comparison.
				SaveFirstPlantMagma(paths, uses == 37 ? "partial-generation" : "generation-complete");
				fmt::print("FIRST_PLANT_MAGMA_ENERGY uses={} generated={} consumed={} service_ticks={} service_remainder={}\n", uses, source->generated_lifetime_j, source->consumed_lifetime_j, generator->active_service_ticks, generator->service_remainder);
				if (uses == 37) return;
			}
			fmt::print("FIRST_PLANT_MAGMA_READY action={} frames={} spent={} cash={}\n", action, frames, (before - Company::Get(_current_company)->money).base(), Company::Get(_current_company)->money.base());
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	};
	AutoRestoreBackup startup{_magma_start, &begin}; AutoRestoreBackup tick{_magma_frame, &frame};
	IConsole::CmdRegister("first_plant_magma", StartFirstPlantMagma);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "first-plant-magma:ticks=8005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
