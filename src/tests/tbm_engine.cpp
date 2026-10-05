/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_engine.cpp Native engine mapping, part construction and ordinary purchase refusal. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../tbm.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../vehicle_func.h"
#include "../articulated_vehicles.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../station_base.h"
#include "../rail.h"
#include "../rail_cmd.h"
#include "../company_func.h"
#include "../autoreplace_cmd.h"
#include "../autoreplace_func.h"
#include "../engine_func.h"
#include "../core/backup_type.hpp"
#include "../game/game_instance.hpp"
#include "../script/api/script_companymode.hpp"
#include "../script/api/script_testmode.hpp"
#include "../script/api/script_engine.hpp"
#include "../script/api/script_vehicle.hpp"

#include "../safeguards.h"

/** Actual native definitions and physical portal owners, isolated without replacing gameplay engine/cargo registries. */
class TBMEngineFixture {
	decltype(_engine_mngr.mappings) original_mapping = _engine_mngr.mappings; ///< Preserve the caller's existing real EIDS configuration.
public:
	ExcavationFixture world; ///< Real accepted profile, company and cargo definitions.
	GRFConfigList package; ///< Exact producer package transport for definition initialization.
	Engine *stock = nullptr; ///< Actual original engine definition retained alongside appended native models.
	Station *origin = nullptr; ///< Actual pool-owned Surface station reference.
	LayerPortalCommissionRequest request; ///< Actual source-site request; no geological route or controller is fabricated.
	/** Install real native engine definitions and sealed geometry through their public owners. */
	TBMEngineFixture()
	{
		REQUIRE(Engine::GetNumItems() == 0); REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Station::GetNumItems() == 0);
		const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
		auto config = std::make_unique<GRFConfig>();
		config->ident = required.package; config->version = required.version; config->status = GRFStatus::Activated;
		package.push_back(std::move(config));
		REQUIRE(Engine::CanAllocateItem());
		stock = Engine::Create(VehicleType::Train, 0);
		_engine_mngr.SetID(VehicleType::Train, 0, INVALID_GRFID, 0, stock->index);
		REQUIRE(InitialiseTBMEngineDefinitions(package));
		ResetLayerPortalWorkState(); ResetRailTypes();
		request.portal = TileXYL(20, 20, MapLayer::Surface);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, request.portal, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		REQUIRE(Station::CanAllocateItem());
		origin = Station::Create(TileXYL(18, 20, MapLayer::Surface));
		origin->owner = _current_company; origin->facilities.Set(StationFacility::Train);
		request.proposal.company = _current_company; request.proposal.origin = origin->index;
		request.proposal.model_id = 0; request.proposal.commissioning_anchor = GetLayerPortalPairTile(request.portal);
		request.proposal.pass_terms.emplace();
	}
	/** Release only fixture-owned pools before content/company teardown, restoring the actual mapping configuration. */
	~TBMEngineFixture()
	{
		_vehicle_pool.CleanPool(); ResetVehicleHash();
		_station_pool.CleanPool(); ResetLayerPortalWorkState();
		_engine_pool.CleanPool(); _engine_mngr.mappings = std::move(original_mapping);
	}
};

/** Use the actual script active-instance owner so script purchase reaches the real command kernel. */
class TBMScriptScope : public ScriptObject {
	ActiveInstance active; ///< Restored before the actual GameInstance is destroyed.
public:
	/** @param game Actual script runtime whose company/command policy is exercised. */
	explicit TBMScriptScope(GameInstance &game) : active(game) {}
};

/** Real EIDS mappings retain identities across definition reconstruction and refuse absent/changed content. */
TEST_CASE("TBM definitions use real EIDS identities and canonical spoil labels", "[tbm-engine]")
{
	TBMEngineFixture fixture;
	const auto stock_id = fixture.stock->index;
	const auto stock_rail = fixture.stock->VehInfo<RailVehicleInfo>();
	std::array<EngineID, 4> ids;
	// Every native design resolves through the real engine manager, independently of its runtime slot number.
	for (uint16_t model = 0; model < 4; ++model) {
		ids[model] = GetTBMEngine(model);
		REQUIRE(Engine::IsValidID(ids[model]));
		CHECK(ids[model] != stock_id);
		CHECK(ids[model].base() != model); // The native logical ID is deliberately different from its actual allocated pool identity.
		CHECK(GetTBMEngineModel(ids[model]) == model);
		CHECK(IsTBMCutterEngine(ids[model]) == (model < 2));
		CHECK(CountArticulatedParts(ids[model]) == (model < 2 ? 1 : 0));
		CHECK(Engine::Get(ids[model])->GetDefaultCargoType() == GetCargoTypeByLabel(model == 3 ? CargoLabel{"ROCK"} : CargoLabel{"ERTH"}));
	}
	REQUIRE(InitialiseTBMEngineDefinitions(fixture.package));
	CHECK(Engine::GetNumItems() == 5);
	CHECK(_engine_mngr.GetID(VehicleType::Train, 0, INVALID_GRFID) == stock_id);
	CHECK(Engine::Get(stock_id) == fixture.stock);
	CHECK(fixture.stock->VehInfo<RailVehicleInfo>().max_speed == stock_rail.max_speed);
	CHECK(fixture.stock->VehInfo<RailVehicleInfo>().power == stock_rail.power);
	for (uint16_t model = 0; model < 4; ++model) CHECK(GetTBMEngine(model) == ids[model]);
	CHECK_FALSE(InitialiseTBMEngineDefinitions({}));
	CHECK(GetTBMEngine(4) == EngineID::Invalid());
	CHECK_FALSE(IsTBMCutterEngine(EngineID::Invalid()));
}

/** Genuine railway construction estimates preserve authority and actual acquisition produces its permanent articulated support. */
TEST_CASE("Validated portal geometry constructs real cutter machinery without changing tile types", "[tbm-site]")
{
	TBMEngineFixture fixture;
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portal = ComputeLayerPortalWorkDigest();
	const auto seeds = _random;
	const auto cash = Company::Get(_current_company)->money;
	Vehicle *vehicle = nullptr;
	REQUIRE(ValidateLayerPortalCommissioning(fixture.request).error == LayerPortalWorkError::None);
	CHECK(BuildRailVehicleForTBM({}, fixture.request, Engine::Get(GetTBMEngine(0)), &vehicle).Succeeded());
	CHECK(vehicle == nullptr); CHECK(Vehicle::GetNumItems() == 0);
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
	CHECK(BuildRailVehicleForTBM(DoCommandFlag::Execute, fixture.request, Engine::Get(GetTBMEngine(0)), &vehicle).Succeeded());
	REQUIRE(vehicle != nullptr);
	auto *cutter = Train::From(vehicle);
	CHECK(cutter->IsFrontEngine()); CHECK(cutter->IsEngine());
	REQUIRE(cutter->Next() != nullptr);
	CHECK(cutter->Next()->IsArticulatedPart()); CHECK(cutter->GetNextUnit() == nullptr);
	CHECK(Vehicle::GetNumItems() == 2);
	CHECK(IsLayerPortalTile(fixture.request.portal));
	CHECK(IsLayerPortalTile(fixture.request.proposal.commissioning_anchor));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
	CHECK(ComputeLayerPortalWorkDigest() == portal);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetGeologyWorldState().terrain.Get(fixture.request.proposal.commissioning_anchor) == ExcavationState::Solid);
	// The ordinary purchase/clone paths refuse before their NoTest/refit/allocation loops can affect a real reference chain.
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, fixture.request.portal, GetTBMEngine(0), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, fixture.request.portal, cutter->index, true)).Failed());
	CHECK(CmdAutoreplaceVehicle({}, cutter->index).Failed());
	CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, cutter->index).Failed());
	CHECK(Vehicle::GetNumItems() == 2);
}

/** The canonical producer refuses solid starter support; this test characterizes the remaining integration gate without creating free floor. */
TEST_CASE("Sealed TBM startup lacks a canonical paid initial-support handoff", "[tbm-sealed-start-gate]")
{
	TBMEngineFixture fixture;
	auto proposal = fixture.request.proposal;
	const ExcavationPlanningPose pose{proposal.commissioning_anchor, Direction::SW, {proposal.commissioning_anchor, proposal.commissioning_anchor, proposal.commissioning_anchor}};
	const auto step = AdvanceExcavationPlanningPose(pose, pose.heading);
	REQUIRE(step.error == ExcavationError::None);
	proposal.requested_tiles = {step.next.position};
	proposal.authorized_tiles = step.cut.swept_tiles; proposal.route = {step.cut};
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto portal = ComputeLayerPortalWorkDigest();
	const auto funds = Company::Get(_current_company)->money;
	REQUIRE(ValidateLayerPortalCommissioning(fixture.request).error == LayerPortalWorkError::None);
	REQUIRE(ValidateExcavationJobGeometry(GetGeologyWorldState(), proposal).error == ExcavationError::None);
	CHECK(PrepareExcavationPass(proposal, *proposal.pass_terms).error == ExcavationError::MissingSupport);
	const auto refused = ReserveExcavationJob(proposal, false);
	CHECK(refused.error == ExcavationError::MissingSupport);
	CHECK(refused.failure_tile == proposal.commissioning_anchor);
	CHECK(ReserveExcavationJob(proposal, true).error == ExcavationError::MissingSupport);
	CHECK(GetGeologyWorldState().terrain.Get(proposal.commissioning_anchor) == ExcavationState::Solid);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeLayerPortalWorkDigest() == portal);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(Vehicle::GetNumItems() == 0);
	CHECK(CommissionLayerPortalTBM(fixture.request, true).error == LayerPortalWorkError::ControllerUnavailable);
}

/** Generic purchase and autoreplace requests must refuse native cutters in a genuine compatible service depot before creating any part. */
TEST_CASE("Ordinary depots cannot purchase or autoreplace portal-only cutters", "[tbm-purchase-gate]")
{
	TBMEngineFixture fixture;
	const auto depot = TileXYL(30, 30, MapLayer::Surface);
	REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::NE).Succeeded());
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto seeds = _random;
	// Both designs are classified through live definitions, and both real command modes refuse before refit/part allocation.
	for (uint16_t model : {uint16_t{0}, uint16_t{1}}) {
		for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::AutoReplace}}) {
			const auto result = CmdBuildVehicle(flags, depot, GetTBMEngine(model), false, INVALID_CARGO, ClientID::Invalid);
			CHECK(std::get<0>(result).Failed()); CHECK(std::get<1>(result) == VehicleID::Invalid());
			CHECK(Vehicle::GetNumItems() == 0);
		}
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
	const auto standard = GetTBMEngine(0), deep = GetTBMEngine(1);
	Engine::Get(deep)->company_avail.Set(_current_company);
	REQUIRE(IsEngineBuildable(deep, VehicleType::Train, _current_company));
	CHECK_FALSE(CheckAutoreplaceValidity(standard, deep, _current_company));
	GameInstance game;
	TBMScriptScope active(game);
	ScriptCompanyMode company(static_cast<ScriptCompany::CompanyID>(_current_company.base()));
	ScriptTestMode estimate;
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Script estimates use the real unpaused command pipeline; no test-only purchase permission is substituted.
	REQUIRE(ScriptEngine::IsBuildable(deep));
	CHECK(ScriptVehicle::BuildVehicle(depot, deep) == ScriptVehicle::VEHICLE_INVALID);
	CHECK(Vehicle::GetNumItems() == 0);
}
