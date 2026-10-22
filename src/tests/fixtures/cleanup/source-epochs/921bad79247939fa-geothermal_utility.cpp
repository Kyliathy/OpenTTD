/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_utility.cpp Actual command capital, shared identity, native coexistence and deterministic installed-path boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group3B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../transport_tube.h"
#include "../rail.h"
#include "../road.h"
#include "../road_map.h"
#include "../pbs.h"
#include "../train.h"
#include "../safeguards.h"

/** @param index Actual native cell. @return Every explicit packed base/extended tile field, read through the public wrapper without raw arrays or host padding. */
static std::array<uint32_t, 10> NativeUtilityFloor(TileIndex index)
{
	Tile tile{index}; return {tile.type(), tile.height(), tile.m1(), tile.m2(), tile.m3(), tile.m4(), tile.m5(), tile.m6(), tile.m7(), tile.m8()};
}

/** @param tile Explicit native world cell. @param domain Actual installed network. @param estimate Quote without capital or identity changes. @param remove Perform removal instead of construction. @return Actual ordinary command-kernel result and its whole installation/removal charge. */
static CommandCost UtilityCellCommand(TileIndex tile, UtilityEnergyDomain domain, bool estimate = false, bool remove = false)
{
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
}

/** @param rig Actual funded native collector fixture. Complete exactly its real construction receipt/work; supplied test floor/material deliveries remain explicit unit inputs. */
static void CompleteUtilityCollector(GeothermalCollectorRig &rig)
{
	REQUIRE(rig.Fund().Succeeded());
	// Deliver the whole bill through real native stock, then let the existing
	// dispatcher complete paid machinery; no operational boolean is fabricated.
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) REQUIRE(rig.Deliver(input.label, input.quantity) == input.quantity);
	rig.Tick(296); REQUIRE(IsGeothermalCollectorOperational(rig.Collector().index));
}

/** Actual command estimates preserve all owners, while execution settles one frozen bill and never modifies native floor metadata or generates heat. */
TEST_CASE("Paid Deep utility cell uses the shared issuer and preserves native floor", "[geothermal-utility-build]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power;
	const auto tile = TileAddWrap(rig.request.tile, -1, -1); const auto domain = GENERATE(UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity);
	const auto cash = Company::Get(_current_company)->money; const auto native = NativeUtilityFloor(tile);
	const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), digest = ComputeUtilityPowerDigest();
	const auto random = std::array{_random.state[0], _random.state[1]};
	REQUIRE(UtilityCellCommand(tile, domain, true).Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(UtilityCellCommand(tile, domain).Succeeded()); const auto cell = GetGeothermalUtilityLink(tile, domain); REQUIRE(cell.has_value());
	CHECK(cell->id == UtilityLinkID{0}); CHECK(cell->capacity_w == GetMultiLayerLocalUtilityLinkContract(domain)->capacity_w);
	CHECK(Company::Get(_current_company)->money == cash - Money{1000}); CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty());
	CHECK(NativeUtilityFloor(tile) == native);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural); CHECK(std::array{_random.state[0], _random.state[1]} == random);
	const auto built = ComputeUtilityPowerDigest(); CHECK(UtilityCellCommand(tile, domain).Failed()); CHECK(ComputeUtilityPowerDigest() == built);
	REQUIRE(UtilityCellCommand(tile, domain, true, true).Succeeded()); CHECK(ComputeUtilityPowerDigest() == built);
	REQUIRE(UtilityCellCommand(tile, domain, false, true).Succeeded()); CHECK(Company::Get(_current_company)->money == cash - Money{1250});
	CHECK_FALSE(GetGeothermalUtilityLink(tile, domain).has_value()); CHECK_FALSE(GetUtilityPowerLink(cell->id)->active);
	CHECK(UtilityCellCommand(tile, domain, false, true).Failed()); REQUIRE(UtilityCellCommand(tile, domain).Succeeded());
	CHECK(GetGeothermalUtilityLink(tile, domain)->id != cell->id); CHECK(GetUtilityLocalLinkRevision() == 3);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateGeothermalUtilityLinks());
}

/** Refused date/floor/ownership/cash/port/occupancy never issues a link or settles capital; no hidden construction or energy fallback exists. */
TEST_CASE("Deep utility construction refuses locked invalid or unowned input before writes", "[geothermal-utility-build]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power; auto tile = TileAddWrap(rig.request.tile, -1, -1);
	SECTION("locked calendar") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("solid floor") { tile = TileXYL(2, 2, MapLayer::DeepUnderground); }
	SECTION("wrong layer") { tile = TileXYL(10, 10, MapLayer::Underground); }
	SECTION("missing tile") { tile = INVALID_TILE; }
	SECTION("cold machinery floor") { tile = rig.request.tile; }
	SECTION("insufficient cash") { Company::Get(_current_company)->money = 999; }
	SECTION("foreign native track") { MakeRailNormal(tile, CompanyID{1}, TrackBits{Track::X}, RAILTYPE_RAIL); }
	SECTION("reserved native track") { MakeRailNormal(tile, _current_company, TrackBits{Track::X}, RAILTYPE_RAIL); REQUIRE(TryReserveRailTrack(tile, Track::X)); }
	const auto digest = ComputeUtilityPowerDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto cash = Company::Get(_current_company)->money;
	CHECK(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Failed()); CHECK(ComputeUtilityPowerDigest() == digest);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Company::Get(_current_company)->money == cash);
}

/** Utility overlays preserve every underlying road/rail/tube field, and ordinary foreign overbuilding cannot acquire them. */
TEST_CASE("Installed Deep utilities coexist with native transport without replacing metadata", "[geothermal-utility-coexist]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power; const auto tile = TileAddWrap(rig.request.tile, -1, -1);
	ResetRailTypes(); ResetRoadTypes(); // This isolated transport-definition fixture uses the same public reset boundary as existing underground facility fixtures.
	SECTION("ordinary rail") { MakeRailNormal(tile, _current_company, TrackBits{Track::X}, RAILTYPE_RAIL); }
	SECTION("tube guideway") { const auto tube = InitialiseTubeRailType(rig.ActivatedContent()); REQUIRE(tube != INVALID_RAILTYPE); MakeRailNormal(tile, _current_company, TrackBits{Track::X}, tube); }
	SECTION("ordinary road") { ResetRoadTypes(); MakeRoadNormal(tile, ROAD_X, ROADTYPE_ROAD, INVALID_ROADTYPE, TownID::Invalid(), _current_company, OWNER_NONE); }
	const auto native = NativeUtilityFloor(tile);
	REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Succeeded()); REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Electricity).Succeeded());
	CHECK(NativeUtilityFloor(tile) == native);
	CHECK(ValidateGeothermalUtilityLinks());
	_current_company = CompanyID{1}; CHECK_FALSE(CanConstructOverGeothermalUtility(tile)); CHECK(CheckExcavationConstruction(std::array{tile}).Failed());
	_current_company = rig.request.owner; REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat, false, true).Succeeded());
	CHECK(NativeUtilityFloor(tile) == native);
	ResetRailTypes(); ResetRoadTypes();
}

/** Only the real funded machinery boundary port accepts installation; cold and arbitrary industry cells provide no connection. */
TEST_CASE("Deep utility source port resolves actual funded collector identity", "[geothermal-utility-port]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power;
	CHECK_FALSE(GetGeothermalUtilityPortOwner(rig.request.tile).has_value()); CompleteUtilityCollector(rig);
	CHECK(GetGeothermalUtilityPortOwner(rig.request.endpoint) == std::optional<CompanyID>{_current_company});
	CHECK(UtilityCellCommand(TileAddWrap(rig.request.tile, 1, 1), UtilityEnergyDomain::Heat).Failed());
	REQUIRE(UtilityCellCommand(rig.request.endpoint, UtilityEnergyDomain::Heat).Succeeded()); CHECK(ValidateGeothermalUtilityLinks());
	const auto neighbor = TileAddWrap(rig.request.tile, -1, 0); REQUIRE(UtilityCellCommand(neighbor, UtilityEnergyDomain::Heat).Succeeded());
	REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, neighbor).has_value());
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Electricity, rig.request.endpoint, neighbor).has_value());
}

/** Bounded shortest paths use actual same-domain/company installed cells; split/rebuild changes topology while repeated resource queries leave the cache stable. */
TEST_CASE("Deep utility paths are deterministic installed topology with paid recovery", "[geothermal-utility-path]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power;
	const auto a = TileAddWrap(rig.request.tile, -1, -1), b = TileAddWrap(a, 1, 0), c = TileAddWrap(a, 2, 0), d = TileAddWrap(a, 3, 0);
	// Build in a deliberately different ID order. Geometry, not insertion
	// order or catchment, supplies each legal cardinal step and bottleneck.
	for (const auto tile : {d, b, a, c}) REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Succeeded());
	const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, a, d); REQUIRE(path.has_value()); CHECK(path->cells.size() == 4); CHECK(path->capacity_w == 1000000);
	CHECK(path->cells == std::vector<UtilityLinkID>{GetGeothermalUtilityLink(a, UtilityEnergyDomain::Heat)->id, GetGeothermalUtilityLink(b, UtilityEnergyDomain::Heat)->id,
		GetGeothermalUtilityLink(c, UtilityEnergyDomain::Heat)->id, GetGeothermalUtilityLink(d, UtilityEnergyDomain::Heat)->id});
	const auto builds = GetGeothermalUtilityCacheBuildCount(), revision = GetUtilityLocalLinkRevision(); const auto shared = ComputeUtilityPowerDigest();
	REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, a, d).has_value()); CHECK(GetGeothermalUtilityCacheBuildCount() == builds); CHECK(ComputeUtilityPowerDigest() == shared);
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, a, TileXYL(TileX(d), TilePlaneY(d), MapLayer::Surface)).has_value());
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Electricity, a, d).has_value());
	const auto old = GetGeothermalUtilityLink(b, UtilityEnergyDomain::Heat)->id;
	REQUIRE(UtilityCellCommand(b, UtilityEnergyDomain::Heat, false, true).Succeeded()); CHECK(GetUtilityLocalLinkRevision() == revision + 1);
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, a, d).has_value());
	REQUIRE(UtilityCellCommand(b, UtilityEnergyDomain::Heat).Succeeded()); const auto repaired = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, a, d); REQUIRE(repaired.has_value());
	CHECK(repaired->cells[1] != old); CHECK_FALSE(GetUtilityPowerLink(old)->active); CHECK(ValidateGeothermalUtilityLinks());
}

/** Actual native company hooks transfer active infrastructure, retire removed ownership and leave tombstones/cache/history with their original issuer. */
TEST_CASE("Deep utility native ownership and restore preserve physical identity history", "[geothermal-utility-owner]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power; const auto tile = TileAddWrap(rig.request.tile, -1, -1);
	REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Succeeded()); const auto original = GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat)->id;
	const auto captured = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	StageLoadedUtilityPowerState(captured); REQUIRE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == digest);
	REQUIRE(Company::CanAllocateItem()); auto *successor = Company::Create(); successor->money = 1000000; const auto next = successor->index;
	ChangeOwnershipOfCompanyItems(rig.request.owner, next); CHECK(GetUtilityPowerLink(original)->local_owner == next);
	CHECK_FALSE(FindGeothermalUtilityPath(rig.request.owner, UtilityEnergyDomain::Heat, tile, tile).has_value());
	REQUIRE(FindGeothermalUtilityPath(next, UtilityEnergyDomain::Heat, tile, tile).has_value()); CHECK(ValidateGeothermalUtilityLinks());
	ChangeOwnershipOfCompanyItems(next, INVALID_OWNER); CHECK_FALSE(GetUtilityPowerLink(original)->active); CHECK_FALSE(GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value());
	delete successor; CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Malformed restored cells cannot borrow an electrical transfer role, inflate capacity or silently discard committed topology; failed staging preserves live authority. */
TEST_CASE("Deep utility shared restored identity refuses malformed topology", "[geothermal-utility-stage]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power; const auto tile = TileAddWrap(rig.request.tile, -1, -1);
	REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Succeeded()); auto bad = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	SECTION("inflated capacity") { ++bad.links[0].local_capacity_w; }
	SECTION("wrong role") { bad.links[0].kind = UtilityLinkKind::ElectricalPortal; }
	SECTION("missing topology receipt") { bad.local_link_revision = 0; }
	SECTION("wrong layer") { bad.links[0].source_endpoint = bad.links[0].destination_endpoint = TileXYL(10, 10, MapLayer::Surface); }
	SECTION("hidden energy") { bad.links[0].flow_j = 1; }
	SECTION("duplicate active cell") { bad.links.push_back(bad.links[0]); bad.links.back().id = UtilityLinkID{1}; ++bad.next_link; ++bad.local_link_revision; ++bad.operation_sequence; }
	CHECK(ValidateUtilityPowerState(bad, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None); StageLoadedUtilityPowerState(std::move(bad));
	CHECK_FALSE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == digest);
}

/** Actual native spatial ownership blocks both installation and removal under a vehicle, preserving paid geometry and capital until it withdraws. */
TEST_CASE("Deep utility construction respects actual native occupied floor", "[geothermal-utility-build]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power; const auto tile = TileAddWrap(rig.request.tile, -1, -1);
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Train::CanAllocateItem()); auto *train = Train::Create();
	train->owner = _current_company; train->SetArticulatedPart(); train->vehstatus.Set(VehState::Hidden);
	train->tile = tile; train->z_pos = 0; train->x_pos = TileX(tile) * TILE_SIZE; train->y_pos = TileY(tile) * TILE_SIZE; train->UpdatePosition();
	const auto empty = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	CHECK(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Failed()); CHECK(ComputeUtilityPowerDigest() == empty); CHECK(Company::Get(_current_company)->money == cash);
	train->tile = INVALID_TILE; train->UpdatePosition(); REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat).Succeeded());
	const auto installed = ComputeUtilityPowerDigest(); train->tile = tile; train->UpdatePosition();
	CHECK(UtilityCellCommand(tile, UtilityEnergyDomain::Heat, false, true).Failed()); CHECK(ComputeUtilityPowerDigest() == installed);
	delete train; REQUIRE(UtilityCellCommand(tile, UtilityEnergyDomain::Heat, false, true).Succeeded());
}

/** A paid cell remains compatible with ordinary track, but new machinery/station floors must remove it before construction rather than hiding it under an undeclared port. */
TEST_CASE("Building footprints cannot bury an installed owned Deep utility", "[geothermal-utility-build]")
{
	GeothermalCollectorRig rig; UtilityPowerReset power;
	REQUIRE(CancelGeothermalSite(rig.site) == GeothermalError::None);
	REQUIRE(UtilityCellCommand(rig.request.tile, UtilityEnergyDomain::Heat).Succeeded());
	CHECK(CheckExcavationConstruction(std::array{rig.request.tile}).Succeeded());
	CHECK(CheckExcavationConstructionArea(rig.request.tile, 1, 1).Failed());
	CHECK(PrepareGeothermalSite(rig.request).error == GeothermalError::Footprint);
	REQUIRE(UtilityCellCommand(rig.request.tile, UtilityEnergyDomain::Heat, false, true).Succeeded());
	CHECK(PrepareGeothermalSite(rig.request).error == GeothermalError::None);
}
