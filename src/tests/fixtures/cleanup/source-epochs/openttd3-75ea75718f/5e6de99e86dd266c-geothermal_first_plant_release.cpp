/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_release.cpp Ordinary loaded first-plant settlement, paid support conversion and actual export access.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "../landscape_cmd.h"
#include "../train_cmd.h"

/**
 * Execute an ordinary paid command and record its already-settled invoice.
 * @tparam Tcmd Existing synchronized native command.
 * @tparam Targs Its public typed arguments.
 * @param invoices Observation of actual command invoices, never a second money owner.
 * @param args Actual native inputs; Company owns the single real debit.
 */
template <Commands Tcmd, typename... Targs>
static void FundLoadedFirstPlant(Money &invoices, Targs... args)
{
	auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
	UpdateSignalsInBuffer();
	const auto &cost = ExtractCommandCost(result);
	INFO("first-plant release command=" << GetCommandName(Tcmd) << " error=" << cost.GetErrorMessage().base());
	REQUIRE(cost.Succeeded()); invoices += cost.GetCost();
	fmt::print("FIRST_PLANT_INVOICE command={} cost={}\n", GetCommandName(Tcmd), cost.GetCost().base());
}

/** @copydoc ReleaseLoadedFirstPlant */
Money ReleaseLoadedFirstPlant(TBMProjectID id)
{
	const auto *live = GetTBMProject(id); REQUIRE(live != nullptr);
	const auto project = *live; const auto &job = *GetExcavationJob(project.job);
	REQUIRE(project.model_id == 1); REQUIRE(job.cursor == job.route.size());
	REQUIRE(project.phase == TBMPhase::Completed); REQUIRE(CargoPacket::GetNumItems() == 0);
	INFO("native release problem=" << GetTBMProjectReleaseProblem(id).base());
	REQUIRE(GetTBMProjectReleaseProblem(id) == INVALID_STRING_ID);
	const auto layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money; Money invoices = 0;
	const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
	FundLoadedFirstPlant<Commands::ReleaseTBMHauler>(invoices, assignments.front().hauler);
	// Preserve the real paid trains at their actual positions. A toggle may
	// stop a still-running owner, but must never restart an already stopped one.
	for (VehicleID vehicle : {assignments.front().hauler, project.cutter}) {
		if (!Train::Get(vehicle)->vehstatus.Test(VehState::Stopped)) FundLoadedFirstPlant<Commands::StartStopVehicle>(invoices, vehicle, true);
	}
	FundLoadedFirstPlant<Commands::ControlTBMProject>(invoices, id, TBMProjectAction::Retire);
	REQUIRE(GetTBMProject(id) == nullptr); CHECK(IsTrainStoppedAtPortalSite(*Train::Get(project.cutter)));
	const auto support = CaptureTBMSupport();
	// Every temporary piece stays owned paid rail. Release removes its work
	// protection; ordinary public use pays each explicit conversion once.
	for (const auto &piece : support) FundLoadedFirstPlant<Commands::ConvertTBMSupport>(invoices, piece.tile);
	FundLoadedFirstPlant<Commands::ChangeLayerPortalConnection>(invoices, project.portal, true);
	CHECK(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Connected);
	// The two machinery-adjacent pairs use only Standard/Deep-earned floor.
	// Both housings and both real approaches are checked and paid; existing
	// owned straight pieces are reused without inventing a route or cavity.
	for (TileIndex upper : {layout.upper_export, layout.deep_export}) {
		const auto lower_layer = GetTileLayer(upper) == MapLayer::Surface ? MapLayer::Underground : MapLayer::DeepUnderground;
		const auto lower = TileXYL(TileX(upper), TilePlaneY(upper), lower_layer);
		for (TileIndex housing : {upper, lower}) {
			if (GetTileLayer(housing) != MapLayer::Surface) REQUIRE(GetGeologyWorldState().terrain.Get(housing) == ExcavationState::Excavated);
			if (IsPlainRailTile(housing)) FundLoadedFirstPlant<Commands::LandscapeClear>(invoices, housing);
		}
		FundLoadedFirstPlant<Commands::BuildLayerPortal>(invoices, upper, TransportType::Rail, lower_layer,
			DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
		CHECK(GetLayerPortalPairTile(upper) == lower);
		for (TileIndex end : {upper, lower}) {
			const auto approach = GetLayerPortalDoorTile(end);
			if (GetTileLayer(approach) != MapLayer::Surface) REQUIRE(GetGeologyWorldState().terrain.Get(approach) == ExcavationState::Excavated);
			if (IsPlainRailTile(approach) && GetTrackBits(approach).Test(Track::Y)) continue; // Only an exact already-paid native straight piece avoids a new track invoice.
			FundLoadedFirstPlant<Commands::BuildRail>(invoices, approach, RAILTYPE_RAIL, Track::Y, false);
		}
		FundLoadedFirstPlant<Commands::ChangeLayerPortalConnection>(invoices, upper, true);
		CHECK(GetLayerPortalWorkState(upper) == LayerPortalWorkState::Connected);
	}
	uint retained = 0;
	// Both original purchased machines remain whole and stopped in their real
	// native magazines; retirement cannot delete, replace or respawn machinery.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || !IsTBMCutterEngine(train->engine_type)) continue;
		++retained; CHECK(IsTrainStoppedAtPortalSite(*train));
	}
	CHECK(retained == 2); CHECK(CaptureTBMProjects().empty()); CHECK(CaptureTBMExchangeAssignments().empty());
	CHECK(CaptureTBMSupport().empty()); CHECK(CargoPacket::GetNumItems() == 0);
	CHECK(CaptureGeothermalGenerators().empty()); CHECK(CaptureGeothermalCollectors().empty());
	CHECK(CaptureGeothermalRisers().empty()); CHECK(CaptureUtilityPowerState().producers.empty());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash - invoices);
	fmt::print("FIRST_PLANT_RELEASE job={} cutter={} retained={} converted={} export_upper={} export_deep={} invoices={} cash={}\n",
		project.job.base(), project.cutter.base(), retained, support.size(), layout.upper_export.base(), layout.deep_export.base(),
		invoices.base(), Company::Get(_current_company)->money.base());
	return invoices;
}
