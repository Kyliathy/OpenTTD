/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_native.cpp Complete native file/preview restoration over genuine retained worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CCC/13CCD.
 * Original persistence phases declare metadata only. Paid phases use actual synchronized commands and native station ingress in a declared retained world; no phase supplies an unassisted or final factory predecessor.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_order_wire.h"
#include "../surface_supply_cmd.h"
#include "../company_func.h"
#include "../company_base.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../fios.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../command_func.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../multilayer_bootstrap.h"
#include "../cargotype.h"
#include "../timer/timer_game_tick.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *supply_order_observer = nullptr; ///< Non-owning invocation callback; no gameplay owner or tick permission is introduced.

/** Run the original production station tick for explicit paid-file continuation; no alternate cargo ingress is introduced. */
extern void OnTick_Station();

/** Save actual original owners without ticking or acquiring a new expense/right.
 * @param path Explicit invocation-owned native output.
 */
static void SaveSurfaceSupplyFile(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param label Original canonical byte sequence. @return Its synchronized command ABI value, independent of host byte order. */
static uint32_t SurfaceSupplyFileLabel(CargoLabel label)
{
	return (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
}

/** Exercise actual original top-level finance rather than directly changing money or calling a raw command body.
 * @param origin Actual receiving station.
 * @param label Canonical stable cargo meaning.
 * @param quantity Complete purchase.
 * @param intent Original monotonic identity, including exact saved retries.
 * @return Native complete cost/result.
 */
static CommandCost PurchaseSurfaceSupplyFile(StationID origin, CargoLabel label, uint16_t quantity, SurfaceSupplyOrderID intent)
{
	return Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
		std::make_tuple(origin, SurfaceSupplyFileLabel(label), quantity, intent));
}

/** @param args Parameter-free native post-load console invocation. @return Whether the actual restored-world observer ran. */
static bool ObserveSurfaceSupplyOrderFile(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(supply_order_observer != nullptr); (*supply_order_observer)(); return true;
}

/** Find a real owned Surface freight origin or build one through the actual paid station command.
 * @return Original native receiving station; no pool/tile/owner is invented by a test-only constructor.
 */
static Station &SurfaceSupplyFileOrigin()
{
	// Existing normal Surface loading facilities are genuine native references; matching coordinates in another layer and airport/bus-only sites cannot supply procurement admission.
	for (auto *station : Station::Iterate()) {
		if (station->owner == _current_company && GetTileLayer(station->xy) == MapLayer::Surface && station->facilities.Any({StationFacility::Train, StationFacility::TruckStop})) return *station;
	}
	uint quoted = 0;
	// A bounded query-only scan finds a native one-cell rail footprint. Only its successful command can create/own the station and debit actual construction capital.
	for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		if (TileX(tile) < 3 || TilePlaneY(tile) < 3) continue; // Keep original world-edge fences out of this component's receiving facility search.
		if (++quoted > 4096) break;
		const auto estimate = CmdBuildRailStation({}, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false);
		if (estimate.Failed()) continue;
		const auto cash = Company::Get(_current_company)->money;
		const auto result = Command<Commands::BuildRailStation>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
			std::make_tuple(tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false));
		REQUIRE(result.Succeeded()); CHECK(Company::Get(_current_company)->money == cash - result.GetCost());
		return *Station::Get(GetStationIndex(tile));
	}
	FAIL("No actual owned Surface freight origin was admitted for the native persistence fixture");
	throw std::exception();
}

/** Stage explicit persistence-only metadata using the already loaded world's original native facts.
 * @param origin Real same-world owned Surface station; no supplied construction inventory is created.
 * @return Declared four-state metadata, never actual supply spending/stock or a procurement receipt.
 */
static SurfaceSupplyOrderTransport SurfaceSupplyFileDeclarations(const Station &origin)
{
	const std::array<CargoLabel, 4> labels{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}};
	const std::array<uint16_t, 4> quantities{2, 3, 1, 1};
	SurfaceSupplyOrderTransport transport; transport.snapshot.next_id = 5; transport.saved_day = TimerGameEconomy::date;
	// These exact states exercise only persistence/reference semantics. Quotes do not pay; even the fulfilled packet index is explicitly historical and creates no native packet or refund right.
	for (size_t index = 0; index < labels.size(); ++index) {
		const SurfaceSupplyOrderRequest request{origin.index, _current_company, labels[index], quantities[index]};
		const auto view = CaptureSurfaceSupplyOrderView(request, 0); const auto quote = PrepareSurfaceSupplyOrder(request, view); REQUIRE(quote.error == SurfaceSupplyOrderError::None);
		SurfaceSupplyOrderRecord record; record.id = index + 1; record.purchaser = record.owner = _current_company; record.origin = origin.index;
		record.origin_x = static_cast<uint16_t>(TileX(origin.xy)); record.origin_y = static_cast<uint16_t>(TilePlaneY(origin.xy));
		record.label = labels[index]; record.quantity = quantities[index]; record.cost = quote.cost; record.inflation_prices = view.inflation_prices;
		record.purchase_year = view.year; record.purchase_day = transport.saved_day; record.packet_buyer = _current_company.base(); record.state = static_cast<SurfaceSupplyOrderState>(index);
		if (record.state == SurfaceSupplyOrderState::Fulfilled) record.issued_packet = CargoPacketID{17};
		if (record.state == SurfaceSupplyOrderState::Lost) { record.owner = CompanyID::Invalid(); record.packet_buyer = Source::Invalid; }
		transport.snapshot.records.push_back(record);
	}
	return transport;
}

/** Count bought stock only in its original physical native station container.
 * @param order Original paid receipt; its historical packet ID grants no live lookup/ownership authority.
 * @return Whole actual non-paying units at this receipt's origin and cargo meaning; no saved/parallel stock counter is introduced.
 */
static uint32_t CountBoughtSurfaceSupplyFile(const SurfaceSupplyOrderRecord &order)
{
	const auto &goods = Station::Get(order.origin)->goods[GetCargoTypeByLabel(order.label)];
	if (!goods.HasData()) return 0;
	uint32_t count = 0; const auto *packets = goods.GetData().cargo.Packets();
	// Native split/merge may retire issued IDs; traverse original custody and require retained non-paying source/buyer/zero-credit rather than recreating packets from history.
	for (StationCargoList::ConstIterator it(packets->begin()); it != packets->end(); ++it) {
		if (!(*it)->GetSource().IsCompanySupply()) continue;
		CHECK((*it)->GetSource().id == order.packet_buyer); CHECK((*it)->GetFeederShare() == 0); CHECK((*it)->GetFirstStation() == order.origin); count += (*it)->Count();
	}
	return count;
}

/** Continue actual paid commitments through original complete native load/save/copied-preview and station custody.
 * @param phase Bounded paid-prepare, paid-restore or paid-resave operation.
 * @param paths Explicit current invocation outputs.
 * @param source Immutable original loaded native input.
 * Existing world stock is declared ancestry; only new command/packet deltas prove procurement, never final Surface factory production.
 */
static void ObservePaidSurfaceSupplyFile(std::string_view phase, const TestArtifactPaths &paths, const char *source)
{
	auto *company = Company::Get(_current_company); auto before = company->money; const auto expense = company->yearly_expenses[0][ExpensesType::Property];
	Money charged{0}, refunded{0};
	const std::array<CargoLabel, 4> labels{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}};
	const std::array<uint16_t, 4> quantities{2, 3, 1, 1};
	if (phase == "paid-prepare") {
		REQUIRE(CaptureSurfaceSupplyOrders().records.empty()); REQUIRE(GetNextSurfaceSupplyOrderID() == 1);
		Station &origin = SurfaceSupplyFileOrigin(); const auto baseline = company->money; const auto baseline_expense = company->yearly_expenses[0][ExpensesType::Property];
		before = baseline; // Separate genuine station construction from the following four actual supply expenses.
		SaveSurfaceSupplyFile(paths.saves / "surface-supply-paid-baseline-440.sav"); const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
		// Every scheduled material is bought through the actual original command finance kernel. No pending right may create usable stock or allocate a physical packet before station dispatch.
		for (size_t index = 0; index < labels.size(); ++index) {
			const auto stock = origin.goods[GetCargoTypeByLabel(labels[index])].TotalCount();
			const auto result = PurchaseSurfaceSupplyFile(origin.index, labels[index], quantities[index], index + 1); REQUIRE(result.Succeeded()); charged += result.GetCost();
			CHECK(GetSurfaceSupplyOrder(index + 1)->state == SurfaceSupplyOrderState::Pending); CHECK(origin.goods[GetCargoTypeByLabel(labels[index])].TotalCount() == stock);
		}
		CHECK(company->money == baseline - charged); CHECK(company->yearly_expenses[0][ExpensesType::Property] == baseline_expense + charged); CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]); SaveSurfaceSupplyFile(paths.saves / "surface-supply-paid-pending-440.sav");
	} else {
		REQUIRE(GetNextSurfaceSupplyOrderID() == 5); REQUIRE(CaptureSurfaceSupplyOrders().records.size() == 4);
		const auto original = EncodeSurfaceSupplyOrders({CaptureSurfaceSupplyOrders(), TimerGameEconomy::date});
		REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CHECK(EncodeSurfaceSupplyOrders(_load_check_data.surface_supply_orders) == original);
		// Saved exact intent retries have no new charge/cargo authority. This is an actual restored paid file, distinct from older declared metadata transport fixtures.
		for (SurfaceSupplyOrderID id = 1; id < 4; ++id) {
			const auto record = *GetSurfaceSupplyOrder(id); const auto retry = PurchaseSurfaceSupplyFile(record.origin, record.label, record.quantity, id);
			REQUIRE(retry.Succeeded()); CHECK(retry.GetCost() == 0);
		}
		CHECK(company->money == before);
		if (phase == "paid-restore") {
			const auto record = *GetSurfaceSupplyOrder(4); REQUIRE(record.state == SurfaceSupplyOrderState::Pending);
			const auto result = Command<Commands::CancelSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE, std::make_tuple(SurfaceSupplyOrderID{4}));
			REQUIRE(result.Succeeded()); refunded = record.cost; CHECK(result.GetCost() == -refunded); CHECK(company->money == before + refunded);
			SaveSurfaceSupplyFile(paths.saves / "surface-supply-paid-cancelled-440.sav");
			std::array<uint32_t, 4> stocks{};
			// Capture only actual native station counts as read-only deltas, never a parallel inventory. The cancelled purchase must not participate in the following real station arrival.
			for (size_t index = 0; index < labels.size(); ++index) stocks[index] = Station::Get(GetSurfaceSupplyOrder(index + 1)->origin)->goods[GetCargoTypeByLabel(labels[index])].TotalCount();
			AutoRestoreBackup counter(TimerGameTick::counter, TimerGameTick::TickCounter{0}); OnTick_Station();
			// Three original whole deliveries become irreversible receipts once; the fourth stays cancelled and owns no usable packet. The real native station owner remains all physical stock.
			for (size_t index = 0; index < labels.size(); ++index) {
				const auto *order = GetSurfaceSupplyOrder(index + 1); const auto stock = Station::Get(order->origin)->goods[GetCargoTypeByLabel(labels[index])].TotalCount();
				CHECK(stock == stocks[index] + (index < 3 ? quantities[index] : 0)); CHECK(order->state == (index < 3 ? SurfaceSupplyOrderState::Fulfilled : SurfaceSupplyOrderState::Cancelled));
				CHECK(CountBoughtSurfaceSupplyFile(*order) == (index < 3 ? quantities[index] : 0));
				CHECK(CmdCancelSurfaceSupplyOrder({}, index + 1).Failed());
			}
			CHECK(company->money == before + refunded); SaveSurfaceSupplyFile(paths.saves / "surface-supply-paid-fulfilled-440.sav");
		} else {
			REQUIRE(phase == "paid-resave");
			// All restored terminal receipts remain non-refundable, including the originally refunded intent; a paused resave must preserve original physical packets and company accounting exactly.
			for (SurfaceSupplyOrderID id = 1; id <= 4; ++id) CHECK(CmdCancelSurfaceSupplyOrder({}, id).Failed());
			// Ordinary full native restoration retains each actually bought packet's non-paying purpose and exact original units; metadata alone cannot satisfy this physical assertion.
			for (size_t index = 0; index < labels.size(); ++index) CHECK(CountBoughtSurfaceSupplyFile(*GetSurfaceSupplyOrder(index + 1)) == (index < 3 ? quantities[index] : 0));
			CHECK(GetSurfaceSupplyOrder(4)->state == SurfaceSupplyOrderState::Cancelled); CHECK(company->money == before);
			SaveSurfaceSupplyFile(paths.saves / "surface-supply-paid-resave-440.sav");
		}
		CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense - refunded);
	}
	REQUIRE(ValidateSurfaceSupplyOrderBook());
	fmt::print("SURFACE_SUPPLY_PAID phase={} before={} after={} charged={} refunded={} native_finance=true original_station_custody=true unassisted=false\n",
		phase, before.base(), company->money.base(), charged.base(), refunded.base());
}

/** Actual native load/save entry; each invocation is isolated and bounded by the external harness, and no iteration advances a dispatch tick. */
TEST_CASE("Native Surface order files preserve metadata and actual paid continuation", "[.surface-supply-order-native]")
{
	const char *source = std::getenv("OTTD_SURFACE_SUPPLY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_SUPPLY_PHASE"); const std::string_view phase = selected ? selected : "inspect";
	const auto paths = GetTestCaseArtifactPaths("space", "surface-supply-order-file"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only after ordinary native reconstruction; supplied persistence fixtures are always explicitly non-procurement. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(Company::IsValidID(_current_company)); REQUIRE(ValidateSurfaceSupplyOrderBook());
			/** @param name Invocation-owned output basename. Serialize actual native owners once; saving grants no elapsed work, packet or financial settlement. */
			auto save = [&](std::string_view name) {
				SaveSurfaceSupplyFile(paths.saves / name);
			};
			if (phase.starts_with("paid-")) {
				ObservePaidSurfaceSupplyFile(phase, paths, source);
			} else if (phase == "prepare") {
				REQUIRE(CaptureSurfaceSupplyOrders().records.empty()); REQUIRE(GetNextSurfaceSupplyOrderID() == 1);
				save("surface-supply-empty-upgrade-440.sav");
				Station &origin = SurfaceSupplyFileOrigin(); const auto declarations = SurfaceSupplyFileDeclarations(origin);
				const auto cash = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
				REQUIRE(StageLoadedSurfaceSupplyOrders(declarations.snapshot, declarations.saved_day)); REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date));
				REQUIRE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount()))); REQUIRE(AcceptLoadedSurfaceSupplyOrders(true));
				CHECK(Company::Get(_current_company)->money == cash); CHECK(CargoPacket::GetNumItems() == packets);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				save("surface-supply-order-fixture-440.sav");
			} else if (phase == "preview-refusal") {
				const char *bad = std::getenv("OTTD_SURFACE_SUPPLY_BAD"); REQUIRE(bad != nullptr);
				const auto before = EncodeSurfaceSupplyOrders({CaptureSurfaceSupplyOrders(), TimerGameEconomy::date}); const auto cash = Company::Get(_current_company)->money;
				const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
				CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(EncodeSurfaceSupplyOrders({CaptureSurfaceSupplyOrders(), TimerGameEconomy::date}) == before);
				CHECK(Company::Get(_current_company)->money == cash); CHECK(CargoPacket::GetNumItems() == packets);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			} else if (phase == "full-refusal") {
				const char *bad = std::getenv("OTTD_SURFACE_SUPPLY_BAD"); REQUIRE(bad != nullptr);
				CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
				CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(GetNextSurfaceSupplyOrderID() == 1);
			} else if (phase == "inspect-empty") {
				CHECK(CaptureSurfaceSupplyOrders().records.empty()); CHECK(GetNextSurfaceSupplyOrderID() == 1);
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.surface_supply_orders_present); CHECK(_load_check_data.surface_supply_orders.snapshot.records.empty());
				save("surface-supply-empty-resave-440.sav");
			} else {
				REQUIRE(phase == "inspect"); REQUIRE(GetNextSurfaceSupplyOrderID() == 5); const auto snapshot = CaptureSurfaceSupplyOrders(); REQUIRE(snapshot.records.size() == 4);
				// Every native state survives exact original restoration. These data were declared solely to test persistence and can never be counted as actually purchased supplies.
				for (size_t index = 0; index < snapshot.records.size(); ++index) CHECK(snapshot.records[index].state == static_cast<SurfaceSupplyOrderState>(index));
				REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.surface_supply_orders_present); CHECK(_load_check_data.surface_supply_orders.snapshot.records.size() == 4);
				save("surface-supply-order-resave-440.sav");
			}
			fmt::print("SURFACE_SUPPLY_ORDER_FILE phase={} native=true paid_continuation={} unassisted=false\n", phase, phase.starts_with("paid-")); observed = true;
		} catch (...) { failure = std::current_exception(); }
		// ReInit tells the normal loader caller to replace this partial world. This terminal observer exits instead, so keep the remainder of the current engine iteration paused until shutdown.
		_pause_mode.Set(PauseMode::Normal);
		_exit_game = true;
	};
	AutoRestoreBackup observer(supply_order_observer, &inspect); IConsole::CmdRegister("surface_supply_order_test", ObserveSurfaceSupplyOrderFile);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
