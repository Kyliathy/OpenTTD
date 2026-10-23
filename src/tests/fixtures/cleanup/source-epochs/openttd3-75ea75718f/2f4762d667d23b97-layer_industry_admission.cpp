/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_admission.cpp Native identity, whole-footprint, seam and calendar admission fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 3 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "mine_fixture.h"
#include "../multilayer_industry.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../newgrf_commons.h"
#include "../industry_cmd.h"
#include "../landscape_cmd.h"
#include "../town.h"
#include "../economy_func.h"
#include "../command_func.h"
#include "../station_map.h"
#include "../tile_cmd.h"

#include "../safeguards.h"


/** Runtime identities and declared output, rather than names/art/cargo resemblance, control rock-layer admission. */
TEST_CASE("Layer industry whitelist requires the exact native identity and declared output", "[layer-industry-admission]")
{
	MineAdmissionRig rig;
	const auto *capability = GetLayerIndustryCapability(rig.request.type);
	REQUIRE(capability != nullptr);
	CHECK(capability->resource == CargoLabel{"COAL"});
	CHECK(capability->layers.Test(MapLayer::Underground));
	CHECK_FALSE(capability->layers.Test(MapLayer::Surface));
	CHECK(GetLayerIndustryCapability(IT_INVALID) == nullptr);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, 0).Failed());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), 0).Succeeded());
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), rig.request.type).Failed());
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, rig.request.type).Succeeded());
	auto *spec = const_cast<IndustrySpec *>(GetIndustrySpec(rig.request.type));
	const auto local = spec->grf_prop.local_id;
	spec->grf_prop.local_id = 3; // The same output/art under an unapproved definition has no mine permission.
	CHECK(GetLayerIndustryCapability(rig.request.type) == nullptr);
	spec->grf_prop.local_id = local;
	spec->produced_cargo[0] = GetCargoTypeByLabel(CargoLabel{"IORE"});
	CHECK(GetLayerIndustryCapability(rig.request.type) == nullptr);
}

/** Surface processing has its own exact all-alternative cargo whitelist and cannot use the seam funding command. */
TEST_CASE("Surface processor whitelist requires its declared facility input and output union", "[layer-industry-admission]")
{
	MineAdmissionRig rig;
	IndustrySpec spec = _origin_industry_specs[1];
	spec.callback_mask = {};
	spec.accepts_cargo.fill(INVALID_CARGO);
	spec.produced_cargo.fill(INVALID_CARGO);
	spec.accepts_cargo[0] = GetCargoTypeByLabel(CargoLabel{"CORE"});
	spec.accepts_cargo[1] = GetCargoTypeByLabel(CargoLabel{"BAUX"});
	spec.accepts_cargo[2] = GetCargoTypeByLabel(CargoLabel{"NORE"});
	spec.accepts_cargo[3] = GetCargoTypeByLabel(CargoLabel{"COAL"});
	spec.produced_cargo[0] = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	spec.grf_prop = SubstituteGRFFileProps(1);
	spec.grf_prop.grfid = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
	spec.grf_prop.local_id = LAYER_PROCESSOR_LOCAL_BASE;
	spec.grf_prop.override_id = IT_INVALID;
	const GrfID scope = spec.grf_prop.grfid;
	_industry_mngr.SetEntitySpec(std::move(spec));
	const IndustryType type = _industry_mngr.GetID(LAYER_PROCESSOR_LOCAL_BASE, scope);
	REQUIRE(type != IT_INVALID);
	const auto *capability = GetLayerIndustryCapability(type);
	REQUIRE(capability != nullptr);
	CHECK(capability->layers.Test(MapLayer::Surface));
	CHECK(capability->recipe == MultiLayerRecipeID{"CFMT"});
	CHECK_FALSE(IsLayerMineDefinition(type));
	CHECK(CheckLayerIndustryConstruction(MultiLayerMap::ProjectToSurface(rig.request.tile), type).Succeeded());
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, type).Failed());
	CHECK(CmdBuildLayerMine(DoCommandFlag::Execute, MultiLayerMap::ProjectToSurface(rig.request.tile), type, 0, rig.request.deposit, 123).Failed());
	auto *installed = const_cast<IndustrySpec *>(GetIndustrySpec(type));
	installed->accepts_cargo[1] = INVALID_CARGO;
	CHECK(GetLayerIndustryCapability(type) == nullptr);
}

/** A valid estimate visits all foundations and loading cells while leaving identity, tiles and finite geology unchanged. */
TEST_CASE("Mine admission checks every excavated machinery and loading corner without mutation", "[layer-industry-admission]")
{
	MineAdmissionRig rig;
	const auto before = CaptureGeologyState();
	const auto valid = ValidateMineDepositClaim(rig.request);
	REQUIRE(valid.error == LayerIndustryError::None);
	CHECK(valid.machinery.size() == 4);
	CHECK(valid.loading.size() == 4);
	CHECK(valid.expected_reserve > 0);
	CHECK(Industry::GetNumItems() == 0);
	CHECK(CaptureGeologyState().packed == before.packed);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(before, candidate) == GeologyError::None);
	REQUIRE(candidate.terrain.Set(rig.site.back(), ExcavationState::Solid));
	auto missing_corner = before;
	missing_corner.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	rig.RestoreGeology(missing_corner);
	const auto refused = ValidateMineDepositClaim(rig.request);
	CHECK(refused.error == LayerIndustryError::Footprint);
	CHECK(refused.failure_tile == rig.site.back());
	CHECK(GetTileType(rig.request.tile) == TileType::Clear);
	CHECK(Industry::GetNumItems() == 0);
}

/** Idempotent company claims cannot admit a second mine; exhausted and mismatched seams retain their physical authority. */
TEST_CASE("Mine admission rejects claimed exhausted and mismatched selected deposits", "[layer-industry-admission]")
{
	MineAdmissionRig rig;
	auto state = CaptureGeologyState();
	auto selected = std::ranges::find(state.deposits, rig.request.deposit, &GeologyDeposit::id);
	REQUIRE(selected != state.deposits.end());
	selected->claimant = rig.request.company;
	rig.RestoreGeology(state);
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::Claimed);
	selected->claimant = CompanyID::Invalid();
	selected->remaining_reserve = 0;
	rig.RestoreGeology(state);
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::Depleted);
	rig.request.deposit = GeologyDepositID{UINT32_MAX};
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::InvalidDeposit);
}

/** Deep admission uses the calendar boundary on both policy and full-site queries, independent of the selected camera. */
TEST_CASE("Native Deep mine admission shares the 1980 calendar gate", "[layer-industry-admission]")
{
	MineAdmissionRig rig{5};
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::None);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(CheckLayerIndustryConstruction(rig.request.tile, rig.request.type).Failed());
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::Unavailable);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::None);
}

/** The live geological owner refuses stale/repeated claims and reserve debits, retaining the original finite seam on release. */
TEST_CASE("Live mine geology handoff owns claim release and stamped whole reserve debit", "[layer-mine-geology]")
{
	MineAdmissionRig rig;
	const auto reserve = GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve;
	CHECK(ClaimLayerMineDeposit(rig.request.deposit, rig.request.company, reserve - 1) == GeologyError::InvalidReserve);
	CHECK(ClaimLayerMineDeposit(rig.request.deposit, rig.request.company, reserve) == GeologyError::None);
	CHECK(ClaimLayerMineDeposit(rig.request.deposit, rig.request.company, reserve) == GeologyError::ClaimConflict);
	CHECK(ConsumeLayerMineReserve(rig.request.deposit, rig.request.company, 1, reserve - 1) == GeologyError::InvalidReserve);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == reserve);
	CHECK(ConsumeLayerMineReserve(rig.request.deposit, rig.request.company, 1, reserve) == GeologyError::None);
	CHECK(ConsumeLayerMineReserve(rig.request.deposit, rig.request.company, 1, reserve) == GeologyError::InvalidReserve);
	CHECK(ReleaseLayerMineDeposit(rig.request.deposit, rig.request.company) == GeologyError::None);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == reserve - 1);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->initial_reserve == reserve);
}

/** Command estimates are pure; execution acquires one stable industry/claim with exactly the same complete construction charge. */
TEST_CASE("Mine funding commits identity footprint and one claim together", "[layer-mine-funding]")
{
	MineAdmissionRig rig;
	rig.PrepareFunding();
	const auto before = CaptureGeologyState();
	const auto estimate = rig.Fund({});
	REQUIRE(estimate.Succeeded());
	CHECK(estimate.GetCost() > 0);
	CHECK(Industry::GetNumItems() == 0);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	const auto built = rig.Fund(DoCommandFlag::Execute);
	REQUIRE(built.Succeeded());
	CHECK(built.GetCost() == estimate.GetCost());
	REQUIRE(Industry::GetNumItems() == 1);
	Industry *industry = *Industry::Iterate().begin();
	const auto *mine = GetLayerMineState(industry->index);
	REQUIRE(mine != nullptr);
	CHECK(mine->deposit == rig.request.deposit);
	CHECK(mine->company == _current_company);
	CHECK(industry->location.tile == rig.request.tile);
	CHECK(GetGeologyWorldState().deposits.Get(mine->deposit)->claimant == _current_company);
	const auto original = std::ranges::find(before.deposits, rig.request.deposit, &GeologyDeposit::id);
	REQUIRE(original != before.deposits.end());
	CHECK(GetGeologyWorldState().deposits.Get(mine->deposit)->remaining_reserve == original->remaining_reserve);
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(Industry::GetNumItems() == 1);
	CHECK(CaptureGeologyState().packed == before.packed);
	// Every admitted machinery cell, and only machinery, becomes the newly allocated industry; loading access stays clear.
	for (size_t index = 0; index < rig.site.size(); ++index) {
		CHECK(GetTileType(rig.site[index]) == (index < 4 ? TileType::Industry : TileType::Clear));
	}
}

/** A solid corner, unavailable funds or a claimed seam fails before any native industry/tile/claim allocation. */
TEST_CASE("Mine funding failure preflight preserves all geology and map tiles", "[layer-mine-funding]")
{
	MineAdmissionRig rig;
	rig.PrepareFunding();
	const auto before = CaptureGeologyState();
	Company::Get(_current_company)->money = 0;
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(Industry::GetNumItems() == 0);
	Company::Get(_current_company)->money = 1000000;
	GeologyWorldState candidate;
	REQUIRE(PrepareGeologyRestoration(before, candidate) == GeologyError::None);
	REQUIRE(candidate.terrain.Set(rig.site[3], ExcavationState::Solid));
	auto blocked = before;
	blocked.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
	rig.RestoreGeology(blocked);
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(GetTileType(rig.request.tile) == TileType::Clear);
	CHECK(Industry::GetNumItems() == 0);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	CHECK(CaptureGeologyState().packed == blocked.packed);
	// A depleted or exclusively claimed seam must fail through the actual command, as well as the pure admission predicate.
	rig.RestoreGeology(before);
	REQUIRE(ClaimLayerMineDeposit(rig.request.deposit, rig.request.company,
		GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve) == GeologyError::None);
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(Industry::GetNumItems() == 0);
	REQUIRE(ReleaseLayerMineDeposit(rig.request.deposit, rig.request.company) == GeologyError::None);
	auto exhausted = before;
	auto seam = std::ranges::find(exhausted.deposits, rig.request.deposit, &GeologyDeposit::id);
	REQUIRE(seam != exhausted.deposits.end());
	seam->remaining_reserve = 0;
	rig.RestoreGeology(exhausted);
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(Industry::GetNumItems() == 0);
	CHECK(GetTileType(rig.request.tile) == TileType::Clear);
}

/** Removing an empty partially depleted mine releases its exclusive claim; rebuilding cannot create new reserve or geology identity. */
TEST_CASE("Mine demolition and rebuilding preserve the original remaining reserve and cavity", "[layer-mine-funding]")
{
	MineAdmissionRig rig;
	rig.PrepareFunding();
	auto before = CaptureGeologyState();
	auto seam = std::ranges::find(before.deposits, rig.request.deposit, &GeologyDeposit::id);
	REQUIRE(seam != before.deposits.end());
	seam->remaining_reserve -= 20;
	const uint64_t remaining = seam->remaining_reserve;
	rig.RestoreGeology(before);
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	REQUIRE(Industry::GetNumItems() == 1);
	_game_mode = GameMode::Normal; // Exercise the mine claimant's normal demolition permission, independently of editor privileges.
	Industry *first = *Industry::Iterate().begin();
	first->produced[0].waiting = 1;
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
	CHECK(first->produced[0].waiting == 1);
	first->produced[0].waiting = 0; // Fixture setup only: extraction/dispatch accounting is tested by the later production groups.
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK(Industry::GetNumItems() == 0);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == remaining);
	CHECK(CaptureGeologyState().packed == before.packed);
	_game_mode = GameMode::Editor;
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	Industry *rebuilt = *Industry::Iterate().begin();
	CHECK(GetLayerMineState(rebuilt->index)->deposit == rig.request.deposit);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == remaining);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->initial_reserve == remaining + 20);
}

/** Native mine construction and catchment stay on the actual rock layer, even when a Surface station has identical coordinates. */
TEST_CASE("Mine tile loops and station associations stay on their own explicit layer", "[layer-mine-lifecycle]")
{
	MineAdmissionRig rig;
	rig.PrepareFunding();
	const TileIndex local_tile = TileAddWrap(rig.request.tile, 3, 0);
	Station *local = rig.StationAt(local_tile);
	Station *surface = rig.StationAt(MultiLayerMap::ProjectToSurface(local_tile));
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	Industry *industry = *Industry::Iterate().begin();
	CHECK(industry->stations_near.contains(local));
	CHECK_FALSE(industry->stations_near.contains(surface));
	// The live tile dispatch completes the hidden rock machinery without hitting the legacy Surface assertion or producing free cargo.
	for (uint tick = 0; tick < 32; ++tick) {
		_tile_type_procs[TileType::Industry]->tile_loop_proc(rig.request.tile);
	}
	CHECK(IsIndustryCompleted(rig.request.tile));
	CHECK(industry->produced[0].waiting == 0);
}

/** Takeover moves the existing claim; company removal keeps output and a unique inactive physical binding without replenishing reserve. */
TEST_CASE("Mine takeover and abandonment preserve geology binding and physical output", "[layer-mine-lifecycle]")
{
	MineAdmissionRig rig;
	rig.PrepareFunding();
	REQUIRE(rig.Fund(DoCommandFlag::Execute).Succeeded());
	Industry *industry = *Industry::Iterate().begin();
	industry->produced[0].waiting = 3; // Explicit bounded fixture stock; extraction is validated in Group 7.
	const auto remaining = GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve;
	REQUIRE(Company::CanAllocateItem());
	Company *successor = Company::Create();
	ChangeLayerIndustryOwnership(rig.request.company, successor->index);
	CHECK(GetLayerMineState(industry->index)->company == successor->index);
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == successor->index);
	CHECK(industry->produced[0].waiting == 3);
	ChangeLayerIndustryOwnership(successor->index, CompanyID::Invalid());
	CHECK(GetLayerMineState(industry->index)->phase == LayerMinePhase::Abandoned);
	CHECK(GetLayerMineState(industry->index)->company == CompanyID::Invalid());
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->claimant == CompanyID::Invalid());
	CHECK(GetGeologyWorldState().deposits.Get(rig.request.deposit)->remaining_reserve == remaining);
	CHECK(FindLayerMineIndustry(rig.request.deposit) == industry->index);
	CHECK(ValidateMineDepositClaim(rig.request).error == LayerIndustryError::Claimed);
	CHECK(industry->produced[0].waiting == 3);
	delete successor;
}

/** Editor authors cannot operate Deep minerals before 1980, and neither the command nor pure technology predicate consumes anything on refusal. */
TEST_CASE("Mine operational date is shared by policy and commands even with editor override", "[layer-mine-lifecycle]")
{
	MineAdmissionRig rig{5};
	rig.PrepareFunding();
	rig.SetEditorOverride(true);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	const auto *capability = GetLayerIndustryCapability(rig.request.type);
	REQUIRE(capability != nullptr);
	CHECK_FALSE(IsLayerIndustryTechnologyAvailable(*capability, MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date));
	CHECK(rig.Fund(DoCommandFlag::Execute).Failed());
	CHECK(Industry::GetNumItems() == 0);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	CHECK(rig.Fund(DoCommandFlag::Execute).Succeeded());
}
