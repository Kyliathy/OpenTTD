/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_access.cpp Genuine native credit and coordinated Surface spoil/rail/portal construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.2.
 * Original commands own all cash, debt, industry, infrastructure and pair state.
 * A local geometric candidate supplies no excavation permission or saved work.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../industry_map.h"
#include "../multilayer_industry.h"
#include "../spoil_receiver.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../rail_cmd.h"
#include "../rail.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_area_preview.h"
#include "../tbm_exchange.h"
#include "../underground_vehicle.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_access_observer = nullptr; ///< Current original-world callback, never a finance or terrain owner.

/** Invoke this actual original game's access preparation.
 * @param args Parameter-free native console command.
 * @return True after the current native callback ran.
 */
static bool ObserveSurfaceAccess(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_access_observer != nullptr);
	(*surface_access_observer)(); return true;
}

/** Serialize actual native owners without a simulation tick.
 * @param path Fresh invocation-owned output, never the original input.
 */
static void SaveSurfaceAccess(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Execute or estimate an original synchronized command with normal settlement.
 * @tparam Tcmd Original native command identity.
 * @tparam Targs Typed complete native command arguments.
 * @param location Actual action tile, or zero for coordinate-free banking.
 * @param estimate True performs the native pure query only.
 * @param args Complete original arguments, including a tile when the command owns one.
 * @return Original result and its actual native invoice.
 */
template <Commands Tcmd, typename... Targs>
static auto SurfaceAccessCommand(TileIndex location, bool estimate, Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate,
		location, std::make_tuple(args...));
	INFO("access command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	if (!estimate) fmt::print("SURFACE_ACCESS_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), location.base(), ExtractCommandCost(result).GetCost().base());
	return result;
}

/** Clip a proposed straight Surface access step before any command sees it.
 * @param tile Original Surface anchor.
 * @param direction Native cardinal map direction.
 * @param distance Positive physical distance in tiles.
 * @return Actual Surface tile, or INVALID_TILE for a fence/layer escape.
 */
static TileIndex SurfaceAccessStep(TileIndex tile, DiagDirection direction, uint distance)
{
	const auto step = TileIndexDiffCByDiagDir(direction);
	const auto result = TileAddWrap(tile, step.x * distance, step.y * distance);
	if (!MultiLayerMap::IsTileInLayer(result, MapLayer::Surface)) return INVALID_TILE;
	return result;
}

/** Ephemeral whole infrastructure candidate; actual public commands remain the sole placement/finance authority. */
struct SurfaceAccessLayout {
	TileIndex portal = INVALID_TILE; ///< Proposed real Surface housing, not a saved portal record.
	DiagDirection shaft = DiagDirection::NE; ///< Native upper housing orientation.
	TileIndex station = INVALID_TILE; ///< Actual proposed platform's minimum coordinate anchor.
	Axis axis = Axis::X; ///< Original rail platform/corridor axis.
	uint distance = 0; ///< Portal-to-near-platform physical separation.
	TileIndex depot = INVALID_TILE; ///< Proposed original paid Surface rail depot.
	std::array<TileIndex, 2> receivers{INVALID_TILE, INVALID_TILE}; ///< Independent Earth/Rock command candidates.
	Money capital{0}; ///< Pure whole infrastructure quote, excluding future machinery/hauler purchases.
};

/** Read actual possible receiver footprint cells for conservative nonoverlap planning.
 * @param anchor Proposed original Surface industry anchor.
 * @param type Original native receiver definition.
 * @return Union of every actual layout's cells, or empty for a wrapped/invalid shape.
 */
static std::set<TileIndex> SurfaceReceiverFootprint(TileIndex anchor, IndustryType type)
{
	std::set<TileIndex> result;
	// Native funding may try more than one layout. Protect every real candidate
	// cell so no selected variant can consume the planned rail/source envelope.
	for (const auto &layout : GetIndustrySpec(type)->layouts) for (const auto &cell : layout) {
		const auto tile = TileAddWrap(anchor, cell.ti.x, cell.ti.y);
		if (!MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface)) return {};
		result.insert(tile);
	}
	return result;
}

/** Query paired original disposal sites within a real proposed Surface station catchment.
 * @param platform Four actual planned station cells, not an invented catchment owner.
 * @param reserved Protected source/portal/rail/depot cells.
 * @param sites Receives exact admitted Earth/Rock anchors on success.
 * @param total Receives their actual summed pure native quotes.
 * @return Whether both independently fundable, nonoverlapping actual receivers fit.
 */
static bool FindSurfaceDisposalSites(const std::array<TileIndex, 4> &platform, const std::set<TileIndex> &reserved,
	std::array<TileIndex, 2> &sites, Money &total)
{
	/** One actual pure receiver candidate; no facility or inventory is created by this record. */
	struct Candidate {
		TileIndex tile; ///< Original native receiver anchor.
		std::set<TileIndex> cells; ///< Actual possible layout cells, used only for conservative geometric filtering.
		Money cost; ///< Original command's pure whole quote.
	};
	std::array<std::vector<Candidate>, 2> candidates;
	// The real station catches nearby industry cells. Native BuildIndustry still
	// decides complete terrain, original technology and whole placement legality;
	// this local filter only protects the original game and planned rail access.
	for (uint index = 0; index < candidates.size(); ++index) {
		const auto type = GetSpoilReceiverIndustryType(static_cast<SpoilReceiverKind>(index));
		for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
			if (!IsTileType(tile, TileType::Clear)) continue;
			const auto cells = SurfaceReceiverFootprint(tile, type); if (cells.empty()) continue;
			bool conflict = false, nearby = false;
			for (TileIndex cell : cells) {
				if (reserved.contains(cell)) { conflict = true; break; }
				if (!IsTileType(cell, TileType::Clear)) { conflict = true; break; }
				for (TileIndex stop : platform) {
					if (std::max(std::abs(int(TileX(stop)) - int(TileX(cell))), std::abs(int(TileY(stop)) - int(TileY(cell)))) <= 4) nearby = true;
				}
			}
			// A native placement outside this actual station's future catchment
			// cannot provide origin admission, even if its funding quote succeeds.
			if (conflict || !nearby) continue;
			const auto quote = CmdBuildIndustry({}, tile, type, 0, true, 123);
			if (quote.Failed()) continue;
			candidates[index].push_back({tile, cells, quote.GetCost()});
		}
	}
	// Backtrack actual legal receiver pairs; the nearest first site must not
	// silently consume the only legal complementary material-disposal footprint.
	for (const auto &earth : candidates[0]) for (const auto &rock : candidates[1]) {
		if (std::ranges::any_of(earth.cells, [&rock](TileIndex cell) { return rock.cells.contains(cell); })) continue;
		sites = {earth.tile, rock.tile}; total = earth.cost + rock.cost; return true;
	}
	return false;
}

/** Select a complete real Surface access layout near the original finite Salt seam.
 * @return Pure command-admitted housing, station, depot, rail and receiver geometry; no spending permission is cached.
 */
static SurfaceAccessLayout FindSurfaceAccessLayout()
{
	const GeologyDeposit *salt = nullptr;
	for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
		if (deposit.resource != CargoLabel{"SALT"}) continue;
		if (deposit.layer == MapLayer::Underground) { salt = &deposit; break; }
	}
	REQUIRE(salt != nullptr); REQUIRE(!salt->spans.empty());
	const auto x = salt->spans.front().first_x, y = salt->spans.front().y;
	std::vector<TileIndex> candidates;
	// Preserve existing Surface owners; only native empty ground can host this
	// new source envelope before any actual placement command is considered.
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) if (IsTileType(tile, TileType::Clear)) candidates.push_back(tile);
	/** @param tile Actual candidate anchor. @return Geological approach distance then original stable tile identity. */
	const auto rank = [x, y](TileIndex tile) { return std::make_pair(std::abs(int(TileX(tile)) - int(x)) + std::abs(int(TileY(tile)) - int(y)), tile.base()); };
	std::ranges::sort(candidates, {}, rank);
	const auto portal_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildLayerPortal>());
	const auto station_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailStation>());
	const auto depot_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailDepot>());
	const auto rail_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailLong>());
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(80);
	// Protect the actual source assembly and original terrain before considering
	// each complete native station/rail/depot arrangement. Only the housing needs
	// native flat placement; commissioning itself accepts clear or owned plain
	// rail across its3x3Surface envelope, without invented flat-floor rules.
	for (TileIndex tile : candidates) {
		REQUIRE(std::chrono::steady_clock::now() < deadline);
		const auto envelope = GetExcavationCutterEnvelope(tile); if (envelope.size() != 9) continue;
		if (std::ranges::any_of(envelope, [](TileIndex cell) { return !IsTileType(cell, TileType::Clear); })) continue;
		for (const auto shaft : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto portal = CmdBuildLayerPortal(portal_flags, tile, TransportType::Rail, MapLayer::Underground, shaft, RAILTYPE_RAIL, ROADTYPE_ROAD);
			if (portal.Failed()) continue;
			const auto door = ReverseDiagDir(shaft); const auto axis = DiagDirToAxis(door);
			for (uint distance = 3; distance <= 8; ++distance) {
				std::array<TileIndex, 4> platform;
				std::set<TileIndex> reserved(envelope.begin(), envelope.end()); bool valid = true;
				// The complete straight route is real empty Surface terrain. This
				// protects every earlier factory, road and parked product vehicle.
				for (uint step = 1; step <= distance + 6; ++step) {
					const auto cell = SurfaceAccessStep(tile, door, step);
					if (cell == INVALID_TILE) { valid = false; break; }
					if (!IsTileType(cell, TileType::Clear)) { valid = false; break; }
					reserved.insert(cell);
				}
				if (!valid) continue;
				for (uint index = 0; index < platform.size(); ++index) platform[index] = SurfaceAccessStep(tile, door, distance + index);
				const auto anchor = std::min(platform.front(), platform.back());
				const auto depot = SurfaceAccessStep(tile, door, distance + 6);
				const auto station = CmdBuildRailStation(station_flags, anchor, RAILTYPE_RAIL, axis, 1, 4, STAT_CLASS_DFLT, 0, NEW_STATION, true);
				if (station.Failed()) continue;
				const auto service = CmdBuildTrainDepot(depot_flags, depot, RAILTYPE_RAIL, shaft); if (service.Failed()) continue;
				const auto first = CmdBuildRailroadTrack(rail_flags, SurfaceAccessStep(tile, door, distance - 1), SurfaceAccessStep(tile, door, 1), RAILTYPE_RAIL, AxisToTrack(axis), false, true);
				if (first.Failed()) continue;
				const auto second = CmdBuildRailroadTrack(rail_flags, SurfaceAccessStep(tile, door, distance + 5), SurfaceAccessStep(tile, door, distance + 4), RAILTYPE_RAIL, AxisToTrack(axis), false, true);
				if (second.Failed()) continue;
				SurfaceAccessLayout result{tile, shaft, anchor, axis, distance, depot}; Money receiver_cost{0};
				if (!FindSurfaceDisposalSites(platform, reserved, result.receivers, receiver_cost)) continue;
				result.capital = portal.GetCost() + station.GetCost() + service.GetCost() + first.GetCost() + second.GetCost() + receiver_cost;
				return result;
			}
		}
	}
	FAIL("No whole actual source/disposal/rail layout admitted without terrain modification"); return {};
}

/** Fund the complete original Surface infrastructure without supplying rock floor or construction progress.
 * @param plan Freshly selected actual whole placement candidate.
 */
static void BuySurfaceAccessInfrastructure(const SurfaceAccessLayout &plan)
{
	const auto cash = Company::Get(_current_company)->money; Money paid{0};
	// Independent real receiver purchases precede their native construction.
	// Their footprints were kept out of every source/rail cell; each original
	// synchronized command still rechecks current placement and full finance.
	for (uint index = 0; index < plan.receivers.size(); ++index) {
		const auto kind = static_cast<SpoilReceiverKind>(index); const auto type = GetSpoilReceiverIndustryType(kind);
		const auto anchor = plan.receivers[index];
		paid += SurfaceAccessCommand<Commands::BuildIndustry>(anchor, false, anchor, type, uint32_t{0}, true, uint32_t{123}).GetCost();
		const auto *industry = Industry::GetByTile(anchor); REQUIRE(industry->founder == _current_company);
		CHECK_FALSE(IsLayerIndustryComplete(*industry));
		fmt::print("SURFACE_ACCESS_RECEIVER kind={} id={} type={} anchor={} complete=false\n", index, industry->index.base(), type, anchor.base());
	}
	paid += SurfaceAccessCommand<Commands::BuildRailStation>(plan.station, false, plan.station, RAILTYPE_RAIL, plan.axis,
		uint8_t{1}, uint8_t{4}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true).GetCost();
	const auto origin = GetStationIndex(plan.station); REQUIRE(Station::Get(origin)->owner == _current_company);
	paid += SurfaceAccessCommand<Commands::BuildRailDepot>(plan.depot, false, plan.depot, RAILTYPE_RAIL, plan.shaft).GetCost();
	paid += SurfaceAccessCommand<Commands::BuildLayerPortal>(plan.portal, false, plan.portal, TransportType::Rail,
		MapLayer::Underground, plan.shaft, RAILTYPE_RAIL, ROADTYPE_ROAD).GetCost();
	const auto door = ReverseDiagDir(plan.shaft);
	paid += SurfaceAccessCommand<Commands::BuildRailLong>(SurfaceAccessStep(plan.portal, door, plan.distance - 1), false,
		SurfaceAccessStep(plan.portal, door, plan.distance - 1), SurfaceAccessStep(plan.portal, door, 1), RAILTYPE_RAIL, AxisToTrack(plan.axis), false, true).GetCost();
	paid += SurfaceAccessCommand<Commands::BuildRailLong>(SurfaceAccessStep(plan.portal, door, plan.distance + 5), false,
		SurfaceAccessStep(plan.portal, door, plan.distance + 5), SurfaceAccessStep(plan.portal, door, plan.distance + 4), RAILTYPE_RAIL, AxisToTrack(plan.axis), false, true).GetCost();
	CHECK(paid == plan.capital); CHECK(Company::Get(_current_company)->money == cash - paid);
	CHECK(GetLayerPortalWorkState(plan.portal) == LayerPortalWorkState::Sealed);
	LayerPortalCommissionRequest request;
	request.portal = plan.portal; request.earth_wagons = 2; request.rock_wagons = 2;
	request.proposal.company = _current_company; request.proposal.origin = origin; request.proposal.model_id = 0;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(plan.portal);
	request.proposal.pass_terms.emplace(); // Positive separate proposed capacities only; actual commissioning derives the real purchased consist.
	const auto proof = IssueLayerPortalBootstrap(request);
	INFO("bootstrap refusal=" << to_underlying(proof.error)); REQUIRE(proof.error == LayerPortalWorkError::None);
	// An accepted source proof is never an excavated destination. The sole
	// native first-cut owner must still earn every actual solid pocket cell.
	REQUIRE(proof.pocket.size() == 9);
	for (TileIndex tile : proof.pocket) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid);
	CHECK(CaptureTBMProjects().empty());
	fmt::print("SURFACE_ACCESS_INFRASTRUCTURE_COMPLETE origin={} depot={} portal={} target={} expense={} source_proof=true solid_pocket=9\n",
		origin.base(), plan.depot.base(), plan.portal.base(), proof.destination.base(), paid.base());
}

/** Purchase and assign actual empty machinery at the genuine completed source facilities.
 * The fingerprint-bound public commission owns the actual route, claims and hidden cutter assembly;
 * ordinary depot purchases/coupling own the spare hopper train. No part is moved by the observer.
 */
static void BuySurfaceAccessMachinery()
{
	const TileIndex portal{1949}, depot{1373}; const StationID origin{4};
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(CaptureTBMProjects().empty()); REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	const auto target = GetLayerPortalPairTile(portal);
	const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(target));
	const auto first = TileAddWrap(target, step.x, step.y), last = TileAddWrap(target, step.x * 3, step.y * 3);
	const auto preview = PreviewTBMCommissionArea(_current_company, portal, 0, 2, 2, origin, first, last, false);
	INFO("complete genuine commissioning quote error=" << preview.purchase.GetErrorMessage().base());
	REQUIRE(preview.purchase.Succeeded()); CHECK_FALSE(preview.area.partial);
	fmt::print("SURFACE_ACCESS_COMMISSION_QUOTE portal={} target={} first={} last={} price={} route={} cells={} earth={} rock={}\n",
		portal.base(), target.base(), first.base(), last.base(), preview.purchase.GetCost().base(), preview.area.plan.route.size(),
		preview.area.totals.new_cells, preview.area.totals.earth_t, preview.area.totals.rock_t);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureTBMProjects().empty());
	const auto paid = SurfaceAccessCommand<Commands::ConfirmTBMCommission>(portal, false, portal, uint16_t{0}, uint8_t{2}, uint8_t{2},
		origin, first, last, preview.fingerprint, ExcavationPlanIntent::Area);
	CHECK(paid.GetCost() == preview.purchase.GetCost());
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto project = projects.front();
	CHECK(project.portal == portal); CHECK(project.origin == origin); CHECK(project.exchange_bay == portal);
	CHECK(project.phase == TBMPhase::Commissioning); CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	const auto *cutter = Train::Get(project.cutter); CHECK(cutter->vehstatus.Test(VehState::Stopped));
	CHECK(InspectTBMConsist(*cutter, project).error == TBMBlockedReason::None);
	Money total = paid.GetCost(); Train *head = nullptr;
	// Buy each actual empty native part once, preserving engine/cargo ownership.
	// A default auto-attachment is accepted only when it is already this hauler;
	// otherwise the normal whole-unit coupling command attaches the paid wagon.
	for (uint16_t model : {4, 2, 2, 3, 3}) {
		const auto engine = GetUndergroundEngine(model); REQUIRE(engine != EngineID::Invalid());
		const auto [invoice, id, capacity, mail, refit] = SurfaceAccessCommand<Commands::BuildVehicle>(depot, false,
			depot, engine, false, INVALID_CARGO, ClientID::Server);
		total += invoice.GetCost(); auto *part = Train::Get(id); CHECK(part->cargo.TotalCount() == 0);
		if (head == nullptr) { head = part; REQUIRE(head->IsFrontEngine()); }
		else if (part->First() != head) SurfaceAccessCommand<Commands::MoveRailVehicle>(depot, false, id, head->Last()->index, false);
		fmt::print("SURFACE_ACCESS_PART model={} engine={} id={} capacity={} cost={}\n", model, engine.base(), id.base(), capacity, invoice.GetCost().base());
	}
	REQUIRE(head != nullptr); CHECK(head->IsStoppedInDepot()); CHECK(head->tile == depot);
	uint hauler_parts = 0;
	// The paid haul set must contain the one locomotive and all four empty
	// dedicated hoppers; ordinary coupling cannot lose or duplicate a part.
	for (const Train *part = head; part != nullptr; part = part->Next()) { ++hauler_parts; CHECK(part->cargo.TotalCount() == 0); }
	CHECK(hauler_parts == 5);
	SurfaceAccessCommand<Commands::AssignTBMHauler>(portal, false, project.id, head->index);
	Order delivery; delivery.MakeGoToStation(origin);
	SurfaceAccessCommand<Commands::InsertOrder>(depot, false, head->index, VehicleOrderID{0}, delivery);
	CHECK(Company::Get(_current_company)->money == cash - total);
	const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr); CHECK(job->cursor == 0);
	// Commissioning reserves solid work for this real paid cutter. Reservation
	// is not earned floor: every authorized pocket cell must remain unexcavated,
	// and every physical wagon must still be empty before the first native tick.
	for (TileIndex tile : GetExcavationCutterEnvelope(target)) CHECK(GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated);
	for (const Train *part = cutter; part != nullptr; part = part->Next()) CHECK(part->cargo.TotalCount() == 0);
	fmt::print("SURFACE_ACCESS_MACHINERY_COMPLETE project={} job={} cutter={} hauler={} expense={} cash={} first_cut=false\n",
		project.id.base(), project.job.base(), project.cutter.base(), head->index.base(), total.base(), Company::Get(_current_company)->money.base());
}

/** Real bank debt, pure complete layout queries and normally funded Surface infrastructure preserve original products. */
TEST_CASE("Authentic Salt access uses native credit and coordinated paid original infrastructure", "[.surface-entry-access]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"), *mode = std::getenv("OTTD_SURFACE_ACCESS_PHASE"); REQUIRE(source != nullptr); REQUIRE(mode != nullptr);
	const std::string_view phase{mode}; REQUIRE((phase == "credit" || phase == "plan" || phase == "infrastructure" || phase == "machinery"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-access");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Propagate genuine native assertion failures through ordinary shutdown without authoring compensation or replacement stock. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company);
			SaveSurfaceAccess(paths.saves / fmt::format("surface-access-before-{}-440.sav", phase));
			if (phase == "credit") {
				// Bind this repeatable loan operation to the exact original bank
				// state. A restored credited output must never borrow a second time.
				REQUIRE(company->money == 699555); REQUIRE(company->current_loan == 2000000); REQUIRE(company->GetMaxLoan() == 5000000);
				const auto expenses = company->yearly_expenses;
				CHECK(SurfaceAccessCommand<Commands::IncreaseLoan>(TileIndex{0}, true, LoanCommand::Amount, Money{1500000}).GetCost() == 0);
				CHECK(company->money == 699555); CHECK(company->current_loan == 2000000);
				SaveSurfaceAccess(paths.saves / "surface-access-credit-query-440.sav");
				CHECK(SurfaceAccessCommand<Commands::IncreaseLoan>(TileIndex{0}, false, LoanCommand::Amount, Money{1500000}).GetCost() == 0);
				CHECK(company->money == 2199555); CHECK(company->current_loan == 3500000); CHECK(company->yearly_expenses == expenses);
			} else if (phase == "machinery") {
				REQUIRE(company->current_loan == 3500000); BuySurfaceAccessMachinery();
			} else {
				const auto plan = FindSurfaceAccessLayout(); REQUIRE(plan.portal != INVALID_TILE);
				fmt::print("SURFACE_ACCESS_PLAN portal={} shaft={} station={} axis={} distance={} depot={} earth={} rock={} capital={}\n",
					plan.portal.base(), to_underlying(plan.shaft), plan.station.base(), to_underlying(plan.axis), plan.distance,
					plan.depot.base(), plan.receivers[0].base(), plan.receivers[1].base(), plan.capital.base());
				if (phase == "infrastructure") {
					REQUIRE(company->current_loan == 3500000); REQUIRE(company->money == 2199555);
					BuySurfaceAccessInfrastructure(plan);
				}
			}
			SaveSurfaceAccess(paths.saves / fmt::format("surface-access-{}-440.sav", phase));
			fmt::print("SURFACE_ACCESS_COMPLETE phase={} cash={} debt={} supplied_floor=false\n", phase, company->money.base(), company->current_loan.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_access_observer, &inspect); IConsole::CmdRegister("surface_entry_access", ObserveSurfaceAccess);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
